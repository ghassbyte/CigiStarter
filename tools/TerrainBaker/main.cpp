//=============================================================================
//  TerrainBaker.cpp — build CigiStarter's HAT/HOT terrain database from the sim
//
//  Why this exists
//    CigiStarter answers HAT/HOT from Prepar3D's own terrain at run time (see
//    TerrainProbe.h). This tool exists for the IG PCs where it cannot: it copies
//    the sim's ground out into an ordinary SRTM ".hgt" tile, so TerrainDb can
//    answer from it when the add-on is not loaded.
//
//    A plain SRTM tile is not enough on its own. SRTM is a generic world model
//    and has no idea that a particular strip of ground was levelled by Prepar3D
//    for a runway. The HOST drives the aircraft to whatever Height Of Terrain
//    the IG reports, so at a flattened field a generic SRTM height that is a few
//    metres out is a few metres of aircraft flying into or floating over the
//    pavement. So the tile this tool writes is SRTM everywhere, sim-exact where
//    it sampled.
//
//    So there are two ways to fill "terrain\":
//      1. plain   -- tools/get_terrain.ps1, generic SRTM, no sim needed.
//      2. baked   -- this tool: SRTM base + sim-exact patches at the fields.
//    Option 2 builds on option 1: the SRTM tile is the base it overlays.
//
//  How it samples
//    Through the same add-on CigiStarter uses: PdkTerrainProbe.dll, loaded
//    inside Prepar3D, answers points from the sim's own surface query and
//    publishes them in shared memory (see TerrainProbeIpc.h). This tool fills
//    that table a batch at a time -- 256 points per simulation tick, at 60 Hz --
//    so a whole tile takes seconds.
//
//    It deliberately does NOT use FSUIPC. The earlier version walked the grid
//    one cell at a time, writing the aircraft's position over FSUIPC and reading
//    0x0020 back, which took hours per tile and depended on the sim's own mode
//    (it froze while the sim was held in slew). The surface query is answered
//    about the *world* rather than about where the aircraft is, so there is
//    nothing to reposition, nothing to hold still, and no mode to get right.
//
//  Three modes
//    probe    sample a handful of well-spread points and print sim vs SRTM.
//             Run this first: it proves the add-on is loaded and answering, and
//             shows how far the bake will move the ground at each field.
//    survey   walk every grid point that falls inside the chosen boxes,
//             appending each reading to "<out>\<tile>.samples.csv".
//             Interruptible and resumable -- re-running skips what is already
//             in the CSV.
//    compose  fold the base SRTM tile and the survey CSV into "<out>\<tile>.hgt".
//             Needs no sim, so it can be run on any machine.
//
//  Usage: TerrainBaker <probe|survey|compose|plan> [options]   (see --help)
//=============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include "../../TerrainProbeIpc.h"   // the shared-memory link to PdkTerrainProbe.dll

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
    double      altitude = 3500.0; // altitude the surface is asked about from, m
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
// The link to Prepar3D's own terrain
//
// PdkTerrainProbe.dll runs inside the sim and answers every enabled slot of the
// shared table from Prepar3D's own surface query on each simulation tick (see
// TerrainProbeIpc.h). Filling the table is what makes a bake fast: the query is
// answered about the *world*, at the coordinate asked for, so there is no
// aircraft to reposition, no slew mode to get right and no settling to wait for
// -- the earlier FSUIPC sweep needed all three, one cell at a time.
// ---------------------------------------------------------------------------
class ProbeLink {
public:
    ~ProbeLink() { close(); }

