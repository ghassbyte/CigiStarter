//=============================================================================
//  PdkSimLink contract test
//
//  PdkSimLink.dll can only be exercised for real inside Prepar3D, but both
//  halves either side of it can, and the contract between them is where every
//  dangerous mistake lives.  This harness plays the add-on's half of the shared
//  region and drives the real code both sides run:
//
//    * CigiStarter's half (SimLink.cpp, compiled in) -- the publisher and the
//      health it reports;
//    * the add-on's half (SimLinkMap.h, compiled in, the very code the DLL
//      calls) -- which rows to render, how a row is read, and the unit/order
//      conversion into the PDK.
//
//  What it pins down, in the order the mistakes would hurt:
//
//    * entity 0 is the HOST's own aircraft and is rendered -- it is the one
//      entity that must never be skipped;
//    * nothing else is rendered: a disabled row, an all-zero pose, a Standby (0)
//      or Remove/Destroyed (2) entity all mean "take the object back out", so a
//      HOST that stops driving an entity does not leave it parked in the sim --
//      while Active (1), the state the reference HOST actually sends, is
//      rendered;
//    * the pose that arrives is the pose that was sent, in the PDK's units and
//      order -- degrees to radians, metres to feet, and pitch/heading/bank in
//      the slots SetPosition wants (a heading in the pitch slot still flies,
//      just backwards, which is why it is checked);
//    * a row caught mid-rewrite is never believed (the seqlock), read flat out
//      with the IG rewriting underneath;
//    * an entity id is not silently overwritten when another id hashes to the
//      same slot;
//    * a publisher whose add-on is not there does nothing, and a region left
//      behind by a sim that has stopped ticking is not believed;
//    * the read-back the add-on publishes is in the same units and order as the
//      pose that was commanded, the drift between the two is measured correctly
//      (including the heading wrap), and the layout that carries it is pinned.
//
//  Build/run:  msbuild test\PdkSimLinkTest.vcxproj /p:Configuration=Release
//              test\x64\Release\PdkSimLinkTest.exe
//
//  No Prepar3D and no PDK needed.  Close Prepar3D first if a real PdkSimLink is
//  loaded: the harness uses a mapping name of its own, but a real add-on holds
//  the same region for real.
//=============================================================================

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "cigi.h"          // the Entity Control decoder
#include "SimLink.h"       // the half under test (CigiStarter's)
#include "SimLinkIpc.h"    // the contract
#include "SimLinkMap.h"    // the half under test (the add-on's, PDK-free)

using simlink::Entity;
using simlink::Shared;

namespace {

int g_checks   = 0;
int g_failures = 0;

void check(bool ok, const char* what) {
    ++g_checks;
    if (ok) {
        std::printf("  ok    %s\n", what);
    } else {
        ++g_failures;
        std::printf("  FAIL  %s\n", what);
    }
}

bool approx(double a, double b) { return std::fabs(a - b) < 1e-9; }

//-----------------------------------------------------------------------------
// Building a CIGI Entity Control message, field by field, the way the HOST sends
// one.  Written out here rather than by calling the parser, so the test pins the
// wire layout rather than agreeing with itself.
//-----------------------------------------------------------------------------
void putU16(uint8_t* p, uint16_t v) { std::memcpy(p, &v, 2); }
void putF32(uint8_t* p, float v)    { std::memcpy(p, &v, 4); }
void putF64(uint8_t* p, double v)   { std::memcpy(p, &v, 8); }

void buildEntityControl(uint8_t* msg, uint16_t id, uint8_t state,
                        float roll, float pitch, float yaw,
                        double lat, double lon, double alt) {
    std::memset(msg, 0, cigi::kEntityControlSize);
    msg[0] = static_cast<uint8_t>(cigi::MsgEntityControl);
    msg[1] = static_cast<uint8_t>(cigi::kEntityControlSize);
    putU16(msg + 2, id);
    msg[4] = state;
    putF32(msg + 12, roll);
    putF32(msg + 16, pitch);
    putF32(msg + 20, yaw);
    putF64(msg + 24, lat);
    putF64(msg + 32, lon);
    putF64(msg + 40, alt);
}

//-----------------------------------------------------------------------------
// The add-on's half, standing in for PdkSimLink.dll: it owns the region and its
// heartbeat advances.  It never touches `entities[]` -- those belong to the IG --
// which is the same rule the real DLL follows.
//-----------------------------------------------------------------------------
class FakeWriter {
public:
    bool start() {
        m_mapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                         0, sizeof(Shared), simlink::kMappingName);
        if (!m_mapping) { std::printf("  (could not create the mapping)\n"); return false; }

