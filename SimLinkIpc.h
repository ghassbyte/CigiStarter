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
//   5. the IG also writes `view` -- the HOST's own eye, from its View Control
//      message -- because the sim's view is a *camera*, not an object: the
//      entity the camera rides can be moved perfectly and the picture stays
//      where it was until the camera is moved too, and the HOST says where to
//      move it.  One row, because every capture carries exactly one view.
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
// 3: adds `view`, the HOST's own eye (a CIGI View Control), so the picture can
//    be flown from the cockpit the HOST is flying rather than from the entity's
//    own reference point.  It is appended after `held[]`, so the two existing
//    tables keep the offsets they had and a reader written against version 2
//    still finds them where it expects them.
//   4: adds `environment`, the HOST's world beyond the entities -- time of day,
//    atmosphere and weather -- and the add-on's `environmentStatus` read-back.
//    Both are appended after `view`, so the three tables above keep the offsets
//    they had and a version-3 reader still finds them where it expects them.
//   5: `environmentStatus` gains the sim's own answer about the sky it now
//    holds -- the cloud layer the weather system reports after an apply -- so a
//    cloud the HOST commanded can be shown to have reached the sim and not
//    merely been written into the region.  Nothing above it moves: the row is
//    the last thing in the region, so it only grows.
constexpr uint32_t kVersion = 5u;

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
constexpr uint32_t kPhysicsFree      = 1u << 29;  // the objects are the add-on's own: no flight model and no ground contact
constexpr uint32_t kModePauseOrSlew  = 1u << 30;  // at least one object reports itself paused or in slew
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

// The HOST's own eye: one View Control message, in CIGI's units, exactly as it
// arrives.  The offsets and angles are relative to the entity the view is
// attached to (`entityId`), so the add-on combines the two -- see
// SimLinkMap.h's viewEyePose() -- and nothing here is absolute.
//
// It is a single row and not a table because the captures carry a single view:
// the reference HOST sent one View Control per frame, byte-identical for the
// whole sortie.  `valid` is 0 until one has arrived, which is how a camera gets
// the entity's own reference point (the behaviour before this existed) rather
// than a pose from an all-zero row.
//
// The seqlock is the same discipline as Entity's, for the same reason: a reader
// can be preempted mid-copy.
struct View {
    volatile uint32_t seq;        // seqlock: odd while the IG rewrites this row
    uint32_t valid;               // 1 once a View Control has been published
    uint32_t entityId;            // the entity the view is attached to
    uint32_t viewId;              // the view / view group the HOST named
    double   xOffset;             // metres, +X out the nose
    double   yOffset;             // metres, +Y out the right wing
    double   zOffset;             // metres, +Z out the bottom
    double   rollDeg;             // degrees, relative to the entity
    double   pitchDeg;            // degrees, relative to the entity
    double   yawDeg;              // degrees, relative to the entity
};

// The HOST's world beyond the entities: the time of day it wants rendered, the
// atmosphere (temperature, visibility, wind, pressure, humidity) and its clouds
// and precipitation.  This is the one block the IG resolves before it publishes,
// because the operator can override parts of it by hand: the row already carries
// what is to be applied, not the raw CIGI fields, so the add-on does no policy
// of its own beyond deciding what changed.
//
// The CIGI messages behind it (cigi.h): a Celestial Sphere Control (id 9) is the
// wall clock -- the four time-of-day captures carry one each, 06:15 / 12:00 /
// 17:30 / 23:30 local, date 2000-01-01; an Atmosphere Control (id 10) is the
// air the aircraft flies in, and the captures carry it in their warm-up frames
// (temperature, visibility, wind, pressure); a Weather Control (id 12) is cloud
// and precipitation, and no capture carries one, so that path is built to the
// ICD and contract-tested rather than replayed.
//
// A row is written only when the resolved world actually changes, and
// `generation` advances with each change: that is what tells the add-on "there
// is something new to apply" without it having to diff the whole block on every
// sim frame, and -- just as important -- it is why an unchanged time is not
// re-applied every frame, which would pin the sim's own clock to the commanded
// second.  `flags` says *which* parts of this generation changed, so a weather
// edit does not move the clock.
struct Environment {
    volatile uint32_t seq;         // seqlock: odd while the IG rewrites this row
    uint32_t generation;           // 0 = nothing published yet; advances on change
    uint32_t flags;                // EnvFlags: what is present and what changed
    uint32_t hour;                 // local wall clock (the captures read as local)
    uint32_t minute;
    uint32_t second;
    uint32_t year;                 // only meaningful with kEnvDateValid
    uint32_t month;
    uint32_t day;
    uint32_t cloudType;            // CIGI CloudTypeGrp (cigi.h): 0 none, 6 Cb, 7 Cu
    uint32_t severity;             // 0..5, least..most severe (CIGI Weather Control)
    uint32_t precipType;           // PrecipType: what the layer is dropping
    double   airTempC;             // degrees C
    double   visibilityMeters;
    double   coverage;             // cloud coverage, 0..100
    double   baseElevMeters;       // cloud base, metres
    double   thicknessMeters;      // cloud layer thickness, metres
    double   horizWindMps;         // horizontal wind, m/s
    double   vertWindMps;          // vertical wind, m/s
    double   windDirDeg;           // direction the wind comes FROM, degrees
    double   baroPressureMb;       // millibars
    double   humidityPct;          // relative humidity, percent (for the dew point)
};

