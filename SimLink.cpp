//=============================================================================
//  SimLink.cpp -- the CigiStarter side of the PDK entity writer
//
//  See SimLink.h for what this is for.  It is deliberately small: it owns one
//  block of shared memory and a table of entity rows, and it never talks to
//  Prepar3D itself.
//
//  Threading
//    publish() is called once per Entity Control message, from the CIGI receive
//    thread; health() is called from the window thread.  Every entry point takes
//    a lock, so both may call in at once.  The shared region is written here and
//    read by the add-on's frame callback inside Prepar3D; the per-row seqlock in
//    the contract (SimLinkIpc.h) is the only synchronisation with the other
//    process.
//=============================================================================

#include "SimLink.h"
#include "SimLinkIpc.h"
#include "SimLinkMap.h"   // slotOf(): the one definition of which slot an id lands in

#include <windows.h>

#include <cstring>
#include <mutex>

namespace simlink {
namespace {

HANDLE  g_mapping = nullptr;
Shared* g_shm     = nullptr;

// publish() and health() come in from different threads, so the module's own
// state is guarded.  Recursive so either may use a helper that locks too; the
// lock is uncontended and held for the length of one write.
std::recursive_mutex g_mutex;

bool g_everTried   = false;
DWORD g_nextTryAtMs = 0;

// The status-word watchdog, shared by health() so both judge staleness from the
// same clock.  A mapping left behind by a Prepar3D that has since exited would
// otherwise look usable forever.  `g_mutex` must be held.
uint32_t g_lastTicks  = 0;
DWORD    g_lastChange = 0;

// Re-try the mapping this often while Prepar3D is not up.
constexpr DWORD kReopenIntervalMs = 2000;

// The writer is treated as gone when its heartbeat has not moved for this long.
constexpr DWORD kStaleAfterMs = 1500;

// The environment publisher's memory.  publishEnvironment() is called from the
// CIGI receive loop for every Atmosphere Control -- the HOST repeats those far
// more often than it changes them -- so the row is only written when the
// resolved world differs from what was last written.  `g_envHaveCache` is
// cleared whenever the mapping is (re)opened, so the first write after a
// reconnect pushes the whole world rather than assuming the region already holds
// this IG's last values.  Guarded by g_mutex, like everything else here.
Environment g_envCache;
bool        g_envHaveCache = false;

// The bits of `Environment::flags` that are *state* rather than "this changed":
// they ride through the publisher untouched, and a change in one of them is a
// reason to rewrite the row even when no value moved (an operator preset can
// land on exactly the HOST's numbers).
constexpr uint32_t kEnvStateMask = kEnvTimeManual | kEnvWeatherManual |
                                   kEnvDateValid  | kEnvLightning |
                                   kEnvTimeValid  | kEnvAtmosValid |
                                   kEnvWeatherValid | kEnvWeatherPattern;

// Which group changed, so the add-on is told what to re-apply and, just as
// importantly, what not to.  Exact compares are right here: both sides of each
// comparison are values that were set, not computed results that might differ in
// the last bit for the same input.
bool envTimeChanged(const Environment& a, const Environment& b) {
    return a.hour != b.hour || a.minute != b.minute || a.second != b.second ||
           a.year != b.year || a.month != b.month || a.day != b.day ||
           (a.flags & kEnvDateValid) != (b.flags & kEnvDateValid);
}

bool envAtmosChanged(const Environment& a, const Environment& b) {
    return a.airTempC != b.airTempC ||
           a.visibilityMeters != b.visibilityMeters ||
           a.horizWindMps != b.horizWindMps ||
           a.vertWindMps != b.vertWindMps ||
           a.windDirDeg != b.windDirDeg ||
           a.baroPressureMb != b.baroPressureMb ||
           a.humidityPct != b.humidityPct;
}

bool envWeatherChanged(const Environment& a, const Environment& b) {
    return a.cloudType != b.cloudType || a.severity != b.severity ||
           a.precipType != b.precipType || a.coverage != b.coverage ||
           a.baseElevMeters != b.baseElevMeters ||
           a.thicknessMeters != b.thicknessMeters ||
           (a.flags & kEnvLightning) != (b.flags & kEnvLightning) ||
           (a.flags & kEnvWeatherPattern) != (b.flags & kEnvWeatherPattern);
}

bool envAnyChanged(const Environment& a, const Environment& b) {
    return envTimeChanged(a, b) || envAtmosChanged(a, b) ||
           envWeatherChanged(a, b) ||
           (a.flags & kEnvStateMask) != (b.flags & kEnvStateMask);
}

bool tryOpen() {
    if (g_shm) return true;

    const DWORD now = ::GetTickCount();
    if (g_everTried && static_cast<LONG>(now - g_nextTryAtMs) < 0) return false;
    g_everTried   = true;
    g_nextTryAtMs = now + kReopenIntervalMs;

    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMappingName);
    if (!h) return false;

