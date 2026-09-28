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

#include <cmath>
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

// A pose back in CIGI's own units, which is how the read-back has to be
// published: the commanded pose and the held pose are then compared field by
// field in the same units, and neither side has to remember the PDK's order.
struct CigiPose {
    double latDeg    = 0.0;
    double lonDeg    = 0.0;
    double altMeters = 0.0;
    double rollDeg   = 0.0;
    double pitchDeg  = 0.0;
    double yawDeg    = 0.0;
};

// The reverse of toPdkPose: the PDK's two vectors -- (longitude, altitude,
// latitude) and (pitch, heading, bank) -- back to degrees and metres. Passed as
// plain doubles so this header keeps its distance from the PDK's Types.h.
inline CigiPose toCigiPose(double lonRad, double altFeet, double latRad,
                           double pitchRad, double headingRad, double bankRad) {
    CigiPose c;
    c.latDeg    = latRad     / kDegToRad;
    c.lonDeg    = lonRad     / kDegToRad;
    c.altMeters = altFeet    / kMetersToFt;
    c.pitchDeg  = pitchRad   / kDegToRad;
    c.yawDeg    = headingRad / kDegToRad;
    c.rollDeg   = bankRad    / kDegToRad;
    return c;
}

// How far apart two poses are, in metres.  This is the drift measurement, so it
// has to be honest at the scale of a few metres and cheap enough to run every
// tick: the equirectangular approximation on a spherical earth, which is exact
// to well under a metre over the distances a between-writes drift can reach.
constexpr double kMetersPerDegLat = 111320.0;  // mean metres per degree of latitude

inline double poseGapMeters(const CigiPose& a, const CigiPose& b) {
    const double mPerDegLon = kMetersPerDegLat * std::cos(a.latDeg * kDegToRad);
    const double dNorth = (b.latDeg    - a.latDeg)    * kMetersPerDegLat;
    const double dEast  = (b.lonDeg    - a.lonDeg)    * mPerDegLon;
    const double dUp    =  b.altMeters - a.altMeters;
    return std::sqrt(dNorth * dNorth + dEast * dEast + dUp * dUp);
}

// The largest single-axis attitude error, in degrees.  Heading wraps -- 359 and
// 1 are two degrees apart, not 358 -- and a wrap that is not handled would show
// a two-degree drift as a 358-degree one.
inline double attitudeGapDeg(const CigiPose& a, const CigiPose& b) {
    double dRoll  = std::fabs(b.rollDeg  - a.rollDeg);
    double dPitch = std::fabs(b.pitchDeg - a.pitchDeg);
    double dYaw   = std::fabs(b.yawDeg   - a.yawDeg);
    if (dYaw > 180.0) dYaw = 360.0 - dYaw;
    double m = dRoll;
    if (dPitch > m) m = dPitch;
    if (dYaw   > m) m = dYaw;
    return m;
}

// Read the HOST's own eye under its seqlock, the same way an entity row is
// read, so a camera that catches the row half-rewritten keeps the pose it had
// rather than jumping to a mix of two views.
inline bool readViewRow(const View& row, View& out) {
    const uint32_t before = row.seq;
    if (before & 1u) return false;
    out = row;
    return row.seq == before;
}

// Where the HOST wants its eye, given the entity its view is attached to.
//
// The offsets are body offsets from the entity's reference point, so they have
// to be rotated out of the body frame by that entity's own attitude before they
// can be added to a latitude and longitude -- which is what makes this more
// than arithmetic on the row: 2.8 m out the nose is a different place on the
// globe for every heading, and skipping the rotation would leave the eye on the
// reference point whenever the aircraft was level and drift away from it as
// soon as it was not.
//
// The angles are the other way round: the view's roll, pitch and yaw are
// *relative* to the entity, so they are simply added to it.  Heading wraps.
//
// The entity attitude is also what the offset is rotated by, not the view's own
// angles: the HOST places the eye a fixed distance along the aircraft, and then
// points it.  The two are the same thing for the captures, whose view is turned
// two degrees; they are not the same thing in general.
inline CigiPose viewEyePose(const Entity& e, const View& v) {
    CigiPose c;
    c.rollDeg  = e.rollDeg  + v.rollDeg;
    c.pitchDeg = e.pitchDeg + v.pitchDeg;
    c.yawDeg   = e.yawDeg   + v.yawDeg;

    c.yawDeg = std::fmod(c.yawDeg, 360.0);
    if (c.yawDeg < 0.0) c.yawDeg += 360.0;

    const double roll  = e.rollDeg  * kDegToRad;
    const double pitch = e.pitchDeg * kDegToRad;
    const double yaw   = e.yawDeg   * kDegToRad;

    const double cr = std::cos(roll),  sr = std::sin(roll);
    const double cp = std::cos(pitch), sp = std::sin(pitch);
    const double cy = std::cos(yaw),   sy = std::sin(yaw);

    const double x = v.xOffset, y = v.yOffset, z = v.zOffset;

    // Body (X out the nose, Y out the right wing, Z out the bottom) into north,
    // east and down.
    const double north = cp * cy * x + (sr * sp * cy - cr * sy) * y +
                         (cr * sp * cy + sr * sy) * z;
    const double east  = cp * sy * x + (sr * sp * sy + cr * cy) * y +
                         (cr * sp * sy - sr * cy) * z;
    const double down  = -sp * x + sr * cp * y + cr * cp * z;

    // The same small-area approximation poseGapMeters() measures with, so the
    // two cannot disagree about what a metre away means.
    c.latDeg    = e.latDeg + north / kMetersPerDegLat;
    c.lonDeg    = e.lonDeg + east /
                  (kMetersPerDegLat * std::cos(e.latDeg * kDegToRad));
    c.altMeters = e.altMeters - down;   // down is positive, altitude is up
    return c;
}

} // namespace simlink