    bool open() {
        if (m_shm) return true;
        HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE,
                                      terrain::probe::kMappingName);
        if (!h) {
            m_error = "PdkTerrainProbe.dll is not loaded in Prepar3D "
                      "(no shared region to open)";
            return false;
        }
        void* v = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        if (!v) {
            ::CloseHandle(h);
            m_error = "could not map the probe's shared region";
            return false;
        }
        auto* s = static_cast<terrain::probe::Shared*>(v);
        if (s->magic != terrain::probe::kMagic ||
            s->version != terrain::probe::kVersion ||
            s->capacity != terrain::probe::kMaxQueries) {
            ::UnmapViewOfFile(v);
            ::CloseHandle(h);
            m_error = "the shared region belongs to a different build of the probe"
                      " (deploy the same revision of PdkTerrainProbe.dll and TerrainBaker)";
            return false;
        }
        m_map = h;
        m_shm = s;
        return true;
    }

    void close() {
        if (m_shm) { ::UnmapViewOfFile(m_shm); m_shm = nullptr; }
        if (m_map) { ::CloseHandle(m_map); m_map = nullptr; }
    }

    // The add-on is loaded and its simulation tick is running. Checked by
    // watching the tick counter move, so a DLL that failed to get the surface
    // service -- the usual failure -- is caught here rather than after a run of
    // empty answers.
    bool live() {
        if (!open()) return false;
        if (!m_shm->alive) {
            m_error = "the probe is loaded but has no surface service";
            return false;
        }
        const uint32_t before = m_shm->ticks;
        ::Sleep(250);
        if (m_shm->ticks == before) {
            m_error = describeStoppedTick();
            return false;
        }
        return true;
    }

    const std::string& error() const { return m_error; }

    // A compact readout of the diagnostic counters for the probe mode's header:
    // a healthy baseline to compare a later failing run against.  Empty when the
    // add-on is an older build that leaves the diagnostic word unset.
    std::string summary() {
        if (!open()) return std::string();
        const uint32_t d = m_shm->diag;
        if (!(d & terrain::probe::diag::kPresent)) return std::string();

        const auto f = [d](uint32_t shift, uint32_t mask) {
            return terrain::probe::diag::field(d, shift, mask);
        };
        return "probe: creates " + std::to_string(f(terrain::probe::diag::kCreatesShift, terrain::probe::diag::kCreatesMask)) +
               ", removes " + std::to_string(f(terrain::probe::diag::kRemovesShift, terrain::probe::diag::kRemovesMask)) +
               ", attach fails " + std::to_string(f(terrain::probe::diag::kAttachFailsShift, terrain::probe::diag::kAttachFailsMask)) +
               ", attached " + ((d & terrain::probe::diag::kAttached) ? "yes" : "no") +
               ", host is user " + ((d & terrain::probe::diag::kHostIsUser) ? "yes" : "no") +
               ", tick rate " + std::to_string(m_shm->tickRateHz) + " Hz" +
               ", frames " + std::to_string(m_shm->frameTicks) +
               (m_shm->frameTicks ? " (frame tick)" : " (object tick only)") +
               ", own thread " + ((d & terrain::probe::diag::kOwnThreadTick) ? "yes" : "no") +
               ", sim " + simState() +
               ", query failures " + std::to_string(m_shm->queryFails);
    }

    // Ask about a batch of points; one height and one "answered" flag per point.
    // A point the sim has no surface for comes back with `ok` false, which the
    // caller resolves by leaving that cell to the SRTM base.  `material`, when
    // passed, gets the raw `Result::materialCode` for each point, for a caller
    // that wants to know what kind of surface the sim called it (see surface::*).
    bool sample(const std::vector<std::pair<double,double>>& pts, double altitude,
                std::vector<double>& hot, std::vector<uint8_t>& ok,
                std::vector<uint32_t>* material = nullptr) {
        if (!open()) return false;
        hot.assign(pts.size(), 0.0);
        ok.assign(pts.size(), 0);
        if (material) material->assign(pts.size(), 0u);

        const int cap = terrain::probe::kMaxQueries;
        for (std::size_t base = 0; base < pts.size(); base += std::size_t(cap)) {
            const int count = int(std::min<std::size_t>(cap, pts.size() - base));

            for (int i = 0; i < count; ++i) {
                terrain::probe::Query& q = m_shm->queries[i];
                q.latDeg    = pts[base + i].first;
                q.lonDeg    = pts[base + i].second;
                q.altMeters = altitude;
                q.key       = uint32_t(i);
                q.enabled   = 1;
            }
            for (int i = count; i < cap; ++i) m_shm->queries[i].enabled = 0;
            ++m_shm->reqSeq;

            // Take a completed pass, but only one that read the table after we
            // wrote it -- a pass already in flight answers the previous batch,
            // and the position echoed back says so.
            bool matched = false;
            for (int attempt = 0; attempt < 8 && !matched; ++attempt) {
                if (!waitForPass(2000)) {
                    m_error = "the sim stopped answering";
                    return false;
                }
                matched = true;
                for (int i = 0; i < count; ++i) {
                    double   h    = 0.0;
                    uint32_t st   = 0;
                    uint32_t code = 0;
                    if (!readSlot(unsigned(i), pts[base + i].first,
                                  pts[base + i].second, h, st, code)) {
                        matched = false;   // answered a batch we had replaced
                        break;
                    }
                    hot[base + i] = h;
                    ok[base + i]  = (st == terrain::probe::StatusOk) ? 1 : 0;
                    if (material) (*material)[base + i] = code;
                }
            }
            if (!matched) {
                m_error = "the sim did not answer a batch (the probe may be wedged)";
                return false;
            }
        }
        return true;
    }

