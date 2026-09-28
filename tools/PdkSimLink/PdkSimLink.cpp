//-----------------------------------------------------------------------------
// PdkSimLink.cpp
//
// A Prepar3D PDK add-on that renders the HOST's entities inside the sim, from
// the poses CigiStarter publishes in shared memory.
//
// CigiStarter has always received the HOST's whole world as CIGI -- an Entity
// Control per entity per frame, roll/pitch/yaw and lat/lon/alt -- and until now
// only read entity 0's position out of it, to anchor the HAT/HOT answers. The
// vendor program that used to put that world on screen did it by repositioning
// the sim's own aircraft over FSUIPC, which is a fight: the flight model keeps
// simulating between writes, and everything the sim owns (time, weather,
// effects) is left to be reconciled by hand.
//
// This DLL takes the other route. It runs in-process (loaded from an add-on.xml
// as a PDK DLL), reads CigiStarter's table on Prepar3D's own frame tick, and
// moves the sim object that belongs to each entity to the pose the HOST
// commanded. CigiStarter never touches the sim.
//
// First cut: pose only, and the object is created with the sim's own object
// manager from a container title configured next to this DLL. That native
// object still has its own simulation between the writes -- the point of this
// build is to prove the link end to end on the bench, and the follow-up is a
// custom ISimObject with no flight model of its own, which is the version that
// does not have to race the native one at all.  Which is why the pose is read
// back (IBaseObjectV400::GetPosition) each tick and the gap to the commanded
// pose published: that number is the race, and it is what decides whether the
// follow-up is optional.
//
// See ../../SimLinkIpc.h for the shared-memory contract, ../../SimLink.h for
// the CigiStarter half, and README.md in this folder for how it is installed.
//-----------------------------------------------------------------------------

#include <windows.h>

#include <Pdk.h>
#include <ISimObject.h>
// The EVENTID_* GUIDs are declared with DEFINE_GUID, which is an extern
// declaration unless INITGUID is in scope. This is the only translation unit in
// the DLL, so instantiate them here.
#include <initguid.h>
#include <IEventService.h>

#include "SimLinkIpc.h"
#include "SimLinkMap.h"   // the PDK-free half: what to render, and into what units

using namespace P3D;