        void* view = ::MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
        if (!view) {
            ::CloseHandle(m_mapping);
            m_mapping = nullptr;
            std::printf("  (could not map the view)\n");
            return false;
        }

        m_shm = static_cast<Shared*>(view);
        std::memset(m_shm, 0, sizeof(*m_shm));
        m_shm->magic    = simlink::kMagic;
        m_shm->version  = simlink::kVersion;
        m_shm->capacity = simlink::kMaxEntities;
        m_shm->simPid   = ::GetCurrentProcessId();

        m_running.store(true);
        m_thread = std::thread([this] { loop(); });
        return true;
    }

    void stop() {
        m_running.store(false);
        if (m_thread.joinable()) m_thread.join();
        if (m_shm) { m_shm->alive = 0; ::UnmapViewOfFile(m_shm); m_shm = nullptr; }
        if (m_mapping) { ::CloseHandle(m_mapping); m_mapping = nullptr; }
    }

    // Keep the region alive but stop the heartbeat.  This is the "mapping left
    // behind" case: `alive` still says a sim is there, but nothing is advancing.
    void freezeHeartbeat() { m_beating.store(false); }

    Shared* shm() { return m_shm; }

private:
    void loop() {
        m_shm->alive = 1;
        while (m_running.load()) {
            if (m_beating.load()) ++m_shm->ticks;
            ::Sleep(5);
        }
    }

    HANDLE            m_mapping = nullptr;
    Shared*           m_shm     = nullptr;
    std::thread       m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_beating{true};
};

// Publish one entity the way the CIGI receive loop does.  The default state is
// Active (1), which is what the reference HOST sends for the entities it drives;
// the captures never contain a Standby (0) entity.
void publishPose(uint16_t id, double lat, double lon, double alt,
                 float roll, float pitch, float yaw,
                 uint8_t state = 1) {
    cigi::EntityControl e;
    e.entityId  = id;
    e.stateByte = state;
    e.latDeg    = lat;
    e.lonDeg    = lon;
    e.altMeters = alt;
    e.rollDeg   = roll;
    e.pitchDeg  = pitch;
    e.yawDeg    = yaw;
    simlink::publish(e);
}

// The row an entity lands in, as the reader would find it.
bool readSlot(FakeWriter& fake, uint16_t id, Entity& out) {
    return simlink::readEntityRow(fake.shm()->entities[simlink::slotOf(id)], out);
}

// Wait for the add-on's first heartbeat.  The fake advances its tick on its own
// thread, so right after start() the region can still read "not ticking yet" --
// which is correct.  What matters is that once the heartbeat has moved, it reads
// as alive and ticking.  The deadline keeps the check from waiting forever.
bool waitAliveTicking(int ms) {
    const DWORD deadline = ::GetTickCount() + static_cast<DWORD>(ms);
    for (;;) {
        const simlink::Health h = simlink::health();
        if (h.alive && h.ticking) return true;
        if (static_cast<LONG>(::GetTickCount() - deadline) >= 0) return false;
        ::Sleep(5);
    }
}

} // namespace

