#pragma once
//-----------------------------------------------------------------------------
// TerrainProbeIpc.h
//
// The shared-memory contract between CigiStarter (the IG process) and
// PdkTerrainProbe.dll, the in-process Prepar3D add-on that answers terrain
// height questions from the sim's own terrain engine.
//
// Why this exists
// ---------------
// CigiStarter has to answer the HOST's HAT/HOT requests with the height of the
// terrain the aircraft is flying over.  Two approximations were tried before:
//
//   * an offline SRTM `.hgt` database (TerrainDb.h), which cannot know about a
//     runway's flattening or any changed scenery, and has to be re-baked
//     whenever the scenery changes; and
//   * the ground altitude under the local aircraft, read from Prepar3D over
//     FSUIPC, which describes one point only -- the ground beneath the sim's own
//     aircraft -- and froze outright while the program that drives the aircraft
//     held the sim in slew.
//
// Prepar3D itself will answer the question directly, from the same terrain it
// will actually put the aircraft on, through the PDK's surface query service.
// That service is a free-standing query (no SimObject, no probe object to move
// around the world), so a small add-on DLL can pick it up and publish the
// answers here for CigiStarter to read.  Nothing needs to be baked, and a new
// scenery add-on is answered live without any matching pass.
//
// The contract
// ------------
// CigiStarter owns `queries[]` and the probe owns `results[]`:
//
//   1. the IG writes `queries[i]` (with `enabled` set and its own `key`), then
//      bumps `reqSeq` so the probe knows there is something to do;
//   2. the probe walks the table on its next sim tick, asks Prepar3D for each
//      enabled point, and writes `results[i]`, guarding the whole batch with
//      `rspSeq` (odd while the rows are being rewritten, even when they are
//      consistent -- a seqlock);
//   3. the IG accepts `results[i]` when its `status` is Ok, its `key` matches,
//      and the `latDeg`/`lonDeg` it carries still match the point the IG asked
//      about.
//
// Step 3's echo is what makes a one-tick-old answer safe: a reading is only
// used for the point it was actually taken at, so a request that has since
// moved is refused and the caller falls back, rather than being answered with
// somebody else's terrain.
//
// Coordinates are in CIGI's units (degrees and metres); only the probe converts
// to the radians/feet the PDK expects.
//
// The fields are all fixed width, so both sides agree on every field's offset.
// The array *strides* are not implied by that, though: a struct's size still
// depends on the packing, and if one side were built at a different /Zp the two
// would agree on slot 0 and disagree on every slot after it -- a silently wrong
// answer rather than a visible failure.  So the layout is pinned below with an
// explicit pack(8), and the static_asserts make any change to it fail the build
// on both sides.  Change the layout and you must bump kVersion as well, so a
// stale counterpart is refused at attach instead of misread.
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>