// Bits in `Environment::flags`.  The first three are "this group changed in this
// generation, apply it"; the `*Valid` bits are what makes that safe -- they say
// the group is actually *present*, from the HOST or an operator override, so a
// group nobody has spoken for is not applied at all.  Without them a HOST that
// never sends a Celestial Sphere (the real one does not: AGENTS.md's per-frame
// list has an Atmosphere Control and no clock) would resolve to the default
// 00:00 and slam the sim's own clock to midnight on the first atmosphere
// message.  The manual bits are state, so a reader can say whether an operator
// is overriding the HOST; the date is a property of the time.
//
// Keeping "changed" in the same word as "present" and "manual" is deliberate:
// the add-on only ever acts on the apply bits and `generation`, so it never has
// to know why the value changed -- an operator preset and a HOST command are the
// same thing to it, which is exactly the point of resolving on the IG side.
enum EnvFlags : uint32_t {
    kEnvApplyTime    = 1u << 0,  // the wall clock changed; apply it
    kEnvApplyAtmos   = 1u << 1,  // temperature/visibility/wind/pressure changed
    kEnvApplyWeather = 1u << 2,  // cloud/precipitation changed
    kEnvTimeManual   = 1u << 3,  // the clock is an operator override, not the HOST's
    kEnvWeatherManual = 1u << 4, // cloud/visibility are an operator override
    kEnvDateValid    = 1u << 5,  // year/month/day are present and to be applied
    kEnvLightning    = 1u << 6,  // lightning is present (CIGI Random Lightning En)
    kEnvTimeValid    = 1u << 7,  // a clock is present (HOST or override); apply it
    kEnvAtmosValid   = 1u << 8,  // the air is present (HOST atmosphere); apply it
    kEnvWeatherValid = 1u << 9,  // a sky is present (HOST weather or override)
    kEnvWeatherPattern = 1u << 10, // the sky is the HOST's *cloud pattern*
                                   // (a Component Control on the cloud entity),
                                   // not an operator preset or a Weather Control
};

// What is falling out of the sky.  The CIGI Weather Control names it with its
// LayerID rather than with the severity or the cloud type -- `CigiBaseWeatherCtrl.h`
// `LayerIDGrp`: 0 ground fog, 1-3 cloud layers, 4 rain, 5 snow, 6 sleet, 7 hail,
// 8 sand, 9 dust -- so this is that field resolved to the kinds that fall, and
// the add-on synthesises "-RA" / "+SN" from it and the severity without having
// to re-derive CIGI's numbering.  The cloud-layer and fog ids are no
// precipitation, which is why `kPrecipNone` is the zero value.
enum PrecipType : uint32_t {
    kPrecipNone  = 0,
    kPrecipRain  = 1,
    kPrecipSnow  = 2,
    kPrecipSleet = 3,
    kPrecipHail  = 4,
    kPrecipSand  = 5,
    kPrecipDust  = 6,
};