namespace {

//-----------------------------------------------------------------------------
// Module state
//
// The PDK owns the service pointers, so they are borrowed and never released.
// The shared mapping and the objects we create are ours.
//-----------------------------------------------------------------------------
HANDLE                 g_mapping = nullptr;
simlink::Shared*       g_shm     = nullptr;

// The object manager the created entities live on.  It grew a method between the
// SDKs and the versions do not share a GUID (V520 on v5, V540 on v6), exactly as
// PdkTerrainProbe handles it.
#if defined(P3D_MANAGER_V520)
using ObjectManager = P3D::ISimObjectManagerV520;
#else
using ObjectManager = P3D::ISimObjectManagerV540;
#endif
ObjectManager*         g_objects = nullptr;

// The container title every entity object is created from.  There is no title
// that is guaranteed to exist in an arbitrary installation, so this is a deploy-
// ment input: PdkSimLink.ini beside the DLL, [simlink] title=<name>.  With no
// title the add-on reports itself present but not creating anything, which the
// operator window says.
wchar_t                g_title[260] = L"";
bool                   g_haveTitle  = false;

// One created object per entity slot.
//
// `seq` is deliberately not cached: the pose is re-asserted every tick even when
// the row has not changed, because the object is the sim's own and would other-
// wise drift back onto its own flight model between writes.  Re-asserting is
// what holds it on the HOST's pose.
//
// `lastWritten` is the pose SetPosition was last given, kept so the next tick's
// read-back can be measured against it.  That gap -- where the object ended up
// versus where it was told to be -- is the native flight model's fight.
struct Slot {
    IBaseObjectV400* base   = nullptr;
    bool             active = false;
    bool             haveWrite = false;
    simlink::PdkPose lastWritten{};
};
Slot                   g_slots[simlink::kMaxEntities];

// Prepar3D's global event service, and the frame callback.  The frame event is
// the tick of a running sim, and a running sim is exactly what this writer
// needs: if the sim is paused nothing is rendered anyway, so unlike the terrain
// probe there is no fallback thread of our own.
IEventServiceV400*     g_events  = nullptr;
class LinkCallback;
LinkCallback*          g_frameCb = nullptr;

// Deferral state, as in the terrain probe: the frame event is the fast path, and
// the object simulation below only services the table until one has been seen
// (and again if one stops).  The two never both service it, so they never race.
volatile LONG          g_frameSeen   = 0;
volatile DWORD         g_lastFrameMs = 0;
constexpr DWORD        kFrameFallbackMs = 500;

// An object to hang the fallback tick on, and the last simulation's delta.
IBaseObjectV400*       g_host = nullptr;

// Only one pass may rewrite the objects at a time.  The frame event and an
// object simulation are both delivered on Prepar3D's main thread; the guard is
// here because a class of Prepar3D bugs (an event delivered re-entrantly during
// our own CreateObject) would otherwise recurse.
volatile LONG          g_servicing = 0;

// The wall clock of the last pass, so SetPosition's dDeltaT is a real interval
// rather than a nominal one.  (The degrees/radians and metres/feet factors live
// in SimLinkMap.h with the rest of the conversion.)
DWORD                  g_lastTickMs = 0;

// Diagnostic counters published in `Shared::diag` (SimLinkIpc.h).  They exist so
// a reader can tell *why* nothing is moving: no title to create from, no object
// manager, no tick, or the sim refusing the creates.
void publishDiag() {
    if (!g_shm) return;
    const auto sat = [](uint32_t n) { return n > 0xFFu ? 0xFFu : n; };
    uint32_t v = simlink::diag::kPresent;
    v |= sat(g_shm->objectsCreated) << simlink::diag::kObjectsShift;
    v |= sat(g_shm->setFails)       << simlink::diag::kSetFailsShift;
    v |= sat(g_shm->createFails)    << simlink::diag::kCreateFailsShift;
    if (g_objects)   v |= simlink::diag::kHasManager;
    if (g_frameSeen) v |= simlink::diag::kHasTick;
    if (!g_haveTitle) v |= simlink::diag::kNoTitle;
    if (g_shm->positionReads) v |= simlink::diag::kHasReadback;
    g_shm->diag = v;
}

void publish(long hr) {
    if (g_shm) g_shm->lastError = hr;
}

//-----------------------------------------------------------------------------
// The read-back: the pose the sim's object is actually at
//
// `held[]` is this add-on's table -- the IG never writes it -- so it is guarded
// by its own seqlock exactly as the IG's rows are, for the reader's sake.
//-----------------------------------------------------------------------------
void publishHeld(int i, const simlink::CigiPose& held,
                 double driftMeters, double driftDeg, uint32_t entityId) {
    simlink::Held& h = g_shm->held[i];
    ++h.seq;
    h.entityId    = entityId;
    h.tick        = g_shm->ticks;
    h.valid       = 1;
    h.latDeg      = held.latDeg;
    h.lonDeg      = held.lonDeg;
    h.altMeters   = held.altMeters;
    h.rollDeg     = held.rollDeg;
    h.pitchDeg    = held.pitchDeg;
    h.yawDeg      = held.yawDeg;
    h.driftMeters = driftMeters;
    h.driftDeg    = driftDeg;
    ++h.seq;
}

void clearHeld(int i) {
    simlink::Held& h = g_shm->held[i];
    if (!h.valid) return;
    ++h.seq;
    h.valid = 0;
    ++h.seq;
}

//-----------------------------------------------------------------------------
// Shared memory
//-----------------------------------------------------------------------------
void openShared() {
    g_mapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                     0, sizeof(simlink::Shared),
                                     simlink::kMappingName);
    if (!g_mapping) return;

