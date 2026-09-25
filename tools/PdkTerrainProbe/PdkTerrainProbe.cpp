//-----------------------------------------------------------------------------
// PdkTerrainProbe.cpp
//
// A Prepar3D PDK add-on that answers terrain height queries from the sim's own
// terrain engine, for CigiStarter to read out of shared memory.
//
// CigiStarter has to tell the HOST how high the terrain is under the aircraft
// (the HAT/HOT exchange).  Rather than keeping an offline copy of the world
// that has to be re-baked whenever the scenery changes, this DLL asks the sim
// itself, through the PDK's ISurfaceQueryManagerV400 - which is a free-standing
// surface query, not tied to any object, so no probe object has to be created
// or teleported around the world.  What comes back is the surface Prepar3D will
// actually place the aircraft on, runway flattening, add-on scenery and all.
//
// It runs in-process (loaded from an add-on.xml as a PDK DLL).  Prepar3D's own
// callbacks drive it wherever they reach: the per-frame event, which is the tick
// of a running sim, and -- until that event has been seen -- an ISimulation on
// an object, the shape the SDK's own RadarAltimeter sample uses.
//
// Why the frame event, and why a clock behind it
// ----------------------------------------------
// An ISimulation on an object is only called while the sim is *simulating* it,
// and the program that drives the aircraft holds Prepar3D paused while it does
// so (the rig's vendor program does so throughout, with slew off).  A paused sim
// stops calling the object's simulation, so a probe driven by it goes silent
// exactly when the HOST is asking for terrain.  Prepar3D's global per-frame event
// was the answer: it is signalled once per rendered frame and was taken to keep
// running while the sim is paused.  On v5 it does.
//
// On the rig's v6 it does not: a pause stops the frame loop as well, so both of
// Prepar3D's ticks go quiet, and with the sim held paused throughout the HOST
// gets no terrain answer at all.  Prepar3D offers nothing else that ticks on its
// own while it is paused, so the probe keeps a thread of its own (watchProc), and
// uses it only when every tick Prepar3D provides has fallen silent.  That one
// call is the only time the terrain engine is touched off Prepar3D's thread, and
// it can only happen while the sim's own loop has stopped.
//
// See TerrainProbe.h for the shared-memory contract, and README.md in this
// folder for how it is installed on the IG PC.
//-----------------------------------------------------------------------------

#include <windows.h>

#include <Pdk.h>
#include <ISimObject.h>
// The EVENTID_* GUIDs are declared with DEFINE_GUID, which is an extern
// declaration unless INITGUID is in scope. This is the only translation unit in
// the DLL, so instantiate them here.
#include <initguid.h>
#include <IEventService.h>

#include "TerrainProbeIpc.h"

using namespace P3D;

