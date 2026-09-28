//=============================================================================
//  BinToScenario contract test
//
//  BinToScenario turns a recorded HOST->IG CIGI stream into a CIGI Host
//  Emulator script.  Nothing in it touches a simulator, so the whole contract
//  can be driven here, against the code the tool actually runs:
//
//    * the .bin container -- record framing, and a truncated or trailing-
//      rubbish file refused rather than half-converted;
//    * Entity Control -> ENTITY_POS + ENTITY_YPR, with yaw/pitch/roll in that
//      order (CIGI sends roll/pitch/yaw; a level capture never shows a swap);
//    * the HOST's own aircraft, entity 0, is rendered -- the one entity that
//      must never be skipped for being id 0;
//    * an all-zero pose is "no position", so it is neither created nor moved;
//    * Celestial Sphere Control -> TIME (the day/dusk/night/dawn difference);
//    * Atmosphere Control -> GLOBAL_* (the warm-up the captures carry);
//    * HAT/HOT Request -> HAT_HOT_REQ_*, entity-relative and geodetic;
//    * frames: unchanged runs collapse to FRAME_WAIT, and --no-compress writes
//      one FRAME per datagram, both covering exactly one frame per datagram;
//    * ids that are deliberately not translated are reported, not silently
//      dropped, and do not disturb the messages around them.
//
//  Build/run:  msbuild test\BinToScenarioTest.vcxproj /p:Configuration=Release
//              test\x64\Release\BinToScenarioTest.exe
//
//  No Prepar3D, no PDK, no emulator, no .bin file on disk.
//=============================================================================

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <string>
#include <vector>

#include "BinToScenario.h"
#include "cigi.h"

namespace {

int g_checks   = 0;
int g_failures = 0;

void check(bool ok, const char* what) {
    ++g_checks;
    if (ok) {
        std::printf("  ok    %s\n", what);
    } else {
        ++g_failures;
        std::printf("  FAIL  %s\n", what);
    }
}

using Bytes = std::vector<uint8_t>;

bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

// The script's command lines: everything that is not a header comment and not
// blank. Used to check the report's counts against the text it describes.
std::vector<std::string> commandLines(const std::string& s) {
    std::vector<std::string> out;
    size_t at = 0;
    while (at < s.size()) {
        const size_t nl = s.find('\n', at);
        const std::string line =
            s.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
        at = (nl == std::string::npos) ? s.size() : nl + 1;
        if (!line.empty() && line[0] != '#') out.push_back(line);
    }
    return out;
}

size_t occurrences(const std::string& hay, const std::string& needle) {
    size_t n = 0, at = 0;
    while ((at = hay.find(needle, at)) != std::string::npos) { ++n; at += needle.size(); }
    return n;
}

//-----------------------------------------------------------------------------
// Building wire bytes here, by hand, rather than by calling the tool's own
// decoders: the test then pins the layout instead of agreeing with itself.
//-----------------------------------------------------------------------------

void putU16(Bytes& b, uint16_t v) { b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8)); }
void putU32(Bytes& b, uint32_t v) {
    b.push_back(uint8_t(v)); b.push_back(uint8_t(v >> 8));
    b.push_back(uint8_t(v >> 16)); b.push_back(uint8_t(v >> 24));
}
void putF32(Bytes& b, float v) {
    uint8_t t[4]; std::memcpy(t, &v, 4);
    b.insert(b.end(), t, t + 4);
}
void putF64(Bytes& b, double v) {
    uint8_t t[8]; std::memcpy(t, &v, 8);
    b.insert(b.end(), t, t + 8);
}

Bytes cat(std::initializer_list<Bytes> parts) {
    Bytes b;
    for (const auto& p : parts) b.insert(b.end(), p.begin(), p.end());
    return b;
}

// Entity Control, 48 bytes: id, size, entity id, state, then roll/pitch/yaw at
// 12/16/20 and lat/lon/alt as doubles at 24/32/40.
Bytes msgEntity(uint16_t id, float roll, float pitch, float yaw,
                double lat, double lon, double alt) {
    Bytes m;
    m.push_back(uint8_t(cigi::MsgEntityControl));
    m.push_back(cigi::kEntityControlSize);
    putU16(m, id);
    m.push_back(0x09);            // entity state: active
    m.push_back(0x00);
    for (int i = 0; i < 6; ++i) m.push_back(0x00);  // type, parent, opacity, rsvd
    putF32(m, roll);
    putF32(m, pitch);
    putF32(m, yaw);
    putF64(m, lat);
    putF64(m, lon);
    putF64(m, alt);
    while (m.size() < cigi::kEntityControlSize) m.push_back(0);
    return m;
}