    // Map whatever the add-on created rather than our own idea of the size, so
    // a pair built from different revisions is rejected by the version check
    // below -- with the region left untouched -- instead of failing with an
    // unexplained "not loaded".
    void* view = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!view) {
        ::CloseHandle(h);
        return false;
    }

    Shared* s = static_cast<Shared*>(view);
    if (s->magic != kMagic || s->version != kVersion || s->capacity != kMaxEntities) {
        // Somebody else's mapping, or a version we do not speak.
        ::UnmapViewOfFile(view);
        ::CloseHandle(h);
        return false;
    }

    g_mapping = h;
    g_shm     = s;
    g_lastTicks  = 0;
    g_lastChange = 0;
    g_envHaveCache = false;   // first publish pushes the whole world again
    return true;
}

// True while the add-on's heartbeat is advancing; false once it has not moved
// for kStaleAfterMs.  `g_mutex` must be held.
bool tickFresh() {
    const uint32_t ticks = g_shm->ticks;
    const DWORD    now   = ::GetTickCount();
    if (ticks != g_lastTicks) {
        g_lastTicks  = ticks;
        g_lastChange = now;
    } else if (static_cast<DWORD>(now - g_lastChange) > kStaleAfterMs) {
        return false;
    }
    return true;
}

} // namespace

void publish(const cigi::EntityControl& e) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!tryOpen()) return;

    // The table is indexed by the low bits of the entity ID (SimLinkMap.h's
    // slotOf(): a power of two, so this is a mask).  Entity IDs in this
    // deployment are a handful, so no two entities map to the same slot; if one
    // ever does, the resident row wins and the new one is dropped rather than
    // silently overwriting another entity's pose.
    Entity& row = g_shm->entities[slotOf(e.entityId)];
    if (row.enabled && row.entityId != e.entityId) return;

    const bool usable = cigi::poseUsable(e);

    // Seqlock: an odd `seq` tells the add-on the row is being rewritten and it
    // should retry rather than trust a half-written pose.  The fields are plain
    // stores between the two bumps; x64 does not reorder stores with stores, and
    // `volatile` keeps the compiler from doing so either.
    ++row.seq;
    row.entityId  = e.entityId;
    row.enabled   = 1;
    row.flags     = usable ? kFlagPoseValid : 0u;
    row.state     = static_cast<uint32_t>(e.stateByte & 0x03u);
    row.latDeg    = e.latDeg;
    row.lonDeg    = e.lonDeg;
    row.altMeters = e.altMeters;
    row.rollDeg   = static_cast<double>(e.rollDeg);
    row.pitchDeg  = static_cast<double>(e.pitchDeg);
    row.yawDeg    = static_cast<double>(e.yawDeg);
    ++row.seq;
}

void publishView(const cigi::ViewControl& v) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!tryOpen()) return;

    // One row, last one wins: a HOST that sends several views in a frame (one
    // per view group) is asking for several pictures and this link carries one,
    // so the latest is the one the camera will fly.  The captures send exactly
    // one, identical in every frame, so this is a rule that never has to
    // choose in practice.
    View& row = g_shm->view;

    ++row.seq;
    row.valid     = 1;
    row.entityId  = v.entityId;
    row.viewId    = v.viewId;
    row.xOffset   = static_cast<double>(v.xOffset);
    row.yOffset   = static_cast<double>(v.yOffset);
    row.zOffset   = static_cast<double>(v.zOffset);
    row.rollDeg   = static_cast<double>(v.rollDeg);
    row.pitchDeg  = static_cast<double>(v.pitchDeg);
    row.yawDeg    = static_cast<double>(v.yawDeg);
    ++row.seq;
}

void publishEnvironment(const Environment& env) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!tryOpen()) return;

    // Most calls land here: the HOST repeats its atmosphere, and the operator's
    // overrides do not move between messages.  Dropping an unchanged row is the
    // whole point -- pushing it with a new generation would make the add-on
    // re-apply the clock every frame and pin the sim's own time to the second.
    if (g_envHaveCache && !envAnyChanged(env, g_envCache)) return;

    uint32_t flags = env.flags & kEnvStateMask;
    // A group is applied only when it is both present and changed.  The
    // presence gate is what keeps the defaults in a group nobody has spoken
    // for -- the 00:00 clock, the clear sky -- from becoming a command: the
    // HOST in this deployment sends an Atmosphere Control and no Celestial
    // Sphere or Weather Control, so without it the sim's own clock would be
    // pinned to midnight and its weather cleared on startup.
    if ((env.flags & kEnvTimeValid) &&
        (!g_envHaveCache || envTimeChanged(env, g_envCache)))
        flags |= kEnvApplyTime;
    if ((env.flags & kEnvAtmosValid) &&
        (!g_envHaveCache || envAtmosChanged(env, g_envCache)))
        flags |= kEnvApplyAtmos;
    if ((env.flags & kEnvWeatherValid) &&
        (!g_envHaveCache || envWeatherChanged(env, g_envCache)))
        flags |= kEnvApplyWeather;

    Environment& row = g_shm->environment;
    const uint32_t generation = row.generation + 1;

    // Seqlock, exactly as an entity row: an odd `seq` says the row is being
    // rewritten, so the add-on retries rather than applying half a world.
    ++row.seq;
    row.generation     = generation;
    row.flags          = flags;
    row.hour           = env.hour;
    row.minute         = env.minute;
    row.second         = env.second;
    row.year           = env.year;
    row.month          = env.month;
    row.day            = env.day;
    row.cloudType      = env.cloudType;
    row.severity       = env.severity;
    row.precipType     = env.precipType;
    row.airTempC       = env.airTempC;
    row.visibilityMeters = env.visibilityMeters;
    row.coverage       = env.coverage;
    row.baseElevMeters = env.baseElevMeters;
    row.thicknessMeters = env.thicknessMeters;
    row.horizWindMps   = env.horizWindMps;
    row.vertWindMps    = env.vertWindMps;
    row.windDirDeg     = env.windDirDeg;
    row.baroPressureMb = env.baroPressureMb;
    row.humidityPct    = env.humidityPct;
    ++row.seq;

    // Remember what was actually written, flags included, so the next call
    // compares against the region's contents rather than the caller's wish.
    g_envCache = env;
    g_envCache.generation = generation;
    g_envCache.flags      = flags;
    g_envHaveCache = true;
}

