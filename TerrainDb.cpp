//=============================================================================
//  TerrainDb.cpp — SRTM ".hgt" tile reader feeding the HAT/HOT answer
//
//  See TerrainDb.h for the rationale and the file format. Everything that
//  knows about SRTM lives here; the rest of CigiStarter only asks for a height
//  at a latitude and longitude.
//=============================================================================
#include "TerrainDb.h"

#include <windows.h>            // GetModuleFileNameA -- locate the exe folder

#include <algorithm>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace terrain {
namespace db {
namespace {

// The two SRTM products, and the exact byte size each gives (samples per side
// squared, two bytes per sample). A file that is neither is not a tile.
constexpr int    kSrtm1      = 3601;   // 1 arc-second (~30 m)
constexpr int    kSrtm3      = 1201;   // 3 arc-second (~90 m)
constexpr size_t kSrtm1Bytes = std::size_t(kSrtm1) * kSrtm1 * 2;
constexpr size_t kSrtm3Bytes = std::size_t(kSrtm3) * kSrtm3 * 2;

// SRTM marks a sample with no radar return with this value.
constexpr int16_t kVoid = -32768;

struct Tile {
    int                  n = 0;  // samples per side (kSrtm1 or kSrtm3)
    std::vector<int16_t> h;      // n*n, row 0 = north, column 0 = west
};

struct Entry {
    std::string           path;
    std::unique_ptr<Tile> data;  // null until the tile is first needed
};

std::mutex                          g_mutex;
std::map<std::pair<int,int>, Entry> g_tiles;   // (lat0, lon0) -> tile
double                              g_offset = 0.0;
std::string                         g_dir;
std::string                         g_error;

std::string exeFolder() {
    char  buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) {
        return ".";
    }
    std::string s(buf, n);
    const std::size_t slash = s.find_last_of("\\/");
    return (slash == std::string::npos) ? std::string(".") : s.substr(0, slash);
}

// "S08E112" -> lat0 = -8, lon0 = 112.  Direction letter, two latitude digits,
// direction letter, three longitude digits; case-insensitive.
bool parseTileName(const std::string& stem, int& lat0, int& lon0) {
    if (stem.size() < 7) {
        return false;
    }
    const char ns = static_cast<char>(std::toupper(static_cast<unsigned char>(stem[0])));
    const char ew = static_cast<char>(std::toupper(static_cast<unsigned char>(stem[3])));
    if ((ns != 'N' && ns != 'S') || (ew != 'E' && ew != 'W')) {
        return false;
    }
    static const int kDigitPos[5] = {1, 2, 4, 5, 6};
    for (int i : kDigitPos) {
        if (!std::isdigit(static_cast<unsigned char>(stem[i]))) {
            return false;
        }
    }
    int lat = (stem[1] - '0') * 10 + (stem[2] - '0');
    int lon = (stem[4] - '0') * 100 + (stem[5] - '0') * 10 + (stem[6] - '0');
    if (lat >= 90 || lon >= 180) {   // N90 / E180 are not real tile corners
        return false;
    }
    lat0 = (ns == 'S') ? -lat : lat;
    lon0 = (ew == 'W') ? -lon : lon;
    return true;
}

// Read one tile into memory, converting its big-endian samples. Called with
// g_mutex held.
bool loadTile(const std::string& path, Tile& out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        return false;
    }
    const std::streamoff size = f.tellg();
    int n = 0;
    if (static_cast<std::size_t>(size) == kSrtm1Bytes) {
        n = kSrtm1;
    } else if (static_cast<std::size_t>(size) == kSrtm3Bytes) {
        n = kSrtm3;
    } else {
        return false;
    }

    std::vector<uint8_t> raw(static_cast<std::size_t>(size));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char*>(raw.data()), size)) {
        return false;
    }

    out.n = n;
    out.h.resize(static_cast<std::size_t>(n) * n);
    for (std::size_t i = 0; i < out.h.size(); ++i) {
        const uint16_t be = static_cast<uint16_t>(raw[i * 2] << 8 | raw[i * 2 + 1]);
        out.h[i] = static_cast<int16_t>(be);
    }
    return true;
}

} // namespace

