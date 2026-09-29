//=============================================================================
//  cigi.cpp — CIGI 3.3 message packing
//
//  x64 (little-endian) only, so host byte order equals the CIGI wire order and
//  the writers below are plain memcpy with explicit little-endian semantics.
//=============================================================================
#include "cigi.h"

#include <cmath>
#include <cstring>

namespace cigi {

namespace {

inline void writeU16LE(uint8_t* p, uint16_t v) { std::memcpy(p, &v, 2); }
inline void writeU32LE(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }
inline void writeF32LE(uint8_t* p, float v)   { std::memcpy(p, &v, 4); }
inline void writeF64LE(uint8_t* p, double v)  { std::memcpy(p, &v, 8); }

inline uint16_t readU16LE(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }
inline uint32_t readU32LE(const uint8_t* p) { uint32_t v; std::memcpy(&v, p, 4); return v; }
inline float    readF32LE(const uint8_t* p) { float    v; std::memcpy(&v, p, 4); return v; }
inline double   readF64LE(const uint8_t* p) { double   v; std::memcpy(&v, p, 8); return v; }

} // namespace

//-----------------------------------------------------------------------------
// Start of Frame (24 bytes)
//
//   off  len  field
//   0    1    message id       = 0x65
//   1    1    message size     = 24
//   2    1    version          = 3
//   3    1    database number  = 0
//   4    1    IG status        = 0
//   5    1    IG mode byte     = 0x30 (mode 0 + minor version 3)
//   6    2    byte-swap flag   = 0x00 0x80 (little-endian)
//   8    4    HOST frame counter, echoed
//   12   4    timestamp        = 0
//   16   4    last host frame  = 0
//   20   4    reserved         = 0
//
// The frame-number slot carries the HOST's counter, not this IG's count of
// transmitted SOFs. The reference Master IG did exactly that: in
// Host-Master.pcapng the SOF's live field advances ~0.52 per SOF while the
// HOST's own counter advances 0.54 per datagram -- the HOST asks for ~98 SOF
// replies a second but only runs ~53 frames a second, so the field tracks the
// HOST, not the link rate. Counting SOFs here instead makes the field run away
// from the HOST by (rate - 53) every second, which is what broke the HOST above
// ~60 Hz; echoing its counter keeps the SOF byte-identical to the capture's.
//-----------------------------------------------------------------------------
void packStartOfFrame(uint8_t* out, uint32_t hostFrameNumber, uint8_t igModeByte) {
    out[0] = kSofMessageId;
    out[1] = kSofSize;
    out[2] = kSofVersion;
    out[3] = 0x00;               // database number
    out[4] = 0x00;               // IG status
    out[5] = igModeByte;         // 0x30 (standby) or 0x31 (operate)
    out[6] = 0x00;               // byte-swap flag: little-endian
    out[7] = 0x80;
    writeU32LE(out + 8, hostFrameNumber);
    writeU32LE(out + 12, 0);     // timestamp: the reference sent 0
    writeU32LE(out + 16, 0);     // last host frame: unused (see the header)
    writeU32LE(out + 20, 0);     // reserved
}

//-----------------------------------------------------------------------------
// HAT/HOT Response (16 bytes)
//
//   off  len  field
//   0    1    message id      = 0x66 (102)
//   1    1    message size    = 16
//   2    2    HAT/HOT ID      (uint16 LE)
//   4    1    flags           (bit 0 = valid, bit 1 = response type)
//   5    3    reserved        = 0
//   8    8    Height          (double LE, metres)
//
// The 3-byte reserved gap after the flags byte is what makes the double land
// on an 8-byte boundary -- 3.3 added it; 3.2 packed Height at offset 5.
//-----------------------------------------------------------------------------
void packHatHotResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                        uint8_t responseType, uint8_t hostFrameLsn, double height) {
    out[0] = static_cast<uint8_t>(MsgHATHOTResponse);    // 0x66
    out[1] = kHatHotRespSize;                            // 16
    writeU16LE(out + 2, hatHotId);
    out[4] = static_cast<uint8_t>((valid ? 0x01u : 0x00u) |
                                  ((responseType & 0x01u) << 1) |
                                  ((hostFrameLsn & 0x0Fu) << 4));
    out[5] = out[6] = out[7] = 0x00; // reserved
    writeF64LE(out + 8, height);
}