Health health() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    Health s;
    if (!tryOpen()) return s;   // mappingOpen stays false
    s.mappingOpen = true;

    s.alive          = g_shm->alive != 0;
    s.lastError      = g_shm->lastError;
    s.ticks          = g_shm->ticks;
    s.frameTicks     = g_shm->frameTicks;
    s.objectsCreated = g_shm->objectsCreated;
    s.objectsRemoved = g_shm->objectsRemoved;
    s.createFails    = g_shm->createFails;
    s.setFails       = g_shm->setFails;
    s.applied        = g_shm->applied;
    s.positionReads  = g_shm->positionReads;
    s.getFails       = g_shm->getFails;
    s.driftSamples   = g_shm->driftSamples;
    s.heldValid      = g_shm->heldValid;
    s.driftMaxMeters = g_shm->driftMaxMeters;
    s.driftMaxDeg    = g_shm->driftMaxDeg;

    const uint32_t d = g_shm->diag;
    s.diagnosed = (d & diag::kPresent) != 0;
    if (s.diagnosed) {
        s.hasManager  = (d & diag::kHasManager) != 0;
        s.hasTick     = (d & diag::kHasTick) != 0;
        s.noTitle     = (d & diag::kNoTitle) != 0;
        s.hasReadback = (d & diag::kHasReadback) != 0;
    }

    s.viewCarried = g_shm->view.valid != 0;

    s.envCarried      = g_shm->environment.generation != 0;
    s.envGeneration   = g_shm->environment.generation;
    s.envManualTime   = (g_shm->environment.flags & kEnvTimeManual) != 0;
    s.envManualWeather = (g_shm->environment.flags & kEnvWeatherManual) != 0;
    s.envWeatherPattern = (g_shm->environment.flags & kEnvWeatherPattern) != 0;

    // The add-on's own row is rewritten on every tick -- it carries the sim's
    // sky read-back -- so it is read under its seqlock like every other row here
    // rather than as a run of independent words: the window would otherwise
    // occasionally show a half-written altitude, and the applies count that goes
    // with a generation could belong to the previous one.
    EnvironmentStatus st{};
    if (readEnvironmentStatus(g_shm->environmentStatus, st)) {
        s.envApplies        = st.applies;
        s.envTimeOfDay      = st.timeOfDay;
        s.envLastError      = st.lastError;
        s.envSimCloudLayers = st.cloudLayers;
        s.envSimCloudType   = st.simCloudType;
        s.envSimCloudCover  = st.simCloudCover;
        s.envSimPrecipType  = st.simPrecipType;
        s.envSimCloudBaseM  = st.simCloudBaseMeters;
        s.envSimCloudTopsM  = st.simCloudTopsMeters;
    }

    s.ticking = s.alive && tickFresh();
    return s;
}

void start() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    tryOpen();
}

void stop() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (g_shm) {
        // Retire every row we published, so the add-on removes the objects it
        // made for them rather than leaving them parked in the sim.
        for (int i = 0; i < kMaxEntities; ++i) {
            if (g_shm->entities[i].enabled) {
                ++g_shm->entities[i].seq;
                g_shm->entities[i].enabled = 0;
                ++g_shm->entities[i].seq;
            }
        }
        // The eye goes back to the entity's reference point with it: a view left
        // valid would keep the camera where the last frame of a stopped run put
        // it, which is the same stale picture the rows above are retired for.
        ++g_shm->view.seq;
        g_shm->view.valid = 0;
        ++g_shm->view.seq;
        ::UnmapViewOfFile(g_shm);
        g_shm = nullptr;
    }
    if (g_mapping) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
    g_everTried   = false;
    g_nextTryAtMs = 0;
    g_lastTicks   = 0;
    g_lastChange  = 0;
    g_envHaveCache = false;
}

} // namespace simlink
