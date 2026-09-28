#pragma once
//-----------------------------------------------------------------------------
// SimLinkMap.h
//
// The Prepar3D-independent half of PdkSimLink: which rows are to be rendered,
// how a row is read under its seqlock, and the one place the contract's CIGI
// units (degrees, metres) become the PDK's (radians, feet).
//
// It is a header rather than part of the DLL for one reason: the contract test
// (tools\PdkSimLink\test) compiles exactly this code -- the same decisions the
// add-on makes, not a copy of them -- and drives it against CigiStarter's half
// without Prepar3D.  Nothing here includes a PDK header, so that test needs no
// simulator and no SDK.
//
// Deliberately, everything here is a pure function of a row and its inputs:
// no shared state, no globals.  The DLL owns the objects and the counters; this
// decides what the objects should be and where they should go.
//-----------------------------------------------------------------------------

#include <cstdint>

#include "SimLinkIpc.h"

namespace simlink {

// A pose in the PDK's units and order -- the two DXYZ vectors SetPosition wants,
// spelled out as six doubles so this header stays free of the PDK's Types.h
// (which is C++ with ATL, not something a plain test harness should have to
// include).
//
//   vLonAltLat = (lonRad, altFeet, latRad)
//   vPHB       = (pitchRad, headingRad, bankRad)
//
// The order is the PDK's, not CIGI's, and it is the kind of thing that is wrong
// invisibly: an aircraft with the heading in the pitch slot still flies, just
// backwards.  Hence the test.
struct PdkPose {
    double lonRad     = 0.0;
    double altFeet    = 0.0;
    double latRad     = 0.0;
    double pitchRad   = 0.0;
    double headingRad = 0.0;
    double bankRad    = 0.0;
};

constexpr double kDegToRad   = 3.14159265358979323846 / 180.0;
constexpr double kMetersToFt = 3.280839895013123;

// Which slot an entity id lives in.  The table is a power of two so this is a
// mask, and `kMaxEntities` has to stay a power of two for it to be one.
inline unsigned slotOf(uint32_t entityId) {
    return static_cast<unsigned>(entityId) &
           static_cast<unsigned>(kMaxEntities - 1);
}

// Is this row one the sim should be showing?  A row is rendered only when the
// IG has enabled it, it carries a usable pose, and the HOST calls the entity
// active.  Everything else -- disabled, all-zero, standby, destroyed -- is a
// row the add-on must take its object back out of.
//
// Entity 0 is the HOST's own aircraft, the one entity that must always be on
// screen, so "id 0" is deliberately NOT treated as emptiness: emptiness is
// `enabled == 0`.  Skipping id 0 would render the entire rest of the world and
// leave the aircraft itself invisible.
//
// CigiStarter never publishes an unusable pose (main.cpp checks the same rule),
// so in practice every enabled row is wanted; the flag is checked anyway because
// the add-on is the last line of defence and a pose it did not check would be
// flown by the HOST.
inline bool entityWanted(const Entity& e) {
    return e.enabled != 0 &&
           (e.flags & kFlagPoseValid) != 0 &&
           e.state == kEntityActive;
}

// Read one row under its seqlock.  Returns false when the row was being
// rewritten while it was copied, so the caller skips it this tick rather than
// applying a pose that is half old and half new.
//
// This is the whole of the synchronisation between the two processes, so it is
// the thing the contract test hammers: a reader that believes a torn row would
// fly an entity to a place the HOST never commanded.
inline bool readEntityRow(const Entity& row, Entity& out) {
    const uint32_t before = row.seq;
    if (before & 1u) return false;   // the IG is writing it right now
    out = row;
    return row.seq == before;        // it did not move while we copied it
}

// CIGI's degrees and metres, in the PDK's radians and feet and the PDK's order
// (longitude/altitude/latitude and pitch/heading/bank).
inline PdkPose toPdkPose(const Entity& e) {
    PdkPose p;
    p.lonRad     = e.lonDeg    * kDegToRad;
    p.altFeet    = e.altMeters * kMetersToFt;
    p.latRad     = e.latDeg    * kDegToRad;
    p.pitchRad   = e.pitchDeg  * kDegToRad;
    p.headingRad = e.yawDeg    * kDegToRad;
    p.bankRad    = e.rollDeg   * kDegToRad;
    return p;
}

} // namespace simlink