namespace {

//-----------------------------------------------------------------------------
// Module state
//
// The PDK owns the service pointers, so they are borrowed and never released.
// The shared mapping, the simulation and the object that carries it are ours.
//-----------------------------------------------------------------------------
HANDLE                    g_mapping = nullptr;
terrain::probe::Shared*   g_shm     = nullptr;

ISurfaceQueryManagerV400* g_surface = nullptr;

// The object-manager interface the callbacks below live on.  Registering a
// callback is enough to answer queries, but unregistering one on shutdown needs
// an interface that can do it -- leaving a pointer into an unloading DLL behind
// is a good way to crash the sim on exit.  That interface grew a method over
// time and the versions do not share a GUID: the v5 SDK tops out at V520, the
// v6 SDK at V540.  Query whichever the SDK in use declares; the project defines
// P3D_MANAGER_V520 when it is pointed at the v5 headers.
#if defined(P3D_MANAGER_V520)
using ObjectManager = P3D::ISimObjectManagerV520;
#else
using ObjectManager = P3D::ISimObjectManagerV540;
#endif
ObjectManager*            g_objects = nullptr;

class ProbeSim;
ProbeSim*                 g_sim  = nullptr;  // one ref held by us, one by the host object
IBaseObjectV400*          g_host = nullptr;  // the object we registered the tick on

// Prepar3D's global event service, and the callbacks we leave with it.  The
// frame event is the real tick (see the file header); the message callback only
// records what the sim was doing, so a stopped object tick can be told apart
// from a stopped render loop.
IEventServiceV400*        g_events = nullptr;
class ProbeCallback;
ProbeCallback*            g_frameCb = nullptr;
ProbeCallback*            g_msgCb   = nullptr;

// Set once a frame event has actually arrived, and the wall clock of the most
// recent one.  Update() defers to the frame event while it is live, so the two
// never both service the table (and never race); if the event stops being
// delivered, Update() takes over again on its own.
volatile LONG             g_frameSeen  = 0;
volatile DWORD            g_lastFrameMs = 0;
constexpr DWORD           kFrameFallbackMs = 500;

// The last-resort tick: a thread of our own, because Prepar3D stopping is
// exactly the case that matters here (see the file header).  It only ever
// services the table once *both* Prepar3D-provided ticks -- the frame event and
// the object's simulation -- have been silent for a whole grace period, so a
// running sim never runs it, and the off-thread call into the terrain engine can
// only ever happen while the sim's own loop has stalled.
HANDLE        g_watch      = nullptr;
volatile LONG g_watchStop  = 0;
bool          g_watchLast  = false;  // the newest pass came from the thread
constexpr DWORD kWatchMs      = 50;   // how often the thread looks
constexpr DWORD kWatchGraceMs = 1000; // silence before it steps in at all

// Only one tick may rewrite the table at a time.  The frame event and an object
// simulation are both delivered on Prepar3D's main thread; the thread above is
// the one that is not, and it is the reason this is here rather than left to the
// sim to have serialised.
volatile LONG             g_servicing = 0;

// Diagnostic counters published in `Shared::diag` (TerrainProbeIpc.h).  They
// exist so a reader can tell *why* the probe went quiet: the tick hangs off one
// object, and a sim that hands the user role to a new object -- which a
// reposition can do -- can leave the tick bound to one it no longer simulates.
// Nothing below changes behaviour; it only observes.
uint32_t g_creates           = 0;
uint32_t g_removes           = 0;
uint32_t g_attachFails       = 0;
uint32_t g_frameFails        = 0;
bool     g_attached          = false;
bool     g_hostIsUser        = false;
bool     g_skippedUserCreate = false;

void publishDiag() {
    if (!g_shm) return;
    const auto sat = [](uint32_t n) { return n > 0xFFu ? 0xFFu : n; };
    uint32_t v = terrain::probe::diag::kPresent;
    v |= sat(g_creates)     << terrain::probe::diag::kCreatesShift;
    v |= sat(g_removes)     << terrain::probe::diag::kRemovesShift;
    v |= sat(g_attachFails) << terrain::probe::diag::kAttachFailsShift;
    if (g_attached)          v |= terrain::probe::diag::kAttached;
    if (g_hostIsUser)        v |= terrain::probe::diag::kHostIsUser;
    if (g_skippedUserCreate) v |= terrain::probe::diag::kSkippedUserCreate;
    if (g_watchLast)         v |= terrain::probe::diag::kOwnThreadTick;
    g_shm->diag = v;
    g_shm->frameFails = g_frameFails;
}

// Is the object our tick is on still the sim's user object?  Sampled on each
// tick, so if the user role moves to a new object while the old one is still
// ticking for a moment, the diag word records it before the tick stops.
void sampleHostIsUser() {
    g_hostIsUser = false;
    if (!g_objects || !g_host) return;

    IBaseObjectV400* user = nullptr;
    if (SUCCEEDED(g_objects->GetUserObject(&user)) && user) {
        g_hostIsUser = (static_cast<IUnknown*>(user) == static_cast<IUnknown*>(g_host));
        user->Release();
    }
}

constexpr float  kTickHz      = 60.0f;
constexpr double kDegToRad    = 3.14159265358979323846 / 180.0;
constexpr double kMetersToFt  = 3.280839895013123;
constexpr double kFtToMeters  = 1.0 / 3.280839895013123;

void publish(long hr) {
    if (g_shm) g_shm->lastError = hr;
}

//-----------------------------------------------------------------------------
// Shared memory
//-----------------------------------------------------------------------------
void openShared() {
    g_mapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                     0, sizeof(terrain::probe::Shared),
                                     terrain::probe::kMappingName);
    if (!g_mapping) return;

    void* view = ::MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(terrain::probe::Shared));
    if (!view) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
        return;
    }

    g_shm = static_cast<terrain::probe::Shared*>(view);
    ZeroMemory(g_shm, sizeof(*g_shm));
    g_shm->magic      = terrain::probe::kMagic;
    g_shm->version    = terrain::probe::kVersion;
    g_shm->capacity   = terrain::probe::kMaxQueries;
    g_shm->probePid   = ::GetCurrentProcessId();
    g_shm->alive      = 0;
    g_shm->lastError  = 0;
    publishDiag();  // sets kPresent, so a reader knows the diag word is meaningful
}

