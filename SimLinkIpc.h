#pragma once
//-----------------------------------------------------------------------------
// SimLinkIpc.h
//
// The shared-memory contract between CigiStarter (the IG process) and
// PdkSimLink.dll, the in-process Prepar3D add-on that renders the HOST's
// entities inside the sim.
//
// Why this exists
// ---------------
// The HOST sends the whole flight on the wire as CIGI: an Entity Control per
// entity per frame carrying the pose (roll/pitch/yaw and lat/lon/alt). Until now
// CigiStarter only read that stream to answer HAT/HOT -- it decoded entity 0's
// position and threw the rest away -- so nothing in the sim ever moved. The
// vendor program that used to drive Prepar3D did so by repositioning the sim's
// own aircraft over FSUIPC, which is a fight with the sim: the flight model
// keeps simulating between writes, and everything the sim owns (time, weather,
// effects) is left to be reconciled by hand.
//
// This is the other way round. The IG publishes the HOST's *desired* world --
// where each entity is and how it is oriented -- and an add-on inside Prepar3D
// applies it on the sim's own frame thread. The IG never touches the sim; it
// only fills a table. Nothing here is a request for an answer: the CIGI stream
// already carries everything there is to know, and nothing of this crosses the
// wire back to the HOST. The add-on does write back one thing -- the pose the
// object turned out to be at (`held[]`) -- but that is the sim's own state
// observed for the bench and the operator window, not an answer to anything.
//
// The contract
// ------------
// CigiStarter owns `entities[]`; the add-on owns the header's status words:
//
//   1. the IG writes one `Entity` row (the pose from an Entity Control message)
//      and guards it with the row's own seqlock: `seq` is bumped odd while the
//      row is being rewritten and even once it is consistent, so a reader that
//      catches a half-written row retries rather than trusting it;
//   2. the add-on reads every enabled row on its frame tick, creates a sim
//      object for an entity it has not seen before, and moves it to the pose
//      the row commands;
//   3. the header's status words (`alive`, `ticks`, `applied`, the fail
//      counters and `diag`) are written by the add-on so the operator window can
//      say whether the entities are actually being rendered, and why not;
//   4. the add-on also fills `held[]` and the drift words with the pose each
//      object was actually found at (`GetPosition`) and how far it had moved
//      from the pose it was last commanded -- the measurement that says whether
//      the native flight model is fighting the writer.
//
// Units are CIGI's: degrees and metres. The add-on converts to the radians and
// feet the PDK wants, at the sim boundary. Nothing here is a sim unit.
//
// Field widths are fixed, but a struct's size still depends on the packing: if
// one side were built at a different /Zp the two would agree on the first row
// and silently misread every row after it. So the layout is pinned below with an
// explicit pack(8), and the static_asserts make any change to it fail the build
// on both sides. Change the layout and bump `kVersion` as well, so a stale
// counterpart is refused at attach instead of misread.
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>

