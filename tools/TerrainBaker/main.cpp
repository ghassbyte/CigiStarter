//=============================================================================
//  TerrainBaker.cpp — build CigiStarter's HAT/HOT terrain database from the sim
//
//  Why this exists
//    CigiStarter can answer HAT/HOT from an offline SRTM ".hgt" tile set (see
//    TerrainDb.h), but SRTM is a generic world model: it has no idea that a
//    particular strip of ground was levelled by Prepar3D for a runway. The
//    HOST drives the aircraft to whatever Height Of Terrain the IG reports, so
//    at a flattened field a generic SRTM height that is a few metres out is a
//    few metres of aircraft flying into or floating over the pavement.
//
//    The only thing that knows the *sim's* ground -- runway flattening
//    included -- is the simulator itself. This tool asks it. It walks a grid of
//    points over the areas that matter, reads Prepar3D's own ground altitude at
//    each through FSUIPC, and writes the result back out as an ordinary SRTM
//    ".hgt" tile: SRTM everywhere, sim-exact where it sampled. TerrainDb reads
//    the output unchanged.
//
//    So there are two ways to fill "terrain\":
//      1. plain   -- tools/get_terrain.ps1, generic SRTM, no sim needed.
//      2. baked   -- this tool: SRTM base + sim-exact patches at the fields.
//    Option 2 builds on option 1: the SRTM tile is the base it overlays.
//
//  Three modes
//    probe    sample a handful of well-spread points and print sim vs SRTM.
//             Run this first: it proves the FSUIPC link and, crucially, whether
//             the sim's ground model follows a reposition (see --slew).
//    survey   sweep every grid point that falls inside the chosen boxes,
//             appending each reading to "<out>\<tile>.samples.csv".
//             Interruptible and resumable -- re-running skips what is already
//             in the CSV.
//    compose  fold the base SRTM tile and the survey CSV into "<out>\<tile>.hgt".
//             Needs no sim, so it can be run on any machine.
//
//  The ground model and --slew
//    FSUIPC's 0x0020 is "ground altitude under the player aircraft". Reading it
//    for a *specific* cell only means anything if the position write has taken
//    effect first, and slew mode decides that. Measured on Prepar3D v5:
//      --slew on   the writes land, the ground under the requested cell comes
//                  back, and the reading repeats to ~0.05 m at any settle >= 25 ms.
//      --slew off  the writes are mostly not applied: the number still changes
//                  from point to point, but it belongs to wherever the aircraft
//                  already was, and drifts by up to ~8 m along a survey row.
//    So neither is assumed: probe reports which setting tracks (values change)
//    and repeats (points revisited), and is worth running on the machine that
//    will do the bake -- it is a property of the sim, not of this tool. The
//    default is --slew on, because in slew the aircraft is held at the written
//    coordinate, so 0x0020 is that cell's ground by construction; --slew off is
//    kept for a sim where slew freezes the ground model instead.
//
//  Usage: TerrainBaker <probe|survey|compose|plan> [options]   (see --help)
//=============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "FSUIPC_User64.h"
#include "../../../CigiP3dBridge/src/Conversion.h"   // same fixed-point maths as the bridge

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// FSUIPC offsets. Kept identical to CigiP3dBridge/src/FsuipcOffsets.h and
// CigiStarter/TerrainSource.cpp so all three programs agree on the layout.
// ---------------------------------------------------------------------------
constexpr DWORD kGroundAltOffset = 0x0020;  // 4 bytes, signed, ground altitude m x 256
constexpr DWORD kLatitudeOffset  = 0x0560;  // 8 bytes, SINT64
constexpr DWORD kLongitudeOffset = 0x0568;  // 8 bytes, SINT64
constexpr DWORD kAltitudeLow     = 0x0570;  // 4 bytes, UINT32, fractional metres
constexpr DWORD kAltitudeHigh    = 0x0574;  // 4 bytes, INT32,  integer metres
constexpr DWORD kPitchOffset     = 0x0578;  // 4 bytes, INT32
constexpr DWORD kBankOffset      = 0x057C;  // 4 bytes, INT32
constexpr DWORD kHeadingOffset   = 0x0580;  // 4 bytes, UINT32
constexpr DWORD kSlewModeOffset  = 0x05DC;  // 2 bytes, WORD, 1 = slew on
constexpr DWORD kSlewCtlOffset   = 0x05DE;  // 2 bytes, companion write for 0x05DC

// ---------------------------------------------------------------------------
// The two tile products TerrainDb.cpp accepts, and the byte size each gives.
// The bake has to emit one of these or TerrainDb will refuse the file, so the
// resolution choice is really "which of the two shapes".
// ---------------------------------------------------------------------------
constexpr int    kSrtm1      = 3601;   // 1 arc-second (~30 m)
constexpr int    kSrtm3      = 1201;   // 3 arc-second (~90 m)
constexpr size_t kSrtm1Bytes = std::size_t(kSrtm1) * kSrtm1 * 2;
constexpr size_t kSrtm3Bytes = std::size_t(kSrtm3) * kSrtm3 * 2;
constexpr int16_t kVoid      = -32768; // SRTM "no data" marker

volatile bool g_stop = false;

BOOL WINAPI onCtrl(DWORD) {
    g_stop = true;
    std::fputs("\n[stop] finishing the current sample and closing ...\n", stdout);
    return TRUE;
}