int main(int /*argc*/, char** /*argv*/) {
    std::printf("PdkSimLink contract test\n\n");

    // The Atang Senjaya waterline the reference capture flew, and a pose that is
    // not level: a level aircraft would pass whatever order the attitude fields
    // were written in.
    const double kLat = -6.5428, kLon = 106.7566, kAlt = 178.0;
    const float  kRoll = 12.5f, kPitch = -3.25f, kYaw = 271.75f;

    // -- 1. Decoding what the HOST sends -------------------------------------
    // The CIGI wire layout, pinned here rather than trusted: an attitude written
    // to the wrong offset is a wrong picture that still looks plausible.
    std::printf("the Entity Control message\n");
    {
        uint8_t msg[128];
        buildEntityControl(msg, 7, 1, kRoll, kPitch, kYaw, kLat, kLon, kAlt);

        cigi::EntityControl e;
        check(cigi::parseEntityControl(msg, cigi::kEntityControlSize, e),
              "a well-formed Entity Control is decoded");
        check(e.entityId == 7, "the entity id comes from offset 2");
        check(e.stateByte == 1, "the state byte comes from offset 4");
        check(e.rollDeg == kRoll && e.pitchDeg == kPitch && e.yawDeg == kYaw,
              "roll/pitch/yaw are the floats at 12/16/20");
        check(e.latDeg == kLat && e.lonDeg == kLon && e.altMeters == kAlt,
              "lat/lon/alt are the doubles at 24/32/40");

        check(!cigi::parseEntityControl(msg, cigi::kEntityControlSize - 1, e),
              "a truncated message is refused");
        msg[0] = 99;
        check(!cigi::parseEntityControl(msg, cigi::kEntityControlSize, e),
              "a message with the wrong id is refused");

        // A HOST sends rows for entities it is not driving, carrying an all-zero
        // pose. That means "no position", not the Gulf of Guinea, so it must not
        // be anchored to -- and must not be rendered.
        check(!cigi::poseUsable([] { cigi::EntityControl z; return z; }()),
              "an all-zero pose is not usable");
        check(!cigi::poseUsable([] {
                  cigi::EntityControl z; z.latDeg = 123.0; return z;
              }()),
              "a pose off the globe is not usable");
        check(cigi::poseUsable([] {
                  cigi::EntityControl z; z.latDeg = kLat; z.lonDeg = kLon; z.altMeters = kAlt;
                  return z;
              }()),
              "a real pose is usable");
    }

    // -- 2. Before the add-on exists -----------------------------------------
    std::printf("\nno add-on loaded\n");
    {
        check(!simlink::health().mappingOpen, "reports the region as not open");
        check(!simlink::health().alive, "and no writer alive");
        // Publishing must be a no-op, not a crash and not a region of our own:
        // the add-on owns the mapping, and the IG only fills it.
        publishPose(0, kLat, kLon, kAlt, kRoll, kPitch, kYaw);
        check(!simlink::health().mappingOpen, "publishing creates nothing while the sim is away");
    }

    // The module re-tries the region every 2 s while the sim is away; stop() is
    // what clears that, so the test does not have to sleep through it.
    simlink::stop();

    FakeWriter fake;
    if (!fake.start()) { std::printf("\nharness could not start\n"); return 2; }
    simlink::start();
    std::printf("\nadd-on loaded\n");
    check(simlink::health().mappingOpen, "the region opens once the add-on is there");
    check(waitAliveTicking(1000),
          "and the add-on reads as alive and ticking once its heartbeat has moved");

    // -- 3. The ordinary case, field by field --------------------------------
    std::printf("\nthe pose that arrives\n");
    {
        publishPose(7, kLat, kLon, kAlt, kRoll, kPitch, kYaw);

        Entity e;
        check(readSlot(fake, 7, e), "the row is readable");
        check(e.entityId == 7, "it is the entity that was published");
        check(e.enabled != 0, "and it is enabled");
        check((e.flags & simlink::kFlagPoseValid) != 0, "with a usable pose");
        check(e.state == simlink::kEntityActive, "and active");
        check(approx(e.latDeg, kLat) && approx(e.lonDeg, kLon) && approx(e.altMeters, kAlt),
              "the position is the position that was sent, in CIGI units");
        check(approx(e.rollDeg, kRoll) && approx(e.pitchDeg, kPitch) && approx(e.yawDeg, kYaw),
              "and the attitude is too");

        const simlink::PdkPose p = simlink::toPdkPose(e);
        const double d2r = simlink::kDegToRad;
        check(approx(p.latRad, kLat * d2r) && approx(p.lonRad, kLon * d2r),
              "latitude and longitude become radians");
        check(approx(p.altFeet, kAlt * simlink::kMetersToFt),
              "altitude becomes feet");
        check(approx(p.pitchRad, kPitch * d2r) &&
              approx(p.headingRad, kYaw * d2r) &&
              approx(p.bankRad, -kRoll * d2r),
              "pitch/heading/bank go to the PDK's slots -- yaw is the heading, and the "
              "bank is CIGI's roll negated, because Prepar3D's bank axis is the other "
              "way round (measured by eye, not by a read-back; SimLinkMap.h says how)");
    }

    // -- 3b. The read-back: the pose the sim is actually holding --------------
    // `SetPosition` returning S_OK says the call was accepted, not that the
    // object stayed.  The add-on reads the object's own pose back
    // (IBaseObjectV400::GetPosition) and publishes it as a Held row; these are
    // the pure functions that make that number mean something, and the layout
    // that carries it.  A drift that is reported in the wrong units or with the
    // heading wrap unhandled would read as a large number that is not real.
    std::printf("\nthe read-back\n");
    {
        check(simlink::kVersion == 3, "the contract version is 3 (the HOST's eye added)");
        check(sizeof(simlink::Held) == 80, "a Held row is 80 bytes");
        check(offsetof(simlink::Shared, entities) == 88, "entities[] starts at 88");
        check(offsetof(simlink::Shared, held) == 4696, "held[] starts at 4696");
        check(sizeof(simlink::View) == 64, "a View row is 64 bytes");
        check(offsetof(simlink::Shared, view) == 9816,
              "the view is appended after held[], so the tables keep their offsets");
        check(sizeof(simlink::Shared) == 9880,
              "the region is the header plus all three tables");

        // The status word is a contract too.  The two object-kind bits are what
        // a reader uses to tell the physics-free object from the sim's own, and
        // the e2e test decodes them by number, so pin both the number and the
        // fact that they do not overlap the packed counters -- a bit moved by
        // one would otherwise read as a plausible, wrong status.
        const uint32_t packedFields =
            simlink::diag::kObjectsMask |
            (simlink::diag::kSetFailsMask    << simlink::diag::kSetFailsShift) |
            (simlink::diag::kCreateFailsMask << simlink::diag::kCreateFailsShift);
        check((simlink::diag::kPhysicsFree     & packedFields) == 0 &&
              (simlink::diag::kModePauseOrSlew & packedFields) == 0 &&
              (simlink::diag::kPhysicsFree & simlink::diag::kModePauseOrSlew) == 0,
              "the object-kind status bits do not collide with the counters");
        check(simlink::diag::kPhysicsFree == (1u << 29) &&
              simlink::diag::kModePauseOrSlew == (1u << 30),
              "the object-kind status bits are the numbers the reader was told");

        // The PDK's units out, and back again: whatever the sim reports has to
        // be readable in the units the commanded pose is in, or the two cannot
        // be compared field by field.
        Entity e{};
        e.latDeg = kLat; e.lonDeg = kLon; e.altMeters = kAlt;
        e.rollDeg = kRoll; e.pitchDeg = kPitch; e.yawDeg = kYaw;
        const simlink::PdkPose p = simlink::toPdkPose(e);
        const simlink::CigiPose back =
            simlink::toCigiPose(p.lonRad, p.altFeet, p.latRad,
                                p.pitchRad, p.headingRad, p.bankRad);
        check(approx(back.latDeg, kLat) && approx(back.lonDeg, kLon) &&
              approx(back.altMeters, kAlt),
              "a pose survives the trip to the PDK's units and back");
        check(approx(back.rollDeg, kRoll) && approx(back.pitchDeg, kPitch) &&
              approx(back.yawDeg, kYaw),
              "and so does the attitude, roll and yaw included");

        // The drift itself: nothing for an object sitting on its commanded pose,
        // metres for one that has moved -- checked against known offsets, so a
        // wrong factor cannot quietly read as a plausible number.
        check(simlink::poseGapMeters(back, back) == 0.0,
              "an object on its commanded pose has zero drift");
        simlink::CigiPose moved = back;
        moved.latDeg = back.latDeg + 0.001;   // ~111.3 m north
        check(std::fabs(simlink::poseGapMeters(back, moved) - 111.32) < 0.5,
              "0.001 degrees of latitude is about 111 metres of drift");
        simlink::CigiPose climbed = back;
        climbed.altMeters = back.altMeters + 5.0;
        check(std::fabs(simlink::poseGapMeters(back, climbed) - 5.0) < 1e-9,
              "a drift straight up is measured on the altitude axis");

        // Heading wraps: 359 and 1 are two degrees apart, not 358.
        simlink::CigiPose a = back, b = back;
        a.yawDeg = 359.0; b.yawDeg = 1.0;
        check(std::fabs(simlink::attitudeGapDeg(a, b) - 2.0) < 1e-9,
              "the heading wrap is handled: 359 to 1 is two degrees, not 358");
        b = back; b.pitchDeg = back.pitchDeg + 30.0;
        check(approx(simlink::attitudeGapDeg(back, b), 30.0),
              "the largest axis is the one reported");
    }

    // -- 3c. The HOST's own eye ----------------------------------------------
    // The sim's view is a camera, not an object: the entity it rides can be
    // moved perfectly and the picture stays where it was until the camera is
    // moved too. The HOST says where to move it in a View Control, and the
    // captures carry exactly one, identical in every frame of every one of them
    // -- the pilot's seat, which is what makes this a cockpit view rather than a
    // picture drawn from the airframe's reference point.
    std::printf("\nthe HOST's own eye (View Control)\n");
    {
        // The bytes one of the captures carries, written out by hand rather than
        // read from a file: 10 20 | 00 00 | 01 | 3F | 00 00 | 2.8 | 0.05 | -0.5
        // | 0 | 0 | 359.
        uint8_t msg[cigi::kViewControlSize];
        std::memset(msg, 0, sizeof(msg));
        msg[0] = static_cast<uint8_t>(cigi::MsgViewControl);
        msg[1] = static_cast<uint8_t>(cigi::kViewControlSize);
        putU16(msg + 2, 0);
        msg[4] = 1;
        msg[5] = 0x3F;
        putU16(msg + 6, 0);
        putF32(msg + 8,   2.8f);
        putF32(msg + 12,  0.05f);
        putF32(msg + 16, -0.5f);
        putF32(msg + 20,  0.0f);
        putF32(msg + 24,  0.0f);
        putF32(msg + 28,  359.0f);

        cigi::ViewControl v;
        check(cigi::parseViewControl(msg, sizeof(msg), v),
              "a well-formed View Control is decoded");
        check(v.entityId == 0, "the entity it is attached to comes from offset 6");
        // The row carries single-precision floats, so 2.8 lands as 2.799999952;
        // this is the decode, not the arithmetic, and its tolerance is the
        // float's own.
        check(std::fabs(v.xOffset - 2.8) < 1e-6 && std::fabs(v.yOffset - 0.05) < 1e-6 &&
              std::fabs(v.zOffset + 0.5) < 1e-6,
              "the offsets are the floats at 8/12/16");
        check(approx(v.yawDeg, 359.0) && v.rollDeg == 0.0f && v.pitchDeg == 0.0f,
              "the angles are the floats at 20/24/28");
        check(!cigi::parseViewControl(msg, sizeof(msg) - 1, v),
              "a truncated view is refused");
        msg[0] = 99;
        check(!cigi::parseViewControl(msg, sizeof(msg), v),
              "a message with the wrong id is refused");

        // Through the region, which is how the camera actually gets it: the
        // decode above is CigiStarter's half, this is the row the add-on reads.
        simlink::publishView(v);
        simlink::View row{};
        check(simlink::readViewRow(fake.shm()->view, row) && row.valid != 0,
              "publishing a view puts a valid row in the region");
        check(row.entityId == v.entityId && approx(row.zOffset, -0.5) &&
              approx(row.yawDeg, 359.0),
              "with the fields the camera reads");
        check(simlink::health().viewCarried,
              "and the writer reports the eye as carried");

        // Where that eye ends up. The offset is a body offset, so it has to be
        // rotated by the *entity's* attitude: 2.8 m out the nose is a different
        // place on the globe for every heading, and skipping the rotation would
        // look right on a north heading and wrong on every other one.
        Entity e{};
        e.latDeg = 0.0; e.lonDeg = 0.0; e.altMeters = 100.0;
        e.rollDeg = e.pitchDeg = e.yawDeg = 0.0;   // level, pointing north
        simlink::CigiPose eye = simlink::viewEyePose(e, row);
        check(approx(eye.latDeg, 2.8 / simlink::kMetersPerDegLat),
              "pointing north, the eye is 2.8 m north of the reference point");
        check(approx(eye.lonDeg, 0.05 / simlink::kMetersPerDegLat),
              "and 5 cm -- the lateral offset -- to the east");
        check(approx(eye.altMeters, 100.5),
              "and half a metre up: Z is out the bottom in CIGI's body frame");
        check(approx(eye.yawDeg, 359.0), "turned a degree left of the nose");

        e.yawDeg = 90.0;   // pointing east
        eye = simlink::viewEyePose(e, row);
        check(approx(eye.latDeg, -0.05 / simlink::kMetersPerDegLat),
              "pointing east, 2.8 m is due east and the 5 cm lateral offset is due south -- the offset turns with the aircraft");
        check(approx(eye.lonDeg, 2.8 / simlink::kMetersPerDegLat),
              "so it is the longitude that moves");

        e.yawDeg = 1.0;   // a degree east of north
        eye = simlink::viewEyePose(e, row);
        check(approx(eye.yawDeg, 0.0),
              "a view is relative to the entity, and the heading wraps");

        // The invariant that catches a wrong rotation without a reference
        // answer: whatever the attitude, the eye is exactly the offset's length
        // from the reference point -- sqrt(2.8^2 + 0.05^2 + 0.5^2) m.
        const double span = std::sqrt(2.8 * 2.8 + 0.05 * 0.05 + 0.5 * 0.5);
        const double yaws[4]   = {0.0, 37.0, 180.0, 271.75};
        const double rolls[4]  = {0.0, 0.0, -62.0, 12.5};
        const double pitchs[4] = {0.0, 0.0, 41.0, -3.25};
        bool sameSpan = true;
        for (int i = 0; i < 4; ++i) {
            e.latDeg = kLat; e.lonDeg = kLon; e.altMeters = kAlt;
            e.rollDeg = rolls[i]; e.pitchDeg = pitchs[i]; e.yawDeg = yaws[i];
            const simlink::CigiPose p = simlink::viewEyePose(e, row);
            simlink::CigiPose ref;
            ref.latDeg = e.latDeg;
            ref.lonDeg = e.lonDeg;
            ref.altMeters = e.altMeters;
            if (std::fabs(simlink::poseGapMeters(ref, p) - span) > 0.02) sameSpan = false;
        }
        check(sameSpan,
              "the eye is the offset's length from the reference point at every attitude");
    }

    // -- 4. The HOST's own aircraft ------------------------------------------
    // Entity 0 is the aircraft the HOST's flight model is flying. It is the one
    // entity that must be on screen: the whole point of the writer is to show
    // the aircraft, and everything else in the world is arranged around it.
    // "Id 0" is not emptiness -- emptiness is `enabled == 0`.
    std::printf("\nthe HOST's own aircraft (entity 0)\n");
    {
        publishPose(0, kLat, kLon, kAlt, kRoll, kPitch, kYaw);

        Entity e;
        const bool got = readSlot(fake, 0, e);
        check(got, "an entity-0 row is readable");
        check(got && e.entityId == 0, "it carries the id");
        check(got && e.enabled != 0, "it is enabled");
        check(got && simlink::entityWanted(e),
              "entity 0 is rendered -- not skipped for being id 0");
    }

    // -- 5. What is rendered, and what is not --------------------------------
    // Standby (0) and Remove (2) both mean "take the object out", so a HOST that
    // stops driving an entity does not leave it parked in the sim.  Active (1)
    // is the state the reference HOST actually sends -- its entities 0 and 3 sit
    // at 1 for every frame of every capture -- so the numbering is checked by
    // behaviour rather than assumed.  Read the field the other way round and
    // every pose still arrives perfectly and nothing is rendered, which looks
    // exactly like a link that is working.
    std::printf("\nwhat is rendered, and what is not\n");
    {
        Entity e;

        publishPose(1, kLat, kLon, kAlt, kRoll, kPitch, kYaw, /*active*/ 1);
        check(readSlot(fake, 1, e) && e.enabled != 0 && simlink::entityWanted(e),
              "an Active entity (the state the reference HOST sends) is rendered");

        publishPose(2, kLat, kLon, kAlt, kRoll, kPitch, kYaw, /*standby*/ 0);
        check(readSlot(fake, 2, e) && e.enabled != 0 && !simlink::entityWanted(e),
              "a Standby entity is published but not rendered");

        publishPose(6, kLat, kLon, kAlt, kRoll, kPitch, kYaw, /*remove*/ 2);
        check(readSlot(fake, 6, e) && e.enabled != 0 && !simlink::entityWanted(e),
              "a Remove/Destroyed entity is not rendered");

        // An all-zero pose: the HOST has a row for the id but is not driving it.
        publishPose(3, 0.0, 0.0, 0.0, 0.0f, 0.0f, 0.0f);
        check(readSlot(fake, 3, e) && (e.flags & simlink::kFlagPoseValid) == 0,
              "an all-zero pose is published without the usable flag");
        check(!simlink::entityWanted(e),
              "and is not rendered, so the sim never gets the Gulf of Guinea");
    }

    // -- 6. Retiring an entity -----------------------------------------------
    // When the IG stops -- or stops publishing an entity -- the row is disabled
    // so the add-on takes its object back out. Otherwise a stopped IG would
    // leave aircraft parked over the scenery forever.
    std::printf("\nretiring an entity\n");
    {
        publishPose(4, kLat, kLon, kAlt, kRoll, kPitch, kYaw);

        Entity e;
        check(readSlot(fake, 4, e) && e.enabled != 0,
              "the row is live while the IG is publishing it");

        simlink::stop();   // retires every row, then releases the region

        // The fake still holds its own view of the same pages, so it can see
        // what the IG wrote on the way out.
        check(readSlot(fake, 4, e) && e.enabled == 0,
              "stop() retires the row, so the add-on removes the object");

        simlink::View view{};
        check(simlink::readViewRow(fake.shm()->view, view) && view.valid == 0,
              "and takes the eye back, so a stopped IG leaves no stale viewpoint");
        simlink::start();  // re-open for the sections that follow
    }

    // -- 7. Two entities, one slot -------------------------------------------
    // The table is indexed by the low bits of the id, so two ids can land
    // together. The resident entity must win: silently dropping one is better
    // than silently flying the other.
    std::printf("\ntwo entities that hash to one slot\n");
    {
        const uint16_t a = 5;
        const uint16_t b = static_cast<uint16_t>(5 + simlink::kMaxEntities);
        check(simlink::slotOf(a) == simlink::slotOf(b),
              "the two ids really do share a slot");

        publishPose(a, kLat, kLon, kAlt, kRoll, kPitch, kYaw);
        publishPose(b, kLat + 1.0, kLon + 1.0, kAlt + 1.0, 1.0f, 1.0f, 1.0f);

        Entity e;
        check(readSlot(fake, a, e), "the slot is readable");
        check(e.entityId == a, "the resident entity is not overwritten");
        check(approx(e.latDeg, kLat), "and neither is its pose");
    }

    // -- 8. A heartbeat that stops -------------------------------------------
    std::printf("\nthe add-on stops ticking\n");
    {
        // Watch it tick first: staleness is about a heartbeat that *stops*, so
        // the IG has to have seen it running before it can call it stopped.
        check(simlink::health().ticking, "the heartbeat is being observed");

        fake.freezeHeartbeat();
        ::Sleep(1700);
        const simlink::Health h = simlink::health();
        check(h.mappingOpen, "the region is still there");
        check(h.alive, "and still says a sim is alive");
        check(!h.ticking, "but a heartbeat that has stopped is not believed");
    }

    // -- 9. Readings taken mid-rewrite ---------------------------------------
    // The seqlock is the whole of the synchronisation between the two
    // processes. Read flat out with the IG rewriting underneath: no torn row
    // may ever be believed. Every field carries the same tag, so a row that is
    // half one publish and half another is detectable (lon-lat == 10, and so
    // on), independently of the seq check.
    std::printf("\nreadings taken mid-rewrite\n");
    {
        simlink::stop();
        fake.stop();

        FakeWriter stress;
        if (!stress.start()) { std::printf("harness could not restart\n"); return 2; }
        simlink::start();

        const uint16_t id = 0;
        const unsigned slot = simlink::slotOf(id);

        auto publishTag = [&](int t) {
            publishPose(id, 10.0 + t, 20.0 + t, 30.0 + t,
                        static_cast<float>(40 + t), static_cast<float>(50 + t),
                        static_cast<float>(60 + t));
        };

        std::atomic<bool> run{true};
        std::atomic<int>  reads{0};
        std::atomic<int>  torn{0};

        std::thread reader([&] {
            Entity e;
            while (run.load()) {
                if (!simlink::readEntityRow(stress.shm()->entities[slot], e)) continue;
                if (!e.enabled) continue;
                ++reads;
                const double lat = e.latDeg;
                if (!(e.lonDeg - lat == 10.0 && e.altMeters - lat == 20.0 &&
                      e.rollDeg - lat == 30.0 && e.pitchDeg - lat == 40.0 &&
                      e.yawDeg - lat == 50.0)) {
                    ++torn;
                }
            }
        });

        constexpr int kPublishes = 300000;
        for (int t = 1; t <= kPublishes; ++t) publishTag(t);

        run.store(false);
        reader.join();

        std::printf("  ----  %d readings taken, %d of them mid-rewrite (all exact)\n",
                    reads.load(), reads.load() - torn.load());
        check(reads.load() > 0, "readings keep coming through while the IG rewrites");
        check(torn.load() == 0, "not one torn or half-written row is believed");

        stress.stop();
    }

    // -- 10. Shutting down ---------------------------------------------------
    // retiring the rows is what tells the add-on to take its objects back out,
    // so a stopped IG does not leave aircraft parked over the scenery.
    std::printf("\nshutting the publisher down\n");
    {
        simlink::stop();
        check(!simlink::health().mappingOpen, "the region is released");
    }

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
