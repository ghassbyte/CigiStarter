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
//   8    4    frame number     (uint32 LE)
//   12   4    timestamp        (uint32 LE, microseconds)
//   16   4    last host frame  (uint32 LE)
//   20   4    reserved         = 0
//-----------------------------------------------------------------------------
void packStartOfFrame(uint8_t* out, uint32_t frameNumber, uint32_t timestamp,
                      uint32_t lastHostFrame, uint8_t igModeByte) {
    out[0] = kSofMessageId;
    out[1] = kSofSize;
    out[2] = kSofVersion;
    out[3] = 0x00;               // database number
    out[4] = 0x00;               // IG status
    out[5] = igModeByte;         // 0x30 (standby) or 0x31 (operate)
    out[6] = 0x00;               // byte-swap flag: little-endian
    out[7] = 0x80;
    writeU32LE(out + 8, frameNumber);
    writeU32LE(out + 12, timestamp);
    writeU32LE(out + 16, lastHostFrame);
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

} // namespace cigi
