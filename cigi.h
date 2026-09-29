//=============================================================================
//  cigi.h — Common Image Generator Interface (CIGI) 3.3 message definitions
//
//  Purpose
//    Minimal, dependency-free definitions for the CIGI 3.3 protocol as used
//    between a flight-simulation HOST and an Image Generator (IG). Only the
//    messages required by this project are declared here.
//
//  Byte order
//    This project targets x64 (little-endian) exclusively. CIGI 3.3 uses
//    little-endian on the wire in this deployment, so host-order == wire-order
//    and packing helpers below simply memcpy. See cigi.cpp for the packers.
//
//  Reference
//    Common Image Generator Interface (CIGI) ICD, version 3.3.
//=============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

namespace cigi {

//-----------------------------------------------------------------------------
// CIGI 3.x message identifiers (host <-> IG)
//-----------------------------------------------------------------------------
enum MsgId : uint8_t {
    MsgIGControl         = 1,   // host -> IG : IG control / mode
    MsgEntityControl     = 2,   // host -> IG : entity state + pose
    MsgComponentControl  = 4,   // host -> IG : a component attached to an entity
    MsgCelestialSphere   = 9,   // host -> IG : time of day / date
    MsgAtmosphere        = 10,  // host -> IG : temperature, visibility, wind, pressure
    MsgWeatherControl    = 12,  // host -> IG : cloud, precipitation, region weather
    MsgViewControl       = 16,  // host -> IG : where the HOST's own eye is
    MsgHATHOTRequest     = 24,  // host -> IG : height-above/height-of terrain request
    MsgStartOfFrame      = 101, // IG -> host : start-of-frame (frame sync)
    MsgHATHOTResponse    = 102, // IG -> host : HAT/HOT response
    MsgHATHOTExtResponse = 103, // IG -> host : HAT/HOT extended response
};

//-----------------------------------------------------------------------------
// Message sizes (total, including the 2-byte [id][size] header)
//-----------------------------------------------------------------------------
constexpr uint8_t kSofSize            = 24;  // Start of Frame
constexpr uint8_t kEntityControlSize  = 48;  // Entity Control
constexpr uint8_t kComponentControlSize = 32; // Component Control
constexpr uint8_t kCelestialSphereSize = 16; // Celestial Sphere Control
constexpr uint8_t kAtmosphereSize      = 32; // Atmosphere Control
constexpr uint8_t kWeatherControlSize  = 56; // Weather Control
constexpr uint8_t kViewControlSize    = 32;  // View Control
constexpr uint8_t kHatHotReqSize      = 32;  // HAT/HOT Request
constexpr uint8_t kHatHotRespSize     = 16;  // HAT/HOT Response
constexpr uint8_t kHatHotExtRespSize  = 40;  // HAT/HOT Extended Response

//-----------------------------------------------------------------------------
// Start-of-Frame header field values
//-----------------------------------------------------------------------------
constexpr uint8_t kSofMessageId   = 0x65;  // 101
constexpr uint8_t kSofVersion     = 0x03;  // CIGI 3
// Byte 5 packs: IG mode (bits 0-1), timestamp-valid (bit 2), earth-reference
// model (bit 3) and the CIGI minor version (bits 4-7). 3.3 => 0x30.
// The two named values are simply the common cases of sofIgModeByte() below.
constexpr uint8_t kSofIgModeStandby = 0x30;  // IG mode = 0 (Reset/Standby)
constexpr uint8_t kSofIgModeOperate = 0x31;  // IG mode = 1 (Operate)

// Build SOF byte 5 for an arbitrary CIGI IG mode (0..3), keeping the CIGI 3.3
// minor-version bits. An IG does not choose its own mode: it mirrors back the
// mode the HOST last commanded in an IG Control packet, so callers feed the
// decoded Host mode in here.
inline uint8_t sofIgModeByte(uint8_t igMode) {
    return static_cast<uint8_t>(kSofIgModeStandby | (igMode & 0x03u));
}

//-----------------------------------------------------------------------------
// Struct views (documentation / field layout). Packing is done explicitly in
// cigi.cpp so the wire bytes are unambiguous regardless of compiler packing.
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct StartOfFrame {
    uint8_t  msgId;          // 0x65
    uint8_t  msgSize;        // 24
    uint8_t  version;        // 3
    uint8_t  dbNumber;       // 0
    uint8_t  igStatus;       // 0
    uint8_t  igModeByte;     // 0x30
    uint8_t  byteSwap[2];    // { 0x00, 0x80 } => little-endian
    uint32_t hostFrameNumber;// the HOST's frame counter, echoed (see below)
    uint32_t timestamp;      // 0 -- the reference Master never sent one
    uint32_t lastHostFrame;  // 0 -- ditto: the counter rides in the slot above
    uint32_t reserved;       // 0
};
#pragma pack(pop)
static_assert(sizeof(StartOfFrame) == 24, "StartOfFrame must be 24 bytes");

//-----------------------------------------------------------------------------
// Packing helpers
//-----------------------------------------------------------------------------

// Pack a 24-byte Start-of-Frame message into `out` (must hold >= 24 bytes).
//
// `hostFrameNumber` is the frame counter out of the HOST's last IG Control, and
// it goes in the frame-number slot at offset 8 -- an IG-side count of the SOFs
// this program has sent must NOT be put there. That is what the reference Master
// IG sent: every SOF in `Host-Master.pcapng` carries it in that slot, advancing
// ~0.52 per SOF, which tracks the HOST's counter (0.54 per datagram) rather
// than the SOF rate -- the HOST emits a datagram every ~10.2 ms (~98 Hz) while
// its frame counter only advances ~53 times a second.
//
// Offsets 12 (timestamp) and 16 (last host frame) and 20 (reserved) are zero,
// again as in the capture. See cigi.cpp for the field-by-field layout, and
// SofPacer.h for why exceeding the HOST's own frame rate with an IG-side counter
// used to take the HOST down.
void packStartOfFrame(uint8_t* out, uint32_t hostFrameNumber, uint8_t igModeByte);

// Pack a 16-byte HAT/HOT Response (message id 102) into `out` (>= 16 bytes).
//
//   off  len  field
//   0    1    message id      = 102
//   1    1    message size    = 16
//   2    2    HAT/HOT ID      (uint16 LE, echoed from the request)
//   4    1    flags: bit 0 Valid, bit 1 Response Type, bits 4-7 Host Frame LSN
//   5    3    reserved        = 0
//   8    8    Height          (double LE, metres)
//
// `responseType` is the request's type (0 HAT, 1 HOT) and selects what
// `height` means: HAT (m above the terrain) or HOT (m MSL). `valid` is 0 or 1.
void packHatHotResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                        uint8_t responseType, uint8_t hostFrameLsn, double height);