private:
    // What the sim has been doing, from the message events the add-on saw.
    // Prepar3D's own ticks stop while the sim is paused -- that is the case the
    // add-on's thread exists for -- so a pause is only the whole story when
    // nothing is ticking at all.
    std::string simState() {
        const uint32_t e = m_shm->simEvents;
        std::string s = (e & terrain::probe::simEvent::kPausedNow) ? "paused" : "running";
        if (e & terrain::probe::simEvent::kSawPause)        s += ", saw pause";
        if (e & terrain::probe::simEvent::kSawUnpause)      s += ", saw unpause";
        if (e & terrain::probe::simEvent::kSawScenarioLoad) s += ", scenario load";
        if (e & terrain::probe::simEvent::kSawVehicleChange) s += ", vehicle change";
        if (e & terrain::probe::simEvent::kSawFrozen)       s += ", frozen";
        return s;
    }

    // Nothing is ticking at all, which now takes all three: the per-frame event,
    // the object's simulation and the add-on's own thread.  Spelling out what
    // Prepar3D was willing to deliver saves a rig trip.
    std::string describeStoppedTick() {
        const uint32_t d = m_shm->diag;
        if (!(d & terrain::probe::diag::kPresent)) {
            return "the probe is loaded but its tick is not running "
                   "(an older add-on with no diagnostics; deploy the current one)";
        }
        const uint32_t creates = terrain::probe::diag::field(
            d, terrain::probe::diag::kCreatesShift, terrain::probe::diag::kCreatesMask);
        const uint32_t removes = terrain::probe::diag::field(
            d, terrain::probe::diag::kRemovesShift, terrain::probe::diag::kRemovesMask);
        const bool paused = (m_shm->simEvents & terrain::probe::simEvent::kPausedNow) != 0;

        if (m_shm->frameTicks == 0) {
            std::string s = "the probe is loaded but no tick is running: ";
            if (m_shm->frameFails > 0) {
                s += "the per-frame event could not be set up (" +
                     std::to_string(m_shm->frameFails) + " failures)";
            } else {
                s += "the per-frame event was registered but has not been delivered";
            }
            return s + (paused ? ", the sim is paused" : "") +
                   ", and the add-on's own thread has not taken over"
                   ". Deploy the current PdkTerrainProbe.dll.";
        }
        return "the probe's per-frame tick has stopped after " + std::to_string(m_shm->frameTicks) +
               " frames" + (paused ? " (sim paused)" : "") +
               ", so Prepar3D's frame loop is not running"
               ", and the add-on's own thread has not taken over"
               " (creates " + std::to_string(creates) +
               ", removes " + std::to_string(removes) + ")";
    }
    // Wait for one whole write pass: the sequence goes odd at its start and back
    // to even at its end.
    bool waitForPass(int ms) {
        const DWORD deadline = ::GetTickCount() + DWORD(ms);
        const auto past = [&] { return LONG(::GetTickCount() - deadline) >= 0; };

        while (m_shm->rspSeq & 1u)     { if (past()) return false; ::Sleep(1); }
        const uint32_t start = m_shm->rspSeq;
        while (m_shm->rspSeq == start) { if (past()) return false; ::Sleep(1); }
        while (m_shm->rspSeq & 1u)     { if (past()) return false; ::Sleep(1); }
        return true;
    }

    // One slot, read under the seqlock. False when the row is not an answer to
    // (lat, lon): the probe has not picked up this batch yet, so the caller
    // should wait for another pass rather than record someone else's height.
    // The doubles are copied through unchanged, hence the exact compare.
    bool readSlot(unsigned i, double lat, double lon, double& hot,
                  uint32_t& status, uint32_t& material) {
        for (int attempt = 0; attempt < 8; ++attempt) {
            const uint32_t before = m_shm->rspSeq;
            if (before & 1u) { ::Sleep(1); continue; }
            const terrain::probe::Result r = m_shm->results[i];
            if (m_shm->rspSeq != before) continue;
            if (r.latDeg != lat || r.lonDeg != lon) return false;
            hot      = r.hotMeters;
            status   = r.status;
            material = r.materialCode;
            return true;
        }
        return false;
    }

    HANDLE                   m_map = nullptr;
    terrain::probe::Shared*  m_shm = nullptr;
    std::string              m_error;
};

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