namespace terrain {
namespace probe {

// 'PTBP' in little-endian byte order, so a hex dump of the region is readable.
constexpr uint32_t kMagic   = 0x50425450u;  // 'P','T','B','P'
// 2: the probe can be driven by Prepar3D's per-frame event instead of the
//    object's own simulation, and a diagnostic tail was appended after the
//    tables (see Shared).  The tick hung off the user object stops whenever the
//    sim is paused -- and the rig is flown with the sim held paused -- so a tick
//    that survives a pause is not optional.  The append does not move
//    any existing offset, but the mapping is bigger, so the version was bumped
//    to make a half-updated pair fail visibly rather than read a short region.
constexpr uint32_t kVersion = 2u;

// Slots in the table; the probe answers every enabled one on each simulation
// tick.  The HOST only ever asks about six points (see the capture analysis:
// three gear legs, the reference point, the tail and one 10 m below), so for the
// IG this is enormous -- but it is also what lets TerrainBaker fill a whole tile
// in seconds instead of the hours a reposition-per-cell FSUIPC sweep took, by
// keeping the table full.  A slot that is not enabled costs the probe one compare
// per tick, so a capacity this size is free at run time.
constexpr int kMaxQueries = 256;

// Session-local, because CigiStarter and Prepar3D both run in the logged-on
// user's session (the IG PC starts unattended at logon rather than as a
// service).  A `Global\` name would need elevation on one side or the other.
//
// The name is a plain narrow literal so a build can override it, and is widened
// below.  The contract test (tools\PdkTerrainProbe\test) overrides it on
// purpose: it creates a region of its own, and if it shared this name
// CreateFileMapping would hand it a *running* add-on's region and its initial
// memset would wipe the live header -- which then reads as a probe that is
// loaded but has stopped answering.  No shipped binary defines the override.
#ifndef TERRAIN_PROBE_MAPPING_NAME
#define TERRAIN_PROBE_MAPPING_NAME "Local\\CigiStarter.TerrainProbe.v1"
#endif
#define TERRAIN_PROBE_WIDE_(x) L##x
#define TERRAIN_PROBE_WIDE(x)  TERRAIN_PROBE_WIDE_(x)
constexpr const wchar_t* kMappingName = TERRAIN_PROBE_WIDE(TERRAIN_PROBE_MAPPING_NAME);

enum Status : uint32_t {
    StatusNone    = 0,  // not asked, or not answered yet
    StatusOk      = 1,  // `hotMeters` is the terrain height
    StatusFailed  = 2,  // Prepar3D's terrain system refused the query (E_FAIL)
    StatusInvalid = 3,  // the point was not usable (NaN/out of range)
};

// The layout of `Shared::diag`.  Purely diagnostic: nothing on either side makes
// a decision from it, so a stale or absent value is harmless.  The add-on sets
// `kPresent` once it fills the word at all, which is how a reader tells a new
// add-on (diagnosis available) from an older one (diag stays 0).
namespace diag {
constexpr uint32_t kCreatesShift      = 0;         // user-object creates
constexpr uint32_t kCreatesMask       = 0xFFu;
constexpr uint32_t kRemovesShift      = 8;         // the tick's object, or a user object
constexpr uint32_t kRemovesMask       = 0xFFu;
constexpr uint32_t kAttachFailsShift  = 16;
constexpr uint32_t kAttachFailsMask   = 0xFFu;
constexpr uint32_t kAttached          = 1u << 24;  // a simulation is bound right now
constexpr uint32_t kHostIsUser        = 1u << 25;  // bound object == GetUserObject() at the last tick
constexpr uint32_t kSkippedUserCreate = 1u << 26;  // a user-object create was ignored (already bound)
constexpr uint32_t kOwnThreadTick     = 1u << 27;  // the newest pass was serviced by the add-on's own thread
constexpr uint32_t kPresent           = 1u << 31;  // the add-on fills this word

inline uint32_t field(uint32_t d, uint32_t shift, uint32_t mask) {
    return (d >> shift) & mask;
}
} // namespace diag

// Bits in `Shared::simEvents`, the simulator messages the add-on has seen.  These
// say what the *sim* was doing, which is what tells a paused sim apart from a
// genuine loss of the terrain service: Prepar3D stops signalling its per-frame
// event while it is paused, so "paused" plus a silent frame event is the normal
// state the add-on's own thread exists to cover.  Bit 0 is live (set for as long
// as the sim is paused); bits 8-15 are sticky, so a reader that looks after the
// fact can still see what happened.
namespace simEvent {
constexpr uint32_t kPausedNow        = 1u << 0;   // live: sim paused right now
constexpr uint32_t kSawPause         = 1u << 8;
constexpr uint32_t kSawUnpause       = 1u << 9;
constexpr uint32_t kSawSimStop       = 1u << 10;  // SimConnect "simulating" stopped
constexpr uint32_t kSawSimStart      = 1u << 11;
constexpr uint32_t kSawScenarioLoad  = 1u << 12;
constexpr uint32_t kSawScenarioReady = 1u << 13;
constexpr uint32_t kSawVehicleChange = 1u << 14;
constexpr uint32_t kSawFrozen        = 1u << 15;
constexpr uint32_t kSawUnfrozen      = 1u << 16;
} // namespace simEvent

// The layout of `Result::materialCode`: what kind of surface the sim answered
// with.  The PDK's surface query returns more than an elevation -- alongside
// `m_fElevation` it fills a `SurfaceInfoV400`, whose category says whether the
// surface is hard, soft or water and whose `m_bOnPlatform` says whether it is an
// *object's* surface rather than terrain.  That second flag is the one that
// matters for a vessel or an oil rig standing in the sea: an aircraft can rest
// on such a platform (Prepar3D's collision knows the deck) while the terrain
// under it is still sea level, so a HAT/HOT answer taken from the elevation
// alone would fly the aircraft into the hull.
//
// The field has always been reserved for this; it was simply left 0 while the
// add-on asked the elevation-only query.  Nothing about the layout changes, so
// `kVersion` does not move either -- a reader must still check `status` first,
// and must treat a `kReported` bit that is clear as "this add-on does not say"
// rather than as a category.
namespace surface {
constexpr uint32_t kCategoryShift = 0;
constexpr uint32_t kCategoryMask  = 0xFFu;
constexpr uint32_t kOnPlatform    = 1u << 8;   // SurfaceInfoV400::m_bOnPlatform
constexpr uint32_t kReported      = 1u << 31;  // the field carries a reading

// SurfaceInfoV400::SURFACE_TYPE_CATEGORY, copied out unchanged (`INVALID` is
// -1, so the low byte of that encoding is 0xFF).
constexpr int kCategoryInvalid = -1;
constexpr int kCategoryHard    = 0;
constexpr int kCategorySoft    = 1;
constexpr int kCategoryWater   = 2;

constexpr uint32_t encode(int category, bool onPlatform) {
    return kReported
         | (onPlatform ? kOnPlatform : 0u)
         | ((static_cast<uint32_t>(category) & kCategoryMask) << kCategoryShift);
}
} // namespace surface

// One point to look up.  `altMeters` is the altitude of the point being tested
// (the HAT point, i.e. where the HOST wants to know the height *of* the
// terrain), not the terrain height itself.  Prepar3D uses it to decide which
// surface it is being asked about: the reference IG answers a point that is
// underground with the layer below the runway flattening and a point above it
// with the runway itself, so the altitude has to be passed through rather than
// guessed.
// The layout below is the contract with the other process: pinned, not left to
// the compiler's default /Zp.
#pragma pack(push, 8)
struct Query {
    double   latDeg;
    double   lonDeg;
    double   altMeters;
    uint32_t key;      // opaque to the probe; echoed back so the reader can match
    uint32_t enabled;  // 0 to leave the slot alone
};

struct Result {
    // Echo of the query this answers, so the reader can tell a current answer
    // from one taken for a point it has since left.  Written for every enabled
    // slot the probe looks at, refused ones included, so `status` alone decides
    // whether there is a height here.
    double   latDeg;
    double   lonDeg;
    double   hotMeters;     // terrain elevation, metres above mean sea level
    uint32_t key;
    uint32_t status;        // Status
    uint32_t materialCode;  // surface, see surface::*: category + on-platform flag
    float    nrmAzimuthDeg; // surface normal, 0 until wired up
    float    nrmElevationDeg;
};

struct Shared {
    uint32_t magic;         // kMagic
    uint32_t version;       // kVersion
    uint32_t capacity;      // kMaxQueries
    uint32_t tickRateHz;    // the rate the probe managed to register for