// Pack a 40-byte HAT/HOT Extended Response (message id 103) into `out`.
//
//   off  len  field
//   0    1    message id      = 103
//   1    1    message size    = 40
//   2    2    HAT/HOT ID      (uint16 LE)
//   4    1    flags: bit 0 Valid, bits 4-7 Host Frame Number LSN
//   5    3    reserved        = 0     (aligns HAT to an 8-byte boundary)
//   8    8    HAT             (double LE, metres above the terrain surface)
//   16   8    HOT             (double LE, metres MSL)
//   24   4    Material Code   (uint32 LE)
//   28   4    Norm Vec Azimuth(float LE, degrees)
//   32   4    Norm Vec Elev.  (float LE, degrees)
//   36   4    reserved        = 0
//
// Do NOT move HAT/HOT to offsets 5/13: CIGI 3.2 packed them there, but 3.3
// inserted the 3-byte reserved gap so both doubles are 8-byte aligned. Getting
// this wrong makes the HOST read HOT out of the material-code bytes (i.e.
// zero) and drive the aircraft into the ground. `valid` is 0 or 1.
void packHatHotExtResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                           uint8_t hostFrameLsn, double hat, double hot);

//-----------------------------------------------------------------------------
// HAT/HOT Request (message id 24) decoding
//-----------------------------------------------------------------------------
enum HatHotRequestType : uint8_t {
    kHatHotRequestHat      = 0,  // sentinel: answer with the HAT/HOT Response
    kHatHotRequestHot      = 1,  //          "
    kHatHotRequestExtended = 2,  // answer with the HAT/HOT Extended Response
};