// Every grid point of `g` that falls inside some box, walked in a serpentine
// order so the CSV reads row by row like a raster and a resumed run continues
// where the last one stopped.
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
    std::printf("plan: %d arc-sec (%s, ~%.0f m)\n",
                o.res, (o.res == 1) ? "3601x3601" : "1201x1201", cell * 111320.0);
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
        const double perSec = 60.0 * terrain::probe::kMaxQueries;
        std::printf("through the probe: up to %.0f samples/s (60 ticks/s x %d slots), "
                    "so about %.1f min,\n"
                    "plus whatever the surface query costs the sim per tick.\n",
                    perSec, terrain::probe::kMaxQueries, total / perSec / 60.0);
    }
    std::printf("\nrun:  TerrainBaker survey  (same options)  then  TerrainBaker compose\n");
    return 0;
}

// The SRTM height at a point, for comparison only; false when there is no base
// tile covering it (or the cell is a "no data" void).
bool srtmAt(const Options& o, double lat, double lon, double& out) {
    const std::string tile = tileName(int(std::floor(lat)), int(std::floor(lon)));
    Grid raw;
    std::string reason;
    if (!readHgt(tilePath(o.baseDir, tile), raw, reason)) return false;
    raw.lat0 = int(std::floor(lat));
    raw.lon0 = int(std::floor(lon));

    Grid g;
    std::string err;
    if (!toResolution(raw, (o.res == 1) ? kSrtm1 : kSrtm3, g, err)) return false;

    const int     r = std::clamp(g.rowOfLat(lat), 0, g.n - 1);
    const int     c = std::clamp(g.colOfLon(lon), 0, g.n - 1);
    const int16_t v = g.h[g.idx(r, c)];
    if (v == kVoid) return false;
    out = double(v);
    return true;
}

// Confirm the add-on is loaded and answering before anything depends on it.
int requireProbe(ProbeLink& link, const char* mode) {
    if (link.live()) return 0;
    std::fprintf(stderr,
                 "[%s] %s\n"
                 "       The bake reads Prepar3D's own terrain through PdkTerrainProbe.dll,\n"
                 "       the same add-on CigiStarter answers HAT/HOT from. Install it and\n"
                 "       restart the sim -- see tools\\PdkTerrainProbe\\README.md.\n"
                 "       (TerrainBaker does not use FSUIPC, so FSUIPC being loaded or not\n"
                 "       makes no difference here.)\n",
                 mode, link.error().c_str());
    return 2;
}

// What the sim called the surface, from `Result::materialCode` (see surface::*
// in TerrainProbeIpc.h), as a short suffix for the probe readout.  Empty for a point the sim
// refused, or for an add-on built before the surface was published.  A
// "platform" here is an object's surface -- a vessel's deck, a rig -- rather
// than terrain, which is the difference between a HAT/HOT answer made from real
// ground and one made from the sea below a deck.
std::string surfaceNote(uint32_t code) {
    if (!(code & terrain::probe::surface::kReported)) return std::string();

    const uint32_t cat = (code >> terrain::probe::surface::kCategoryShift)
                         & terrain::probe::surface::kCategoryMask;
    const char* name = "surface";
    switch (cat) {
    case terrain::probe::surface::kCategoryHard:  name = "hard";  break;
    case terrain::probe::surface::kCategorySoft:  name = "soft";  break;
    case terrain::probe::surface::kCategoryWater: name = "water"; break;
    default: break;
    }
    return std::string(" [") + name +
           ((code & terrain::probe::surface::kOnPlatform) ? ", platform]" : "]");
}