// ---------------------------------------------------------------------------
// A one-degree cell, held as the raw grid the .hgt file is: n x n, row 0 the
// northern edge, column 0 the western edge.
// ---------------------------------------------------------------------------
struct Grid {
    int                  lat0 = 0, lon0 = 0, n = 0;
    std::vector<int16_t> h;

    std::size_t idx(int r, int c) const { return std::size_t(r) * n + c; }

    // Grid line -> geographic coordinate (the two edges are on whole degrees).
    double latOfRow(int r) const { return lat0 + 1.0 - double(r) / (n - 1); }
    double lonOfCol(int c) const { return lon0 + double(c) / (n - 1); }

    int rowOfLat(double lat) const {
        return int(std::llround((lat0 + 1.0 - lat) * (n - 1)));
    }
    int colOfLon(double lon) const {
        return int(std::llround((lon - lon0) * (n - 1)));
    }
};

std::string exeFolder() {
    char  buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return ".";
    std::string s(buf, n);
    const std::size_t slash = s.find_last_of("\\/");
    return (slash == std::string::npos) ? std::string(".") : s.substr(0, slash);
}

// "S08E112" -> lat0 = -8, lon0 = 112. Same rule as TerrainDb.cpp.
bool parseTileName(const std::string& stem, int& lat0, int& lon0) {
    if (stem.size() < 7) return false;
    const char ns = char(std::toupper((unsigned char)stem[0]));
    const char ew = char(std::toupper((unsigned char)stem[3]));
    if ((ns != 'N' && ns != 'S') || (ew != 'E' && ew != 'W')) return false;
    static const int kPos[5] = {1, 2, 4, 5, 6};
    for (int i : kPos) {
        if (!std::isdigit((unsigned char)stem[i])) return false;
    }
    const int lat = (stem[1] - '0') * 10 + (stem[2] - '0');
    const int lon = (stem[4] - '0') * 100 + (stem[5] - '0') * 10 + (stem[6] - '0');
    if (lat >= 90 || lon >= 180) return false;
    lat0 = (ns == 'S') ? -lat : lat;
    lon0 = (ew == 'W') ? -lon : lon;
    return true;
}

std::string tileName(int lat0, int lon0) {
    char b[16];
    std::snprintf(b, sizeof(b), "%c%02d%c%03d",
                  lat0 < 0 ? 'S' : 'N', std::abs(lat0),
                  lon0 < 0 ? 'W' : 'E', std::abs(lon0));
    return b;
}

// Read a ".hgt" into `g`. False if the file is missing or not one of the two
// accepted shapes (with `reason` set for the operator).
bool readHgt(const std::string& path, Grid& g, std::string& reason) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { reason = "not found"; return false; }
    const std::streamoff size = f.tellg();
    int n = 0;
    if (std::size_t(size) == kSrtm1Bytes)      n = kSrtm1;
    else if (std::size_t(size) == kSrtm3Bytes) n = kSrtm3;
    else { reason = "unexpected size " + std::to_string(size) + " bytes"; return false; }

    std::vector<uint8_t> raw(static_cast<std::size_t>(size));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char*>(raw.data()), size)) { reason = "read failed"; return false; }

    g.n = n;
    g.h.resize(std::size_t(n) * n);
    for (std::size_t i = 0; i < g.h.size(); ++i) {
        const uint16_t be = uint16_t((raw[i * 2] << 8) | raw[i * 2 + 1]);
        g.h[i] = int16_t(be);
    }
    return true;
}

bool writeHgt(const std::string& path, const Grid& g) {
    std::vector<uint8_t> raw(static_cast<std::size_t>(g.n) * g.n * 2);
    for (std::size_t i = 0; i < g.h.size(); ++i) {
        const uint16_t be = uint16_t(g.h[i]);
        raw[i * 2]     = uint8_t(be >> 8);
        raw[i * 2 + 1] = uint8_t(be & 0xFF);
    }
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(raw.data()), std::streamsize(raw.size()));
    return bool(f);
}