enum HatHotCoordinateSystem : uint8_t {
    kHatHotGeodetic = 0,  // X/Y/Z are latitude (deg), longitude (deg), altitude (m MSL)
    kHatHotEntity   = 1,  // X/Y/Z are body offsets (m) from Entity ID
};

// One decoded HAT/HOT Request.
//
// Entity-relative offsets use the entity's local NED frame: +X out the nose,
// +Y out the right wing, +Z out the *bottom* (i.e. a positive Z offset is
// below the reference point). See cigi.cpp for the wire layout.
struct HatHotRequest {
    uint16_t hatHotId     = 0;
    uint8_t  requestType  = 0;  // HatHotRequestType
    uint8_t  coordSystem  = 0;  // HatHotCoordinateSystem
    uint8_t  updatePeriod = 0;  // 0 = one-shot; n = answer every n-th frame
    uint16_t entityId     = 0;
    double   x = 0.0;
    double   y = 0.0;
    double   z = 0.0;
};

// Decode a HAT/HOT Request message. Returns false when `msg` is not a
// well-formed HAT/HOT Request (wrong id, or shorter than the 32-byte form).
bool parseHatHotRequest(const uint8_t* msg, size_t size, HatHotRequest& out);

//-----------------------------------------------------------------------------
// Entity Control (message id 2) decoding
//-----------------------------------------------------------------------------
// The pose the HOST is commanding for one entity, as carried by its Entity
// Control message. Entity 0 is the aircraft the HOST's own flight model flies,
// and the entity every entity-relative HAT/HOT request is anchored to, so this
// pose -- not the sim's own position -- is what those answers must be computed
// from: the HOST asks about points around the aircraft it believes it is
// flying, and it judges the answer against that same aircraft's altitude.
//
// The same message is also what the sim link renders with: the HOST owns every
// entity's pose, so this is the whole of what the sim has to be told (SimLink.h).
struct EntityControl {
    uint16_t entityId  = 0;
    uint8_t  stateByte = 0;    // byte 4: entity state (bits 0-1) + flags
    double   latDeg    = 0.0;  // degrees
    double   lonDeg    = 0.0;  // degrees
    double   altMeters = 0.0;  // metres MSL
    float    rollDeg   = 0.0f; // degrees, right wing down positive
    float    pitchDeg  = 0.0f; // degrees, nose up positive
    float    yawDeg    = 0.0f; // degrees, true heading
};

// Decode an Entity Control message. Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 48-byte form).
bool parseEntityControl(const uint8_t* msg, size_t size, EntityControl& out);

// True when an Entity Control carries a pose that can be used: finite, on the
// globe, and not the all-zero pose a HOST sends for entities it is not driving
// (the reference capture's entity 5 carries exactly that). An all-zero pose
// means "no position", not the Gulf of Guinea, so it is neither anchored to nor
// rendered. Both the HAT/HOT anchor (main.cpp) and the sim link apply the same
// rule, from here, so they cannot disagree about which entities exist.
bool poseUsable(const EntityControl& e);