int modeProbe(const Options& o) {
    ProbeLink link;
    if (const int rc = requireProbe(link, "probe")) return rc;

    std::printf("probe: surface queries through PdkTerrainProbe.dll, asked from %.0f m\n\n",
                o.altitude);

    // The add-on's own view of what it is bound to, when this build fills it:
    // a healthy baseline to compare a later failing run against.
    const std::string probeDiag = link.summary();
    if (!probeDiag.empty()) std::printf("%s\n\n", probeDiag.c_str());

    // Centre plus the four corners of each box. Far apart on purpose: it shows
    // the sim really is answering per point rather than handing back one number.
    std::vector<std::pair<double,double>> pts;
    for (const Box& b : o.boxes) {
        const double e = 0.05 * (b.lat1 - b.lat0);
        pts.emplace_back((b.lat0 + b.lat1) / 2, (b.lon0 + b.lon1) / 2);
        pts.emplace_back(b.lat0 + e, b.lon0 + e);
        pts.emplace_back(b.lat0 + e, b.lon1 - e);
        pts.emplace_back(b.lat1 - e, b.lon0 + e);
        pts.emplace_back(b.lat1 - e, b.lon1 - e);
    }
    if (o.limit > 0 && int(pts.size()) > o.limit) pts.resize(std::size_t(o.limit));

    std::vector<double>   sim;
    std::vector<uint8_t>  ok;
    std::vector<uint32_t> material;
    if (!link.sample(pts, o.altitude, sim, ok, &material)) {
        std::fprintf(stderr, "[probe] %s\n", link.error().c_str());
        return 2;
    }

    int    answered = 0;
    int    worstD   = 0;
    double sumAbsD  = 0.0;
    int    compared = 0;
    std::set<int> distinct;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        const bool have = ok[i] != 0;
        if (have) ++answered;

        double base = 0.0;
        const bool haveBase = srtmAt(o, pts[i].first, pts[i].second, base);
        std::string vs;
        if (haveBase && have) {
            const int d = int(std::lround(sim[i] - base));
            sumAbsD += std::abs(sim[i] - base);
            ++compared;
            if (std::abs(d) > std::abs(worstD)) worstD = d;
            vs = std::to_string(int(std::lround(base))) + " m   d " + std::to_string(d);
        } else if (haveBase) {
            vs = std::to_string(int(std::lround(base))) + " m";
        } else {
            vs = "n/a";
        }

        const std::string simText =
            have ? std::to_string(int(std::lround(sim[i]))) + " m" + surfaceNote(material[i])
                 : std::string("no surface");
        std::printf("  %9.5f %9.5f   sim %s   srtm %s\n",
                    pts[i].first, pts[i].second, simText.c_str(), vs.c_str());
        if (have) distinct.insert(int(std::lround(sim[i] * 16.0)));   // ~6 cm buckets
    }

    // The query is stateless, so asking twice must give the same height; a
    // disagreement means the add-on is being fed the wrong point.
    std::vector<double>  again;
    std::vector<uint8_t> againOk;
    double worstRepeat = 0.0;
    if (!link.sample(pts, o.altitude, again, againOk)) {
        std::fprintf(stderr, "[probe] %s\n", link.error().c_str());
        return 2;
    }
    for (std::size_t i = 0; i < pts.size(); ++i) {
        if (ok[i] && againOk[i]) worstRepeat = std::max(worstRepeat, std::abs(again[i] - sim[i]));
    }

    std::printf("\n%d of %d point(s) answered; %d distinct height(s); worst repeat |d| %.2f m\n",
                answered, int(pts.size()), int(distinct.size()), worstRepeat);
    if (answered == 0) {
        std::printf("VERDICT: the add-on is running but answered nothing -- check that\n"
                    "         Prepar3D is not paused and that its scenery is loaded.\n");
        return 1;
    }
    if (answered < int(pts.size())) {
        std::printf("VERDICT: some points had no surface (open water or no data). Those\n"
                    "         cells keep their SRTM height in the bake, which is what you want.\n");
    }
    if (worstRepeat > 0.05) {
        std::printf("VERDICT: repeat queries disagree (%.2f m), so the answers are not\n"
                    "         tied to the points asked. Do not bake with this.\n", worstRepeat);
        return 1;
    }
    std::printf("VERDICT: the sim is answering per point and repeating; sample against\n"
                "         it is sound.%s\n",
                compared > 0 ? " Where SRTM and the sim differ, the bake moves the ground to the sim."
                             : "");
    return 0;
}

