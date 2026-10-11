//=============================================================================
//  TerrainProbe.h -- terrain heights from Prepar3D's own terrain engine
//
//  Why this exists
//    The HOST drives its aircraft altitude from the Height Of Terrain (HOT)
//    the IG reports, so a wrong or stale HOT flies the aircraft into the ground.
//    The offline SRTM database (TerrainDb.h) is a copy of the world that has to
//    be re-baked whenever the scenery changes, and the FSUIPC reading
//    (TerrainSource.h) only ever describes the ground under the sim's own
//    aircraft.  Prepar3D will answer the same question directly, from the
//    terrain it is actually about to place the aircraft on, if a small PDK
//    add-on asks it -- see TerrainProbeIpc.h and tools\PdkTerrainProbe.
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

#include <cstdint>

#include "TerrainProbeIpc.h"   // the surface::* encoding of Result::materialCode

namespace terrain {
namespace probe {

// True when the add-on is loaded in the local Prepar3D and its tick is
// advancing. Cheap enough to call per request.
bool available();

// A fuller picture than available(), for the operator window and a bench
// readout: why the probe is or is not answering.  Everything here is advisory
// -- no decision is taken from it -- and it degrades to zeros when the mapping
// is absent or the add-on is an older build that leaves the diagnostic word
// unset (`diagnosed` is then false and the diagnostics below must be ignored).
struct Health {
    bool     mappingOpen = false;  // the shared region is open
    bool     alive       = false;  // the add-on has its surface service
    bool     ticking     = false;  // its tick advanced within the last 1.5 s
    int32_t  lastError   = 0;      // last HRESULT the add-on recorded
    uint32_t tickRateHz  = 0;      // rate it registered the tick at
    uint32_t ticks       = 0;      // heartbeat counter, as last read
    uint32_t queryFails  = 0;      // points Prepar3D has refused
    bool     diagnosed         = false;  // the add-on fills the diagnostic word
    uint32_t objectCreates     = 0;      // object-create callbacks it saw
    uint32_t objectRemoves     = 0;      // object-remove callbacks it saw
    uint32_t attachFails       = 0;      // RegisterSimulation failures
    bool     attached          = false;  // a simulation is bound right now
    bool     hostIsUser        = false;  // bound object was the user object
    bool     skippedUserCreate = false;  // a user-object create was ignored
    // The tick that actually runs.  Prepar3D stops signalling its per-frame
    // event while it is paused, so the probe also keeps a thread of its own and
    // falls back to it once every Prepar3D-provided tick has gone quiet; these
    // say which tick is answering, what the sim last did, and what the object's
    // mode was when its tick last ran.  Zero when an older add-on is loaded
    // (kVersion 1), which is also the case `diagnosed` covers.
    uint32_t frameTicks  = 0;      // frame-event callbacks the add-on has seen
    uint32_t simEvents   = 0;      // simulator messages, see simEvent::*
    uint32_t simMode     = 0;      // GetMode() at the last object tick
    uint32_t frameFails  = 0;      // frame-event RegisterCallback failures
    bool     simPaused   = false;  // the sim reported itself paused (simEvents bit 0)
    bool     ownThread   = false;  // the newest pass came from the add-on's own thread
};

Health health();

// What the sim's surface query said about the ground it answered with, when the
// add-on is a build that reports it (`reported` is false for an older one, and
// for a point the add-on never asked about).
//
// `onPlatform` is the one worth watching: it says the surface is an *object's*,
// not terrain.  A vessel moored in the sea, an oil rig or a bridge is answered
// with the terrain below it -- sea level, for the vessel -- unless the platform
// flag is honoured, so a caller that only reads `hotMeters` can fly an aircraft
// into the deck.  This is diagnostic for now: nothing in the answer path reads
// it, so that a bench run can say whether Prepar3D reports the deck at all
// before any policy is built on it.
struct Surface {
    bool reported   = false;
    bool onPlatform = false;
    int  category   = surface::kCategoryInvalid;  // surface::kCategory*
};

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
//   surface     optional: the sim's classification of that surface, when it
//               reports one. Left default-constructed whenever the height did
//               not come from the sim.
//
// Returns false when the probe is unavailable, has no answer for this point
// yet, or has only an answer that was taken somewhere else. The caller should
// then fall back to another terrain source.
bool elevation(unsigned key, double latDeg, double lonDeg, double altMeters,
               double& hotMeters, Surface* surface = nullptr);

// Open the shared region (lazily re-tried by the calls above; this just forces
// an attempt) and release it. Safe to call in any order, any number of times.
void start();
void stop();

} // namespace probe
} // namespace terrain
