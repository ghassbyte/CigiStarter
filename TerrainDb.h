//=============================================================================
//  TerrainDb.h — terrain heights from an offline SRTM tile set
//
//  Why this exists
//    The HOST drives its aircraft altitude from the Height Of Terrain (HOT)
//    the IG reports. The local Prepar3D can supply that ground through the
//    PdkTerrainProbe add-on (TerrainProbe.h), which is the surface Prepar3D will
//    actually place the aircraft on -- but that needs the sim up and the add-on
//    loaded.
//
//    This module gives the HAT/HOT path a terrain source that does not depend
//    on the live sim at all: Shuttle Radar Topography Mission (SRTM) tiles,
//    read straight from disk. It is the same idea the original Master IG used
//    -- answer the terrain request from a database rather than from the sim.
//
//    (The sim's own ground altitude was once read over FSUIPC as the last
//    fallback. That is gone: it describes only the ground under the sim's *own*
//    aircraft, which is no longer flown -- the HOST's entities are separate
//    objects now (SimLink.h) -- and it stopped following anyway while the
//    program that drives the sim's aircraft held it in slew.)
//
//  What it reads
//    Raw SRTM ".hgt" tiles in the folder "terrain" next to the executable,
//    one 1-degree by 1-degree cell per file, named by the integer-degree
//    corner it starts at:
//
//      N06E106.hgt   covers  6..7 N, 106..107 E
//      S08E112.hgt   covers -8..-7, 112..113 E
//
//    Both SRTMGL1 (1 arc-second, 3601 x 3601) and SRTMGL3 (3 arc-second,
//    1201 x 1201) are accepted; the sample count is taken from the file size.
//    Samples are signed 16-bit big-endian metres, row 0 the northern edge,
//    column 0 the western edge, -32768 marking a void.
//
//  Lookups are bilinear, a tile is read from disk on first use and kept. The
//  whole thing is optional: with no terrain folder the HAT/HOT path has nothing
//  to answer from when the sim's own probe is not loaded either, and answers
//  Valid = 0 so the HOST ignores the request.
//
//  Threading
//    elevation() is called from the SOF transmit thread and status() from the
//    GUI thread, so both take the module's own lock. The first lookup in a
//    tile reads it from disk under that lock.
//=============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace terrain {
namespace db {

// Index the ".hgt" tiles in `dir`. No samples are read yet. Returns the
// number of tiles indexed; 0 means "no database", which is not an error.
int loadFromDirectory(const std::string& dir);

// Index the tiles in the "terrain" folder next to the executable.
int load();

// True once a load()/loadFromDirectory() has indexed at least one tile.
bool available();

// Terrain height in metres MSL at a geodetic point, bilinear between the four
// surrounding samples. `ok` is cleared when the point lies outside every
// indexed tile, or when the tile has no valid sample there (an SRTM void), in
// which case the return value is meaningless. Loads the tile on first use.
double elevation(double latDeg, double lonDeg, bool& ok);

// A user calibration added to every database height, in metres. SRTM is
// orthometric (EGM96) and the sim is not necessarily, so a constant offset can
// be dialled in from the command line if the reports turn out biassed.
// Default 0.
void   setVerticalOffset(double metres);
double verticalOffset();

// What the operator window reports.
struct Status {
    int         tiles  = 0;  // ".hgt" tiles indexed
    int         loaded = 0;  // tiles whose samples are in memory
    std::size_t bytes  = 0;  // bytes of sample data held
    std::string dir;         // folder that was scanned
    std::string error;       // why there is no database ("" = none)
};
Status status();

} // namespace db
} // namespace terrain