    volatile int32_t  alive;        // 1 while the probe has a service and a tick
    volatile int32_t  lastError;    // last HRESULT the probe saw, for diagnosis
    volatile uint32_t probePid;     // process id of Prepar3D, for `tasklist`
    volatile uint32_t ticks;        // probe heartbeat: advances while it runs
    volatile uint32_t reqSeq;       // bumped by the IG once it has written queries
    volatile uint32_t rspSeq;       // seqlock over results: odd = being rewritten
    volatile uint32_t queryFails;   // how many points Prepar3D has refused
    // Diagnostic word, written by the add-on so a reader can tell *why* the
    // probe is not answering and not merely that it is not.  It occupies the
    // field that used to be padding, so the layout and `kVersion` do not move;
    // an add-on built before this existed leaves it 0 and a reader must treat a
    // 0 as "no diagnosis available" rather than as fact (see diag::kPresent).
    volatile uint32_t diag;

    Query  queries[kMaxQueries];
    Result results[kMaxQueries];

    // --- Diagnostic tail (kVersion 2) -------------------------------------
    // Appended *after* the tables on purpose: no slot offset moves, so the only
    // cost is a slightly larger mapping.  A build that leaves these zero is an
    // older add-on; a reader must treat a zero as "not known" rather than as a
    // reading.  Nothing on either side decides anything from them.
    volatile uint32_t frameTicks;   // Prepar3D frame-event callbacks seen
    volatile uint32_t simEvents;    // simulator messages, see simEvent::*
    volatile uint32_t simMode;      // GetMode() at the last object tick (0 if never)
    volatile uint32_t frameFails;   // frame-event RegisterCallback failures
};
#pragma pack(pop)

// The numbers here are the wire contract, not incidental: a pad byte, an added
// field or a differently packed build shifts every slot after the first, and the
// reader would then answer from the wrong row.  Fail the build instead.  If one
// of these has to change, bump kVersion so the two binaries cannot mix.
static_assert(sizeof(Query)  == 32, "Query must stay 32 bytes");
static_assert(sizeof(Result) == 48, "Result must stay 48 bytes (44 + tail pad)");
static_assert(offsetof(Shared, queries) == 48, "queries[] must start at 48");
static_assert(offsetof(Shared, results) == 48 + kMaxQueries * 32,
              "results[] must follow queries[] exactly");
static_assert(offsetof(Shared, frameTicks) == 48 + kMaxQueries * (32 + 48),
              "the diagnostic tail must sit exactly after both tables");
static_assert(sizeof(Shared) == 48 + kMaxQueries * (32 + 48) + 16,
              "Shared must be header + both tables + the 4-word diagnostic tail");

} // namespace probe
} // namespace terrain