int loadFromDirectory(const std::string& dir) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_tiles.clear();
    g_dir   = dir;
    g_error.clear();

    std::error_code ec;
    std::filesystem::directory_iterator it(dir, ec);
    if (ec) {
        g_error = "no terrain folder (" + dir + ")";
        return 0;
    }

    int skipped = 0;
    for (const auto& de : it) {
        std::error_code fec;
        if (!de.is_regular_file(fec)) {
            continue;
        }
        std::string name = de.path().filename().string();
        const std::size_t dot = name.find_last_of('.');
        if (dot == std::string::npos) {
            continue;   // tiles must carry the .hgt extension
        }
        std::string ext = name.substr(dot + 1);
        for (char& c : ext) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        if (ext != "HGT") {
            continue;
        }
        int lat0 = 0, lon0 = 0;
        if (!parseTileName(name.substr(0, dot), lat0, lon0)) {
            ++skipped;
            continue;
        }
        Entry e;
        e.path = de.path().string();
        g_tiles[std::make_pair(lat0, lon0)] = std::move(e);
    }

    if (g_tiles.empty()) {
        // Distinguish "folder missing" from "folder there, nothing usable in
        // it": both mean no database, but the operator's next move differs.
        g_error = (skipped > 0) ? "no usable .hgt tiles in (" + dir + ")"
                                : "no .hgt tiles in (" + dir + ")";
    }
    return static_cast<int>(g_tiles.size());
}

int load() {
    return loadFromDirectory(exeFolder() + "\\terrain");
}

bool available() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return !g_tiles.empty();
}

double elevation(double latDeg, double lonDeg, bool& ok) {
    ok = false;
    if (latDeg < -90.0 || latDeg > 90.0) {
        return 0.0;
    }
    if (lonDeg < -180.0 || lonDeg > 180.0) {
        return 0.0;
    }

    const int lat0 = static_cast<int>(std::floor(latDeg));
    int       lon0 = static_cast<int>(std::floor(lonDeg));
    if (lon0 >= 180) {
        lon0 = 179;   // the antimeridian; no real tile is named E180
    }

    std::lock_guard<std::mutex> lk(g_mutex);
    const auto it = g_tiles.find(std::make_pair(lat0, lon0));
    if (it == g_tiles.end()) {
        return 0.0;
    }
    Entry& e = it->second;
    if (!e.data) {
        auto t = std::make_unique<Tile>();
        if (!loadTile(e.path, *t)) {
            return 0.0;
        }
        e.data = std::move(t);
    }

    const Tile& t = *e.data;
    const int   n = t.n;

    // Row 0 sits on the tile's northern edge (lat0 + 1), the last row on its
    // southern edge (lat0); column 0 on the western edge (lon0).
    double rowF = (lat0 + 1.0 - latDeg) * (n - 1);
    double colF = (lonDeg - lon0) * (n - 1);
    rowF = std::clamp(rowF, 0.0, static_cast<double>(n - 1));
    colF = std::clamp(colF, 0.0, static_cast<double>(n - 1));

    int r = static_cast<int>(std::floor(rowF));
    int c = static_cast<int>(std::floor(colF));
    if (r > n - 2) r = n - 2;
    if (c > n - 2) c = n - 2;
    const double fr = rowF - r;
    const double fc = colF - c;

    const std::size_t row0 = static_cast<std::size_t>(r) * n;
    const std::size_t row1 = row0 + n;
    const int16_t v00 = t.h[row0 + c];
    const int16_t v01 = t.h[row0 + c + 1];
    const int16_t v10 = t.h[row1 + c];
    const int16_t v11 = t.h[row1 + c + 1];

    const struct { int16_t v; double w; } s[4] = {
        {v00, (1.0 - fr) * (1.0 - fc)},
        {v01, (1.0 - fr) * fc},
        {v10, fr * (1.0 - fc)},
        {v11, fr * fc},
    };

    double sum = 0.0, wsum = 0.0;
    for (const auto& x : s) {
        if (x.v != kVoid) {
            sum  += static_cast<double>(x.v) * x.w;
            wsum += x.w;
        }
    }
    if (wsum > 0.0) {
        ok = true;
        return sum / wsum + g_offset;
    }

    // Every valid corner fell on a zero weight (the point sits exactly on one
    // of the other samples). Fall back to a plain average of whatever is real.
    double avg = 0.0;
    int    cnt = 0;
    for (const auto& x : s) {
        if (x.v != kVoid) {
            avg += static_cast<double>(x.v);
            ++cnt;
        }
    }
    if (cnt == 0) {
        return 0.0;   // a void, or surrounded by them
    }
    ok = true;
    return avg / cnt + g_offset;
}

void setVerticalOffset(double metres) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_offset = metres;
}

double verticalOffset() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_offset;
}

Status status() {
    std::lock_guard<std::mutex> lk(g_mutex);
    Status s;
    s.tiles = static_cast<int>(g_tiles.size());
    s.dir   = g_dir;
    for (const auto& kv : g_tiles) {
        if (kv.second.data) {
            ++s.loaded;
            s.bytes += kv.second.data->h.size() * sizeof(int16_t);
        }
    }
    if (g_tiles.empty()) {
        s.error = g_error;
    }
    return s;
}

} // namespace db
} // namespace terrain