namespace simlink {

// 'PSLK' in little-endian byte order, so a hex dump of the region is readable.
constexpr uint32_t kMagic   = 0x4B4C5350u;  // 'P','S','L','K'
// 1: the first layout (commanded poses only).
// 2: adds the add-on's read-back -- `held[]` and the drift words -- so the sim
//    can be seen *holding* the pose, not merely accepting it.
constexpr uint32_t kVersion = 2u;

// Slots in the entity table. CIGI entity IDs are 16-bit, but the HOST in this
// deployment drives a handful of entities (its capture has three Entity Controls
// per frame), so the table is indexed by the low bits of the entity ID and a row
// that lands on an occupied slot is refused rather than overwriting another
// entity. A slot that is not enabled costs the add-on one compare per tick.
constexpr int kMaxEntities = 64;

// Session-local, like the terrain probe's region: CigiStarter and Prepar3D both
// run in the logged-on user's session (the IG PC starts unattended at logon
// rather than as a service), and a `Global\` name would need elevation on one
// side or the other.
//
// The name is a plain narrow literal so a build can override it, and is widened
// below. No shipped binary defines the override.
#ifndef SIM_LINK_MAPPING_NAME
#define SIM_LINK_MAPPING_NAME "Local\\CigiStarter.SimLink.v1"
#endif
#define SIM_LINK_WIDE_(x) L##x
#define SIM_LINK_WIDE(x)  SIM_LINK_WIDE_(x)
constexpr const wchar_t* kMappingName = SIM_LINK_WIDE(SIM_LINK_MAPPING_NAME);

// CIGI Entity State (Entity Control byte 4, bits 0-1), numbered the way the
// CIGI Common Library numbers it -- CigiBaseEntityCtrl.h, `EntityStateGrp`:
//
//     0 Standby / Inactive    1 Active    2 Remove / Destroyed
//
// The numbering is load-bearing, and getting it backwards is invisible on the
// wire: the reference HOST marks the entities it is driving Active (1) -- in
// every frame of every capture, entities 0 and 3 carry state 1 and a full
// moving pose -- and marks the entity it is not driving, whose pose is all
// zeros, Remove (2). A writer that looked for `state == 0` would publish every
// pose correctly and then render none of them.
enum EntityState : uint32_t {
    kEntityStandby   = 0,  // known, but not to be rendered
    kEntityActive    = 1,  // in the world and moving
    kEntityDestroyed = 2,  // Remove/Destroyed: the add-on removes its object
};

// Bits in `Entity::flags`.
enum EntityFlags : uint32_t {
    kFlagPoseValid = 1u << 0,  // lat/lon/alt and attitude are usable
};

// The layout of `Shared::diag`. Purely diagnostic: nothing on either side makes
// a decision from it, so a stale or absent value is harmless. The add-on sets
// `kPresent` once it fills the word at all, which is how a reader tells a new
// add-on (diagnosis available) from an older one (diag stays 0).
namespace diag {
constexpr uint32_t kObjectsShift     = 0;         // objects created (saturating)
constexpr uint32_t kObjectsMask      = 0xFFu;
constexpr uint32_t kSetFailsShift    = 8;         // SetPosition failures
constexpr uint32_t kSetFailsMask     = 0xFFu;
constexpr uint32_t kCreateFailsShift = 16;        // CreateObject/GetObject failures
constexpr uint32_t kCreateFailsMask  = 0xFFu;
constexpr uint32_t kHasManager       = 1u << 24;  // the sim-object manager was obtained
constexpr uint32_t kHasTick          = 1u << 25;  // a tick has been delivered
constexpr uint32_t kNoTitle          = 1u << 26;  // no container title is configured
constexpr uint32_t kOwnThreadTick    = 1u << 27;  // the newest pass came from the add-on's own thread
constexpr uint32_t kHasReadback      = 1u << 28;  // a GetPosition read-back has been taken
constexpr uint32_t kPresent          = 1u << 31;  // the add-on fills this word

inline uint32_t field(uint32_t d, uint32_t shift, uint32_t mask) {
    return (d >> shift) & mask;
}
} // namespace diag

// One entity the HOST is commanding. `seq` is the row's seqlock and is the only
// synchronisation between the two processes.
//
// The layout below is the contract with the other process: pinned, not left to
// the compiler's default /Zp.
#pragma pack(push, 8)
struct Entity {
    volatile uint32_t seq;        // seqlock: odd while the IG rewrites this row
    uint32_t entityId;            // CIGI entity ID (0 is the HOST's own aircraft)
    uint32_t enabled;             // 0 to retire the row (the add-on removes its object)
    uint32_t flags;              // EntityFlags
    uint32_t state;              // EntityState
    uint32_t reserved;
    double   latDeg;             // degrees
    double   lonDeg;             // degrees
    double   altMeters;          // metres MSL
    double   rollDeg;            // degrees, right wing down positive
    double   pitchDeg;           // degrees, nose up positive
    double   yawDeg;             // degrees, true heading
};

// One read-back row: the pose the sim's object is *actually* at, as the add-on
// last found it with IBaseObjectV400::GetPosition, plus how far that is from the
// pose it was last commanded.
//
// This is the add-on's table -- it is the only writer, and the IG never touches
// it -- and it exists because `SetPosition` returning `S_OK` says the call was
// accepted, not that the object is still there a frame later.  The object this
// build creates is the sim's own and has its own simulation between writes, so
// the gap between where it was told to be and where it is is the difference
// between "the sim took the pose" and "the sim is holding it".  Units are CIGI's
// again (degrees, metres), converted back at the sim boundary, so held and
// commanded can be compared field by field.
//
// The seqlock is the same discipline as Entity's, for the same reason: a reader
// can be preempted mid-copy.
struct Held {
    volatile uint32_t seq;        // seqlock: odd while the add-on rewrites this row
    uint32_t entityId;            // the entity this slot was read for
    uint32_t tick;                // the add-on tick the read was taken on
    uint32_t valid;               // 1 when a pose was read this pass
    double   latDeg;              // read back, in CIGI units
    double   lonDeg;
    double   altMeters;
    double   rollDeg;
    double   pitchDeg;
    double   yawDeg;
    double   driftMeters;         // |held - the pose last written| this pass
    double   driftDeg;            // largest attitude error against that pose
};

struct Shared {
    uint32_t magic;         // kMagic
    uint32_t version;       // kVersion
    uint32_t capacity;      // kMaxEntities

