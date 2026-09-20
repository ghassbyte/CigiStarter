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
constexpr uint8_t kHatHotExtRespSize  = 40;  // HAT/HOT Extended Response

//-----------------------------------------------------------------------------
// Start-of-Frame header field values
//-----------------------------------------------------------------------------
constexpr uint8_t kSofMessageId   = 0x65;  // 101
constexpr uint8_t kSofVersion     = 0x03;  // CIGI 3
// Byte 5 packs: IG mode (bits 0-1), timestamp-valid (bit 2), earth-reference
// model (bit 3) and the CIGI minor version (bits 4-7). 3.3 => 0x30.
constexpr uint8_t kSofIgModeStandby = 0x30;  // IG mode = 0 (Reset/Standby)
constexpr uint8_t kSofIgModeOperate = 0x31;  // IG mode = 1 (Operate)

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

// Pack a 40-byte HAT/HOT Extended Response into `out` (>= 40 bytes).
// `hat`/`hot` are the terrain answers in meters; `valid` is 0 or 1.
void packHatHotExtResponse(uint8_t* out, uint16_t hatHotId, uint8_t valid,
                           double hat, double hot);

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
