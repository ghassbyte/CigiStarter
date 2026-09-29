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
//      (including the heading wrap), and the layout that carries it is pinned;
//    * the HOST's world beyond the entities -- its Celestial Sphere clock, its
//      atmosphere and its weather -- decodes from the same wire layout, folds
//      into one accumulated world, resolves with an operator override on top,
//      and renders the METAR the sim's weather system is handed, while the
//      publisher writes only on change and says which group moved.
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
#include "Environment.h"   // the world beyond the entities: resolve and METAR
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

// The world messages, likewise written out byte by byte.  The three together are
// the HOST's whole environment, and the test pins each field's offset because a
// value read from the wrong one is still a plausible number.
void buildCelestialSphere(uint8_t* msg, uint8_t hour, uint8_t minute,
                          uint8_t flags, uint32_t date) {
    std::memset(msg, 0, cigi::kCelestialSphereSize);
    msg[0] = static_cast<uint8_t>(cigi::MsgCelestialSphere);
    msg[1] = static_cast<uint8_t>(cigi::kCelestialSphereSize);
    msg[2] = hour;                // 8-bit, the ICD's UTC field
    msg[3] = minute;
    msg[4] = flags;               // bit0 ephemeris, bit4 date valid, ...
    // 5..7 reserved
    std::memcpy(msg + 8, &date, 4);   // Month*1000000 + Day*10000 + Year
    // 12..15 star intensity
}

void buildAtmosphere(uint8_t* msg, uint8_t humidity, float tempC, float visM,
                     float horizWind, float vertWind, float windDir, float baroMb) {
    std::memset(msg, 0, cigi::kAtmosphereSize);
    msg[0] = static_cast<uint8_t>(cigi::MsgAtmosphere);
    msg[1] = static_cast<uint8_t>(cigi::kAtmosphereSize);
    msg[2] = 0x00;                // enable bit: the captures send 0, values count anyway
    msg[3] = humidity;
    putF32(msg + 4,  tempC);
    putF32(msg + 8,  visM);
    putF32(msg + 12, horizWind);
    putF32(msg + 16, vertWind);
    putF32(msg + 20, windDir);
    putF32(msg + 24, baroMb);
}

void buildWeather(uint8_t* msg, uint8_t layerId, uint8_t humidity, uint8_t flags,
                  uint8_t flags2, float tempC, float visM, float coverage,
                  float baseElevM, float thicknessM, float horizWind,
                  float vertWind, float windDir, float baroMb) {
    std::memset(msg, 0, cigi::kWeatherControlSize);
    msg[0] = static_cast<uint8_t>(cigi::MsgWeatherControl);
    msg[1] = static_cast<uint8_t>(cigi::kWeatherControlSize);
    putU16(msg + 2, 0);
    msg[4] = layerId;             // the layer id names what is falling
    msg[5] = humidity;
    msg[6] = flags;               // bit0 weather en, bit3 lightning, high nibble cloud
    msg[7] = flags2;              // bits 0-1 scope, bits 2-4 severity
    putF32(msg + 8,  tempC);
    putF32(msg + 12, visM);
    putF32(msg + 16, 0.0f);       // scud
    putF32(msg + 20, coverage);
    putF32(msg + 24, baseElevM);
    putF32(msg + 28, thicknessM);
    putF32(msg + 32, 0.0f);       // transition
    putF32(msg + 36, horizWind);
    putF32(msg + 40, vertWind);
    putF32(msg + 44, windDir);
    putF32(msg + 48, baroMb);
    putF32(msg + 52, 0.0f);       // aerosol
}

// The Component Control (id 4), written out byte by byte like the rest.  The
// values the reference HOST puts in it are the pattern's footprint and altitude
// in metres -- 8 x 1852 m across, 7 x 1852 m deep and 3000 ft up for the first
// pattern -- which is what `cloudPattern()` below reproduces.
void buildComponentControl(uint8_t* msg, uint16_t componentId, uint16_t instanceId,
                           uint8_t componentClass, uint8_t state,
                           float d1, float d2, float d3,
                           float d4, float d5, float d6) {
    std::memset(msg, 0, cigi::kComponentControlSize);
    msg[0] = static_cast<uint8_t>(cigi::MsgComponentControl);
    msg[1] = static_cast<uint8_t>(cigi::kComponentControlSize);
    putU16(msg + 2, componentId);
    putU16(msg + 4, instanceId);
    msg[6] = componentClass;
    msg[7] = state;
    putF32(msg + 8,  d1);
    putF32(msg + 12, d2);
    putF32(msg + 16, d3);
    putF32(msg + 20, d4);
    putF32(msg + 24, d5);
    putF32(msg + 28, d6);
}