    void* view = ::MapViewOfFile(g_mapping, FILE_MAP_ALL_ACCESS, 0, 0,
                                 sizeof(simlink::Shared));
    if (!view) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
        return;
    }

    g_shm = static_cast<simlink::Shared*>(view);
    ZeroMemory(g_shm, sizeof(*g_shm));
    g_shm->magic    = simlink::kMagic;
    g_shm->version  = simlink::kVersion;
    g_shm->capacity = simlink::kMaxEntities;
    g_shm->simPid   = ::GetCurrentProcessId();
    g_shm->alive    = 0;
    g_shm->lastError = 0;
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
// Configuration: the container title to create entities from
//-----------------------------------------------------------------------------
void loadConfig() {
    wchar_t path[MAX_PATH] = L"";
    HMODULE self = nullptr;
    if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              reinterpret_cast<LPCWSTR>(&loadConfig), &self)) {
        return;
    }
    if (!::GetModuleFileNameW(self, path, MAX_PATH)) return;

    // Swap the file name for PdkSimLink.ini, so the two always sit together.
    wchar_t* slash = wcsrchr(path, L'\\');
    if (!slash) return;
    wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"PdkSimLink.ini");

    const DWORD n = ::GetPrivateProfileStringW(L"simlink", L"title", L"",
                                               g_title, ARRAYSIZE(g_title), path);
    if (n == 0) {
        // A sane case, not an error: the add-on is loaded and reporting, it just
        // has nothing to create yet.  Say so in the status words rather than
        // leaving the operator to guess why nothing appears.
        g_haveTitle = false;
        publishDiag();
        return;
    }
    g_haveTitle = true;
}

//-----------------------------------------------------------------------------
// The tick
//
// One pass over the entity table.  Prepar3D never sees the IG's process: the IG
// only drops poses into the table, and this is what turns them into sim objects.
// Every tick funnels through here, so the work has exactly one implementation
// and no two passes can overlap.
//
// Reading a row under its seqlock, deciding which rows are wanted, and the
// conversion into the PDK's units all live in SimLinkMap.h, which the contract
// test compiles too -- so the shipped DLL and the test cannot disagree.
//-----------------------------------------------------------------------------
void retire(int i) {
    Slot& s = g_slots[i];
    if (!s.active) return;

    // Order matters, and getting it wrong is a use-after-free that takes the
    // simulator down with it.  RemoveObject() removes the object from the world
    // and the removal callback fires "just prior to destruction"
    // (ISimObject.h), so an interface held across the call points into
    // something the sim has already destroyed.  The PDK's own WorldObject
    // sample never holds an interface to the object it removes -- it works
    // entirely by id -- and this is the same discipline: take the id, forget
    // the pointer, then let the reference go, then ask for the removal.
    //
    // The slot is cleared before either call for the same reason: the removal
    // callback compares against g_slots, and a reference released as the last
    // one destroys the object, which would re-enter it.
    const UINT id = s.base ? s.base->GetId() : 0;
    IBaseObjectV400* base = s.base;
    s.base   = nullptr;
    s.active = false;
    s.haveWrite = false;
    if (base) base->Release();
    if (g_objects && id) g_objects->RemoveObject(id);
    if (g_shm) {
        ++g_shm->objectsRemoved;

        // The read-back of an object that no longer exists means nothing, and a
        // stale row left behind would be read as a drift by the bench.
        clearHeld(i);

        // The drift words are a maximum for as long as a run is in progress.
        // When the last object goes the run is over, so the next one starts from
        // zero rather than inheriting this run's worst case (the region is the
        // add-on's and outlives any one CigiStarter).
        bool anyActive = false;
        for (int j = 0; j < simlink::kMaxEntities; ++j) {
            if (g_slots[j].active) { anyActive = true; break; }
        }
        if (!anyActive) {
            g_shm->driftMaxMeters = 0.0;
            g_shm->driftMaxDeg    = 0.0;
            g_shm->driftSamples   = 0;
            g_shm->heldValid      = 0;
        }
    }
}

