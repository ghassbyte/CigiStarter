//-----------------------------------------------------------------------------
// BinToScenario.cpp
//
// See BinToScenario.h for what this is for.  The one thing worth repeating
// here: the decoders below are written against the bytes the HOST actually put
// on the wire -- each one says which capture pins its offsets -- because a CIGI
// field read from the wrong offset produces a script that runs perfectly and
// flies somewhere else.
//-----------------------------------------------------------------------------

#include "BinToScenario.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <set>
#include <utility>

#include "cigi.h"   // Entity Control and HAT/HOT Request decoders, and the framer

namespace binscp {

namespace {

// --- little-endian readers --------------------------------------------------
// The .bin format and CIGI 3.3 here are little-endian on the wire, and this
// project is x64-only, but the reads are spelled out rather than cast so the
// code does not depend on either.

uint16_t rdU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}
uint32_t rdU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}
float rdF32(const uint8_t* p) {
    float f;
    std::memcpy(&f, p, sizeof f);
    return f;
}

// --- formatting -------------------------------------------------------------
// Everything the script carries is formatted through these, so the text is
// stable run to run: the contract test compares whole lines, and the
// "has this changed since the last frame" test compares the same strings.

std::string fmt(const char* spec, double v) {
    char b[64];
    std::snprintf(b, sizeof b, spec, v);
    return std::string(b);
}

std::string pad2(unsigned v) {
    char b[8];
    std::snprintf(b, sizeof b, "%02u", v);
    return std::string(b);
}

std::string padRight(std::string s, size_t w) {
    if (s.size() < w) s.append(w - s.size(), ' ');
    return s;
}

// How many script lines a block of text adds, so the report's counts add up to
// the script the operator is looking at.
size_t countLines(const std::string& s) {
    return static_cast<size_t>(std::count(s.begin(), s.end(), '\n'));
}

std::string baseName(const std::string& p) {
    const size_t s = p.find_last_of("/\\");
    return s == std::string::npos ? p : p.substr(s + 1);
}

// --- what one datagram contributes -----------------------------------------

// Celestial Sphere Control, CIGI 3.3, 16 bytes: Hour at 2, Minute at 3.
//
// Grounded on the *_replay captures. The four time-of-day recordings differ in
// exactly these two bytes and nowhere else, and read 12:00 (day), 17:30 (dusk),
// 23:30 (night), 06:15 (dawn) -- the file names on the wire. The date fields
// after them are not valid in these captures (DateVld is clear and the month
// byte holds 17), so only the time of day is carried over.
struct TimeOfDay {
    bool     valid  = false;
    unsigned hour   = 0;
    unsigned minute = 0;
};

bool decodeCelestial(const uint8_t* m, size_t n, TimeOfDay& out) {
    if (n < 16 || m[0] != kCelestialControl) return false;
    out.valid  = true;
    out.hour   = m[2];
    out.minute = m[3];
    return true;
}

// Atmosphere Control, CIGI 3.3, 32 bytes: Humidity at 3, then floats at 4 (air
// temperature), 8 (visibility), 12/16/20 (horizontal wind, vertical wind,
// wind direction) and 24 (barometric pressure).
//
// Grounded on the route captures: the pressure reads 1013.25 exactly and the
// temperature 24.15 C (the hills) / 25.21 C (the coast), which is what pins the
// slots. It is a warm-up message the HOST sends for the first frames only; the
// script sets it once, before the flight, and again if it ever changes.
struct Atmos {
    bool   valid      = false;
    unsigned humidity = 0;
    double airTemp    = 0.0;
    double visibility = 0.0;
    double windHoriz  = 0.0;
    double windVert   = 0.0;
    double windDir    = 0.0;
    double baroPress  = 0.0;
};

bool decodeAtmos(const uint8_t* m, size_t n, Atmos& out) {
    if (n < 32 || m[0] != kAtmosphereControl) return false;
    out.valid      = true;
    out.humidity   = m[3];
    out.airTemp    = rdF32(m + 4);
    out.visibility = rdF32(m + 8);
    out.windHoriz  = rdF32(m + 12);
    out.windVert   = rdF32(m + 16);
    out.windDir    = rdF32(m + 20);
    out.baroPress  = rdF32(m + 24);
    return true;
}