//-----------------------------------------------------------------------------
// HAT/HOT Extended Response (40 bytes)
//
//   off  len  field
//   0    1    message id      = 0x67 (103)
//   1    1    message size    = 40
//   2    2    HAT/HOT ID      (uint16 LE)
//   4    1    flags           (bit 0 = valid, bits 4-7 = host frame number LSN)
//   5    3    reserved        = 0
//   8    8    HAT  (double LE, metres above the terrain surface)
//   16   8    HOT  (double LE, metres MSL)
//   24   4    material code   (uint32 LE)
//   28   4    normal azimuth  (float LE, degrees)
//   32   4    normal elevation(float LE, degrees)
//   36   4    reserved        = 0
//-----------------------------------------------------------------------------
void packHatHotExtResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                           uint8_t hostFrameLsn, double hat, double hot) {
    out[0] = static_cast<uint8_t>(MsgHATHOTExtResponse); // 0x67
    out[1] = kHatHotExtRespSize;                          // 40
    writeU16LE(out + 2, hatHotId);
    out[4] = static_cast<uint8_t>((valid ? 0x01u : 0x00u) |
                                  ((hostFrameLsn & 0x0Fu) << 4));
    out[5] = out[6] = out[7] = 0x00; // reserved (8-byte alignment for HAT)
    writeF64LE(out + 8,  hat);
    writeF64LE(out + 16, hot);
    writeU32LE(out + 24, 0);         // material code
    writeF32LE(out + 28, 0.0f);      // normal vector azimuth
    writeF32LE(out + 32, 0.0f);      // normal vector elevation
    std::memset(out + 36, 0, 4);     // reserved
}

//-----------------------------------------------------------------------------
// HAT/HOT Request (32 bytes)
//
//   off  len  field
//   0    1    message id      = 0x18 (24)
//   1    1    message size    = 32
//   2    2    HAT/HOT ID      (uint16 LE)
//   4    1    bit 0-1 request type, bit 2 coordinate system, bits 3-7 reserved
//   5    1    update period   (frames; 0 = one-shot)
//   6    2    entity ID       (uint16 LE)
//   8    8    Latitude / X Offset  (double LE)
//   16   8    Longitude / Y Offset (double LE)
//   24   8    Altitude / Z Offset  (double LE)
//-----------------------------------------------------------------------------
bool parseHatHotRequest(const uint8_t* msg, size_t size, HatHotRequest& out) {
    if (size < kHatHotReqSize || msg[0] != static_cast<uint8_t>(MsgHATHOTRequest)) {
        return false;
    }
    out.hatHotId     = readU16LE(msg + 2);
    out.requestType  = static_cast<uint8_t>(msg[4] & 0x03u);
    out.coordSystem  = static_cast<uint8_t>((msg[4] >> 2) & 0x01u);
    out.updatePeriod = msg[5];
    out.entityId     = readU16LE(msg + 6);
    out.x            = readF64LE(msg + 8);
    out.y            = readF64LE(msg + 16);
    out.z            = readF64LE(msg + 24);
    return true;
}

//-----------------------------------------------------------------------------
// Entity Control (48 bytes)
//
//   off  len  field
//   0    1    message id      = 0x02 (2)
//   1    1    message size    = 48
//   2    2    entity ID       (uint16 LE)
//   4    1    entity state
//   5    1    flags
//   6    2    entity type
//   8    2    parent ID
//   10   1    opacity
//   11   1    reserved
//   12   4    roll            (float LE, degrees)
//   16   4    pitch           (float LE, degrees)
//   20   4    yaw             (float LE, degrees)
//   24   8    latitude        (double LE, degrees)
//   32   8    longitude       (double LE, degrees)
//   40   8    altitude        (double LE, metres MSL)
//
// The pose is decoded field by field against the rig capture, where the doubles
// at 24/32/40 read back as the aircraft's own Atang-to-Halim track. The attitude
// floats at 12/16/20 are the CIGI 3.3 layout but are not yet seen on the wire
// from this HOST (it is level); they are what the sim link renders with, so a
// bench run is what confirms their sign and order.
//-----------------------------------------------------------------------------
bool parseEntityControl(const uint8_t* msg, size_t size, EntityControl& out) {
    if (size < kEntityControlSize ||
        msg[0] != static_cast<uint8_t>(MsgEntityControl)) {
        return false;
    }
    out.entityId  = readU16LE(msg + 2);
    out.stateByte = msg[4];
    out.rollDeg   = readF32LE(msg + 12);
    out.pitchDeg  = readF32LE(msg + 16);
    out.yawDeg    = readF32LE(msg + 20);
    out.latDeg    = readF64LE(msg + 24);
    out.lonDeg    = readF64LE(msg + 32);
    out.altMeters = readF64LE(msg + 40);
    return true;
}

