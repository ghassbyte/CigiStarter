//=============================================================================
//  TerrainSource.h — the sim-side data CigiStarter needs to answer HAT/HOT
//
//  Why this exists
//    The HOST drives its aircraft altitude from the Height Of Terrain (HOT)
//    that the IG reports. If the IG answers HOT = 0 the aircraft is flown into
//    the ground. The sim's own terrain engine answers the HOT itself, through
//    the PdkTerrainProbe add-on (TerrainProbe.h); this module is the last
//    fallback, and the source of the aircraft's own position and altitude that
//    the entity-relative requests are anchored to.
//
//  What it does
//    Opens FSUIPC against the local Prepar3D and polls the player-aircraft
//    state on a background thread:
//
//      0x0020  4 bytes  ground altitude in metres x 256   -> terrain (HOT)
//      0x0560  8 bytes  latitude,  SINT64, 90/(10001750*2^32) deg per unit
//      0x0568  8 bytes  longitude, SINT64, 360/2^64 deg per unit
//      0x0570  4 bytes  altitude, fractional metres (low half)
//      0x0574  4 bytes  altitude, integer metres    (high half)
//
//    FSUIPC's request buffer is process-global, so exactly one thread may issue
//    FSUIPC_Read/Process. That is this module's poller thread and nobody else —
//    the SOF transmit thread never touches FSUIPC, it only reads the cached
//    snapshot via latest(). That keeps the SOF pacing immune to the latency of
//    an FSUIPC round trip.
//
//  Failure behaviour
//    If the sim is not running (or FSUIPC is not installed) the link simply
//    stays disconnected, the poller keeps retrying, and callers answer HAT/HOT
//    requests with Valid = 0 rather than inventing a terrain height.
//=============================================================================
#pragma once

#include <cstdint>

namespace terrain {

// Latest snapshot of the sim state. `connected` is false until the first
// successful FSUIPC round trip, and drops back to false if the link is lost.
struct Sample {
    bool     connected         = false;
    double   groundAltMeters   = 0.0;  // terrain height under the aircraft, m MSL
    double   aircraftAltMeters = 0.0;  // player aircraft altitude, m MSL
    double   latDeg            = 0.0;  // player aircraft latitude, degrees
    double   lonDeg            = 0.0;  // player aircraft longitude, degrees
    uint64_t tick              = 0;    // increments once per successful read
};

// Start the background poller. Never fails: if the sim is not up yet the
// poller keeps retrying until it is. Returns false only if called twice.
bool start(int pollIntervalMs = 50);

// Stop the poller and close FSUIPC. Safe to call when never started, and it may
// be followed by another start(): CigiStarter's HAT/HOT check box switches the
// poller on and off while the program runs. The cached sample is marked
// disconnected so callers stop reporting a link that is no longer there.
void stop();

// Copy of the most recent snapshot (thread-safe).
Sample latest();

} // namespace terrain
