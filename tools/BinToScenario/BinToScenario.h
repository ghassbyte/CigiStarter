#pragma once
//-----------------------------------------------------------------------------
// BinToScenario.h
//
// Turn a recorded HOST -> IG CIGI stream (the ".bin" the sibling replayers
// read) into a CIGI Host Emulator script (.scp) that drives the same scenario
// on the native emulator.
//
// Why this exists
//   The emulator and the .bin format are two different HOSTs.  Hemu3.exe reads
//   definition tables (.def) and a script (.scp), and never a .bin; the .bin
//   players (CigiP3dSimConnect / CigiP3dBridge) read the .bin and drive
//   Prepar3D themselves.  So a capture cannot simply be handed to the emulator:
//   to run the same flight through the *native* HOST -- where CigiStarter's own
//   output can be watched against it, with no third program in the loop -- the
//   capture has to be rewritten as emulator commands.  That is all this does.
//
// .bin format (Visual\TAP\pcap_to_bin.py, the players, and this reader)
//   Repeating little-endian records: uint32 deltaMicros, uint16 len, then `len`
//   bytes of one UDP datagram.  The payload is CIGI 3.3, framed as
//   [id(1)][size(1)][payload...].
//
// What is translated
//   Entity Control            (2)  -> ENTITY_POS + ENTITY_YPR, once per frame
//   Celestial Sphere Control  (9)  -> TIME
//   Atmosphere Control       (10)  -> GLOBAL_AIR_TEMP / GLOBAL_VISIBILITY / ...
//   HAT/HOT Request          (24)  -> HAT_HOT_REQ_BEGIN / SOURCE / TYPE / END
//
// What is deliberately not, and why
//   IG Control                (1)  the emulator sends its own every frame
//   Component Control         (4)  not field-mapped here (see the README)
//   Articulated Part Ctrl     (6)  a pose-only replay does not need them
//   Rate Control              (8)  the poses are absolute, not integrated
//   View Control             (16)  per-channel angles stay in the sim's own
//                                  viewgroup.xml -- a project decision, not an
//                                  oversight
//
// Every id that appears is counted and reported, translated or not, so nothing
// is dropped without the operator being told.
//
// Nothing here touches a simulator, the PDK, a socket or a file: it is a pure
// function of the bytes, which is what lets the contract test in test\ drive
// exactly this code and check the text it produces.
//-----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace binscp {

// One record out of a .bin: when it was sent relative to the record before it,
// and the raw UDP datagram the HOST put on the wire.
struct BinRecord {
    uint32_t             deltaMicros = 0;
    std::vector<uint8_t> payload;
};

// Parse a whole .bin file image. Returns false, with `err` set, on a truncated
// record or trailing rubbish -- a converter that silently stopped at the first
// bad record would emit a scenario that just stops mid-flight. Only an empty
// file is not an error in itself; the caller sees `out.empty()`.
bool parseBin(const uint8_t* data, size_t len,
              std::vector<BinRecord>& out, std::string& err);

// What the operator can choose about the script that comes out.
struct Options {
    // The emulator creates an entity with ADD_ENTITY <id> <type>, and `type` is
    // the emulator's own: it indexes a definition block in that emulator's
    // Entities.def. 118 is the `f18c` the shipped Default\ database defines, so
    // a script made from any capture runs against a stock emulator; point this
    // at another type for a different .def.
    unsigned entityType = 118;

    // Collapse runs of frames whose state did not change into FRAME_WAIT.
    // Without this a frozen or level capture spends one FRAME per datagram
    // saying nothing; with it, only real motion costs script lines.
    bool frameWaitCompression = true;
};

// What the conversion saw and did. Everything here is for the operator (and
// for the contract test): the header comment on the script says the same thing
// in a shorter form.
struct Report {
    size_t   datagrams   = 0;   // records read out of the .bin
    size_t   bytes       = 0;   // payload bytes across those records
    uint64_t totalMicros = 0;   // sum of the record delays

    std::map<unsigned, size_t> idCounts;  // CIGI message id -> times seen

    std::vector<unsigned> entityIds;      // entities ever carrying a usable pose
    size_t   poseCommands = 0;            // ENTITY_POS + ENTITY_YPR lines written
    size_t   frameCommands = 0;           // FRAME lines written
    size_t   frameWaitFrames = 0;         // frames covered by FRAME_WAIT lines
    size_t   sceneCommands = 0;           // ADD_ENTITY / TIME / GLOBAL_* / HAT_HOT_REQ_* lines

    size_t frames() const { return frameCommands + frameWaitFrames; }
};

// The CIGI 3.3 host->IG ids this tool knows by name. The enum spells out the
// ones CigiStarter's own cigi.h already names, so the two cannot drift.
enum MsgId : uint8_t {
    kIGControl        = 1,
    kEntityControl    = 2,
    kComponentControl = 4,
    kArticulatedPart  = 6,
    kRateControl      = 8,
    kCelestialControl = 9,
    kAtmosphereControl = 10,
    kViewControl      = 16,
    kHatHotRequest    = 24,
};

// A human name for a CIGI id, for the report. Unknown ids read "message <n>".
std::string messageName(unsigned id);

// True when this id is carried into the script at all.
bool isTranslated(unsigned id);

// Convert the records into script text, filling `rep`. `source` is the input
// path or name, quoted in the script's header for provenance; it may be empty.
std::string toScript(const std::vector<BinRecord>& records,
                     const std::string& source,
                     const Options& options, Report& rep);

// The operator-facing summary, as printed to the console. `source` names the
// file the records came from.
std::string formatReport(const Report& rep, const std::string& source);

// Derive an output path from an input path: replace the extension with ".scp".
std::string scriptPathFor(const std::string& binPath);

} // namespace binscp