bool poseUsable(const EntityControl& e) {
    return std::isfinite(e.latDeg) && std::isfinite(e.lonDeg) &&
           std::isfinite(e.altMeters) &&
           e.latDeg >= -90.0  && e.latDeg <= 90.0 &&
           e.lonDeg >= -180.0 && e.lonDeg <= 180.0 &&
           (e.latDeg != 0.0 || e.lonDeg != 0.0 || e.altMeters != 0.0);
}

//-----------------------------------------------------------------------------
// View Control (32 bytes)
//
//   off  len  field
//   0    1    message id      = 0x10 (16)
//   1    1    message size    = 32
//   2    2    view ID / view group ID   (uint16 LE)
//   4    1    view type                 (1 in every capture)
//   5    1    flags                     (0x3F in every capture)
//   6    2    entity ID                 (the entity the view is attached to)
//   8    4    X offset          (float LE, metres, out the nose)
//   12   4    Y offset          (float LE, metres, out the right wing)
//   16   4    Z offset          (float LE, metres, out the bottom)
//   20   4    roll              (float LE, degrees)
//   24   4    pitch             (float LE, degrees)
//   28   4    yaw               (float LE, degrees)
//
// The field layout is re-derived from the captures, where this message is
// identical in every frame -- see the header for what that definition reads as,
// and for the one caveat about the order of the three angles.
//-----------------------------------------------------------------------------
bool parseViewControl(const uint8_t* msg, size_t size, ViewControl& out) {
    if (size < kViewControlSize ||
        msg[0] != static_cast<uint8_t>(MsgViewControl)) {
        return false;
    }
    out.viewId   = readU16LE(msg + 2);
    out.viewType = msg[4];
    out.flags    = msg[5];
    out.entityId = readU16LE(msg + 6);
    out.xOffset  = readF32LE(msg + 8);
    out.yOffset  = readF32LE(msg + 12);
    out.zOffset  = readF32LE(msg + 16);
    out.rollDeg  = readF32LE(msg + 20);
    out.pitchDeg = readF32LE(msg + 24);
    out.yawDeg   = readF32LE(msg + 28);
    return true;
}

//-----------------------------------------------------------------------------
// Celestial Sphere Control (16 bytes)
//
//   off  len  field
//   0    1    message id      = 0x09 (9)
//   1    1    message size    = 16
//   2    1    hour            (uint8, UTC)
//   3    1    minute          (uint8, UTC)
//   4    1    flags           (bit0 Ephemeris, bit1 Sun, bit2 Moon, bit3 Star,
//                              bit4 Date Valid)
//   5    3    reserved        = 0
//   8    4    date            (uint32 LE, Month*1000000 + Day*10000 + Year)
//   12   4    star intensity  (float LE)
//
// The four time-of-day captures carry 12:00, 06:15, 17:30 and 23:30 here with
// flags 0x11 and the date 2000-01-01 (0x000F7120 = 1012000).  Only 16 bytes are
// decoded; a longer message is accepted, a shorter one is not.
//-----------------------------------------------------------------------------
bool parseCelestialSphere(const uint8_t* msg, size_t size, CelestialSphere& out) {
    if (size < kCelestialSphereSize ||
        msg[0] != static_cast<uint8_t>(MsgCelestialSphere)) {
        return false;
    }
    out.hour         = msg[2];
    out.minute       = msg[3];
    out.flags        = msg[4];
    out.date         = readU32LE(msg + 8);
    out.starIntensity = readF32LE(msg + 12);
    return true;
}

//-----------------------------------------------------------------------------
// Atmosphere Control (32 bytes)
//
//   off  len  field
//   0    1    message id      = 0x0A (10)
//   1    1    message size    = 32
//   2    1    atmosphere enable (bit 0; NOT honoured -- see cigi.h)
//   3    1    humidity        (uint8, percent)
//   4    4    air temperature (float LE, C)
//   8    4    visibility      (float LE, m)
//   12   4    horizontal wind (float LE, m/s)
//   16   4    vertical wind   (float LE, m/s)
//   20   4    wind direction  (float LE, degrees, from)
//   24   4    barometric      (float LE, millibars)
//   28   4    reserved        = 0
//-----------------------------------------------------------------------------
bool parseAtmosphere(const uint8_t* msg, size_t size, Atmosphere& out) {
    if (size < kAtmosphereSize || msg[0] != static_cast<uint8_t>(MsgAtmosphere)) {
        return false;
    }
    out.atmosEnable    = msg[2];
    out.humidity       = msg[3];
    out.airTempC       = readF32LE(msg + 4);
    out.visibilityM    = readF32LE(msg + 8);
    out.horizWindMps   = readF32LE(msg + 12);
    out.vertWindMps    = readF32LE(msg + 16);
    out.windDirDeg     = readF32LE(msg + 20);
    out.baroPressureMb = readF32LE(msg + 24);
    return true;
}