void closeShared() {
    if (g_shm) {
        g_shm->alive = 0;
        ::UnmapViewOfFile(g_shm);
        g_shm = nullptr;
    }
    if (g_mapping) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
}

//-----------------------------------------------------------------------------
// The tick
//
// One pass over the query table.  Prepar3D never sees the readers on the other
// side of the mapping: they only drop points into the table, and this is what
// asks the sim about them.  Every tick funnels through here -- the per-frame
// event, the object's own simulation and the add-on's own thread -- so the work
// has exactly one implementation and no two passes can overlap.
//
// `fromSim` is false for the add-on's own thread, the one tick that does not run
// on Prepar3D's thread.  It skips the object-manager calls, which only fill in
// diagnostics; the surface query is the one service it has to borrow.
//-----------------------------------------------------------------------------
void serviceTick(bool fromSim) {
    if (!g_shm) return;
    if (::InterlockedExchange(&g_servicing, 1) != 0) return;  // a pass is already running

    ++g_shm->ticks;
    g_shm->alive = (g_surface != nullptr) ? 1 : 0;

    g_watchLast = !fromSim;
    if (fromSim) sampleHostIsUser();
    publishDiag();

    if (g_surface && g_shm->reqSeq != 0) {
        // Seqlock: an odd value tells the reader the rows below are being
        // rewritten and it should try again rather than trust a half-written
        // one.  This is the only synchronisation between the two processes.
        ++g_shm->rspSeq;

        for (int i = 0; i < terrain::probe::kMaxQueries; ++i) {
            const terrain::probe::Query& q = g_shm->queries[i];
            terrain::probe::Result&      r = g_shm->results[i];

            if (!q.enabled) { r.status = terrain::probe::StatusNone; continue; }

            // Echo the point before deciding anything, so a row always says
            // which point it is about -- including one that is refused.  Only
            // `status` tells an answer from a refusal; without the echo a reader
            // could not tell a refused row from one it had since replaced, and a
            // refused point would look like a wedged probe.
            r.latDeg = q.latDeg;
            r.lonDeg = q.lonDeg;
            r.key    = q.key;

            if (!(q.latDeg == q.latDeg) || !(q.lonDeg == q.lonDeg) ||
                q.latDeg < -90.0 || q.latDeg > 90.0 ||
                q.lonDeg < -180.0 || q.lonDeg > 180.0) {
                r.status = terrain::probe::StatusInvalid;
                continue;
            }

            // World position as the PDK wants it: X = longitude (radians),
            // Y = altitude (feet), Z = latitude (radians).  Note the ordering -
            // it is not the (lat, lon, alt) order the rest of the world uses.
            DXYZ pos;
            pos.dX = q.lonDeg * kDegToRad;
            pos.dY = q.altMeters * kMetersToFt;
            pos.dZ = q.latDeg * kDegToRad;

            float feet = 0.0f;
            const HRESULT hr = g_surface->QuerySurfaceElevation(feet, pos);

            if (SUCCEEDED(hr)) {
                r.hotMeters = static_cast<double>(feet) * kFtToMeters;
                r.status    = terrain::probe::StatusOk;
            } else {
                r.hotMeters = 0.0;
                r.status    = terrain::probe::StatusFailed;
                ++g_shm->queryFails;
            }
        }

        ++g_shm->rspSeq;
    }

    ::InterlockedExchange(&g_servicing, 0);
}

//-----------------------------------------------------------------------------
// The frame event: once per rendered frame, the tick of a running sim
//-----------------------------------------------------------------------------
void probeOnFrame() {
    if (!g_shm) return;
    ::InterlockedIncrement(reinterpret_cast<volatile LONG*>(&g_shm->frameTicks));
    g_lastFrameMs = ::GetTickCount();
    ::InterlockedExchange(&g_frameSeen, 1);
    serviceTick(true);
}