// Bring an SRTM base tile to the resolution being baked. get_terrain.ps1
// fetches the 1 arc-second product, so a 3 arc-second bake normally has to step
// it down -- exactly 3:1, so the sampled cells are the original ones, not an
// average. Stepping *up* (a 3" base for a 1" bake) is refused: it would invent
// detail the base never had, and TerrainDb would answer a finer grid than the
// data supports.
bool toResolution(const Grid& src, int n, Grid& out, std::string& err) {
    if (src.n == n) { out = src; return true; }
    if (src.n < n) {
        err = "base is " + std::to_string(src.n) + "x" + std::to_string(src.n) +
              " but --res " + ((n == kSrtm1) ? "1" : "3") + " needs " +
              std::to_string(n) + "x" + std::to_string(n) +
              "; fetch the 1 arc-second tile with get_terrain.ps1";
        return false;
    }
    out.n    = n;
    out.lat0 = src.lat0;
    out.lon0 = src.lon0;
    out.h.resize(std::size_t(n) * n);
    const double step = double(src.n - 1) / double(n - 1);
    for (int r = 0; r < n; ++r) {
        const int sr = int(std::lround(r * step));
        for (int c = 0; c < n; ++c) {
            const int sc = int(std::lround(c * step));
            out.h[out.idx(r, c)] = src.h[src.idx(sr, sc)];
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Boxes to sample, and the deployment's fields as ready-made ones.
// ---------------------------------------------------------------------------
struct Box { double lat0, lon0, lat1, lon1; };

bool inBox(const Box& b, double lat, double lon) {
    return lat >= b.lat0 && lat <= b.lat1 && lon >= b.lon0 && lon <= b.lon1;
}

// The fields CigiStarter has to be honest about. Coordinates are the ones the
// HOST/Prepar3D work in; `half` is the half-width of the sampled area in
// degrees (~111 km per degree of latitude). Widen it if the HOST's approaches
// start further out; the bake cost is (2*half/cell)^2 per site.
struct Site { const char* name; double lat; double lon; double half; };

const Site kSites[] = {
    // Bogor: Atang Senjaya and the Astina helipad nearby, one box covers both.
    {"atang-senjaya",  -6.5428, 106.7566, 0.10},
    {"halim",          -6.2664, 106.8913, 0.10},
    {"soekarno-hatta", -6.1341, 106.6552, 0.10},
    {"husein",         -6.9000, 107.5761, 0.10},
};

Box siteBox(const Site& s) {
    return Box{s.lat - s.half, s.lon - s.half, s.lat + s.half, s.lon + s.half};
}

// ---------------------------------------------------------------------------
// Options
// ---------------------------------------------------------------------------
struct Options {
    std::string mode;
    std::string baseDir;    // SRTM tiles (get_terrain.ps1 output)
    std::string outDir;     // baked tiles + sample CSVs
    int         res      = 3;      // 1 or 3 arc-seconds
    double      settleMs = 60.0;   // wait after a reposition before reading
    bool        settleGiven = false;  // ...unless the mode picks its own default
    int         repeat   = 1;      // readings per point (last one wins)
    bool        slew     = true;   // pin the aircraft at the sampled coordinate
    double      altitude = 3500.0; // metres to hold the aircraft at while sampling
    bool        resume   = true;
    int         limit    = 0;      // cap on samples this run (0 = no cap)
    bool        allowEmptyBase = false;
    std::vector<Box>   boxes;
    std::vector<std::string> tileArg;   // explicit tiles (survey/compose)
};

std::string tilePath(const std::string& dir, const std::string& tile) {
    return dir + "\\" + tile + ".hgt";
}
std::string csvPath(const std::string& dir, const std::string& tile) {
    return dir + "\\" + tile + ".samples.csv";
}

// ---------------------------------------------------------------------------
// FSUIPC
// ---------------------------------------------------------------------------
bool fsuipcOpen() {
    DWORD dw = 0;
    return FSUIPC_Open(SIM_ANY, &dw) != FALSE;
}

bool writeSlew(bool on) {
    DWORD dw = 0;
    WORD  v  = on ? 1 : 0;
    WORD  c  = 1;
    return FSUIPC_Write(kSlewModeOffset, 2, &v, &dw) != FALSE
        && FSUIPC_Write(kSlewCtlOffset,  2, &c, &dw) != FALSE
        && FSUIPC_Process(&dw) != FALSE;
}

// Survey walks adjacent cells, so a short settle is enough; probe teleports tens
// of kilometres, into scenery that is not resident yet, so it defaults to a long
// one. An explicit --settle-ms always wins.
double settleFor(const Options& o) {
    if (o.settleGiven) return o.settleMs;
    return (o.mode == "probe") ? 400.0 : 60.0;
}

// Inverse of the plain linear scales above, for reading a position back.
constexpr double kLatDegPerFs = 90.0  / (10001750.0 * 65536.0 * 65536.0);
constexpr double kLonDegPerFs = 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);

// Reposition the player aircraft, then read the ground altitude beneath it.
// Separate Process() calls on purpose: the write has to reach the sim before the
// read can be answered for the new position.
//
// A fixed sleep cannot tell us the write landed, and how long it takes depends on
// what the sim is doing -- so the position is read back (in the same round trip
// as the ground) and we wait until it is where we asked. Otherwise the reading
// can belong to the previous cell: a few metres of terrain away, and silently
// wrong. `--settle-ms` is the wait between checks, and `positionErrDeg`, when
// given, receives the residual that was accepted.
bool samplePoint(const Options& o, double lat, double lon, double& groundOut,
                 double* positionErrDeg = nullptr) {
    DWORD dw = 0;

    long long latFs = conv::latitudeToFs(lat);
    long long lonFs = conv::longitudeToFs(lon);
    int         altHi = 0;
    unsigned    altLo = 0;
    conv::altitudeToFs(o.altitude, altHi, altLo);
    DWORD headingFs = conv::headingToFs(0.0);   // face north, wings level
    int   pitchFs   = 0;
    int   bankFs    = 0;

    if (!FSUIPC_Write(kLatitudeOffset,  8, &latFs,    &dw) ||
        !FSUIPC_Write(kLongitudeOffset, 8, &lonFs,    &dw) ||
        !FSUIPC_Write(kAltitudeHigh,    4, &altHi,    &dw) ||
        !FSUIPC_Write(kAltitudeLow,     4, &altLo,    &dw) ||
        !FSUIPC_Write(kHeadingOffset,   4, &headingFs, &dw) ||
        !FSUIPC_Write(kPitchOffset,     4, &pitchFs,   &dw) ||
        !FSUIPC_Write(kBankOffset,      4, &bankFs,    &dw) ||
        !FSUIPC_Process(&dw)) {
        return false;
    }

    constexpr double kTolDeg = 1.0e-5;   // ~1 m: far inside one 3 arc-sec cell
    const int reps = std::max(1, o.repeat);
    for (int attempt = 0; attempt < 20; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(int(settleFor(o))));

        DWORD rw = 0;
        long long gotLatFs = 0, gotLonFs = 0;
        int32_t   groundRaw = 0;
        if (!FSUIPC_Read(kLatitudeOffset,  8, &gotLatFs,  &rw) ||
            !FSUIPC_Read(kLongitudeOffset, 8, &gotLonFs,  &rw) ||
            !FSUIPC_Read(kGroundAltOffset, 4, &groundRaw, &rw) ||
            !FSUIPC_Process(&rw)) {
            return false;
        }

        const double gotLat = double(gotLatFs) * kLatDegPerFs;
        const double gotLon = double(gotLonFs) * kLonDegPerFs;
        const double err    = std::max(std::abs(gotLat - lat), std::abs(gotLon - lon));
        if (err > kTolDeg) continue;   // not there yet; wait and look again

        groundOut = double(groundRaw) / 256.0;
        if (positionErrDeg) *positionErrDeg = err;
        for (int i = 1; i < reps; ++i) {   // further readings, this one already had
            std::this_thread::sleep_for(std::chrono::milliseconds(int(settleFor(o))));
            int32_t raw2 = 0;
            if (!FSUIPC_Read(kGroundAltOffset, 4, &raw2, &rw) || !FSUIPC_Process(&rw)) {
                return false;
            }
            groundOut = double(raw2) / 256.0;
        }
        return true;
    }
    return false;   // never arrived: better to fail than to record the wrong cell
}

// ---------------------------------------------------------------------------
// Sample cache: "<out>\<tile>.samples.csv", one "lat,lon,alt" per reading.
// Appended during survey, replayed by compose, so a multi-hour bake survives an
// interruption and compose stays runnable without the sim.
// ---------------------------------------------------------------------------
struct Samples {
    std::vector<double> lat, lon, alt;
};

Samples loadSamples(const std::string& path) {
    Samples s;
    std::ifstream f(path);
    if (!f) return s;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        double a = 0, b = 0, c = 0;
        if (std::sscanf(line.c_str(), "%lf,%lf,%lf", &a, &b, &c) == 3) {
            s.lat.push_back(a); s.lon.push_back(b); s.alt.push_back(c);
        }
    }
    return s;
}