//-----------------------------------------------------------------------------
// View Control (message id 16) decoding
//-----------------------------------------------------------------------------
// Where the HOST puts its own eye -- the viewpoint it wants the picture drawn
// from.  The view is attached to an entity (entity 0 in every capture), and
// both the offset and the angles are relative to that entity rather than to the
// world: X/Y/Z are body offsets in metres in the same frame the entity-relative
// HAT/HOT requests above use (+X out the nose, +Y out the right wing, +Z out
// the bottom, so a negative Z is *above* the reference point), and roll, pitch
// and yaw are added to the entity's own attitude.
//
// Grounded on the captures rather than only on the ICD.  All 16 recordings
// beside this repository carry exactly one View Control per frame, and it is
// byte-identical in every frame of every one of them:
//
//   10 20 | 00 00 | 01 | 3F | 00 00 | 2.8 | 0.05 | -0.5 | 0 | 0 | 359
//
// -- one static view definition for the whole sortie.  Read with the layout
// below it is the pilot's eye: 2.8 m forward of the entity's reference point,
// 0.5 m above it and 5 cm to the right, turned a degree left of the nose (the
// HOST flies the aircraft, so its view's angles are a boresight, not a heading).
// The yaw is the last float as it stands on the wire, `00 80 B3 43` = 359.00f;
// the whole of the turn is that one degree.
//
// One caveat, and it is written down rather than hidden: the three angle slots
// are read here in the same roll/pitch/yaw order as the Entity Control rows
// above (roll at 20, pitch at 24, yaw at 28), which is what the one non-zero
// angle in every capture reads as.  The ICD's own View Control table may list
// them yaw-first; reading this capture the other way round would bank the
// picture a degree instead of turning it.  TODO.md has it under the rig
// confirmations that are still owed.
struct ViewControl {
    uint16_t viewId   = 0;    // View ID / View Group ID
    uint8_t  viewType = 0;    // byte 4; 1 in every capture
    uint8_t  flags    = 0;    // byte 5; 0x3F in every capture, not used here
    uint16_t entityId = 0;    // the entity the view is attached to
    float    xOffset  = 0.0f; // metres, +X out the nose
    float    yOffset  = 0.0f; // metres, +Y out the right wing
    float    zOffset  = 0.0f; // metres, +Z out the bottom (-0.5 is 0.5 m up)
    float    rollDeg  = 0.0f; // degrees, relative to the entity
    float    pitchDeg = 0.0f; // degrees, relative to the entity
    float    yawDeg   = 0.0f; // degrees, relative to the entity
};

// Decode a View Control message.  Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 32-byte form).
bool parseViewControl(const uint8_t* msg, size_t size, ViewControl& out);

//-----------------------------------------------------------------------------
// Celestial Sphere Control (message id 9) decoding -- the HOST's time of day
//-----------------------------------------------------------------------------
// The one CIGI message that carries a wall clock, and the only way a HOST tells
// its IG what time of day to render.  It carries an hour and a minute (no
// seconds), a date, and a set of enables; the captured sorties use exactly one
// such message at the start of each replay, with the date 2000-01-01 and the
// star field off.
//
//   off  len  field
//   0    1    message id       = 9
//   1    1    message size     = 16
//   2    1    hour             (uint8, UTC)
//   3    1    minute           (uint8, UTC)
//   4    1    flags            (bit0 Ephemeris En, bit1 Sun En, bit2 Moon En,
//                               bit3 Star En, bit4 Date Valid)
//   5    3    reserved
//   8    4    date             (uint32, Month * 1000000 + Day * 10000 + Year)
//   12   4    star intensity   (float)
//
// The four "time of day" captures carry 12:00, 06:15, 17:30 and 23:30 with all
// four enables set and Date Valid clear-or-set the same way; the flags byte
// reads 0x11 (Ephemeris + Date Valid) in all of them.
struct CelestialSphere {
    uint8_t  hour         = 0;
    uint8_t  minute       = 0;
    uint8_t  flags        = 0;
    uint32_t date         = 0;   // Month*1000000 + Day*10000 + Year
    float    starIntensity = 0.0f;

    bool dateValid() const { return (flags & 0x10u) != 0u; }
    bool sunEnabled() const { return (flags & 0x02u) != 0u; }
    bool moonEnabled() const { return (flags & 0x04u) != 0u; }
    bool starEnabled() const { return (flags & 0x08u) != 0u; }
};

// Decode a Celestial Sphere Control message.  Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 16-byte form).
bool parseCelestialSphere(const uint8_t* msg, size_t size, CelestialSphere& out);

