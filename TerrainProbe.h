//=============================================================================
//  TerrainProbe.h -- terrain heights from Prepar3D's own terrain engine
//
//  Why this exists
//    The HOST drives its aircraft altitude from the Height Of Terrain (HOT)
//    the IG reports, so a wrong or stale HOT flies the aircraft into the ground.
//    The offline SRTM database (TerrainDb.h) is a copy of the world that has to
//    be re-baked whenever the scenery changes, and the FSUIPC reading
//    (TerrainSource.h) freezes while the aircraft is slewed.  Prepar3D will
//    answer the same question directly, from the terrain it is actually about
//    to place the aircraft on, if a small PDK add-on asks it -- see
//    TerrainProbeIpc.h and tools\PdkTerrainProbe.
//
//  What it does
//    Talks to PdkTerrainProbe.dll, an add-on loaded into the local Prepar3D,
//    through a block of shared memory.  CigiStarter writes the points it wants
//    looked up; the add-on asks the sim on its next simulation tick and writes
//    the answers back.  Nothing here blocks on the sim, and there is no FSUIPC
//    round trip in the HAT/HOT path.
//
//  Failure behaviour
//    If Prepar3D is not running, or the add-on is not installed, or the add-on
//    has not had a simulation tick yet, the module reports itself unavailable
//    and callers fall through to the database and then to FSUIPC.  The mapping
//    is opened lazily and re-tried, so CigiStarter may start before the sim.
//
//  Note on --terrain-offset
//    That option calibrates the *database's* datum against the sim, because
//    SRTM's heights sit on a different datum from the sim's scenery.  A value
//    from the sim needs no calibration, so the offset is deliberately not
//    applied to it.
//=============================================================================
#pragma once

namespace terrain {
namespace probe {

// True when the add-on is loaded in the local Prepar3D and its tick is
// advancing. Cheap enough to call per request.
bool available();

// Ask the sim for the terrain height at a point.
//
//   key         identifies the caller's own query (CigiStarter uses the HOST's
//               HAT/HOT id). The slot is reused for the same key, so a point
//               that is asked about repeatedly -- every one of the HOST's six
//               probes is, once per six frames -- is answered from a reading
//               already taken, rather than waiting a tick for a fresh one.
//   latDeg/lonDeg/altMeters  the point, in CIGI's units. `altMeters` is the
//               altitude of the test point, not of the terrain.
//   hotMeters   set to the terrain height, metres MSL, on success.
//
// Returns false when the probe is unavailable, has no answer for this point
// yet, or has only an answer that was taken somewhere else. The caller should
// then fall back to another terrain source.
bool elevation(unsigned key, double latDeg, double lonDeg, double altMeters,
               double& hotMeters);

// Open the shared region (lazily re-tried by the calls above; this just forces
// an attempt) and release it. Safe to call in any order, any number of times.
void start();
void stop();

} // namespace probe
} // namespace terrain