// One of the HOST's three cloud patterns, exactly as its capture carries it:
// component 0x1001 on the cloud entity 0x9001, class Entity, the state naming
// the pattern, and the footprint/altitude in the first three data fields.
// `state` of 0 is the same message with the pattern taken away -- and the data
// fields are still filled in, which is exactly what the captures send, so a
// mapping that keyed off the data rather than the state would leave the cloud
// standing.
void buildCloudPattern(uint8_t* msg, uint8_t state) {
    float d1 = 14816.0f, d2 = 12964.0f, d3 = 914.4f;   // cloud pat1: 8 x 7 nmi @ 3000 ft
    if (state == 2) {
        d1 = 11112.0f; d2 = 9260.0f; d3 = 1066.8f;     // cloud pat2: 6 x 5 nmi @ 3500 ft
    } else if (state == 3) {
        d1 = 18520.0f; d2 = 22224.0f; d3 = 914.4f;     // cloud pat3: 10 x 12 nmi @ 3000 ft
    }
    buildComponentControl(msg, 0x1001, 0x9001, cigi::kComponentEntity, state,
                          d1, d2, d3, 0.0f, 0.0f, 0.0f);
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
        simlink::publishEnvironment(simlink::Environment{});
        check(!simlink::health().mappingOpen, "and neither does publishing a world");
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
        check(simlink::kVersion == 5,
              "the contract version is 5 (the sim's sky read-back added)");
        check(sizeof(simlink::Held) == 80, "a Held row is 80 bytes");
        check(offsetof(simlink::Shared, entities) == 88, "entities[] starts at 88");
        check(offsetof(simlink::Shared, held) == 4696, "held[] starts at 4696");
        check(sizeof(simlink::View) == 64, "a View row is 64 bytes");
        check(offsetof(simlink::Shared, view) == 9816,
              "the view is appended after held[], so the tables keep their offsets");
        check(sizeof(simlink::Environment) == 128, "an Environment row is 128 bytes");
        check(sizeof(simlink::EnvironmentStatus) == 256,
              "an EnvironmentStatus row is 256 bytes");
        check(offsetof(simlink::Shared, environment) == 9880,
              "the world is appended after the view, so the tables keep their offsets");
        check(offsetof(simlink::Shared, environmentStatus) == 10008,
              "the add-on's answer goes beside it");
        check(offsetof(simlink::Shared, environmentStatus) +
                  offsetof(simlink::EnvironmentStatus, simMetar) == 10064,
              "and the station observation the sim holds sits at the end of it");
        check(sizeof(simlink::Shared) == 10264,
              "the region is the header, the entity/held/view tables and the world");

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

    // -- 3d. The HOST's world: clock, air and weather -------------------------
    // The environment path has the same shape as the poses above, and the same
    // opening trap: three messages that each carry part of one world, whose
    // fields are the wrong way round in a way that still yields plausible
    // numbers -- a date read without unpacking, a severity read from the layer
    // id, a METAR that rounds instead of reporting.  Everything here is the
    // real code: the decoders CigiStarter runs, the accumulation and the
    // resolution Environment.h owns, the METAR the add-on hands the sim, and
    // the publisher in SimLink.cpp.
    std::printf("\nthe HOST's world (time of day, air and weather)\n");
    {
        // -- The Celestial Sphere Control: the wall clock and the date.  (id 9)
        uint8_t cs[cigi::kCelestialSphereSize];
        buildCelestialSphere(cs, 12, 0, 0x11, 1012000u);   // 2000-01-01 12:00

        cigi::CelestialSphere c;
        check(cigi::parseCelestialSphere(cs, sizeof(cs), c),
              "a well-formed Celestial Sphere Control is decoded");
        check(c.hour == 12 && c.minute == 0,
              "the clock is the bytes at 2 and 3");
        check(c.dateValid(), "date valid is bit 4 of the flags at 4");
        check(c.date == 1012000u, "the packed date is the uint32 at 8");
        check(!cigi::parseCelestialSphere(cs, sizeof(cs) - 1, c),
              "a truncated Celestial Sphere Control is refused");
        cs[0] = 99;
        check(!cigi::parseCelestialSphere(cs, sizeof(cs), c),
              "a message with the wrong id is refused");
        cs[0] = static_cast<uint8_t>(cigi::MsgCelestialSphere);

        // -- The Atmosphere Control: the air the aircraft flies in.  (id 10)
        uint8_t at[cigi::kAtmosphereSize];
        buildAtmosphere(at, 80, 29.9877f, 20000.0f, 0.0f, 0.0f, 0.0f, 1013.25f);

        cigi::Atmosphere a;
        check(cigi::parseAtmosphere(at, sizeof(at), a),
              "a well-formed Atmosphere Control is decoded");
        check(a.humidity == 80, "the humidity is the byte at 3");
        check(std::fabs(a.airTempC - 29.9877f) < 1e-3,
              "the temperature is the float at 4");
        check(std::fabs(a.visibilityM - 20000.0f) < 1.0,
              "the visibility is the float at 8");
        check(std::fabs(a.baroPressureMb - 1013.25f) < 0.01,
              "the pressure is the float at 24");
        check(!cigi::parseAtmosphere(at, sizeof(at) - 1, a),
              "a truncated Atmosphere Control is refused");
        at[0] = 99;
        check(!cigi::parseAtmosphere(at, sizeof(at), a),
              "a message with the wrong id is refused");
        at[0] = static_cast<uint8_t>(cigi::MsgAtmosphere);

        // -- The Weather Control: cloud and precipitation.  (id 12)
        // No capture carries one, so this is the ICD's layout pinned rather than
        // a recording replayed: a cumulus deck, 60% cover, 900 m base, rain.
        uint8_t wc[cigi::kWeatherControlSize];
        buildWeather(wc, /*layer*/ 4, /*humidity*/ 80, /*flags*/ 0x71,
                     /*flags2*/ 0x0C, 29.9877f, 20000.0f, 60.0f, 900.0f,
                     600.0f, 0.0f, 0.0f, 0.0f, 1013.25f);

        cigi::WeatherControl w;
        check(cigi::parseWeatherControl(wc, sizeof(wc), w),
              "a well-formed Weather Control is decoded");
        check(w.layerId == 4,
              "the layer id at 4 is what names the precipitation -- not the severity");
        check(w.cloudType() == 7, "the cloud type is the high nibble of the flags at 6");
        check(w.severity() == 3, "the severity is bits 2-4 of the flags at 7");
        check(!w.lightningEnabled(), "lightning is bit 3 of the flags");
        check(w.coverage >= 59.99f && w.coverage <= 60.01f,
              "the coverage is the float at 20");
        check(std::fabs(w.baseElevM - 900.0f) < 0.01f,
              "and the cloud base is the float at 24");
        check(!cigi::parseWeatherControl(wc, sizeof(wc) - 1, w),
              "a truncated Weather Control is refused");
        wc[0] = 99;
        check(!cigi::parseWeatherControl(wc, sizeof(wc), w),
              "a message with the wrong id is refused");
        wc[0] = static_cast<uint8_t>(cigi::MsgWeatherControl);

        // The layer id is resolved to a kind that falls, because the sim's
        // weather system has no notion of CIGI's numbering.  The ids that are
        // not precipitation -- ground fog, and the three cloud layers -- must
        // come back as none rather than defaulting to rain.
        check(cigienv::precipTypeFromLayer(4) == simlink::kPrecipRain &&
              cigienv::precipTypeFromLayer(5) == simlink::kPrecipSnow &&
              cigienv::precipTypeFromLayer(6) == simlink::kPrecipSleet &&
              cigienv::precipTypeFromLayer(7) == simlink::kPrecipHail &&
              cigienv::precipTypeFromLayer(8) == simlink::kPrecipSand &&
              cigienv::precipTypeFromLayer(9) == simlink::kPrecipDust,
              "layer ids 4-9 name rain/snow/sleet/hail/sand/dust");
        check(cigienv::precipTypeFromLayer(0) == simlink::kPrecipNone &&
              cigienv::precipTypeFromLayer(1) == simlink::kPrecipNone &&
              cigienv::precipTypeFromLayer(3) == simlink::kPrecipNone,
              "and ground fog and the cloud layers are no precipitation");

        // -- Accumulating the world, one message at a time -------------------
        // The messages are partial and arrive independently, so the fold is
        // sticky: each fills its own part and leaves the rest standing.
        cigienv::HostEnvironment host;
        check(!host.haveTime && !host.haveAtmos && !host.haveWeather,
              "a world nothing has been sent for is empty");

        cigienv::applyCelestialSphere(host, c);
        check(host.haveTime && host.hour == 12 && host.minute == 0,
              "a Celestial Sphere Control fills the clock");
        check(host.dateValid && host.year == 2000 && host.month == 1 && host.day == 1,
              "and unpacks Month*1000000 + Day*10000 + Year");

        cigienv::applyAtmosphere(host, a);
        check(host.haveAtmos && host.humidityPct == 80 &&
              std::fabs(host.baroPressureMb - 1013.25f) < 0.01,
              "an Atmosphere Control fills the air");
        check(host.hour == 12 && !host.haveWeather,
              "and leaves the clock and the weather exactly as they were");

        // The Weather Control carries the air again; the newest wins.
        cigi::WeatherControl wRain = w;
        wRain.airTempC = 21.0f;
        cigienv::applyWeather(host, wRain);
        check(host.haveWeather && host.cloudType == 7 &&
              std::fabs(host.coverage - 60.0) < 0.01 &&
              host.precipType == simlink::kPrecipRain && host.severity == 3,
              "a Weather Control fills the cloud and resolves the precipitation");
        check(host.haveAtmos && std::fabs(host.airTempC - 21.0f) < 0.01,
              "and refreshes the air it also carries (newest wins)");

        // Now put the temperature back for the METAR below, whose exact string
        // is easier to read with the numbers the captures carry.
        cigienv::applyWeather(host, w);
        check(std::fabs(host.airTempC - 29.9877f) < 1e-3,
              "the newest message is the one the world carries");

        // -- Resolving with the operator's override on top -------------------
        using cigienv::TimePreset;
        using cigienv::WeatherPreset;

        // Host/Host: exactly the HOST's world, no override claimed.
        simlink::Environment hostWorld =
            cigienv::resolve(host, TimePreset::Host, WeatherPreset::Host);
        check(hostWorld.hour == 12 && hostWorld.minute == 0 && hostWorld.second == 0,
              "the HOST's own clock is carried through");
        check((hostWorld.flags & simlink::kEnvTimeManual) == 0 &&
              (hostWorld.flags & simlink::kEnvWeatherManual) == 0,
              "with no operator override claimed");
        check((hostWorld.flags & simlink::kEnvDateValid) != 0 &&
              hostWorld.year == 2000 && hostWorld.month == 1 && hostWorld.day == 1,
              "the date is present and rides with the clock");
        check(hostWorld.cloudType == 7 && hostWorld.precipType == simlink::kPrecipRain &&
              hostWorld.severity == 3,
              "and the HOST's own cloud and precipitation stand");
        check((hostWorld.flags & simlink::kEnvTimeValid) != 0 &&
              (hostWorld.flags & simlink::kEnvAtmosValid) != 0 &&
              (hostWorld.flags & simlink::kEnvWeatherValid) != 0,
              "and every group is marked present, so all of it will be applied");

        char metar[256];
        int n = cigienv::metarFor(hostWorld, metar, sizeof(metar));
        check(n > 0 &&
              std::strcmp(metar, "GLOB 1200Z 00000KT 9999 RA BKN030 30/26 Q1013") == 0,
              "the HOST's world renders the expected METAR");

        // The clock is the one group an operator preset owns, and pinning it
        // must not touch the sky -- a clear afternoon is still a clear
        // afternoon at dusk.
        simlink::Environment dawn =
            cigienv::resolve(host, TimePreset::Dawn, WeatherPreset::Host);
        check(dawn.hour == 6 && dawn.minute == 15 && dawn.second == 0,
              "Dawn pins 06:15");
        check((dawn.flags & simlink::kEnvTimeManual) != 0,
              "and says the clock is the operator's, not the HOST's");
        check((dawn.flags & simlink::kEnvDateValid) != 0 && dawn.year == 2000,
              "while the date is still the HOST's -- pinning a time is not choosing a day");
        check(dawn.cloudType == 7 && dawn.precipType == simlink::kPrecipRain,
              "and the sky is untouched by a clock preset");

        // The weather preset is the mirror: it replaces cloud, precipitation and
        // visibility, and deliberately leaves temperature, pressure and wind to
        // the flight model.
        simlink::Environment storm =
            cigienv::resolve(host, TimePreset::Host, WeatherPreset::Storm);
        check(storm.cloudType == 6 && storm.coverage == 100.0 && storm.severity == 5 &&
              storm.precipType == simlink::kPrecipRain,
              "Storm is a full cumulonimbus deck of rain");
        check((storm.flags & simlink::kEnvLightning) != 0,
              "with lightning, which is the bit the METAR turns into TS");
        check((storm.flags & simlink::kEnvWeatherManual) != 0,
              "and says the weather is the operator's");
        check(storm.visibilityMeters == 1500.0,
              "the preset brings its own visibility");
        check(storm.airTempC == host.airTempC &&
              storm.baroPressureMb == host.baroPressureMb &&
              storm.horizWindMps == host.horizWindMps,
              "but leaves the air -- temperature, pressure and wind -- to the HOST");
        check(storm.hour == 12 && storm.minute == 0,
              "and the clock to the HOST as well");

        n = cigienv::metarFor(storm, metar, sizeof(metar));
        check(n > 0 &&
              std::strcmp(metar, "GLOB 1200Z 00000KT 1500 +TSRA OVC013CB 30/26 Q1013") == 0,
              "a storm renders as heavy rain, a thunderstorm and a CB deck");

        // Clear is an instruction, not an absence: it has to name a clear sky,
        // or switching back from Cloudy would leave the sim's last observation
        // standing.
        simlink::Environment clear =
            cigienv::resolve(host, TimePreset::Host, WeatherPreset::Clear);
        check(clear.cloudType == 0 && clear.coverage == 0.0 &&
              clear.precipType == simlink::kPrecipNone,
              "Clear takes the cloud and the precipitation away");
        check(clear.visibilityMeters == 200000.0, "and opens the visibility");
        n = cigienv::metarFor(clear, metar, sizeof(metar));
        check(n > 0 &&
              std::strcmp(metar, "GLOB 1200Z 00000KT 9999 SKC 30/26 Q1013") == 0,
              "and the observation names SKC rather than omitting the cloud");

        // A sky with no air: an operator's preset is a sky without an
        // Atmosphere Control behind it, and the METAR must name only what is
        // actually present.  The wind, temperature/dew point and pressure
        // groups are air, and carrying the resolved row's place-holder 0 C /
        // 1013 mb would parse those into the sim.
        {
            cigienv::HostEnvironment noAir;
            simlink::Environment skyOnly =
                cigienv::resolve(noAir, TimePreset::Host, WeatherPreset::Storm);
            check((skyOnly.flags & simlink::kEnvAtmosValid) == 0 &&
                  (skyOnly.flags & simlink::kEnvWeatherValid) != 0,
                  "a preset sky with no HOST atmosphere is present without air");
            n = cigienv::metarFor(skyOnly, metar, sizeof(metar));
            check(n > 0 &&
                  std::strcmp(metar, "GLOB 0000Z 1500 +TSRA OVC013CB") == 0,
                  "so its METAR carries the sky and not the air-derived groups");
        }

        // The dew point is derived, not round-tripped: the sim wants a number
        // where the HOST gives temperature and humidity.
        check(std::fabs(cigienv::dewPointC(30.0, 80.0) - 26.15) < 0.2,
              "the dew point is the Magnus approximation");
        check(std::fabs(cigienv::dewPointC(20.0, 100.0) - 20.0) < 1e-6,
              "saturated air has a dew point equal to its temperature");
        check(std::isfinite(cigienv::dewPointC(20.0, 0.0)),
              "and an absent humidity is clamped rather than sent to infinity");

        // -- Publishing: write on change, name the group that moved ----------
        // Begin with the operator's clock pinned to dawn and the HOST's own
        // sky, so the change that follows is a sky change and nothing else:
        // both rows agree on the clock already.
        simlink::Environment pinnedDawn =
            cigienv::resolve(host, TimePreset::Dawn, WeatherPreset::Host);
        simlink::publishEnvironment(pinnedDawn);

        simlink::Environment row{};
        check(simlink::readEnvironmentRow(fake.shm()->environment, row),
              "a published world is readable under its seqlock");
        check(row.generation == 1, "the first write is generation 1");
        check((row.flags & simlink::kEnvApplyTime) != 0 &&
              (row.flags & simlink::kEnvApplyAtmos) != 0 &&
              (row.flags & simlink::kEnvApplyWeather) != 0,
              "and marks every group as changed, because the add-on has seen none of it");
        check(row.hour == 6 && row.minute == 15 && row.cloudType == 7 &&
              row.precipType == simlink::kPrecipRain,
              "with the resolved values");
        check((row.flags & simlink::kEnvTimeManual) != 0,
              "the operator's clock override rides through as state");

        simlink::Health h = simlink::health();
        check(h.envCarried, "the writer reports the world as carried");
        check(h.envGeneration == 1, "and the generation it is at");

        // The repeat the HOST sends every frame is dropped: re-publishing the
        // same world must not advance the generation, because the add-on keys
        // its clock-set on that generation and re-setting it would stop it.
        simlink::publishEnvironment(pinnedDawn);
        check(simlink::health().envGeneration == 1,
              "an unchanged world is not published again (the generation does not move)");

        // A sky-only change: the same clock, a different cloud.  It must move
        // the weather group -- and the air group with it, because the preset's
        // visibility is part of the air -- but leave the clock alone.
        simlink::Environment stormDawn =
            cigienv::resolve(host, TimePreset::Dawn, WeatherPreset::Storm);
        simlink::publishEnvironment(stormDawn);
        check(simlink::health().envGeneration == 2,
              "a changed sky advances the generation");
        check(simlink::readEnvironmentRow(fake.shm()->environment, row) &&
              row.generation == 2,
              "and is written");
        check((row.flags & simlink::kEnvApplyWeather) != 0,
              "the weather group is flagged as changed");
        check((row.flags & simlink::kEnvApplyTime) == 0,
              "and the clock is not, so a new sky does not move the time");
        check((row.flags & simlink::kEnvApplyAtmos) != 0,
              "the preset's visibility is air, so that group moves too");
        check((row.flags & simlink::kEnvWeatherManual) != 0,
              "and the weather override is visible as state");

        // A clock-only change: the same storm, a different hour.  The mirror
        // check -- a new clock must not re-send the sky or the air.
        simlink::Environment stormDay =
            cigienv::resolve(host, TimePreset::Day, WeatherPreset::Storm);
        simlink::publishEnvironment(stormDay);
        check(simlink::health().envGeneration == 3,
              "a changed clock advances the generation again");
        check(simlink::readEnvironmentRow(fake.shm()->environment, row) &&
              row.generation == 3 && row.hour == 12,
              "and is written");
        check((row.flags & simlink::kEnvApplyTime) != 0,
              "the clock group is flagged as changed");
        check((row.flags & simlink::kEnvApplyWeather) == 0,
              "and the sky is not, so a new clock does not re-send the weather");
        check((row.flags & simlink::kEnvApplyAtmos) == 0,
              "nor the air, which the storm did not change either");

        // Both overrides at once, which is the case the window's two combo
        // boxes make: the state bits for both have to survive one publish,
        // because the publisher replaces the flags word rather than OR-ing it.
        simlink::Environment both =
            cigienv::resolve(host, TimePreset::Dawn, WeatherPreset::Storm);
        simlink::publishEnvironment(both);
        check(simlink::health().envGeneration == 4,
              "an operator with both overrides set publishes one world");

        // The add-on's own answer, which CigiStarter reads for the window: the
        // generation it applied and the time of day the sim actually reported.
        // The DLL writes this; the harness stands in for it.
        {
            simlink::EnvironmentStatus& st = fake.shm()->environmentStatus;
            st.appliedGeneration = 4;
            st.applies           = 4;
            st.flags             = row.flags;
            st.timeOfDay         = 0;   // TIMEOFDAY::DAWN, as the sim might report
            st.lastError         = 0;
            // The sky the sim says it holds, in the shape the add-on writes it:
            // a cumulus layer at 914 m, broken, with the observation it parsed.
            st.cloudLayers        = 1;
            st.simCloudType       = 3;   // P3D::CLOUD_CUMULUS
            st.simCloudCover      = 5;   // P3D::CLOUD_BROKEN_5_8
            st.simPrecipType      = 0;   // P3D::PRECIP_NONE
            st.simCloudBaseMeters = 914.4;
            st.simCloudTopsMeters = 1814.4;
            std::strncpy(st.simMetar, "GLOB 1200Z 00000KT 9999 BKN030 30/26 Q1013",
                         sizeof(st.simMetar) - 1);
            st.simMetar[sizeof(st.simMetar) - 1] = '\0';
        }
        h = simlink::health();
        check(h.envApplies == 4, "the writer surfaces the generations the add-on applied");
        check(h.envTimeOfDay == 0,
              "and the time of day the sim reported back -- how a test sees the clock move");
        check(h.envManualTime && h.envManualWeather,
              "the operator's overrides are visible in the writer's health");

        // The sky read-back, which is the add-on's answer to "did the cloud
        // actually reach the sim?" rather than "was it written into the region?".
        check(h.envSimCloudLayers == 1 && h.envSimCloudType == 3 &&
              h.envSimCloudCover == 5 && h.envSimPrecipType == 0,
              "the sim's own cloud layer is surfaced to the window and the tests");
        check(std::fabs(h.envSimCloudBaseM - 914.4) < 1e-9 &&
              std::fabs(h.envSimCloudTopsM - 1814.4) < 1e-9,
              "with the metres the sim reports, not the ones that were asked for");
        {
            simlink::EnvironmentStatus stSky{};
            check(simlink::readEnvironmentStatus(fake.shm()->environmentStatus, stSky) &&
                  std::strcmp(stSky.simMetar,
                              "GLOB 1200Z 00000KT 9999 BKN030 30/26 Q1013") == 0,
                  "and the station observation the sim holds comes back as a string");
        }
        check(cigienv::p3dCloudTypeName(3)[0] == 'c' &&
              std::strcmp(cigienv::p3dCloudCoverName(5), "BKN") == 0 &&
              std::strcmp(cigienv::p3dCloudCoverName(0), "clear") == 0,
              "the read-back's own numbering is named the way the window shows it");

        simlink::EnvironmentStatus stIn{};
        check(simlink::readEnvironmentStatus(fake.shm()->environmentStatus, stIn) &&
              stIn.applies == 4 && stIn.appliedGeneration == 4,
              "and the add-on's status row reads back under its own seqlock");

        // The group-presence gate: a world the HOST has never spoken for must
        // not become a command.  This is the real HOST's case -- its per-frame
        // list has an Atmosphere Control and no Celestial Sphere or Weather
        // Control -- and without the gate the 00:00 and clear-sky defaults
        // would pin the sim's clock to midnight and clear its weather on the
        // first atmosphere message.
        {
            cigienv::HostEnvironment empty;
            simlink::Environment quiet =
                cigienv::resolve(empty, TimePreset::Host, WeatherPreset::Host);
            check((quiet.flags & simlink::kEnvTimeValid) == 0 &&
                  (quiet.flags & simlink::kEnvAtmosValid) == 0 &&
                  (quiet.flags & simlink::kEnvWeatherValid) == 0,
                  "a world the HOST never spoke for has no group present");

            simlink::publishEnvironment(quiet);
            simlink::Environment quietRow{};
            check(simlink::readEnvironmentRow(fake.shm()->environment, quietRow) &&
                  quietRow.generation == 5,
                  "publishing it still records a generation");
            check((quietRow.flags & simlink::kEnvApplyTime) == 0 &&
                  (quietRow.flags & simlink::kEnvApplyAtmos) == 0 &&
                  (quietRow.flags & simlink::kEnvApplyWeather) == 0,
                  "but applies nothing -- the sim keeps its own clock and sky");
        }

        // -- The cloud pattern: the HOST's other way of describing a sky ------
        // The three `cloud pat*.bin` captures have no Weather Control at all.
        // What they have instead is a Component Control (id 4) on the entity
        // they create for it, and it is the only description of their sky there
        // is.  The wire layout is pinned first, then the mapping, then the
        // publication -- a pattern that decodes perfectly and never reaches the
        // add-on's weather group is a message that renders nothing.
        std::printf("\nthe HOST's cloud pattern (a Component Control, id 4)\n");
        {
            uint8_t cc[cigi::kComponentControlSize];
            buildCloudPattern(cc, 1);

            cigi::ComponentControl c;
            check(cigi::parseComponentControl(cc, sizeof(cc), c),
                  "a well-formed Component Control is decoded");
            check(c.componentId == 0x1001,
                  "the component id is the uint16 at 2");
            check(c.instanceId == 0x9001,
                  "the instance -- the entity it is attached to -- is the uint16 at 4");
            check(c.componentClass == cigi::kComponentEntity,
                  "the class is the byte at 6");
            check(c.componentState == 1, "the state is the byte at 7");
            check(std::fabs(c.data1 - 14816.0f) < 0.01f &&
                  std::fabs(c.data2 - 12964.0f) < 0.01f &&
                  std::fabs(c.data3 - 914.4f) < 0.01f,
                  "the first three data words are the floats at 8, 12 and 16");
            check(c.data4 == 0.0f && c.data5 == 0.0f && c.data6 == 0.0f,
                  "and the last three are the zeroes the captures send");
            check(!cigi::parseComponentControl(cc, sizeof(cc) - 1, c),
                  "a truncated Component Control is refused");
            {
                const uint8_t saved = cc[0];
                cc[0] = 99;
                check(!cigi::parseComponentControl(cc, sizeof(cc), c),
                      "a message with the wrong id is refused");
                cc[0] = saved;
            }

            check(cigi::parseComponentControl(cc, sizeof(cc), c) &&
                  cigienv::isCloudPattern(c),
                  "the HOST's cloud pattern is recognised by its component id");
            // A component that is neither the id nor the class is not a sky.
            // The class is the trap: what the vendor describes is weather but
            // what it *sends* is class Entity, so the check has to be Entity
            // (what the capture carries) and not Global Layered Weather (what
            // the ICD would call it) -- while still refusing an unrelated
            // component that merely shares the id.
            cigi::ComponentControl other = c;
            other.componentId = 0x1002;
            check(!cigienv::isCloudPattern(other),
                  "another component id is not the cloud pattern");
            other = c;
            other.componentClass = cigi::kComponentGlobalLayeredWeather;
            check(!cigienv::isCloudPattern(other),
                  "and neither is the pattern's id on a class that is not an entity's");

            // Each pattern's sky, from the numbers its own capture carries: the
            // base is the pattern's altitude (3000 ft, sent as 914.4 m) and the
            // coverage is its footprint (8 x 7 nmi = 56 square nautical miles).
            const cigienv::WeatherSpec p1 = cigienv::cloudPatternSpec(1, c);
            check(p1.cloudType == cigienv::kCloudCumulus,
                  "a pattern is a cumulus deck -- the wire carries no type");
            check(std::fabs(p1.baseElevMeters - 914.4) < 0.05,
                  "its base is the altitude in data 3 (914.4 m = 3000 ft)");
            check(std::fabs(p1.coverage - 56.0) < 0.5,
                  "and its coverage is the footprint's area in square nautical miles");
            check(p1.precipType == simlink::kPrecipNone && !p1.lightning &&
                  p1.severity == 0,
                  "with no precipitation -- a cloud pattern is cloud, not rain");

            cigi::ComponentControl c2;
            buildCloudPattern(cc, 2);
            check(cigi::parseComponentControl(cc, sizeof(cc), c2),
                  "the second capture's pattern is decoded too");
            const cigienv::WeatherSpec p2 = cigienv::cloudPatternSpec(2, c2);
            check(std::fabs(p2.baseElevMeters - 1066.8) < 0.05 &&
                  std::fabs(p2.coverage - 30.0) < 0.5,
                  "it is its own footprint and altitude, not the first pattern's");

            cigi::ComponentControl c3;
            buildCloudPattern(cc, 3);
            check(cigi::parseComponentControl(cc, sizeof(cc), c3),
                  "and the third");
            const cigienv::WeatherSpec p3 = cigienv::cloudPatternSpec(3, c3);
            check(std::fabs(p3.coverage - 100.0) < 0.01,
                  "a pattern wider than the coverage scale is capped at 100 %");
            check(p3.baseElevMeters > 900.0 && p3.baseElevMeters < 920.0,
                  "and keeps its own 3000 ft base");
            // The three captures are three different skies, which is the whole
            // point of a pattern; a mapping that flattened them would show here.
            check(p1.coverage != p2.coverage && p2.coverage != p3.coverage,
                  "the three patterns resolve to three different skies");

            // State 0: the HOST taking the cloud away (it sends it a thousand
            // datagrams later, with the data fields still filled in) has to be
            // an explicit clear sky, because a group that is merely absent
            // leaves the sim holding the last pattern.
            cigi::ComponentControl off;
            buildCloudPattern(cc, 0);
            check(cigi::parseComponentControl(cc, sizeof(cc), off) &&
                  off.componentState == 0,
                  "the state-0 form of the message is the same message");
            const cigienv::WeatherSpec none = cigienv::cloudPatternSpec(0, off);
            check(none.cloudType == cigienv::kCloudNone && none.coverage == 0.0,
                  "and it takes the cloud away rather than leaving the last pattern");
            check(cigienv::cloudPatternSpec(0, off).precipType == simlink::kPrecipNone,
                  "without inventing any precipitation");

            // Folded in, a pattern *is* the world's sky, and it is marked as the
            // pattern so a reader can tell it from a Weather Control's.
            cigienv::HostEnvironment cp;
            cigienv::applyCloudPattern(cp, c);
            check(cp.haveWeather && cp.havePattern && cp.patternIndex == 1,
                  "the pattern is folded in as the world's sky");
            check(!cp.haveAtmos,
                  "and brings no air with it -- a pattern is a sky, not an atmosphere");

            simlink::Environment patWorld =
                cigienv::resolve(cp, TimePreset::Host, WeatherPreset::Host);
            check((patWorld.flags & simlink::kEnvWeatherValid) != 0,
                  "so the resolved world has a sky present");
            check((patWorld.flags & simlink::kEnvWeatherPattern) != 0,
                  "marked as the HOST's cloud pattern rather than a Weather Control");
            check((patWorld.flags & simlink::kEnvAtmosValid) == 0,
                  "with no air group: the real HOST's cloud captures carry no Atmosphere Control");
            check((patWorld.flags & simlink::kEnvTimeValid) == 0,
                  "and no clock, so the sim keeps its own time through a cloud capture");

            n = cigienv::metarFor(patWorld, metar, sizeof(metar));
            check(n > 0 && std::strcmp(metar, "GLOB 0000Z 9999 BKN030") == 0,
                  "and its METAR is the sky alone: broken cloud at 3000 ft, "
                  "no wind, temperature or pressure");

            // The operator's own weather still wins over the HOST's pattern, the
            // same way it wins over a Weather Control.
            simlink::Environment patStorm =
                cigienv::resolve(cp, TimePreset::Host, WeatherPreset::Storm);
            check(patStorm.cloudType == 6 && (patStorm.flags & simlink::kEnvWeatherManual) != 0,
                  "an operator's weather preset overrides the HOST's pattern");
            check((patStorm.flags & simlink::kEnvWeatherPattern) == 0,
                  "and the pattern bit goes with the sky it described");

            // And it publishes: the pattern has to reach the add-on as a weather
            // change, or the cloud never gets built.
            simlink::publishEnvironment(patWorld);
            simlink::Environment patRow{};
            check(simlink::readEnvironmentRow(fake.shm()->environment, patRow),
                  "the pattern's world is published");
            check((patRow.flags & simlink::kEnvApplyWeather) != 0,
                  "and the add-on is told to re-apply the sky");
            check((patRow.flags & simlink::kEnvApplyAtmos) == 0,
                  "but not the air, which the pattern did not touch");
            check((patRow.flags & simlink::kEnvWeatherPattern) != 0,
                  "with the pattern bit carried as state");
            check(simlink::health().envWeatherPattern,
                  "so the window and the tests can see the sky came from a pattern");

            simlink::EnvironmentStatus patSt{};
            check(simlink::readEnvironmentStatus(fake.shm()->environmentStatus, patSt),
                  "the add-on's status row reads back with the sky in it");
        }
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