    // Written by the add-on. `simPid` is the process that created the region
    // (Prepar3D), for `tasklist`; the IG can read it but never writes it.
    volatile uint32_t simPid;
    volatile int32_t  alive;        // 1 while the add-on has a manager and a tick
    volatile int32_t  lastError;    // last HRESULT the add-on saw, for diagnosis
    volatile uint32_t ticks;        // add-on heartbeat: advances while it runs
    volatile uint32_t frameTicks;   // Prepar3D frame-event callbacks seen
    volatile uint32_t objectsCreated;  // sim objects created for entities
    volatile uint32_t objectsRemoved;  // sim objects removed again
    volatile uint32_t createFails;  // CreateObject/GetObject failures
    volatile uint32_t setFails;     // SetPosition failures
    volatile uint32_t applied;      // entities positioned on the last tick
    volatile uint32_t diag;         // diagnostic word, see diag::*

    // Also written by the add-on: the read-back, and the drift it shows.  The
    // drift words are a maximum over the run -- the add-on clears them when the
    // last of its objects is retired, so a fresh run starts from zero rather
    // than a previous run's worst case.
    volatile uint32_t positionReads;   // successful GetPosition calls
    volatile uint32_t getFails;        // GetPosition calls the sim refused
    volatile uint32_t driftSamples;    // held-vs-commanded comparisons made
    volatile uint32_t heldValid;       // rows carrying a fresh read-back last pass
    volatile double   driftMaxMeters;  // worst |held - last commanded|, metres
    volatile double   driftMaxDeg;     // worst attitude error, degrees

    Entity entities[kMaxEntities];
    Held   held[kMaxEntities];
};
#pragma pack(pop)

// The numbers here are the wire contract, not incidental: a pad byte, an added
// field or a differently packed build shifts every row after the first, and the
// reader would then apply somebody else's pose. Fail the build instead. If one
// of these has to change, bump kVersion so the two binaries cannot mix.
//
//   header   14 u32 (56) + 4 u32 read-back counters (16) + 2 doubles (16) = 88
//   entities 64 * 72 = 4608, starting at 88
//   held     64 * 80 = 5120, starting at 88 + 4608 = 4696
//            sizeof(Shared) = 4696 + 5120 = 9816
static_assert(sizeof(Entity) == 72, "Entity must stay 72 bytes");
static_assert(sizeof(Held) == 80, "Held must stay 80 bytes");
static_assert(offsetof(Shared, entities) == 88, "entities[] must start at 88");
static_assert(offsetof(Shared, held) == 4696, "held[] must start at 4696");
static_assert(sizeof(Shared) == 9816,
              "Shared must be the header plus the entity and held tables");

} // namespace simlink