// Celestial Sphere Control, 16 bytes: hour at 2, minute at 3.
Bytes msgCelestial(unsigned hour, unsigned minute) {
    Bytes m;
    m.push_back(9);
    m.push_back(16);
    m.push_back(uint8_t(hour));
    m.push_back(uint8_t(minute));
    while (m.size() < 16) m.push_back(0);
    return m;
}

// Atmosphere Control, 32 bytes: humidity at 3, then floats at 4/8/12/16/20/24.
Bytes msgAtmos(unsigned humidity, float airTemp, float visibility,
               float windH, float windV, float windDir, float baroPress) {
    Bytes m;
    m.push_back(10);
    m.push_back(32);
    m.push_back(0x00);
    m.push_back(uint8_t(humidity));
    putF32(m, airTemp);
    putF32(m, visibility);
    putF32(m, windH);
    putF32(m, windV);
    putF32(m, windDir);
    putF32(m, baroPress);
    while (m.size() < 32) m.push_back(0);
    return m;
}

// HAT/HOT Request, 32 bytes: id(2), type/coord(4), period(5), entity(6),
// then x/y/z as doubles at 8/16/24.
Bytes msgHatHot(uint16_t hatHotId, unsigned type, unsigned coord, unsigned period,
                uint16_t entity, double x, double y, double z) {
    Bytes m;
    m.push_back(24);
    m.push_back(32);
    putU16(m, hatHotId);
    m.push_back(uint8_t(type | (coord << 2)));
    m.push_back(uint8_t(period));
    putU16(m, entity);
    putF64(m, x);
    putF64(m, y);
    putF64(m, z);
    while (m.size() < 32) m.push_back(0);
    return m;
}

// A message the tool deliberately does not translate, to prove it is reported
// and that it does not disturb the parsing of its neighbours.
Bytes msgViewControl(unsigned viewId) {
    Bytes m;
    m.push_back(16);            // View Control
    m.push_back(32);
    m.push_back(uint8_t(viewId));
    while (m.size() < 32) m.push_back(0);
    return m;
}

//-----------------------------------------------------------------------------
// A .bin image: repeating [uint32 deltaMicros][uint16 len][payload].
//-----------------------------------------------------------------------------
struct BinFile {
    Bytes bytes;

    BinFile& add(uint32_t deltaMicros, const Bytes& payload) {
        putU32(bytes, deltaMicros);
        putU16(bytes, uint16_t(payload.size()));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
        return *this;
    }
};

binscp::Report convert(const BinFile& file, const binscp::Options& opt = {}) {
    std::vector<binscp::BinRecord> recs;
    std::string err;
    if (!binscp::parseBin(file.bytes.data(), file.bytes.size(), recs, err)) {
        std::printf("  (parseBin rejected the harness's own file: %s)\n", err.c_str());
    }
    binscp::Report rep;
    // The script text is what matters; the caller re-derives it from `rep`'s
    // companion call below where it needs the text.
    (void)binscp::toScript(recs, "test.bin", opt, rep);
    return rep;
}

std::string convertText(const BinFile& file, const binscp::Options& opt = {}) {
    std::vector<binscp::BinRecord> recs;
    std::string err;
    binscp::parseBin(file.bytes.data(), file.bytes.size(), recs, err);
    binscp::Report rep;
    return binscp::toScript(recs, "test.bin", opt, rep);
}

} // namespace

