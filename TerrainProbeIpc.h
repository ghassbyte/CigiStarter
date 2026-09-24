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
//     FSUIPC, which freezes while the aircraft is being flown into the sim with
//     slew writes.
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
// Everything is plain fixed-width fields, so both sides agree on the layout
// regardless of compiler packing.  Coordinates are in CIGI's units (degrees and
// metres); only the probe converts to the radians/feet the PDK expects.
//-----------------------------------------------------------------------------

#include <stdint.h>

namespace terrain {
namespace probe {

// 'PTBP' in little-endian byte order, so a hex dump of the region is readable.
constexpr uint32_t kMagic   = 0x50425450u;  // 'P','T','B','P'
constexpr uint32_t kVersion = 1u;

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
constexpr const wchar_t* kMappingName = L"Local\\CigiStarter.TerrainProbe.v1";

enum Status : uint32_t {
    StatusNone    = 0,  // not asked, or not answered yet
    StatusOk      = 1,  // `hotMeters` is the terrain height
    StatusFailed  = 2,  // Prepar3D's terrain system refused the query (E_FAIL)
    StatusInvalid = 3,  // the point was not usable (NaN/out of range)
};

// One point to look up.  `altMeters` is the altitude of the point being tested
// (the HAT point, i.e. where the HOST wants to know the height *of* the
// terrain), not the terrain height itself.  Prepar3D uses it to decide which
// surface it is being asked about: the reference IG answers a point that is
// underground with the layer below the runway flattening and a point above it
// with the runway itself, so the altitude has to be passed through rather than
// guessed.
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
    uint32_t materialCode;  // PDK surface material, 0 until material is wired up
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
    uint32_t          _pad;

    Query  queries[kMaxQueries];
    Result results[kMaxQueries];
};

} // namespace probe
} // namespace terrain