//-----------------------------------------------------------------------------
// Weather Control (56 bytes)
//
//   off  len  field
//   0    1    message id      = 0x0C (12)
//   1    1    message size    = 56
//   2    2    entity/region ID (uint16 LE)
//   4    1    layer ID
//   5    1    humidity        (uint8, percent)
//   6    1    flags           (bit0 Weather En, bit1 Scud En, bit2 Random Winds
//                              En, bit3 Random Lightning En, bits 4-7 cloud type)
//   7    1    flags2          (bits 0-1 scope, bits 2-4 severity)
//   8    4    air temperature (float LE, C)
//   12   4    visibility      (float LE, m)
//   16   4    scud frequency  (float LE)
//   20   4    coverage        (float LE, 0..1)
//   24   4    base elevation  (float LE, m)
//   28   4    thickness       (float LE, m)
//   32   4    transition      (float LE, m)
//   36   4    horizontal wind (float LE, m/s)
//   40   4    vertical wind   (float LE, m/s)
//   44   4    wind direction  (float LE, degrees)
//   48   4    barometric      (float LE, millibars)
//   52   4    aerosol         (float LE)
//-----------------------------------------------------------------------------
bool parseWeatherControl(const uint8_t* msg, size_t size, WeatherControl& out) {
    if (size < kWeatherControlSize ||
        msg[0] != static_cast<uint8_t>(MsgWeatherControl)) {
        return false;
    }
    out.entityOrRegionId = readU16LE(msg + 2);
    out.layerId          = msg[4];
    out.humidity         = msg[5];
    out.flags            = msg[6];
    out.flags2           = msg[7];
    out.airTempC         = readF32LE(msg + 8);
    out.visibilityM      = readF32LE(msg + 12);
    out.scudFreq         = readF32LE(msg + 16);
    out.coverage         = readF32LE(msg + 20);
    out.baseElevM        = readF32LE(msg + 24);
    out.thicknessM       = readF32LE(msg + 28);
    out.transitionM      = readF32LE(msg + 32);
    out.horizWindMps     = readF32LE(msg + 36);
    out.vertWindMps      = readF32LE(msg + 40);
    out.windDirDeg       = readF32LE(msg + 44);
    out.baroPressureMb   = readF32LE(msg + 48);
    out.aerosol          = readF32LE(msg + 52);
    return true;
}

//-----------------------------------------------------------------------------
// Component Control (32 bytes)
//
//   off  len  field
//   0    1    message id      = 0x04 (4)
//   1    1    message size    = 32
//   2    2    component ID    (uint16 LE)
//   4    2    instance ID     (uint16 LE; an Entity component names its entity)
//   6    1    component class (0 entity, 9 global layered weather, ...)
//   7    1    component state
//   8    4    data 1          (float LE)
//   12   4    data 2
//   16   4    data 3
//   20   4    data 4
//   24   4    data 5
//   28   4    data 6
//
// The six data words are the component class's business, not the protocol's, so
// they are carried raw and interpreted where the class is known (cigi.h has the
// cloud pattern the reference HOST sends in them).
//-----------------------------------------------------------------------------
bool parseComponentControl(const uint8_t* msg, size_t size, ComponentControl& out) {
    if (size < kComponentControlSize ||
        msg[0] != static_cast<uint8_t>(MsgComponentControl)) {
        return false;
    }
    out.componentId    = readU16LE(msg + 2);
    out.instanceId     = readU16LE(msg + 4);
    out.componentClass = msg[6];
    out.componentState = msg[7];
    out.data1          = readF32LE(msg + 8);
    out.data2          = readF32LE(msg + 12);
    out.data3          = readF32LE(msg + 16);
    out.data4          = readF32LE(msg + 20);
    out.data5          = readF32LE(msg + 24);
    out.data6          = readF32LE(msg + 28);
    return true;
}

} // namespace cigi