void serviceTick(bool fromSim) {
    if (!g_shm) return;
    if (::InterlockedExchange(&g_servicing, 1) != 0) return;

    ++g_shm->ticks;
    g_shm->alive = (g_objects != nullptr) ? 1 : 0;

    const DWORD now = ::GetTickCount();
    const double dt = (g_lastTickMs && now != g_lastTickMs)
                          ? static_cast<double>(now - g_lastTickMs) / 1000.0
                          : 0.0;
    g_lastTickMs = now;
    (void)fromSim;

    publishDiag();

    if (g_objects && g_haveTitle) {
        uint32_t applied = 0;
        uint32_t heldValid = 0;
        for (int i = 0; i < simlink::kMaxEntities; ++i) {
            simlink::Entity e;
            if (!simlink::readEntityRow(g_shm->entities[i], e)) continue;

            if (!simlink::entityWanted(e)) {
                // A row that is gone, standby or destroyed: remove the object it
                // had, so a HOST that stops driving an entity does not leave it
                // parked in the sim.
                retire(i);
                continue;
            }

            if (!g_slots[i].active) {
                UINT id = 0;
                const HRESULT hr = g_objects->CreateObject(g_title, id);
                if (FAILED(hr) || id == 0) {
                    ++g_shm->createFails;
                    publish(hr);
                    continue;
                }
                IBaseObjectV400* base = nullptr;
                if (FAILED(g_objects->GetObject(id, IID_IBaseObjectV400,
                                                reinterpret_cast<void**>(&base))) ||
                    !base) {
                    // Created but not yet queryable: try again next tick rather
                    // than leaking the id.  Remove it so the retry starts clean.
                    g_objects->RemoveObject(id);
                    ++g_shm->createFails;
                    continue;
                }
                g_slots[i].base   = base;   // GetObject handed us a reference
                g_slots[i].active = true;
                ++g_shm->objectsCreated;
            }

            // CIGI gives metres and degrees; the PDK wants feet and radians, in
            // a (longitude, altitude, latitude) / (pitch, heading, bank) order.
            const simlink::PdkPose p = simlink::toPdkPose(e);

            // First the read-back.  This is the pose the object is *actually* at
            // now -- after the sim's own simulation has run on it since the last
            // write -- and the gap to `lastWritten` is how much the native
            // flight model fought the last write.  It is taken before the write
            // for exactly that reason: read after, and the object would still be
            // where we just put it, whatever its flight model is doing.
            if (g_slots[i].haveWrite) {
                DXYZ gotLonAltLat = {0.0, 0.0, 0.0};
                DXYZ gotPhb       = {0.0, 0.0, 0.0};
                DXYZ gotVelA      = {0.0, 0.0, 0.0};
                DXYZ gotVelB      = {0.0, 0.0, 0.0};
                const HRESULT ghr = g_slots[i].base->GetPosition(
                    gotLonAltLat, gotPhb, gotVelA, gotVelB);
                if (SUCCEEDED(ghr)) {
                    const simlink::CigiPose held = simlink::toCigiPose(
                        gotLonAltLat.dX, gotLonAltLat.dY, gotLonAltLat.dZ,
                        gotPhb.dX, gotPhb.dY, gotPhb.dZ);
                    const simlink::PdkPose& c = g_slots[i].lastWritten;
                    const simlink::CigiPose commanded =
                        simlink::toCigiPose(c.lonRad, c.altFeet, c.latRad,
                                            c.pitchRad, c.headingRad, c.bankRad);

                    const double dm = simlink::poseGapMeters(commanded, held);
                    const double dd = simlink::attitudeGapDeg(commanded, held);
                    ++g_shm->positionReads;
                    ++g_shm->driftSamples;
                    if (dm > g_shm->driftMaxMeters) g_shm->driftMaxMeters = dm;
                    if (dd > g_shm->driftMaxDeg)    g_shm->driftMaxDeg    = dd;
                    publishHeld(i, held, dm, dd, e.entityId);
                    ++heldValid;
                } else {
                    ++g_shm->getFails;
                    publish(ghr);
                    clearHeld(i);
                }
            }

            const DXYZ lonAltLat = {p.lonRad, p.altFeet, p.latRad};
            const DXYZ phb       = {p.pitchRad, p.headingRad, p.bankRad};

            // Velocity is left zero: the pose is refreshed every rendered frame,
            // so there is nothing between writes for the sim to interpolate,
            // and a fabricated velocity would be worse than none.  A HOST that
            // sends rate information will feed this in a later cut.
            const DXYZ zero = {0.0, 0.0, 0.0};

            const HRESULT hr = g_slots[i].base->SetPosition(
                lonAltLat, phb, zero, zero, /*bIsOnGround*/ FALSE, dt);
            if (SUCCEEDED(hr)) {
                ++applied;
                g_slots[i].lastWritten = p;
                g_slots[i].haveWrite   = true;
            } else {
                ++g_shm->setFails;
                publish(hr);
            }
        }
        g_shm->applied   = applied;
        g_shm->heldValid = heldValid;
    } else {
        // No manager, or no title: drop whatever objects exist so a config that
        // arrives later starts from a clean sim rather than a mixture.
        if (!g_haveTitle) {
            for (int i = 0; i < simlink::kMaxEntities; ++i) retire(i);
        }
        g_shm->applied   = 0;
        g_shm->heldValid = 0;
    }

    ::InterlockedExchange(&g_servicing, 0);
}