int modeSurvey(Options o) {
    ProbeLink link;
    if (const int rc = requireProbe(link, "survey")) return rc;

    const auto tiles = tilesForBoxes(o.boxes);
    const int  n     = (o.res == 1) ? kSrtm1 : kSrtm3;
    int  totalTaken = 0;
    int  totalMiss  = 0;
    bool limited    = false;
    bool failed     = false;

    // Create it up front: a survey that cannot write its samples must say so
    // now, not after a run of sampling.
    std::error_code ec;
    std::filesystem::create_directories(o.outDir, ec);

    std::printf("sampling from Prepar3D through PdkTerrainProbe: %d points per tick, "
                "asked from %.0f m\n\n", terrain::probe::kMaxQueries, o.altitude);

    const int cap = terrain::probe::kMaxQueries;

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

        // Hold the file open for the whole tile: one open per batch is slow,
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
        int        got = 0, missed = 0, batches = 0;

        std::vector<std::pair<double,double>> batch;   // the points asked about
        std::vector<int>                      cell;    // their cell in `have`
        cell.reserve(std::size_t(cap));
        batch.reserve(std::size_t(cap));

        auto flush = [&]() -> bool {
            if (batch.empty()) return true;

            std::vector<double>  hot;
            std::vector<uint8_t> ok;
            if (!link.sample(batch, o.altitude, hot, ok)) {
                std::fprintf(stderr, "[%s] %s -- stopping\n", tile.c_str(), link.error().c_str());
                failed = true;
                g_stop = true;
                return false;
            }

            for (std::size_t i = 0; i < batch.size(); ++i) {
                if (!ok[i]) { ++missed; continue; }   // no surface: the SRTM base keeps the cell
                char line[96];
                std::snprintf(line, sizeof(line), "%.8f,%.8f,%.2f\n",
                              batch[i].first, batch[i].second, hot[i]);
                sink << line;
                have[std::size_t(cell[i])] = 1;
                ++got;
                ++totalTaken;
            }
            sink.flush();
            if (!sink) {
                std::fprintf(stderr, "[%s] write failed at %d sample(s) -- stopping\n",
                             tile.c_str(), got);
                failed = true;
                g_stop = true;
                return false;
            }
            batch.clear();
            cell.clear();

            if ((++batches % 10) == 0) {
                const double secs = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - t0).count();
                const double rate = (secs > 0.0) ? got / secs : 0.0;
                const int    left = int(pts.size()) - already - got;
                const double eta  = (rate > 0.0) ? left / rate / 60.0 : 0.0;
                std::printf("  %6d/%6d  %5.1f%%  %6.0f/s  ETA %5.1f min\n",
                            already + got, int(pts.size()),
                            100.0 * (already + got) / std::max<std::size_t>(1, pts.size()),
                            rate, eta);
            }
            return true;
        };

        for (std::size_t i = 0; i < pts.size() && !g_stop; ++i) {
            const int r = pts[i].first, c = pts[i].second;
            if (have[g.idx(r, c)]) continue;
            if (o.limit > 0 && totalTaken >= o.limit) { g_stop = true; limited = true; break; }

            batch.emplace_back(g.latOfRow(r), g.lonOfCol(c));
            cell.push_back(int(g.idx(r, c)));
            if (int(batch.size()) >= cap && !flush()) break;
        }
        if (!g_stop) flush();

        std::printf("[%s] wrote %d new sample(s) to %s%s\n", tile.c_str(), got, csv.c_str(),
                    missed ? (" (" + std::to_string(missed) + " had no surface; left to SRTM)").c_str()
                           : "");
        totalMiss += missed;
    }

    if (totalMiss > 0) {
        std::printf("\n%d point(s) had no surface in the sim and keep their SRTM height.\n",
                    totalMiss);
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
        "  probe     ask the sim about a few well-spread points and print sim vs SRTM;\n"
        "            run this first to prove the add-on is loaded and answering\n"
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
        "  --altitude <m>     the height the surface is asked about from (default 3500).\n"
        "                     The query is answered for the surface at that lat/lon, so\n"
        "                     a height well above the ground gives the ground itself.\n"
        "  --no-resume        ignore the sample CSV and start the tile over\n"
        "  --limit <n>        stop after n samples this run (smoke test)\n"
        "  --allow-empty-base emit a tile from samples alone when no SRTM base is present\n"
        "  --help\n"
        "\n"
        "The sim side is PdkTerrainProbe.dll, the add-on CigiStarter answers HAT/HOT\n"
        "from (tools\\PdkTerrainProbe\\README.md). No FSUIPC, no slew, no repositioning:\n"
        "the surface query is about the world, so a whole tile takes seconds.\n"
        "\n"
        "typical run:\n"
        "  TerrainBaker plan\n"
        "  TerrainBaker probe                       (Prepar3D running, add-on loaded)\n"
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
        else if (a == "--altitude") { const char* v = next("--altitude"); if (!v) return false; o.altitude = std::atof(v); }
        else if (a == "--limit")  { const char* v = next("--limit");  if (!v) return false; o.limit = std::atoi(v); }
        else if (a == "--tile")   { const char* v = next("--tile");   if (!v) return false; o.tileArg.push_back(v); }
        else if (a == "--allow-empty-base") { o.allowEmptyBase = true; }
        else if (a == "--no-resume") { o.resume = false; }
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