// Reading the cache is `loadSamples`. Writing it happens in survey, which keeps
// the file open: opening it once per sample would be hundreds of thousands of
// file handles, and -- more importantly -- a failure has to be reported rather
// than swallowed (a survey that cannot write must not look like it succeeded).

// ---------------------------------------------------------------------------
// Modes
// ---------------------------------------------------------------------------

// Which tiles the boxes touch.
std::vector<std::string> tilesForBoxes(const std::vector<Box>& boxes) {
    std::set<std::string> names;
    for (const Box& b : boxes) {
        for (int la = int(std::floor(b.lat0)); la <= int(std::floor(b.lat1)); ++la) {
            for (int lo = int(std::floor(b.lon0)); lo <= int(std::floor(b.lon1)); ++lo) {
                names.insert(tileName(la, lo));
            }
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

// Every grid point of `g` that falls inside some box, in a serpentine order so
// consecutive teleports land next door and the sim never has to page in a new
// patch of mesh.
std::vector<std::pair<int,int>> pointsInBoxes(const Grid& g, const std::vector<Box>& boxes) {
    std::vector<std::pair<int,int>> pts;
    for (int r = 0; r < g.n; ++r) {
        const double lat = g.latOfRow(r);
        const bool fwd = (r % 2) == 0;
        for (int k = 0; k < g.n; ++k) {
            const int    c   = fwd ? k : (g.n - 1 - k);
            const double lon = g.lonOfCol(c);
            for (const Box& b : boxes) {
                if (inBox(b, lat, lon)) { pts.emplace_back(r, c); break; }
            }
        }
    }
    return pts;
}

int modePlan(const Options& o) {
    const auto tiles = tilesForBoxes(o.boxes);
    const int  n     = (o.res == 1) ? kSrtm1 : kSrtm3;
    const double cell = 1.0 / (n - 1);

    int total = 0;
    std::printf("plan: %d arc-sec (%s, ~%.0f m), %.0f ms settle, slew %s\n",
                o.res, (o.res == 1) ? "3601x3601" : "1201x1201",
                cell * 111320.0, settleFor(o), o.slew ? "ON" : "off");
    std::printf("base: %s\nout : %s\n\n", o.baseDir.c_str(), o.outDir.c_str());

    for (const std::string& tile : tiles) {
        int lat0 = 0, lon0 = 0;
        parseTileName(tile, lat0, lon0);
        Grid g;
        g.n = n;
        g.lat0 = lat0;
        g.lon0 = lon0;

        const auto pts = pointsInBoxes(g, o.boxes);

        Grid base;
        std::string reason;
        bool haveBase = readHgt(tilePath(o.baseDir, tile), base, reason);
        if (haveBase) {
            base.lat0 = lat0;
            base.lon0 = lon0;
            Grid tmp;
            std::string err;
            if (!toResolution(base, n, tmp, err)) { haveBase = false; reason = err; }
        }
        Samples done = o.resume ? loadSamples(csvPath(o.outDir, tile)) : Samples();
        const int todo = std::max(0, int(pts.size()) - int(done.lat.size()));

        total += todo;
        std::printf("  %s  base %-14s  points %6d  already %6d  todo %6d\n",
                    tile.c_str(),
                    haveBase ? "ok" : ("MISSING (" + reason + ")").c_str(),
                    int(pts.size()), int(done.lat.size()), todo);
    }

    std::printf("\ntotal points to sample: %d\n", total);
    if (total > 0) {
        const double perSec = std::max(1.0, 1000.0 / settleFor(o));
        std::printf("ceiling from --settle-ms alone: %.0f samples/s, so >= %.1f h;\n"
                    "a real run is slower. Raise or lower --settle-ms to trade speed for safety.\n",
                    perSec, total / perSec / 3600.0);
    }
    std::printf("\nrun:  TerrainBaker survey  (same options)  then  TerrainBaker compose\n");
    return 0;
}

int modeProbe(const Options& o) {
    if (!fsuipcOpen()) {
        std::fprintf(stderr, "[probe] FSUIPC_Open failed: is Prepar3D running, with FSUIPC6 loaded?\n");
        return 2;
    }
    if (!writeSlew(o.slew)) {
        std::fprintf(stderr, "[probe] could not set slew mode\n");
        return 2;
    }
    if (!o.slew) {
        std::fprintf(stderr, "[probe] warning: --slew off leaves the aircraft free-flying, so a reading\n"
                             "        can belong to wherever it drifted to, not the requested point.\n");
    }

    std::printf("probe: slew %s, holding %.0f m, settling %.0f ms\n\n",
                o.slew ? "ON" : "off", o.altitude, settleFor(o));

    // Centre plus the four corners of each box, so consecutive probes are far
    // apart: if the ground model is frozen the values will not move at all.
    std::vector<std::pair<double,double>> pts;
    for (const Box& b : o.boxes) {
        const double e = 0.05 * (b.lat1 - b.lat0);
        pts.emplace_back((b.lat0 + b.lat1) / 2, (b.lon0 + b.lon1) / 2);
        pts.emplace_back(b.lat0 + e, b.lon0 + e);
        pts.emplace_back(b.lat0 + e, b.lon1 - e);
        pts.emplace_back(b.lat1 - e, b.lon0 + e);
        pts.emplace_back(b.lat1 - e, b.lon1 - e);
    }

    std::set<double> distinct;
    std::vector<std::pair<double,double>> taken;
    std::vector<double>                   takenSim;
    int  shown = 0;
    for (const auto& p : pts) {
        if (o.limit > 0 && shown >= o.limit) break;
        double sim = 0.0, posErr = 0.0;
        if (!samplePoint(o, p.first, p.second, sim, &posErr)) {
            std::fprintf(stderr, "[probe] sample failed at %.5f,%.5f -- the aircraft never\n"
                                 "        reached the requested position\n", p.first, p.second);
            return 2;
        }
        // The SRTM reading at the same spot, for reference only: the point is
        // to see the sim's own number move.
        int  lat0 = 0, lon0 = 0;
        const std::string tile = tileName(int(std::floor(p.first)), int(std::floor(p.second)));
        parseTileName(tile, lat0, lon0);
        const int targetN = (o.res == 1) ? kSrtm1 : kSrtm3;
        Grid raw;
        std::string reason;
        double base = 0.0;
        bool   haveBase = false;
        if (readHgt(tilePath(o.baseDir, tile), raw, reason)) {
            raw.lat0 = lat0;
            raw.lon0 = lon0;
            Grid g;
            std::string err;
            if (toResolution(raw, targetN, g, err)) {
                const int r = std::clamp(g.rowOfLat(p.first), 0, g.n - 1);
                const int c = std::clamp(g.colOfLon(p.second), 0, g.n - 1);
                const int16_t v = g.h[g.idx(r, c)];
                base = double(v);
                haveBase = (v != kVoid);
            }
        }
        std::printf("  %9.5f %9.5f   sim %8.2f m   pos %4.1f m   srtm %s\n",
                    p.first, p.second, sim, posErr * 111320.0,
                    haveBase ? (std::to_string(int(base)) + " m   d " +
                                std::to_string(int(std::lround(sim - base)))).c_str()
                             : "n/a");
        distinct.insert(std::round(sim * 16.0) / 16.0);   // ~6 cm buckets
        taken.emplace_back(p.first, p.second);
        takenSim.push_back(sim);
        ++shown;
    }

    // A frozen value is only one way to be wrong. If the position writes are not
    // being applied at all, the ground model still "moves" -- it just reports
    // wherever the aircraft really is (the previous point, or wherever it
    // drifted to), which fakes its way past the distinct-values check above.
    // Re-visiting points settles it: the same coordinates must give the same
    // ground. Measured spread when the writes take effect is ~0.05 m.
    double worst = 0.0;
    int    checked = 0;
    const int n = int(taken.size());
    const int revisits[3] = {0, n / 2, n - 1};
    for (int k = 0; k < 3 && n > 1; ++k) {
        const int i = revisits[k];
        if (i < 0 || i >= n || (k > 0 && i == revisits[k - 1])) continue;
        double again = 0.0;
        if (!samplePoint(o, taken[i].first, taken[i].second, again)) {
            std::fprintf(stderr, "[probe] revisit failed at %.5f,%.5f\n",
                         taken[i].first, taken[i].second);
            FSUIPC_Close();
            return 2;
        }
        const double d = std::abs(again - takenSim[i]);
        worst = std::max(worst, d);
        ++checked;
        std::printf("  revisit %9.5f %9.5f   first %8.2f m   again %8.2f m   d %.2f m\n",
                    taken[i].first, taken[i].second, takenSim[i], again, d);
    }

    writeSlew(false);   // hand the sim back in normal flight, not sampling mode

    std::printf("\n%d distinct sim values across %d points; %d revisited, worst |d| %.2f m\n",
                int(distinct.size()), shown, checked, worst);
    if (shown > 1 && distinct.size() == 1) {
        std::printf("VERDICT: the ground model did NOT follow the reposition "
                    "(frozen). Try --slew off.\n");
        FSUIPC_Close();
        return 1;
    }
    if (checked > 0 && worst > 0.5) {
        std::printf("VERDICT: readings do not repeat at fixed coordinates "
                    "(worst %.2f m), so the position writes are not taking effect --\n"
                    "         the numbers above are from where the aircraft actually was.\n"
                    "         Try the other --slew setting; do not survey like this.\n", worst);
        FSUIPC_Close();
        return 1;
    }
    std::printf("VERDICT: the ground model tracked the reposition and readings repeat "
                "at fixed coordinates.\n"
                "         Survey with these settings is sound.\n");
    FSUIPC_Close();
    return 0;
}

int modeSurvey(Options o) {
    if (!fsuipcOpen()) {
        std::fprintf(stderr, "[survey] FSUIPC_Open failed: is Prepar3D running, with FSUIPC6 loaded?\n");
        return 2;
    }
    if (!writeSlew(o.slew)) {
        std::fprintf(stderr, "[survey] could not set slew mode\n");
        return 2;
    }
    if (!o.slew) {
        std::fprintf(stderr, "[survey] warning: --slew off leaves the aircraft free-flying, so a reading\n"
                             "         can belong to wherever it drifted to, not the requested cell.\n");
    }

    const auto tiles = tilesForBoxes(o.boxes);
    const int  n     = (o.res == 1) ? kSrtm1 : kSrtm3;
    int  totalTaken = 0;
    bool limited    = false;
    bool failed     = false;
    double worstPos = 0.0;   // worst residual between requested and reached position

    // Create it up front: a survey that cannot write its samples must say so
    // now, not after an hour of sampling.
    std::error_code ec;
    std::filesystem::create_directories(o.outDir, ec);

    for (const std::string& tile : tiles) {
        if (g_stop) break;

        int lat0 = 0, lon0 = 0;
        parseTileName(tile, lat0, lon0);

        Grid g;
        g.n    = n;
        g.lat0 = lat0;
        g.lon0 = lon0;

        const auto pts = pointsInBoxes(g, o.boxes);
        const std::string csv = csvPath(o.outDir, tile);

        // Resume: anything already in the CSV is skipped.
        Samples done = o.resume ? loadSamples(csv) : Samples();
        if (!o.resume && std::filesystem::exists(csv)) {
            std::filesystem::remove(csv);
        }

        // Hold the file open for the whole tile: one open per sample is slow,
        // and -- more to the point -- a write that fails has to stop the run
        // rather than vanish. A survey that cannot record its samples must not
        // look like it succeeded.
        std::ofstream sink(csv, std::ios::app);
        if (!sink) {
            std::fprintf(stderr, "[%s] cannot append to %s -- stopping\n", tile.c_str(), csv.c_str());
            failed = true;
            g_stop = true;
            break;
        }
        std::vector<uint8_t> have(std::size_t(g.n) * g.n, 0);
        for (std::size_t i = 0; i < done.lat.size(); ++i) {
            const int r = g.rowOfLat(done.lat[i]);
            const int c = g.colOfLon(done.lon[i]);
            if (r >= 0 && r < g.n && c >= 0 && c < g.n) have[g.idx(r, c)] = 1;
        }
        const int already = int(done.lat.size());

        std::printf("[%s] %d points in the boxes, %d already sampled\n",
                    tile.c_str(), int(pts.size()), already);

        const auto t0  = std::chrono::steady_clock::now();
        int        got = 0;
        for (std::size_t i = 0; i < pts.size() && !g_stop; ++i) {
            const int r = pts[i].first, c = pts[i].second;
            if (have[g.idx(r, c)]) continue;
            if (o.limit > 0 && totalTaken >= o.limit) { g_stop = true; limited = true; break; }

            const double lat = g.latOfRow(r);
            const double lon = g.lonOfCol(c);
            double ground = 0.0, posErr = 0.0;
            if (!samplePoint(o, lat, lon, ground, &posErr)) {
                std::fprintf(stderr, "[%s] sample failed at %.6f,%.6f -- the aircraft never reached\n"
                                     "        the requested position -- stopping\n",
                             tile.c_str(), lat, lon);
                g_stop = true;
                break;
            }
            worstPos = std::max(worstPos, posErr);
            {
                char line[96];
                std::snprintf(line, sizeof(line), "%.8f,%.8f,%.2f\n", lat, lon, ground);
                sink << line;
                sink.flush();
                if (!sink) {
                    std::fprintf(stderr, "[%s] write failed at %.6f,%.6f -- stopping\n",
                                 tile.c_str(), lat, lon);
                    failed = true;
                    g_stop = true;
                    break;
                }
            }
            have[g.idx(r, c)] = 1;
            ++got;
            ++totalTaken;

            if ((got % 25) == 0) {
                const double secs = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
                const double rate = (secs > 0.0) ? got / secs : 0.0;
                const int    left = int(pts.size()) - already - got;
                const double eta  = (rate > 0.0) ? left / rate / 60.0 : 0.0;
                std::printf("  %6d/%6d  %5.1f%%  %5.1f/s  ETA %5.1f min\n",
                            already + got, int(pts.size()),
                            100.0 * (already + got) / std::max<std::size_t>(1, pts.size()),
                            rate, eta);
            }
        }
        std::printf("[%s] wrote %d new sample(s) to %s\n", tile.c_str(), got, csv.c_str());
    }

    writeSlew(false);   // hand the sim back in normal flight, not sampling mode
    FSUIPC_Close();
    if (totalTaken > 0) {
        std::printf("\nposition: worst residual %.1f m over %d sample(s)\n",
                    worstPos * 111320.0, totalTaken);
    }
    const char* why = failed  ? "FAILED -- "
                    : limited ? "limit reached -- "
                    : g_stop  ? "interrupted -- "
                              : "";
    std::printf("\n%ssurvey done (%d samples this run). Now run: TerrainBaker compose\n",
                why, totalTaken);
    return (failed || (g_stop && !limited)) ? 1 : 0;
}

int modeCompose(const Options& o) {
    const int n = (o.res == 1) ? kSrtm1 : kSrtm3;

    std::error_code ec;
    std::filesystem::create_directories(o.outDir, ec);

    int rc = 0;
    for (const std::string& tile : o.tileArg) {
        int lat0 = 0, lon0 = 0;
        if (!parseTileName(tile, lat0, lon0)) {
            std::fprintf(stderr, "[compose] '%s' is not a tile name like S07E106\n", tile.c_str());
            rc = 2;
            continue;
        }

        Grid g;
        std::string reason;
        Grid base;
        const bool haveBase = readHgt(tilePath(o.baseDir, tile), base, reason);
        if (!haveBase) {
            if (!o.allowEmptyBase) {
                std::fprintf(stderr,
                    "[compose] %s: no base tile (%s).\n"
                    "          Fetch it first:  tools\\get_terrain.ps1 -Tiles %s\n"
                    "          (or pass --allow-empty-base to emit only the sampled cells)\n",
                    tile.c_str(), reason.c_str(), tile.c_str());
                rc = 2;
                continue;
            }
            g.n    = n;
            g.lat0 = lat0;
            g.lon0 = lon0;
            g.h.assign(std::size_t(n) * n, kVoid);
            std::fprintf(stderr, "[compose] %s: no base tile -- output will be void "
                                 "outside the sampled area\n", tile.c_str());
        } else {
            base.lat0 = lat0;
            base.lon0 = lon0;
            std::string err;
            if (!toResolution(base, n, g, err)) {
                std::fprintf(stderr, "[compose] %s: %s\n", tile.c_str(), err.c_str());
                rc = 2;
                continue;
            }
        }

        const Samples s = loadSamples(csvPath(o.outDir, tile));
        std::size_t applied = 0, outOfTile = 0;
        long long   sum = 0; int maxAbs = 0; int nc = 0;
        for (std::size_t i = 0; i < s.lat.size(); ++i) {
            const int r = g.rowOfLat(s.lat[i]);
            const int c = g.colOfLon(s.lon[i]);
            if (r < 0 || r >= g.n || c < 0 || c >= g.n) { ++outOfTile; continue; }

            const int16_t before = g.h[g.idx(r, c)];
            int v = int(std::lround(s.alt[i]));
            v = std::clamp(v, -32767, 32767);
            g.h[g.idx(r, c)] = int16_t(v);
            ++applied;

            if (before != kVoid) {          // how far the sim is from generic SRTM
                const int d = v - int(before);
                sum += d;
                maxAbs = std::max(maxAbs, std::abs(d));
                ++nc;
            }
        }

        const std::string outTile = tilePath(o.outDir, tile);
        if (!writeHgt(outTile, g)) {
            std::fprintf(stderr, "[compose] %s: could not write %s\n", tile.c_str(), outTile.c_str());
            rc = 2;
            continue;
        }

        std::printf("%s: %zu sample(s) applied%s -> %s\n", tile.c_str(), applied,
                    outOfTile ? (" (" + std::to_string(outOfTile) + " outside the tile)").c_str() : "",
                    outTile.c_str());
        if (nc > 0) {
            std::printf("   sim vs SRTM: mean %+.2f m, |max| %d m over %d sampled cell(s)\n",
                        double(sum) / nc, maxAbs, nc);
        }
        std::printf("   coverage: %.2f%% of the tile is sim-exact\n",
                    100.0 * double(applied) / double(std::size_t(g.n) * g.n));
    }
    return rc;
}

void usage() {
    std::printf(
        "TerrainBaker -- sim-exact terrain for CigiStarter's HAT/HOT database\n"
        "\n"
        "usage: TerrainBaker <mode> [options]\n"
        "\n"
        "modes:\n"
        "  plan      print which tiles/points the options would bake (no sim needed)\n"
        "  probe     sample a few points and report whether the sim's ground model\n"
        "            follows the reposition and repeats at fixed coordinates;\n"
        "            run this before a long survey\n"
        "  survey    walk every grid point in the boxes, appending to <out>\\<tile>.samples.csv\n"
        "  compose   fold base SRTM + the sample CSV into <out>\\<tile>.hgt (no sim needed)\n"
        "\n"
        "options:\n"
        "  --base <dir>       folder of SRTM \".hgt\" tiles to build on\n"
        "                     (default: <exeDir>\\terrain, i.e. get_terrain.ps1's output)\n"
        "  --out <dir>        where baked tiles and sample CSVs go (default <exeDir>\\terrain-sim)\n"
        "  --res 1|3          arc-seconds: 1 = 3601x3601 ~30 m, 3 = 1201x1201 ~90 m (default 3)\n"
        "                     (a 1 arc-second base is stepped down to 3 automatically)\n"
        "  --site <name>      add a deployment field's box; repeatable. Names:\n"
        "                       atang-senjaya halim soekarno-hatta husein\n"
        "                     (with none of --site/--box, all of them are used)\n"
        "  --box a,b,c,d      add an explicit box: lat0,lon0,lat1,lon1; repeatable\n"
        "  --tile <name>      for compose, the tile(s) to assemble; repeatable\n"
        "  --settle-ms <ms>   wait between position checks (default 60; probe uses 400)\n"
        "  --repeat <n>       readings per point, last wins (default 1)\n"
        "  --slew on|off      slew while repositioning (default on: holds the aircraft at\n"
        "                     the sampled coordinate). Probe both; measured on P3D v5,\n"
        "                     slew OFF does not apply the position writes.\n"
        "  --altitude <m>     altitude to hold while sampling (default 3500)\n"
        "  --no-resume        ignore the sample CSV and start the tile over\n"
        "  --limit <n>        stop after n samples this run (smoke test)\n"
        "  --allow-empty-base emit a tile from samples alone when no SRTM base is present\n"
        "  --help\n"
        "\n"
        "typical run:\n"
        "  TerrainBaker plan\n"
        "  TerrainBaker probe                       (Prepar3D running, nothing else flying it)\n"
        "  TerrainBaker survey\n"
        "  TerrainBaker compose\n"
        "  copy <out>\\*.hgt over terrain\\   (or point CigiStarter at it: --terrain-dir)\n");
}

bool parseArgs(int argc, char** argv, Options& o) {
    o.baseDir = exeFolder() + "\\terrain";
    o.outDir  = exeFolder() + "\\terrain-sim";

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) { std::fprintf(stderr, "%s needs a value\n", what); return nullptr; }
            return argv[++i];
        };

        if (a == "--help" || a == "-h") { usage(); std::exit(0); }
        else if (a == "--base")   { const char* v = next("--base");   if (!v) return false; o.baseDir = v; }
        else if (a == "--out")    { const char* v = next("--out");    if (!v) return false; o.outDir  = v; }
        else if (a == "--res")    { const char* v = next("--res");    if (!v) return false; o.res = std::atoi(v); }
        else if (a == "--settle-ms") { const char* v = next("--settle-ms"); if (!v) return false; o.settleMs = std::atof(v); o.settleGiven = true; }
        else if (a == "--repeat") { const char* v = next("--repeat"); if (!v) return false; o.repeat = std::atoi(v); }
        else if (a == "--altitude") { const char* v = next("--altitude"); if (!v) return false; o.altitude = std::atof(v); }
        else if (a == "--limit")  { const char* v = next("--limit");  if (!v) return false; o.limit = std::atoi(v); }
        else if (a == "--tile")   { const char* v = next("--tile");   if (!v) return false; o.tileArg.push_back(v); }
        else if (a == "--allow-empty-base") { o.allowEmptyBase = true; }
        else if (a == "--no-resume") { o.resume = false; }
        else if (a == "--slew") {
            const char* v = next("--slew");
            if (!v) return false;
            const std::string s = v;
            if (s == "on") o.slew = true;
            else if (s == "off") o.slew = false;
            else { std::fprintf(stderr, "--slew takes on or off\n"); return false; }
        }
        else if (a == "--site") {
            const char* v = next("--site");
            if (!v) return false;
            bool found = false;
            for (const Site& s : kSites) {
                if (std::strcmp(v, s.name) == 0) { o.boxes.push_back(siteBox(s)); found = true; break; }
            }
            if (!found) {
                std::fprintf(stderr, "unknown --site '%s'; try one of:", v);
                for (const Site& s : kSites) std::fprintf(stderr, " %s", s.name);
                std::fprintf(stderr, "\n");
                return false;
            }
        }
        else if (a == "--box") {
            const char* v = next("--box");
            if (!v) return false;
            double a0 = 0, b0 = 0, c0 = 0, d0 = 0;
            if (std::sscanf(v, "%lf,%lf,%lf,%lf", &a0, &b0, &c0, &d0) != 4) {
                std::fprintf(stderr, "--box wants lat0,lon0,lat1,lon1\n");
                return false;
            }
            o.boxes.push_back(Box{std::min(a0, c0), std::min(b0, d0),
                                  std::max(a0, c0), std::max(b0, d0)});
        }
        else if (o.mode.empty() && a.size() && a[0] != '-') { o.mode = a; }
        else {
            std::fprintf(stderr, "unknown option '%s' (try --help)\n", a.c_str());
            return false;
        }
    }

    if (o.mode.empty()) { usage(); return false; }
    if (o.res != 1 && o.res != 3) {
        std::fprintf(stderr, "--res must be 1 or 3\n");
        return false;
    }
    if (o.boxes.empty()) {
        for (const Site& s : kSites) o.boxes.push_back(siteBox(s));
    }
    if (o.tileArg.empty() && (o.mode == "survey" || o.mode == "compose")) {
        o.tileArg = tilesForBoxes(o.boxes);
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    SetConsoleCtrlHandler(onCtrl, TRUE);

    Options o;
    if (!parseArgs(argc, argv, o)) return 2;

    if (o.mode == "plan")    return modePlan(o);
    if (o.mode == "probe")   return modeProbe(o);
    if (o.mode == "survey")  return modeSurvey(o);
    if (o.mode == "compose") return modeCompose(o);

    std::fprintf(stderr, "unknown mode '%s' (try --help)\n", o.mode.c_str());
    return 2;
}