// A HAT/HOT Request, in the shape the script needs. The wire decoding is
// cigi::parseHatHotRequest's (that code is the one that answers these), so this
// only carries the fields across.
struct HatHot {
    bool     valid  = false;
    unsigned id     = 0;
    int      type   = 0;   // 0 HAT, 1 HOT, 2 Extended
    int      coord  = 0;   // 0 geodetic, 1 entity-relative
    unsigned update = 0;
    unsigned entity = 0;
    double   x = 0.0, y = 0.0, z = 0.0;
};

bool sameAtmos(const Atmos& a, const Atmos& b) {
    return a.humidity == b.humidity && a.airTemp == b.airTemp &&
           a.visibility == b.visibility && a.baroPress == b.baroPress &&
           a.windHoriz == b.windHoriz && a.windVert == b.windVert &&
           a.windDir == b.windDir;
}

bool sameHatHot(const HatHot& a, const HatHot& b) {
    return a.id == b.id && a.type == b.type && a.coord == b.coord &&
           a.update == b.update && a.entity == b.entity &&
           a.x == b.x && a.y == b.y && a.z == b.z;
}

std::string atmosLines(const Atmos& a) {
    std::string s;
    s += "GLOBAL_AIR_TEMP "     + fmt("%.6g", a.airTemp)    + "\n";
    s += "GLOBAL_VISIBILITY "   + fmt("%.6g", a.visibility) + "\n";
    s += "GLOBAL_BARO_PRESSURE " + fmt("%.6g", a.baroPress) + "\n";
    s += "GLOBAL_HUMIDITY "     + std::to_string(a.humidity) + "\n";
    // Sentinel for "the HOST is not asking for wind": all three zero. Emitting
    // it would be noise, and the emulator's own .def default is a still day.
    if (a.windHoriz != 0.0 || a.windVert != 0.0 || a.windDir != 0.0) {
        s += "GLOBAL_WIND " + fmt("%.6g", a.windHoriz) + " " +
              fmt("%.6g", a.windVert) + " " + fmt("%.6g", a.windDir) + "\n";
    }
    return s;
}

std::string hatHotLines(const HatHot& h) {
    std::string s;
    s += "HAT_HOT_REQ_BEGIN " + std::to_string(h.id) + "\n";
    s += std::string("HAT_HOT_REQ_TYPE ") +
         (h.type == 1 ? "HOT" : (h.type == 2 ? "EXTENDED" : "HAT")) + "\n";
    // entity_id is mandatory even for a geodetic source, where the emulator
    // ignores it, so it is always written.
    s += std::string("HAT_HOT_REQ_SOURCE ") + (h.coord == 1 ? "ENTITY" : "GEODETIC") +
         " " + fmt("%.8f", h.x) + " " + fmt("%.8f", h.y) + " " +
         fmt("%.8f", h.z) + " " + std::to_string(h.entity) + "\n";
    // Always written, zero included: the emulator's reference says a zero here
    // is a one-shot request, which is exactly CIGI's meaning, whereas leaving it
    // out would fall back to a default the reference does not state.
    s += "HAT_HOT_REQ_UPDATE_PERIOD " + std::to_string(h.update) + "\n";
    s += "HAT_HOT_REQ_END\n";
    return s;
}