int main() {
    // -- 1. The .bin container ----------------------------------------------
    std::printf("\nthe .bin container\n");
    {
        BinFile f;
        f.add(0, msgEntity(0, 0, 0, 0, 1.0, 2.0, 3.0));
        f.add(10200, msgEntity(0, 0, 0, 0, 1.1, 2.0, 3.0));
        f.add(10000, msgEntity(0, 0, 0, 0, 1.2, 2.0, 3.0));

        std::vector<binscp::BinRecord> recs;
        std::string err;
        check(binscp::parseBin(f.bytes.data(), f.bytes.size(), recs, err),
              "a well-formed file parses");
        check(recs.size() == 3, "every record comes out");
        check(recs[0].deltaMicros == 0 && recs[1].deltaMicros == 10200 &&
              recs[2].deltaMicros == 10000, "the record delays are read");
        check(recs[0].payload.size() == cigi::kEntityControlSize,
              "and the payload length is read");

        Bytes cut = f.bytes;
        cut.pop_back();
        check(!binscp::parseBin(cut.data(), cut.size(), recs, err),
              "a file cut mid-record is refused");
        check(!err.empty(), "and says why");

        Bytes trailing = f.bytes;
        trailing.push_back(0xAB);
        check(!binscp::parseBin(trailing.data(), trailing.size(), recs, err),
              "trailing rubbish after the last record is refused");

        Bytes empty;
        check(binscp::parseBin(empty.data(), 0, recs, err) && recs.empty(),
              "an empty file is not an error, just empty");
    }

    // -- 2. Entity Control -> pose lines -------------------------------------
    std::printf("\nthe pose the HOST commanded\n");
    {
        BinFile f;
        f.add(0, msgEntity(0, 1.5f, -2.25f, 90.0f, 12.5, 34.25, 1000.0));

        const std::string s = convertText(f);
        check(contains(s, "ENTITY_POS 0 12.50000000 34.25000000 1000.000\n"),
              "the position is latitude longitude altitude, in degrees and metres");
        // CIGI sends roll/pitch/yaw at 12/16/20; ENTITY_YPR is yaw pitch roll.
        check(contains(s, "ENTITY_YPR 0 90.0000 -2.2500 1.5000\n"),
              "and the attitude is yaw pitch roll -- not the wire's roll pitch yaw");
        check(contains(s, "ADD_ENTITY 0 118"), "entity 0 is created, not skipped");
        check(contains(s, "FRAME\n"), "and the frame is ended");
    }

    // -- 3. An all-zero pose is \"no position\" --------------------------------
    std::printf("\nan entity the HOST is not driving\n");
    {
        BinFile f;
        f.add(12000, msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0));
        f.add(12000, msgEntity(5, 0, 0, 0, 0.0, 0.0, 0.0));   // the capture's entity 5
        f.add(12000, msgEntity(0, 0, 0, 0, 12.6, 34.25, 1000.0));

        const std::string s = convertText(f);
        check(!contains(s, "ADD_ENTITY 5"), "an all-zero pose is not created");
        check(!contains(s, "ENTITY_POS 5"), "and is never moved");
        check(contains(s, "ENTITY_POS 0"), "while the driven entity still is");
    }

    // -- 4. Scene: time of day ----------------------------------------------
    std::printf("\nthe time of day\n");
    {
        BinFile f;
        f.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0), msgCelestial(17, 30)}));
        f.add(10000, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0), msgCelestial(17, 30)}));

        const std::string s = convertText(f);
        check(contains(s, "TIME 17:30\n"), "Celestial Sphere Control becomes TIME hh:mm");
        check(occurrences(s, "TIME ") == 1, "and only when it changes");

        BinFile dawn;
        dawn.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0), msgCelestial(6, 15)}));
        check(contains(convertText(dawn), "TIME 06:15\n"),
              "the minutes are zero-padded, so 06:15 does not read 6:15");
    }

    // -- 5. Scene: the atmosphere warm-up -----------------------------------
    std::printf("\nthe atmosphere warm-up\n");
    {
        BinFile f;
        f.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0),
                      msgAtmos(50, 24.1518f, 200000.0f, 0, 0, 0, 1013.25f)}));

        const std::string s = convertText(f);
        check(contains(s, "GLOBAL_AIR_TEMP 24.1518\n"), "air temperature is carried over");
        check(contains(s, "GLOBAL_VISIBILITY 200000\n"), "so is visibility");
        check(contains(s, "GLOBAL_BARO_PRESSURE 1013.25\n"), "and the pressure");
        check(contains(s, "GLOBAL_HUMIDITY 50\n"), "and the humidity");
        check(!contains(s, "GLOBAL_WIND"), "a still day writes no wind line");

        BinFile windy;
        windy.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0),
                          msgAtmos(50, 24.0f, 2000.0f, 5.5f, 0.5f, 270.0f, 1013.0f)}));
        check(contains(convertText(windy), "GLOBAL_WIND 5.5 0.5 270\n"),
              "and a windy one writes horizontal, vertical and direction");
    }

    // -- 6. HAT/HOT requests -------------------------------------------------
    std::printf("\nHAT/HOT requests\n");
    {
        BinFile f;
        f.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0),
                      msgHatHot(5, 2 /*extended*/, 1 /*entity*/, 0, 0, 0.0, 0.0, 10.0)}));

        const std::string s = convertText(f);
        check(contains(s, "HAT_HOT_REQ_BEGIN 5\n"), "the request id is kept");
        check(contains(s, "HAT_HOT_REQ_TYPE EXTENDED\n"), "and its type");
        check(contains(s, "HAT_HOT_REQ_SOURCE ENTITY 0.00000000 0.00000000 10.00000000 0\n"),
              "an entity-relative source keeps its offsets and entity");
        check(contains(s, "HAT_HOT_REQ_END\n"), "and the request is sent");
        check(contains(s, "HAT_HOT_REQ_UPDATE_PERIOD 0\n"),
              "a zero update period is written too -- the emulator reads that as one-shot");

        BinFile g;
        g.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0),
                      msgHatHot(9, 0 /*hat*/, 0 /*geodetic*/, 30, 0, 12.5, 34.25, 1000.0)}));
        const std::string sg = convertText(g);
        check(contains(sg, "HAT_HOT_REQ_TYPE HAT\n"), "a HAT request reads HAT");
        check(contains(sg, "HAT_HOT_REQ_SOURCE GEODETIC "), "a geodetic source reads GEODETIC");
        check(contains(sg, "HAT_HOT_REQ_UPDATE_PERIOD 30\n"), "a periodic request keeps its period");
    }

    // -- 7. Frame pacing -----------------------------------------------------
    std::printf("\nframes\n");
    {
        BinFile still;
        for (int i = 0; i < 5; ++i) {
            still.add(10000, msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0));
        }
        const std::string s = convertText(still);
        check(occurrences(s, "FRAME\n") == 1, "a still capture writes one FRAME");
        check(contains(s, "FRAME_WAIT 4\n"), "and folds the rest into one FRAME_WAIT");
        check(occurrences(s, "ENTITY_POS") == 1, "without repeating the pose");

        binscp::Options raw;
        raw.frameWaitCompression = false;
        const std::string r = convertText(still, raw);
        check(occurrences(r, "FRAME\n") == 5, "--no-compress writes one FRAME per datagram");
        check(!contains(r, "FRAME_WAIT"), "and no FRAME_WAIT at all");

        // Every datagram is a frame, either way.
        std::vector<binscp::BinRecord> recs;
        std::string err;
        binscp::parseBin(still.bytes.data(), still.bytes.size(), recs, err);
        binscp::Report rep, repRaw;
        binscp::toScript(recs, "test.bin", {}, rep);
        binscp::toScript(recs, "test.bin", raw, repRaw);
        check(rep.frames() == 5 && repRaw.frames() == 5,
              "both pacing modes cover exactly one frame per datagram");

        BinFile moving;
        for (int i = 0; i < 4; ++i) {
            moving.add(10000, msgEntity(0, 0, 0, 0, 12.5 + 0.001 * i, 34.25, 1000.0));
        }
        const std::string m = convertText(moving);
        check(occurrences(m, "ENTITY_POS") == 4, "a moving aircraft writes a pose a frame");
        check(occurrences(m, "FRAME\n") == 4, "and a FRAME a frame");
        check(!contains(m, "FRAME_WAIT"), "with nothing to fold");
    }

    // -- 8. What is not translated is still reported --------------------------
    std::printf("\nwhat is deliberately not translated\n");
    {
        BinFile f;
        f.add(0, cat({msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0),
                      msgViewControl(1),
                      msgCelestial(12, 0)}));
        f.add(10000, cat({msgEntity(0, 0, 0, 0, 12.6, 34.25, 1000.0),
                          msgViewControl(1),
                          msgCelestial(12, 0)}));

        std::vector<binscp::BinRecord> recs;
        std::string err;
        binscp::parseBin(f.bytes.data(), f.bytes.size(), recs, err);
        binscp::Report rep;
        const std::string s = binscp::toScript(recs, "test.bin", {}, rep);

        check(rep.idCounts.count(binscp::kViewControl) &&
              rep.idCounts.at(binscp::kViewControl) == 2,
              "an untranslated id is counted");
        check(!binscp::isTranslated(binscp::kViewControl),
              "and reads as untranslated");
        check(binscp::isTranslated(binscp::kEntityControl),
              "while the translated ones do not");
        check(contains(s, "#   - 16  View Control"),
              "and the script's own header says so");
        check(contains(s, "#   + 2   Entity Control"),
              "as it does for the translated ones");
        check(!contains(s, "VIEW_"), "no view command is written");

        const std::string r = binscp::formatReport(rep, "test.bin");
        check(contains(r, "View Control"), "the console report names it too");
        check(contains(r, "not translated"), "and flags that it was left out");
    }

    // -- 9. The report, and helpers ------------------------------------------
    std::printf("\nthe report and the small helpers\n");
    {
        BinFile f;
        f.add(0,          msgEntity(0, 0, 0, 0, 12.5, 34.25, 1000.0));
        f.add(10000,      msgEntity(2, 0, 0, 0, 12.6, 34.25, 1000.0));
        f.add(20000,      msgEntity(0, 0, 0, 0, 12.7, 34.25, 1000.0));

        std::vector<binscp::BinRecord> recs;
        std::string err;
        binscp::parseBin(f.bytes.data(), f.bytes.size(), recs, err);
        binscp::Report rep;
        const std::string s = binscp::toScript(recs, "hlm.bin", {}, rep);

        check(rep.datagrams == 3, "the report counts the datagrams");
        check(rep.entityIds.size() == 2 && rep.entityIds[0] == 0 && rep.entityIds[1] == 2,
              "and lists the entities that carry a pose, in order");
        check(rep.totalMicros == 30000, "and sums the recorded span");
        check(contains(s, "hlm.bin"), "the script names its source");

        // The line counts have to describe the script that was actually written,
        // scene lines included -- a report that under-counts is worse than none.
        BinFile counted;
        counted.add(0, cat({msgEntity(0, 1.5f, -2.25f, 90.0f, 12.5, 34.25, 1000.0),
                            msgCelestial(12, 0),
                            msgAtmos(50, 24.0f, 2000.0f, 0, 0, 0, 1013.0f),
                            msgHatHot(5, 2, 1, 0, 0, 0.0, 0.0, 10.0)}));
        counted.add(10000, msgEntity(0, 1.5f, -2.25f, 91.0f, 12.6, 34.25, 1000.0));

        std::vector<binscp::BinRecord> crecs;
        binscp::parseBin(counted.bytes.data(), counted.bytes.size(), crecs, err);
        binscp::Report crep;
        const std::string cs = binscp::toScript(crecs, "test.bin", {}, crep);

        // One ADD_ENTITY, one TIME, four atmosphere lines (no wind that day) and
        // one five-line HAT/HOT request.
        check(crep.sceneCommands == 1 + 1 + 4 + 5,
              "the scene count covers ADD_ENTITY, the time, the atmosphere and the request");
        check(crep.poseCommands == 4, "the pose count covers both lines of both frames");
        check(crep.frames() == 2, "and the frames cover both datagrams");

        size_t plain = 0;
        for (const auto& l : commandLines(cs)) {
            if (l.rfind("FRAME_WAIT", 0) != 0) ++plain;
        }
        check(plain == crep.sceneCommands + crep.poseCommands + crep.frameCommands,
              "and the three counts add up to the script's own command lines");

        check(binscp::scriptPathFor("C:\\cap\\hlm.bin") == "C:\\cap\\hlm.scp",
              "an output path replaces the extension");
        check(binscp::scriptPathFor("night_replay.bin") == "night_replay.scp",
              "with or without a directory");
        check(binscp::scriptPathFor("noext") == "noext.scp",
              "and appends one when there is none");
        check(binscp::messageName(binscp::kCelestialControl) == "Celestial Sphere Control",
              "message ids have names");
        check(contains(binscp::messageName(200), "200"),
              "and an unknown one still says which id it was");
    }

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