//-----------------------------------------------------------------------------
// Atmosphere Control (message id 10) decoding -- temperature, visibility, wind
//-----------------------------------------------------------------------------
//   off  len  field
//   0    1    message id       = 10
//   1    1    message size     = 32
//   2    1    atmosphere enable (bit 0)
//   3    1    humidity         (uint8, percent)
//   4    4    air temperature  (float, degrees C)
//   8    4    visibility range (float, metres)
//   12   4    horizontal wind  (float, m/s)
//   16   4    vertical wind    (float, m/s)
//   20   4    wind direction   (float, degrees, the direction it comes FROM)
//   24   4    barometric press (float, millibars)
//   28   4    reserved
//
// The atmospheric enable bit is NOT honoured here: the captured warm-up frames
// send 0x00 in it while carrying the meaningful values the aircraft is meant to
// fly in, so gating on it would throw the whole message away.  A field is taken
// when the message is present, which is the only reading that matches the wire.
struct Atmosphere {
    uint8_t atmosEnable   = 0;   // byte 2, bit 0 -- informational only
    uint8_t humidity      = 0;   // percent
    float   airTempC      = 0.0f;
    float   visibilityM   = 0.0f;
    float   horizWindMps  = 0.0f;
    float   vertWindMps   = 0.0f;
    float   windDirDeg    = 0.0f;
    float   baroPressureMb = 0.0f;
};

// Decode an Atmosphere Control message.  Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 32-byte form).
bool parseAtmosphere(const uint8_t* msg, size_t size, Atmosphere& out);

//-----------------------------------------------------------------------------
// Weather Control (message id 12) decoding -- cloud, precipitation, region
//-----------------------------------------------------------------------------
//   off  len  field
//   0    1    message id       = 12
//   1    1    message size     = 56
//   2    2    entity/region ID (uint16)
//   4    1    layer ID         (0 ground fog, 1-3 cloud layer, 4 rain, 5 snow,
//                               6 sleet, 7 hail, 8 sand, 9 dust -- this, not the
//                               severity, is what names the precipitation:
//                               CigiBaseWeatherCtrl.h `LayerIDGrp`)
//   5    1    humidity         (uint8, percent)
//   6    1    flags            (bit0 Weather En, bit1 Scud En, bit2 Random Winds
//                               En, bit3 Random Lightning En, bits 4-7 cloud type)
//   7    1    flags2           (bits 0-1 scope 0=Global 1=Regional 2=Entity,
//                               bits 2-4 severity 0-5, least..most severe)
//   8    4    air temperature  (float, C)
//   12   4    visibility range (float, m)
//   16   4    scud frequency   (float)
//   20   4    coverage         (float, 0..100 percent)
//   24   4    base elevation   (float, m)
//   28   4    thickness        (float, m)
//   32   4    transition       (float, m)
//   36   4    horizontal wind  (float, m/s)
//   40   4    vertical wind    (float, m/s)
//   44   4    wind direction   (float, degrees)
//   48   4    barometric press (float, mb)
//   52   4    aerosol          (float)
//
// No capture carries one of these -- the reference HOST commanded a whole
// environment with the warm-up Atmosphere Control alone and never sent weather
// -- so this layout is the ICD's, parsed faithfully and exercised by the
// contract test rather than against a recording.
struct WeatherControl {
    uint16_t entityOrRegionId = 0;
    uint8_t  layerId          = 0;
    uint8_t  humidity         = 0;
    uint8_t  flags            = 0;
    uint8_t  flags2           = 0;
    float    airTempC         = 0.0f;
    float    visibilityM      = 0.0f;
    float    scudFreq         = 0.0f;
    float    coverage         = 0.0f;
    float    baseElevM        = 0.0f;
    float    thicknessM       = 0.0f;
    float    transitionM      = 0.0f;
    float    horizWindMps     = 0.0f;
    float    vertWindMps      = 0.0f;
    float    windDirDeg       = 0.0f;
    float    baroPressureMb   = 0.0f;
    float    aerosol          = 0.0f;

    bool weatherEnabled() const { return (flags & 0x01u) != 0u; }
    bool lightningEnabled() const { return (flags & 0x08u) != 0u; }
    unsigned cloudType() const { return (flags >> 4) & 0x0Fu; }
    unsigned scope() const { return flags2 & 0x03u; }       // 0 global, 1 regional, 2 entity
    unsigned severity() const { return (flags2 >> 2) & 0x07u; }
};

// Decode a Weather Control message.  Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 56-byte form).
bool parseWeatherControl(const uint8_t* msg, size_t size, WeatherControl& out);