void probeOnMessage(IParameterListV400* p) {
    if (!g_shm || !p || p->GetCount() < 1) return;

    const uint64_t id = p->GetParameter(0).Value;
    uint32_t e = g_shm->simEvents;
    switch (id) {
    case EVENT_MESSAGE_SIM_PAUSED:
        e |= terrain::probe::simEvent::kPausedNow | terrain::probe::simEvent::kSawPause;
        break;
    case EVENT_MESSAGE_SIM_UNPAUSED:
        e |= terrain::probe::simEvent::kSawUnpause;
        e &= ~terrain::probe::simEvent::kPausedNow;
        break;
    case EVENT_MESSAGE_SIMCONNECT_SIMULATION_STOP:  e |= terrain::probe::simEvent::kSawSimStop;       break;
    case EVENT_MESSAGE_SIMCONNECT_SIMULATION_START: e |= terrain::probe::simEvent::kSawSimStart;      break;
    case EVENT_MESSAGE_SCENARIO_LOADING:            e |= terrain::probe::simEvent::kSawScenarioLoad;  break;
    case EVENT_MESSAGE_SCENARIO_LOADED:             e |= terrain::probe::simEvent::kSawScenarioReady; break;
    case EVENT_MESSAGE_CHANGE_USER_VEHICLE_START:   e |= terrain::probe::simEvent::kSawVehicleChange; break;
    case EVENT_MESSAGE_SIM_FROZEN:                  e |= terrain::probe::simEvent::kSawFrozen;        break;
    case EVENT_MESSAGE_SIM_UNFROZEN:                e |= terrain::probe::simEvent::kSawUnfrozen;      break;
    default: return;
    }
    g_shm->simEvents = e;
}

//-----------------------------------------------------------------------------
// The last resort: a clock of our own
//
// Prepar3D documents the frame event as "signaled once per frame", and on v5 it
// kept signalling it while the sim was paused -- which is why the probe was
// moved off the object's simulation in the first place.  On the rig's v6 it does
// not: a pause stops the frame loop as well, so both of Prepar3D's ticks go
// quiet, and with the sim held paused throughout the HOST is left without a
// terrain answer at all.  Nothing Prepar3D offers ticks on its own while it is
// paused, so the probe keeps one.
//
// It is deliberately the *last* resort, not a second opinion: it only services
// the table after every tick Prepar3D provides has been silent for a whole grace
// period, so a running sim never reaches it.  That also bounds what it can cost:
// the one call it makes off Prepar3D's thread happens only while Prepar3D's own
// loop has stopped, when the terrain engine is not being driven by a frame.
//-----------------------------------------------------------------------------
bool frameLive() {
    return g_frameSeen != 0 && (::GetTickCount() - g_lastFrameMs) <= kFrameFallbackMs;
}

DWORD WINAPI watchProc(LPVOID) {
    uint32_t lastTicks  = 0;
    DWORD    lastChange = ::GetTickCount();

    while (::InterlockedCompareExchange(&g_watchStop, 0, 0) == 0) {
        ::Sleep(kWatchMs);
        if (::InterlockedCompareExchange(&g_watchStop, 0, 0) != 0) break;
        if (!g_shm) { lastTicks = 0; lastChange = ::GetTickCount(); continue; }

        // Watch the heartbeat rather than the frame event alone: any tick that
        // services the table refreshes it, so the thread stays out of the way
        // for as long as something Prepar3D provides is still doing the work.
        const uint32_t t   = g_shm->ticks;
        const DWORD    now = ::GetTickCount();
        if (t != lastTicks) { lastTicks = t; lastChange = now; }

        if (frameLive()) continue;
        if (static_cast<DWORD>(now - lastChange) < kWatchGraceMs) continue;

        serviceTick(false);
    }
    return 0;
}

// The events carry no payload we need, so one callback class serves both; the
// kind only picks the handler.
class ProbeCallback final : public ICallbackV400
{
public:
    enum Kind { Frame, Message };
    explicit ProbeCallback(Kind k) : m_kind(k) {}

    ULONG STDMETHODCALLTYPE AddRef() override { return ::InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = ::InterlockedDecrement(&m_ref);
        if (n == 0) delete this;
        return n;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (::IsEqualIID(riid, IID_IUnknown) || ::IsEqualIID(riid, IID_ICallbackV400)) {
            *ppv = static_cast<ICallbackV400*>(this);
        }
        if (!*ppv) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    void Invoke(IParameterListV400* pParams) override {
        if (m_kind == Frame) probeOnFrame();
        else                 probeOnMessage(pParams);
    }

private:
    volatile LONG m_ref = 1;
    Kind          m_kind;
};

//-----------------------------------------------------------------------------
// The object fallback
//
// Only used until a frame event has been seen (and again if one stops arriving).
// Prepar3D calls Update() at the rate registered on attach, but a paused sim
// does not call it at all -- which is why the frame event became the tick, and
// why the add-on keeps a clock of its own behind both of them.
//-----------------------------------------------------------------------------
class ProbeSim final : public ISimulation
{
public:
    volatile LONG m_ref = 1;