// The add-on's answer: what it last applied, and what the sim now says the time
// of day and the cloud are.  Strictly the add-on's table (the IG only reads it),
// guarded by its own seqlock.
//
// `timeOfDay` is Prepar3D's own `TIMEOFDAY` (DAWN 0, DAY 1, DUSK 2, NIGHT 3,
// INVALID -1), read back with `IGlobalData::GetGeneralTimeOfDay()` after the
// clock is set -- the only way an outside test can tell that a commanded 12:00
// actually became day rather than merely being accepted.
//
// The sky is the same idea, and it takes two forms because the two answer
// different questions.  `simMetar` is the *observation* the sim now holds for
// the station the world was applied to (`GLOB`), read back with
// `GetStationMetarData`: it is what the weather parser made of the string the
// add-on sent, which is as close as an add-on can get to "the sim read it".  The
// `simCloud*` words are the cloud *layers* that observation became, from
// `IWeatherStationV430::GetCloudLayerCount`/`GetCloudAtIndex` on that station:
// the cloud that is actually being drawn.  Layer 0 is the lowest -- the one a
// viewer is under -- and is the one recorded; `simPrecipType` is -1 when the
// station holds no layer at all, which is how a reader tells "no sky" from "a
// clear sky".  Both are refreshed every tick, because the weather system settles
// asynchronously.
constexpr size_t kSimMetarChars = 200;   // narrow characters, NUL-terminated

struct EnvironmentStatus {
    volatile uint32_t seq;         // seqlock: odd while the add-on rewrites it
    uint32_t appliedGeneration;    // the last `Environment::generation` applied
    uint32_t applies;              // generations applied since the add-on loaded
    uint32_t flags;                // EnvFlags the add-on last acted on
    int32_t  timeOfDay;            // TIMEOFDAY the sim reports (-1 = never applied)
    int32_t  lastError;            // last HRESULT from a weather/time call
    // The sim's own sky, read back from its weather system (kVersion 5).
    uint32_t cloudLayers;          // cloud layers the station now holds
    uint32_t simCloudType;         // P3D::CLOUD_TYPE of layer 0 (0 = none)
    uint32_t simCloudCover;        // P3D::CLOUD_COVER of layer 0 (0 = clear)
    int32_t  simPrecipType;        // P3D::PRECIPTYPE of layer 0 (-1 = no layer)
    double   simCloudBaseMeters;   // layer 0 base, metres
    double   simCloudTopsMeters;   // layer 0 tops, metres
    char     simMetar[kSimMetarChars];  // the station's observation, as the sim holds it
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

    // Appended last on purpose: the tables above keep the offsets they had at
    // kVersion 2 (and view at 9816, where kVersion 3 put it), so a change is
    // visible in the version and in the size, not in the middle of the region.
    View   view;

    // The resolved world (kVersion 4): what the IG wants applied, and the
    // add-on's answer. The clock and weather rows are the IG's to write; only
    // the add-on writes `environmentStatus`, and the IG only reads it.
    Environment       environment;
    EnvironmentStatus environmentStatus;
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
//   view     4 u32 (16) + 6 doubles (48) = 64, starting at 4696 + 5120 = 9816
//   environment       12 u32 (48) + 10 doubles (80) = 128, starting at 9880
//   environmentStatus 10 u32 (40) + 2 doubles (16) + 200 chars = 256,
//                     starting at 9880 + 128 = 10008
//            sizeof(Shared) = 10008 + 256 = 10264
static_assert(sizeof(Entity) == 72, "Entity must stay 72 bytes");
static_assert(sizeof(Held) == 80, "Held must stay 80 bytes");
static_assert(sizeof(View) == 64, "View must stay 64 bytes");
static_assert(sizeof(Environment) == 128, "Environment must stay 128 bytes");
static_assert(sizeof(EnvironmentStatus) == 256,
              "EnvironmentStatus must stay 256 bytes");
static_assert(offsetof(Shared, entities) == 88, "entities[] must start at 88");
static_assert(offsetof(Shared, held) == 4696, "held[] must start at 4696");
static_assert(offsetof(Shared, view) == 9816, "view must start at 9816");
static_assert(offsetof(Shared, environment) == 9880,
              "environment must start at 9880");
static_assert(offsetof(Shared, environmentStatus) == 10008,
              "environmentStatus must start at 10008");
static_assert(sizeof(Shared) == 10264,
              "Shared must be the header, the entity/held/view tables and the "
              "environment block");

} // namespace simlink