//-----------------------------------------------------------------------------
// The frame event: once per rendered frame, the tick of a running sim
//-----------------------------------------------------------------------------
void linkOnFrame() {
    if (!g_shm) return;
    ::InterlockedIncrement(reinterpret_cast<volatile LONG*>(&g_shm->frameTicks));
    g_lastFrameMs = ::GetTickCount();
    ::InterlockedExchange(&g_frameSeen, 1);
    serviceTick(true);
}

bool frameLive() {
    return g_frameSeen != 0 && (::GetTickCount() - g_lastFrameMs) <= kFrameFallbackMs;
}

class LinkCallback final : public ICallbackV400
{
public:
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

    void Invoke(IParameterListV400* /*pParams*/) override { linkOnFrame(); }

private:
    volatile LONG m_ref = 1;
};

//-----------------------------------------------------------------------------
// The object fallback
//
// Only used until a frame event has been seen (and again if one stops arriving).
// Prepar3D calls Update() at the rate registered on attach; a paused sim does
// not call it at all, which is fine here -- a paused sim renders nothing for the
// writer to keep in step with.
//-----------------------------------------------------------------------------
class LinkSim final : public ISimulation
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
        if (!frameLive()) serviceTick(true);
        return S_OK;
    }

    STDMETHOD(SaveLoadState)(__in __notnull PSaveLoadCallback /*pfnCallback*/,
                             __in const BOOL /*bSave*/) override {
        return S_OK;
    }
};

//-----------------------------------------------------------------------------
// The object-lifecycle callbacks
//
// A tick is hung off the sim's *user* object -- the one that lives for the whole
// session -- purely so there is a tick when the frame event is not delivered.
// The objects this add-on creates are deliberately never used for that, or the
// tick would move with an entity.
//-----------------------------------------------------------------------------
void attachTo(IBaseObjectV400* obj) {
    if (g_host || !obj) return;
    LinkSim* sim = new LinkSim();
    if (FAILED(obj->RegisterSimulation(sim, 60.0f))) {
        sim->Release();
        return;
    }
    obj->AddRef();
    g_host = obj;
}

