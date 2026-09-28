//=============================================================================
//  SimLink.h -- put the HOST's entities into Prepar3D
//
//  Why this exists
//    The HOST owns the whole world: it sends each entity's pose every frame as
//    CIGI Entity Control, and expects the IG to render exactly that.  Presen-
//    tation has to happen in Prepar3D, and Prepar3D does not speak CIGI here
//    (its own CIGI feature is a Professional Plus extra).  The vendor program
//    that used to bridge the two drove the sim's *own* aircraft over FSUIPC --
//    a fight with the sim, and one that left time, weather and effects to be
//    reconciled by hand.
//
//  What it does
//    Publishers the HOST's commanded pose for each entity into a block of shared
//    memory (SimLinkIpc.h); PdkSimLink.dll, an add-on loaded into the local
//    Prepar3D, reads it on the sim's own frame tick and moves a sim object to
//    it.  The link is one-way: the CIGI stream already carries everything the
//    sim needs, so nothing of this crosses the wire back to the HOST, and the
//    IG never touches the sim itself.
//
//  Scope of this first cut
//    Pose only -- position and attitude.  Time of day, weather and effects are
//    the next things the same channel will carry; the table has room and the
//    version field is the guard against a half-updated pair.
//
//  Failure behaviour
//    If Prepar3D is not running, or the add-on is not installed, or it has not
//    had a frame tick yet, the module simply reports itself unavailable and the
//    CIGI receive loop carries on.  The mapping is opened lazily and re-tried,
//    so CigiStarter may start before the sim.
//=============================================================================
#pragma once

#include <cstdint>

#include "cigi.h"

namespace simlink {

// Publish the HOST's commanded state for one entity. Cheap and non-blocking:
// it opens the mapping lazily if it is not open, and does nothing when the
// add-on is not there. Safe to call from the CIGI receive thread.
void publish(const cigi::EntityControl& e);

// A picture of the writer for the operator window and a bench readout: whether
// the add-on is there, whether it is applying, and why not.  Everything here is
// advisory -- no decision is taken from it -- and it degrades to zeros when the
// mapping is absent or the add-on leaves the diagnostic word unset
// (`diagnosed` is then false).
struct Health {
    bool     mappingOpen    = false;  // the shared region is open
    bool     alive          = false;  // the add-on has its object manager and a tick
    bool     ticking        = false;  // its tick advanced within the last 1.5 s
    int32_t  lastError      = 0;      // last HRESULT the add-on recorded
    uint32_t ticks          = 0;      // heartbeat counter, as last read
    uint32_t frameTicks     = 0;      // Prepar3D frame-event callbacks seen
    uint32_t objectsCreated = 0;      // sim objects created for entities
    uint32_t objectsRemoved = 0;      // sim objects removed again
    uint32_t createFails    = 0;      // CreateObject/GetObject failures
    uint32_t setFails       = 0;      // SetPosition failures
    uint32_t applied        = 0;      // entities positioned on the last tick
    uint32_t positionReads  = 0;      // successful GetPosition read-backs
    uint32_t getFails       = 0;      // GetPosition calls the sim refused
    uint32_t driftSamples   = 0;      // held-vs-commanded comparisons made
    uint32_t heldValid      = 0;      // rows carrying a fresh read-back last tick
    double   driftMaxMeters = 0.0;    // worst |held - last commanded| this run
    double   driftMaxDeg    = 0.0;    // worst attitude error this run
    bool     diagnosed      = false;  // the add-on fills the diagnostic word
    bool     hasManager     = false;  // the sim-object manager was obtained
    bool     hasTick         = false; // a tick has been delivered
    bool     hasReadback    = false;  // a pose has been read back from the sim
    bool     noTitle        = false;  // PdkSimLink.ini has no container title
};

Health health();

// Open the shared region (lazily re-tried by publish(); this just forces an
// attempt) and release it. Safe to call in any order, any number of times.
void start();
void stop();

} // namespace simlink