    ULONG STDMETHODCALLTYPE AddRef() override { return ::InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = ::InterlockedDecrement(&m_ref);
        if (n == 0) delete this;
        return n;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        *ppv = nullptr;
        if (::IsEqualIID(riid, IID_IUnknown) || ::IsEqualIID(riid, IID_ISimulationV310) ||
            ::IsEqualIID(riid, IID_ISimulationV01)) {
            *ppv = static_cast<ISimulation*>(this);
        }
        if (!*ppv) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    STDMETHOD(Update)(double /*dDeltaT*/) override {
        if (!g_shm) return S_OK;

        // Record the object's mode while it is still being simulated.  Once the
        // sim pauses this freezes, and a frozen mode alongside a frozen object
        // tick is the fingerprint of the failure the frame tick exists to fix.
        if (g_host) g_shm->simMode = static_cast<uint32_t>(g_host->GetMode());

        // Defer to the frame event while it is live, so the table is serviced by
        // one tick and not two.
        if (!frameLive()) serviceTick(true);
        return S_OK;
    }

    STDMETHOD(SaveLoadState)(__in __notnull PSaveLoadCallback /*pfnCallback*/,
                             __in const BOOL /*bSave*/) override {
        return S_OK;
    }
};

//-----------------------------------------------------------------------------
// Attaching the tick to an object
//
// The surface query service is global - it does not care whose object we hang
// the tick on - so the first object the sim offers that accepts a simulation is
// good enough.  The user's own aircraft is preferred when it is the one on
// offer, because it is the object that lives for the whole session.
//-----------------------------------------------------------------------------
void attachTo(IBaseObjectV400* obj) {
    if (g_sim || !obj) return;

    ProbeSim* sim = new ProbeSim();

    const HRESULT hr = obj->RegisterSimulation(sim, kTickHz);
    if (FAILED(hr)) {
        publish(hr);
        ++g_attachFails;
        publishDiag();
        sim->Release();
        return;
    }

    obj->AddRef();
    g_host = obj;
    g_sim  = sim;
    g_attached = true;
    publish(0);
    publishDiag();

    // Record the rate we asked the sim for, so the reader can tell a probe that
    // is merely running from one the sim is actually ticking.
    if (g_shm) g_shm->tickRateHz = static_cast<uint32_t>(kTickHz);
}

void detach() {
    if (g_host) { g_host->Release(); g_host = nullptr; }
    if (g_sim)  { g_sim->Release();  g_sim  = nullptr; }
    if (g_shm)  { g_shm->alive = 0; }
    g_attached   = false;
    g_hostIsUser = false;
    publishDiag();
}

HRESULT STDMETHODCALLTYPE onObjectCreate(__in IUnknown& obj) {
    IBaseObjectV400* base = nullptr;
    if (FAILED(obj.QueryInterface(IID_IBaseObjectV400, reinterpret_cast<void**>(&base))) || !base) {
        return S_OK;
    }

    if (base->IsUser()) {
        // Counted only for the user object: that is the only create that can
        // move the tick, and counting every object would saturate the field
        // during scenario load and hide the one change that matters.
        ++g_creates;
        publishDiag();

        if (g_sim) {
            // The sim has offered a new user object while we are still bound to
            // the old one.  The old object may stop being simulated -- which is
            // the "tick stopped" failure this diagnostic is here to catch -- so
            // record that it happened.  Deliberately no re-attach: this build
            // observes only.
            g_skippedUserCreate = true;
            publishDiag();
        } else {
            attachTo(base);
        }
    }
    base->Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE onObjectRemove(__in IUnknown& obj) {
    IBaseObjectV400* base = nullptr;
    if (FAILED(obj.QueryInterface(IID_IBaseObjectV400, reinterpret_cast<void**>(&base))) || !base) {
        return S_OK;
    }

    // Only the object we tick on, or a user object, is worth counting.
    const bool ours = g_host && static_cast<IUnknown*>(base) == static_cast<IUnknown*>(g_host);
    if (ours || base->IsUser()) {
        ++g_removes;
        publishDiag();
    }
    if (ours) detach();
    base->Release();
    return S_OK;
}

} // namespace

//-----------------------------------------------------------------------------
// Add-on entry points (see the .def; Prepar3D calls these as it loads the DLL)
//-----------------------------------------------------------------------------
STDMETHODIMP_(void) DLLStart(__in __notnull IPdk* pPdk)
{
    if (!pPdk) return;

    openShared();
    if (!g_shm) return;

    // The service that does the actual work.  Both the service id and the
    // interface are the same in the v4, v5 and v6 SDKs, so one build serves any
    // of them.
    //
    // QueryService takes the *service* id first and the interface second, and
    // for this one the two are not the same GUID: SID_SurfaceQueryManager is
    // {0099b8b4-...} while IID_ISurfaceQueryManagerV400 is {87fff968-...}.
    // Passing the IID as the service returns E_NOINTERFACE, which left the probe
    // ticking with no terrain service behind it (alive == 0).
    if (FAILED(pPdk->QueryService(SID_SurfaceQueryManager, IID_ISurfaceQueryManagerV400,
                                  reinterpret_cast<void**>(&g_surface)))) {
        g_surface = nullptr;
        publish(E_NOINTERFACE);
    }

    // The frame event is the tick of a running sim (see the file header).
    // Registered before the object manager so the probe still ticks if that
    // query is the one that fails -- and so there is no moment where the object
    // tick is the only one.  The interface grew over the SDKs and the versions do
    // not share a GUID, so ask for the one the headers in use declare, exactly as
    // the SDK's own PdkServices helper does.
#if defined(P3D_MANAGER_V520)
    const HRESULT evHr = pPdk->QueryService(SID_EventService, IID_IEventServiceV510,
#else
    const HRESULT evHr = pPdk->QueryService(SID_EventService, IID_IEventServiceV600,
#endif
                                            reinterpret_cast<void**>(&g_events));
    if (SUCCEEDED(evHr) && g_events) {
        g_frameCb = new ProbeCallback(ProbeCallback::Frame);
        g_msgCb   = new ProbeCallback(ProbeCallback::Message);
        if (FAILED(g_events->RegisterCallback(EVENTID_Frame, g_frameCb))) ++g_frameFails;
        g_events->RegisterCallback(EVENTID_Message, g_msgCb);
    } else {
        g_events = nullptr;
        ++g_frameFails;
    }
    publishDiag();

    // The clock of our own, for the times Prepar3D stops ticking: see watchProc.
    // Started here rather than after the object manager because the surface
    // service is the only thing it needs, and it must be running before the sim
    // can be paused.
    g_watchStop = 0;
    g_watch = ::CreateThread(nullptr, 0, watchProc, nullptr, 0, nullptr);

    if (FAILED(pPdk->QueryService(SID_SimObjectManager, __uuidof(ObjectManager),
                                  reinterpret_cast<void**>(&g_objects)))) {
        g_objects = nullptr;
        if (!g_surface) publish(E_NOINTERFACE);
        return;
    }

    g_objects->RegisterOnObjectCreateCallback(onObjectCreate);
    g_objects->RegisterOnObjectRemoveCallback(onObjectRemove);
}

STDMETHODIMP_(void) DLLStop()
{
    // The thread first: it touches the mapping and the surface service, so
    // nothing below may run while it could still be in a pass.  If it will not
    // come back the process is going away anyway; stop rather than pull the
    // services out from under it.
    if (g_watch) {
        ::InterlockedExchange(&g_watchStop, 1);
        const DWORD done = ::WaitForSingleObject(g_watch, 5000);
        ::CloseHandle(g_watch);
        g_watch = nullptr;
        if (done != WAIT_OBJECT_0) return;
    }

    if (g_events) {
        if (g_frameCb) g_events->UnregisterCallback(EVENTID_Frame, g_frameCb);
        if (g_msgCb)   g_events->UnregisterCallback(EVENTID_Message, g_msgCb);
        g_events = nullptr;
    }
    if (g_frameCb) { g_frameCb->Release(); g_frameCb = nullptr; }
    if (g_msgCb)   { g_msgCb->Release();   g_msgCb   = nullptr; }

    if (g_objects) {
        g_objects->UnRegisterOnObjectCreateCallback(onObjectCreate);
        g_objects->UnRegisterOnObjectRemoveCallback(onObjectRemove);
        g_objects = nullptr;
    }
    detach();
    closeShared();
    g_surface = nullptr;
}