//-----------------------------------------------------------------------------
// Component Control (message id 4) decoding -- a component on an entity
//-----------------------------------------------------------------------------
//   off  len  field
//   0    1    message id       = 4
//   1    1    message size     = 32
//   2    2    component ID     (uint16; which component on the entity)
//   4    2    instance ID      (uint16; for an Entity component, the entity ID)
//   6    1    component class  (ComponentClass below)
//   7    1    component state  (class-specific: off/on, or an index)
//   8    4    data 1           (float)
//   12   4    data 2
//   16   4    data 3
//   20   4    data 4
//   24   4    data 5
//   28   4    data 6
//
// CIGI's way of attaching something to an entity that is not the entity
// itself: a light, a sensor, a weather volume.  The class says *what kind* of
// thing it is and the id says which one, and what the six data fields mean is
// the class's business, not the protocol's.
//
// The class enum, as in the ICD and in Wireshark's packet-cigi.c:
enum ComponentClass : uint8_t {
    kComponentEntity               = 0,  // an entity's own appearance
    kComponentView                 = 1,
    kComponentViewGroup            = 2,
    kComponentSensor               = 3,
    kComponentRegionalSeaSurface   = 4,
    kComponentRegionalTerrainSurface = 5,
    kComponentRegionalLayeredWeather = 6,
    kComponentGlobalSeaSurface     = 7,
    kComponentGlobalTerrainSurface = 8,
    kComponentGlobalLayeredWeather = 9,
    kComponentAtmosphere           = 10,
    kComponentCelestialSphere      = 11,
    kComponentEvent                = 12,
    kComponentSystem               = 13,
};
//
// The reference HOST uses exactly one component id, and it is the cloud
// pattern: the three `cloud pat*.bin` captures -- and no other capture beside
// this repository -- carry a Component Control naming component 0x1001 on the
// entity 0x9001 they also create for it, class Entity, with the state 1/2/3
// and then 0 and with the first three data fields set.  Those three are a
// footprint and an altitude, all in metres as CIGI carries them: 14816 = 8 x
// 1852 m across, 12964 = 7 x 1852 m deep and 914.4 m = 3000 ft up for the first
// pattern -- the vendor's tool writing nautical miles and feet and converting
// both.  That is why the cloud pattern is decoded as a *size* and a *height*
// rather than as a position: the position is the entity's own Entity Control,
// in the same datagram.  Environment.h turns the pair into the sky Prepar3D can
// actually be given.
//
// The class is *not* checked against Global Layered Weather: the vendor sends
// Entity (0) for a cloud, so gating on the class that would make sense would
// silently drop every pattern.  It is checked to *be* Entity (0) instead, which
// is what the captures carry, so an unrelated component on another entity is
// not mistaken for a sky.
struct ComponentControl {
    uint16_t componentId    = 0;
    uint16_t instanceId     = 0;
    uint8_t  componentClass = 0;   // ComponentClass
    uint8_t  componentState = 0;
    float    data1 = 0.0f;
    float    data2 = 0.0f;
    float    data3 = 0.0f;
    float    data4 = 0.0f;
    float    data5 = 0.0f;
    float    data6 = 0.0f;
};

// Decode a Component Control message.  Returns false when `msg` is not a
// well-formed one (wrong id, or shorter than the 32-byte form).
bool parseComponentControl(const uint8_t* msg, size_t size, ComponentControl& out);

//-----------------------------------------------------------------------------
// Parsing helper
//-----------------------------------------------------------------------------

// Iterate the messages inside a single CIGI datagram. A message is framed by
// [id(1)][size(1)][payload...] where `size` is the TOTAL byte length of that
// message (header + payload). `cb(id, data, size)` is invoked per message.
template <typename Fn>
void forEachMessage(const uint8_t* data, size_t len, Fn&& cb) {
    size_t off = 0;
    while (off + 2 <= len) {
        const uint8_t id   = data[off];
        const uint8_t size = data[off + 1];
        if (size < 2 || off + size > len) {
            // Malformed framing: skip a byte and keep scanning.
            ++off;
            continue;
        }
        cb(id, data + off, static_cast<size_t>(size));
        off += size;
    }
}

} // namespace cigi
