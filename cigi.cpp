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
// HAT/HOT Extended Response (40 bytes)
//
//   off  len  field
//   0    1    message id      = 0x67 (103)
//   1    1    message size    = 40
//   2    2    hat/hot id      (uint16 LE)
//   4    1    flags           (bit 0 = valid)
//   5    8    HAT  (double, meters)
//   13   8    HOT  (double, meters)
//   21   4    material code   (uint32 LE)
//   25   4    normal azimuth  (float, radians)
//   29   4    normal elevation(float, radians)
//   33   7    reserved        = 0
//-----------------------------------------------------------------------------
void packHatHotExtResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                           double hat, double hot) {
    out[0] = static_cast<uint8_t>(MsgHATHOTExtResponse); // 0x67
    out[1] = kHatHotExtRespSize;                          // 40
    writeU16LE(out + 2, hatHotId);
    out[4] = valid ? 0x01 : 0x00;
    writeF64LE(out + 5, hat);
    writeF64LE(out + 13, hot);
    writeU32LE(out + 21, 0);    // material code
    writeF32LE(out + 25, 0.0f); // normal vector azimuth
    writeF32LE(out + 29, 0.0f); // normal vector elevation
    std::memset(out + 33, 0, 7); // reserved
}

} // namespace cigi
