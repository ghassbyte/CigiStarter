//=============================================================================
//  cigi.cpp — CIGI 3.3 message packing
//
//  x64 (little-endian) only, so host byte order equals the CIGI wire order and
//  the writers below are plain memcpy with explicit little-endian semantics.
//=============================================================================
#include "cigi.h"

#include <cstring>

namespace cigi {

namespace {

inline void writeU16LE(uint8_t* p, uint16_t v) { std::memcpy(p, &v, 2); }
inline void writeU32LE(uint8_t* p, uint32_t v) { std::memcpy(p, &v, 4); }
inline void writeF32LE(uint8_t* p, float v)   { std::memcpy(p, &v, 4); }
inline void writeF64LE(uint8_t* p, double v)  { std::memcpy(p, &v, 8); }

inline uint16_t readU16LE(const uint8_t* p) { uint16_t v; std::memcpy(&v, p, 2); return v; }
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
// Only the pose is decoded: the offsets below are the CIGI 3.3 layout, checked
// field by field against the rig capture, where the doubles at 24/32/40 read
// back as the aircraft's own Atang-to-Halim track.
//-----------------------------------------------------------------------------
bool parseEntityControl(const uint8_t* msg, size_t size, EntityControl& out) {
    if (size < kEntityControlSize ||
        msg[0] != static_cast<uint8_t>(MsgEntityControl)) {
        return false;
    }
    out.entityId  = readU16LE(msg + 2);
    out.latDeg    = readF64LE(msg + 24);
    out.lonDeg    = readF64LE(msg + 32);
    out.altMeters = readF64LE(msg + 40);
    return true;
}

} // namespace cigi
