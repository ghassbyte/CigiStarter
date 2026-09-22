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
    MsgHATHOTRequest     = 24,  // host -> IG : height-above/height-of terrain request
    MsgStartOfFrame      = 101, // IG -> host : start-of-frame (frame sync)
    MsgHATHOTResponse    = 102, // IG -> host : HAT/HOT response
    MsgHATHOTExtResponse = 103, // IG -> host : HAT/HOT extended response
};

//-----------------------------------------------------------------------------
// Message sizes (total, including the 2-byte [id][size] header)
//-----------------------------------------------------------------------------
constexpr uint8_t kSofSize            = 24;  // Start of Frame
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
    uint32_t frameNumber;    // IG frame number
    uint32_t timestamp;      // microseconds (0 = not valid)
    uint32_t lastHostFrame;  // echo of host frame counter
    uint32_t reserved;       // 0
};
#pragma pack(pop)
static_assert(sizeof(StartOfFrame) == 24, "StartOfFrame must be 24 bytes");

//-----------------------------------------------------------------------------
// Packing helpers
//-----------------------------------------------------------------------------

// Pack a 24-byte Start-of-Frame message into `out` (must hold >= 24 bytes).
void packStartOfFrame(uint8_t* out, uint32_t frameNumber, uint32_t timestamp,
                      uint32_t lastHostFrame, uint8_t igModeByte);

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