HRESULT STDMETHODCALLTYPE onObjectCreate(__in IUnknown& obj) {
    IBaseObjectV400* base = nullptr;
    if (FAILED(obj.QueryInterface(IID_IBaseObjectV400,
                                  reinterpret_cast<void**>(&base))) || !base) {
        return S_OK;
    }
    if (base->IsUser()) {
        // The user object is offered first and preferred: it is the one that
        // outlives the session.
        attachTo(base);
    } else if (!g_host) {
        attachTo(base);
    }
    base->Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE onObjectRemove(__in IUnknown& obj) {
    IBaseObjectV400* base = nullptr;
    if (FAILED(obj.QueryInterface(IID_IBaseObjectV400,
                                  reinterpret_cast<void**>(&base))) || !base) {
        return S_OK;
    }
    if (g_host && static_cast<IUnknown*>(base) == static_cast<IUnknown*>(g_host)) {
        g_host->Release();
        g_host = nullptr;
    }

    // An entity object taken out by the sim itself -- the world culled it, or
    // the user deleted it.  The slot has to forget it here, or the next tick
    // would SetPosition() through a pointer the sim has just destroyed.
    // retire() clears its own slot before it asks for the removal, so this only
    // ever matches a removal this add-on did not start.
    const UINT id = base->GetId();
    for (int i = 0; i < simlink::kMaxEntities; ++i) {
        Slot& s = g_slots[i];
        if (!s.active || !s.base || s.base->GetId() != id) continue;
        IBaseObjectV400* dying = s.base;
        s.base   = nullptr;  // before the release: letting the last reference go
        s.active = false;    // destroys the object, which re-enters here
        s.haveWrite = false;
        dying->Release();
        if (g_shm) {
            ++g_shm->objectsRemoved;
            clearHeld(i);    // an object that is gone has no read-back
        }
        break;
    }

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

    loadConfig();

    // The frame event is the tick of a running sim (see the file header).
    // Registered before the object manager so the writer is ready to move an
    // object the moment one exists.
#if defined(P3D_MANAGER_V520)
    const HRESULT evHr = pPdk->QueryService(SID_EventService, IID_IEventServiceV510,
#else
    const HRESULT evHr = pPdk->QueryService(SID_EventService, IID_IEventServiceV600,
#endif
                                            reinterpret_cast<void**>(&g_events));
    if (SUCCEEDED(evHr) && g_events) {
        g_frameCb = new LinkCallback();
        g_events->RegisterCallback(EVENTID_Frame, g_frameCb);
    } else {
        g_events = nullptr;
    }

    if (FAILED(pPdk->QueryService(SID_SimObjectManager, __uuidof(ObjectManager),
                                  reinterpret_cast<void**>(&g_objects)))) {
        g_objects = nullptr;
        publish(E_NOINTERFACE);
    }

    if (g_objects) {
        g_objects->RegisterOnObjectCreateCallback(onObjectCreate);
        g_objects->RegisterOnObjectRemoveCallback(onObjectRemove);
    }

    publishDiag();
}

STDMETHODIMP_(void) DLLStop()
{
    if (g_events) {
        if (g_frameCb) g_events->UnregisterCallback(EVENTID_Frame, g_frameCb);
        g_events = nullptr;
    }
    if (g_frameCb) { g_frameCb->Release(); g_frameCb = nullptr; }

    if (g_objects) {
        g_objects->UnRegisterOnObjectCreateCallback(onObjectCreate);
        g_objects->UnRegisterOnObjectRemoveCallback(onObjectRemove);

        // Take our objects out of the sim before letting go of the manager.
        for (int i = 0; i < simlink::kMaxEntities; ++i) retire(i);
        g_objects = nullptr;
    }

    if (g_host) { g_host->Release(); g_host = nullptr; }
    closeShared();
}