// The comment block at the top of the script. It is deliberately a summary of
// the same Report the console prints: an operator reading the .scp months later
// should not have to re-run the converter to know what was left out.
std::string scriptHeader(const Report& rep, const Options& opt,
                         const std::string& source) {
    std::string h;
    h += "# CIGI Host Emulator script (.scp)\n";
    h += "# Generated by BinToScenario from " +
         (source.empty() ? std::string("(unknown)") : baseName(source)) +
         "\n# Do not edit by hand -- regenerate it; edits will be overwritten.\n";
    h += "#\n";
    h += "# A recorded HOST->IG CIGI stream rewritten as emulator commands, so the\n";
    h += "# scenario can be run on the native CIGI Host Emulator (Hemu3.exe)\n";
    h += "# instead of being replayed through CigiP3dSimConnect / CigiP3dBridge.\n";
    h += "#\n";
    h += "#   datagrams      " + std::to_string(rep.datagrams) + "\n";
    h += "#   payload bytes  " + std::to_string(rep.bytes) + "\n";
    h += "#   recorded span  " + fmt("%.3f", rep.totalMicros / 1e6) + " s\n";
    h += "#   entities       ";
    if (rep.entityIds.empty()) {
        h += "(none carried a usable pose)\n";
    } else {
        for (size_t i = 0; i < rep.entityIds.size(); ++i) {
            if (i) h += " ";
            h += std::to_string(rep.entityIds[i]);
        }
        h += "   (created with entity type " + std::to_string(opt.entityType) + ")\n";
    }
    h += "#\n";
    h += "# Message ids seen this run ( + carried into this script, - reported\n";
    h += "# only, and why: the pose is absolute so rates and articulated parts add\n";
    h += "# nothing, per-channel view angles belong in the sim's viewgroup.xml,\n";
    h += "# and the emulator sends its own IG Control ):\n";
    for (const auto& kv : rep.idCounts) {
        h += "#   ";
        h += isTranslated(kv.first) ? '+' : '-';
        h += ' ';
        h += padRight(std::to_string(kv.first), 4);
        h += padRight(messageName(kv.first), 30);
        h += std::to_string(kv.second);
        h += '\n';
    }
    h += "#\n";
    return h;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
// Small public queries
//-----------------------------------------------------------------------------

std::string messageName(unsigned id) {
    switch (id) {
    case kIGControl:          return "IG Control";
    case kEntityControl:      return "Entity Control";
    case kComponentControl:   return "Component Control";
    case 5:                   return "Short Component Control";
    case kArticulatedPart:    return "Articulated Part Control";
    case 7:                   return "Short Articulated Part Control";
    case kRateControl:        return "Rate Control";
    case kCelestialControl:   return "Celestial Sphere Control";
    case kAtmosphereControl:  return "Atmosphere Control";
    case 11:                  return "Environment Region Control";
    case 12:                  return "Weather Control";
    case 13:                  return "Maritime Surface Control";
    case 14:                  return "Wave Control";
    case 15:                  return "Terrestrial Surface Control";
    case kViewControl:        return "View Control";
    case 17:                  return "Sensor Control";
    case 18:                  return "Motion Track Control";
    case 19:                  return "Earth Model Definition";
    case 20:                  return "Trajectory Definition";
    case 21:                  return "View Definition";
    case 22:                  return "Collision Detection Segment Definition";
    case 23:                  return "Collision Detection Volume Definition";
    case kHatHotRequest:      return "HAT/HOT Request";
    case 25:                  return "Line of Sight Segment Request";
    case 26:                  return "Line of Sight Vector Request";
    case 27:                  return "Position Request";
    case 28:                  return "Environmental Conditions Request";
    case 101:                 return "Start of Frame";
    case 102:                 return "HAT/HOT Response";
    case 103:                 return "HAT/HOT Extended Response";
    default:                  return "message " + std::to_string(id);
    }
}

bool isTranslated(unsigned id) {
    return id == kEntityControl || id == kCelestialControl ||
           id == kAtmosphereControl || id == kHatHotRequest;
}

//-----------------------------------------------------------------------------
// The .bin container
//-----------------------------------------------------------------------------

bool parseBin(const uint8_t* data, size_t len,
              std::vector<BinRecord>& out, std::string& err) {
    out.clear();
    err.clear();
    size_t off = 0;
    while (off + 6 <= len) {
        BinRecord r;
        r.deltaMicros = rdU32(data + off);
        const uint16_t n = rdU16(data + off + 4);
        off += 6;
        if (off + n > len) {
            err = "record at byte " + std::to_string(off - 6) +
                  " claims " + std::to_string(n) +
                  " payload bytes but only " + std::to_string(len - off) +
                  " remain -- truncated?";
            return false;
        }
        r.payload.assign(data + off, data + off + n);
        off += n;
        out.push_back(std::move(r));
    }
    if (off != len) {
        err = std::to_string(len - off) +
              " trailing byte(s) after the last whole record -- corrupt?";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// The conversion
//-----------------------------------------------------------------------------

std::string toScript(const std::vector<BinRecord>& records,
                     const std::string& source,
                     const Options& options, Report& rep) {
    rep = Report{};
    rep.datagrams = records.size();
    for (const auto& r : records) {
        rep.bytes += r.payload.size();
        rep.totalMicros += r.deltaMicros;
    }

    // Pass 1: the id census, and every entity that ever carries a pose worth
    // rendering. The emulator has to be told to create those before it can be
    // told where they are, and one that is only ever all-zero must not be
    // created (the capture's entity 5 is exactly that: not "at the Gulf of
    // Guinea", but "no position").
    std::set<unsigned> entitySet;
    for (const auto& r : records) {
        cigi::forEachMessage(r.payload.data(), r.payload.size(),
            [&](uint8_t id, const uint8_t* msg, size_t n) {
                rep.idCounts[id]++;
                cigi::EntityControl e;
                if (cigi::parseEntityControl(msg, n, e) && cigi::poseUsable(e)) {
                    entitySet.insert(e.entityId);
                }
            });
    }
    rep.entityIds.assign(entitySet.begin(), entitySet.end());

    std::string out = scriptHeader(rep, options, source);
    out += "\n";

    for (unsigned id : rep.entityIds) {
        out += "ADD_ENTITY " + std::to_string(id) + " " +
               std::to_string(options.entityType) + "\n";
        rep.sceneCommands++;
    }
    if (!rep.entityIds.empty()) out += "\n";

    // Pass 2: the frames. "Last written" is kept as the *formatted* text, not
    // the numbers, so a value that would print identically is not printed
    // again -- that is both the change test and the FRAME_WAIT compression.
    std::map<unsigned, std::pair<std::string, std::string>> lastPose; // id -> pos, ypr
    TimeOfDay lastTime;
    Atmos     lastAtmos;
    HatHot    lastHatHot;
    bool      haveTime = false, haveAtmos = false, haveHatHot = false;

    unsigned skip = 0;

    for (const auto& r : records) {
        // Gather this datagram first, so the script's lines come out in a fixed
        // order (world, then entities) rather than wire order.
        TimeOfDay cel;
        Atmos     atm;
        HatHot    hat;
        std::vector<cigi::EntityControl> poses;

        cigi::forEachMessage(r.payload.data(), r.payload.size(),
            [&](uint8_t id, const uint8_t* msg, size_t n) {
                if (id == kEntityControl) {
                    cigi::EntityControl e;
                    if (cigi::parseEntityControl(msg, n, e) && cigi::poseUsable(e)) {
                        poses.push_back(e);
                    }
                } else if (id == kCelestialControl) {
                    decodeCelestial(msg, n, cel);
                } else if (id == kAtmosphereControl) {
                    decodeAtmos(msg, n, atm);
                } else if (id == kHatHotRequest) {
                    cigi::HatHotRequest q;
                    if (cigi::parseHatHotRequest(msg, n, q)) {
                        hat.valid  = true;
                        hat.id     = q.hatHotId;
                        hat.type   = q.requestType;
                        hat.coord  = q.coordSystem;
                        hat.update = q.updatePeriod;
                        hat.entity = q.entityId;
                        hat.x = q.x; hat.y = q.y; hat.z = q.z;
                    }
                }
            });

        std::string chunk;

        if (cel.valid &&
            (!haveTime || cel.hour != lastTime.hour || cel.minute != lastTime.minute)) {
            const std::string line = "TIME " + pad2(cel.hour) + ":" + pad2(cel.minute) + "\n";
            chunk += line;
            rep.sceneCommands += countLines(line);
            lastTime = cel;
            haveTime = true;
        }
        if (atm.valid && (!haveAtmos || !sameAtmos(atm, lastAtmos))) {
            const std::string lines = atmosLines(atm);
            chunk += lines;
            rep.sceneCommands += countLines(lines);
            lastAtmos = atm;
            haveAtmos = true;
        }
        if (hat.valid && (!haveHatHot || !sameHatHot(hat, lastHatHot))) {
            const std::string lines = hatHotLines(hat);
            chunk += lines;
            rep.sceneCommands += countLines(lines);
            lastHatHot = hat;
            haveHatHot = true;
        }

        for (const auto& e : poses) {
            const std::string pos =
                fmt("%.8f", e.latDeg) + " " + fmt("%.8f", e.lonDeg) + " " +
                fmt("%.3f", e.altMeters);
            // ENTITY_YPR takes yaw pitch roll. CIGI sends roll pitch yaw. An
            // aircraft with those two swapped rolls when it should pitch, which
            // a level capture never shows.
            const std::string ypr =
                fmt("%.4f", e.yawDeg) + " " + fmt("%.4f", e.pitchDeg) + " " +
                fmt("%.4f", e.rollDeg);
            auto& last = lastPose[e.entityId];
            if (last.first != pos) {
                chunk += "ENTITY_POS " + std::to_string(e.entityId) + " " + pos + "\n";
                last.first = pos;
                rep.poseCommands++;
            }
            if (last.second != ypr) {
                chunk += "ENTITY_YPR " + std::to_string(e.entityId) + " " + ypr + "\n";
                last.second = ypr;
                rep.poseCommands++;
            }
        }

        if (!chunk.empty()) {
            if (skip) {
                if (options.frameWaitCompression) {
                    out += "FRAME_WAIT " + std::to_string(skip) + "\n";
                    rep.frameWaitFrames += skip;
                } else {
                    for (unsigned i = 0; i < skip; ++i) {
                        out += "FRAME\n";
                        rep.frameCommands++;
                    }
                }
                skip = 0;
            }
            out += chunk;
            out += "FRAME\n";
            rep.frameCommands++;
        } else {
            ++skip;
        }
    }

    // Frames at the end that said nothing still have to pass, or the last pose
    // would be the end of the scenario.
    if (skip) {
        if (options.frameWaitCompression) {
            out += "FRAME_WAIT " + std::to_string(skip) + "\n";
            rep.frameWaitFrames += skip;
        } else {
            for (unsigned i = 0; i < skip; ++i) {
                out += "FRAME\n";
                rep.frameCommands++;
            }
        }
    }

    return out;
}

//-----------------------------------------------------------------------------
// The operator-facing summary
//-----------------------------------------------------------------------------

std::string formatReport(const Report& rep, const std::string& source) {
    std::string s;
    s += "BinToScenario: " + source + "\n";
    s += "  " + std::to_string(rep.datagrams) + " datagrams, " +
         std::to_string(rep.bytes) + " payload bytes, span " +
         fmt("%.3f", rep.totalMicros / 1e6) + " s\n";

    s += "  entities rendered: ";
    if (rep.entityIds.empty()) {
        s += "(none)\n";
    } else {
        for (size_t i = 0; i < rep.entityIds.size(); ++i) {
            if (i) s += " ";
            s += std::to_string(rep.entityIds[i]);
        }
        s += "\n";
    }

    s += "  messages:\n";
    for (const auto& kv : rep.idCounts) {
        s += "    ";
        s += isTranslated(kv.first) ? '+' : '-';
        s += ' ';
        s += padRight(std::to_string(kv.first), 4);
        s += padRight(messageName(kv.first), 30);
        s += std::to_string(kv.second);
        s += '\n';
    }

    s += "  wrote " + std::to_string(rep.sceneCommands) + " scene, " +
         std::to_string(rep.poseCommands) + " pose, " +
         std::to_string(rep.frameCommands) + " FRAME, " +
         std::to_string(rep.frameWaitFrames) + " frame(s) in FRAME_WAIT -> " +
         std::to_string(rep.frames()) + " frames total\n";

    for (const auto& kv : rep.idCounts) {
        if (!isTranslated(kv.first)) {
            s += "  note: '-' messages were seen but not translated (see README.md)\n";
            break;
        }
    }
    return s;
}

std::string scriptPathFor(const std::string& binPath) {
    const size_t slash = binPath.find_last_of("/\\");
    const size_t dot   = binPath.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return binPath + ".scp";
    }
    return binPath.substr(0, dot) + ".scp";
}

} // namespace binscp
