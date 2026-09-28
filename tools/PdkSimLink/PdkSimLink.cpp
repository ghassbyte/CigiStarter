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
// The object is created with the sim's own object manager from a container
// title configured next to this DLL, and there are two kinds of container it can
// be given:
//
//   * the sim's own (any stock title, an aircraft): it keeps its own simulation
//     between the writes and refuses a pose far below the ground under it (the
//     object is held above the ground there); it is not otherwise pulled off the
//     pose it is given;
//   * one of ours, carrying this DLL's category GUID (CigiEntity.h): a custom
//     ISimObject with no simulation and no contact with the ground, so the pose
//     written is the pose that stays and a command below the sim's terrain is
//     where the object goes.
//
// The pose is read back (IBaseObjectV400::GetPosition) each tick either way, and
// the gap to the commanded pose is published, because that number is what says
// which of the two is happening on this installation.
//
// One entity is special: the one the sim's view rides (`camera=`, and "The
// camera" below). The eye is inside it, so it is created from its own container
// (`own_ship_title`) -- one that carries no model, like the shipped
// CigiStarter.Entity -- and only the drawing is missing: its pose is written and
// read back like every other entity's.
//
// See ../../SimLinkIpc.h for the shared-memory contract, ../../SimLink.h for
// the CigiStarter half, and README.md in this folder for how it is installed.
//-----------------------------------------------------------------------------

#include <windows.h>

#include <cstdio>   // the opt-in camera log (camera_log=1) writes a file

#include <Pdk.h>
#include <ISimObject.h>
// The EVENTID_* GUIDs are declared with DEFINE_GUID, which is an extern
// declaration unless INITGUID is in scope. This is the only translation unit in
// the DLL, so instantiate them here.
#include <initguid.h>
#include <IEventService.h>
// The window-and-camera service: how the sim's own view is flown from an entity
// (see "The camera" below). It is the same header the sim's CameraSystemPDK
// sample uses, and it pulls in Helpers\ListBuilder.h for the window list.
#include <IWindowPluginSystem.h>

#include "CigiEntity.h"   // the physics-free ISimObject a container can be routed to
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

// The physics-free object (CigiEntity.h).  Registering the category is what makes
// a container carrying our CategoryId ours to drive.  These two are recomputed
// each tick from what the objects themselves report -- not from what the
// configuration hoped -- so `diag` says what is actually happening.
volatile LONG          g_physicsFreeObjects = 0;
volatile LONG          g_modePauseOrSlew    = 0;

// The container title every entity object is created from.  There is no title
// that is guaranteed to exist in an arbitrary installation, so this is a deploy-
// ment input: PdkSimLink.ini beside the DLL, [simlink] title=<name>.  With no
// title the add-on reports itself present but not creating anything, which the
// operator window says.
wchar_t                g_title[260] = L"";
bool                   g_haveTitle  = false;

// The container the entity the camera rides is created from -- the view's own
// aircraft, [simlink] own_ship_title.  The eye sits *inside* that entity, so it
// must not be drawn: a model there is a hull across the whole picture, which is
// exactly what the bench showed the first time the camera ran against a
// container that had been given a model (see "The camera" below).  The default
// is the model-less container that ships for this and nothing else
// (SimObjects\CigiEntity.OwnShip\sim.cfg); leaving the key empty draws the own
// ship too, from `title`, which is only wanted where the camera is not riding
// the picture that matters.
wchar_t                g_ownShipTitle[260] = L"CigiStarter.Entity.OwnShip";

// One created object per entity slot.
//
// `seq` is deliberately not cached: the pose is re-asserted every tick even when
// the row has not changed.  With the sim's own object that is what holds it on
// the HOST's pose against its own flight model; with ours it is only a
// re-assertion of a pose nothing else would move.
//
// `lastWritten` is the pose SetPosition was last given, kept so the next tick's
// read-back can be measured against it.  That gap -- where the object ended up
// versus where it was told to be -- is the native flight model's fight, and is
// zero when the object is one of ours.
//
// `physicsFree` is what the object itself reports its category to be (see
// isOurCategory), so the status word is a fact about the object and not an
// assumption about the title that created it.
struct Slot {
    IBaseObjectV400* base   = nullptr;
    bool             active = false;
    bool             haveWrite = false;
    bool             physicsFree = false;
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

//-----------------------------------------------------------------------------
// The camera
//
// The sim's view never leaves the user's own aircraft while the entity objects
// are separate, non-user objects -- so an entity can move perfectly and the
// picture stays still. This is what flies the picture.
//
// Where the eye goes comes from the HOST.  CigiStarter publishes the HOST's own
// View Control (`view` in the shared region, SimLinkIpc.h): one static
// definition in every capture, 2.8 m forward of entity 0's reference point, half
// a metre above it and 5 cm right, turned a degree -- the pilot's seat, not
// the airframe reference point a position-only camera has to sit on.  A
// degree and two metres is the whole difference between "the picture is drawn
// from the aircraft" and a cockpit view, and only the HOST knows which its view
// is.  In CIGI's body frame (+X nose, +Y right wing, +Z down) the offset has to
// be rotated by the entity's attitude before it becomes a latitude and
// longitude; SimLinkMap.h's viewEyePose() is that arithmetic, and the contract
// test drives it without a simulator.
//
// The sim's camera is a per-window service. A ViewGroup renders its channels
// from the window's *active* camera and applies each channel's frustum on top,
// so the view flies iff that camera does. Prep3D already ships the camera for
// exactly this job: the `Cigi View` definition in cameras.cfg is Origin = Fixed,
// MenuHidden/CycleHidden, HideUserAircraft -- the out-the-window camera an IG
// positions itself. A Fixed-origin camera is the only kind `SetLLA` applies to,
// and driving it from entity 0 leaves every per-channel angle in viewgroup.xml
// untouched, which is the whole point.
//
// The user chose an AI-object camera on entity 0; that route is not reachable
// from a PDK add-on, and the README records why. This is the reachable one, and
// it puts the eye in the same place.
//
// Entity objects are non-user objects, so Prepar3D does not treat the entity the
// eye is inside as an own ship: it draws it like any other object, and the eye
// ends up inside its model.  The entity that container was given on the bench
// was the AS332's body -- the operator's words for the result were "the view is
// inside the cabin of aircraft, not the cockpit view like usual".  Prepar3D
// hides the sim's *own* aircraft for this camera (the definition's
// HideUserAircraft) and this is the same rule for the entity this add-on
// created: the entity the camera rides is created from `own_ship_title`, a
// container that carries no model, so the pose is still written and read back
// and only the drawing is gone.  Measured on the bench, 2026-09-29: with the
// AS332 junctioned into the entities' container, the same parked pose that had
// shown the inside of a fuselage showed the apron, the hangars and the
// mountains instead.  The airframe is therefore deliberately not drawn: the
// cockpit view here is the HOST's eyepoint and the world it sees, not a
// modelled cabin around it.
//-----------------------------------------------------------------------------
IWindowPluginSystemV440* g_windows  = nullptr;
class CameraPlugin;
CameraPlugin*            g_camPlugin = nullptr;
bool                     g_cameraOn  = false;  // [simlink] camera= is set
int                      g_cameraEntity = -1;  // the entity the camera rides
wchar_t                  g_cameraView[128] = L"Cigi View";

// Bench diagnostics for the camera, [simlink] camera_log=1: a line per decision,
// appended beside the DLL.  Off by default -- the add-on writes no files in
// normal operation -- and it exists because the camera is the one part of this
// that has no status word of its own: the shared region says the objects moved,
// nothing says whether the sim accepted the camera.
bool                     g_camLog = false;
wchar_t                  g_logPath[260] = L"";
unsigned                 g_camLogLines = 0;

// Append one line beside the DLL. The path is worked out once and then cached;
// the counter caps the file so a long session cannot fill a disk with it.
void camLog(const wchar_t* line) {
    if (!g_camLog || g_camLogLines > 5000) return;
    ++g_camLogLines;

    if (!*g_logPath) {
        wchar_t path[MAX_PATH] = L"";
        HMODULE self = nullptr;
        if (!::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                      GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  reinterpret_cast<LPCWSTR>(&camLog), &self)) {
            return;
        }
        if (!::GetModuleFileNameW(self, path, MAX_PATH)) return;
        wchar_t* slash = wcsrchr(path, L'\\');
        if (!slash) return;
        wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path),
                 L"PdkSimLink.camera.log");
        wcscpy_s(g_logPath, path);
    }

    FILE* f = nullptr;
    if (_wfopen_s(&f, g_logPath, L"a, ccs=UTF-8") != 0 || !f) return;
    fputws(line, f);
    fputws(L"\n", f);
    fclose(f);
}

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
    if (g_physicsFreeObjects) v |= simlink::diag::kPhysicsFree;
    if (g_modePauseOrSlew)    v |= simlink::diag::kModePauseOrSlew;
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

    // The camera is read before the title, and separately from it: flying the
    // sim's view needs no entity object to have been created, so a deployment
    // that only wants the picture (or is still bringing the objects up) still
    // gets one.
    const int camera = ::GetPrivateProfileIntW(L"simlink", L"camera", -1, path);
    g_cameraEntity = (camera >= 0) ? camera : -1;
    g_cameraOn     = (camera >= 0);
    ::GetPrivateProfileStringW(L"simlink", L"camera_view", L"Cigi View",
                               g_cameraView, ARRAYSIZE(g_cameraView), path);
    g_camLog = ::GetPrivateProfileIntW(L"simlink", L"camera_log", 0, path) != 0;

    {
        wchar_t line[512] = L"";
        swprintf_s(line, L"config: camera=%d view='%s' log=%d",
                   g_cameraEntity, g_cameraView, g_camLog ? 1 : 0);
        camLog(line);
    }

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

    // The own ship's container: read after the title, and independently of it.
    // An absent key leaves the shipped default in place; a present but empty one
    // means "no separate container", i.e. the own ship is drawn from `title`.
    ::GetPrivateProfileStringW(L"simlink", L"own_ship_title",
                               L"CigiStarter.Entity.OwnShip",
                               g_ownShipTitle, ARRAYSIZE(g_ownShipTitle), path);
    if (g_cameraOn) {
        wchar_t line[512] = L"";
        swprintf_s(line, L"config: title='%s' own_ship_title='%s' (entity %d)",
                   g_title, g_ownShipTitle, g_cameraEntity);
        camLog(line);
    }
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
    s.physicsFree = false;
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

// Is this the physics-free object, or the sim's own?  Asked of the object rather
// than assumed from the title: the category GUID is the same one the sim routed
// the container by, so a title that happens to be a stock object reports itself
// as one and the status word stays honest.
bool isOurCategory(IBaseObjectV400* base) {
    IBaseObjectV430* b430 = nullptr;
    if (FAILED(base->QueryInterface(IID_IBaseObjectV430,
                                    reinterpret_cast<void**>(&b430))) || !b430) {
        return false;
    }
    GUID cat = {};
    const bool ours = SUCCEEDED(b430->GetCategoryId(cat)) &&
                      ::IsEqualGUID(cat, CigiEntity::CategoryId());
    b430->Release();
    return ours;
}

// The container an entity's object is created from.  Normally the configured
// `title`; for the entity the camera rides, the own-ship container instead,
// because the eye is inside that object and drawing it fills the picture with
// the model (the camera section above has the measurement that found this).
const wchar_t* titleFor(const simlink::Entity& e) {
    if (g_cameraOn && g_cameraEntity >= 0 &&
        static_cast<int>(e.entityId) == g_cameraEntity && *g_ownShipTitle) {
        return g_ownShipTitle;
    }
    return g_title;
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
        uint32_t physicsFree = 0;
        bool     modePauseOrSlew = false;
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
                const wchar_t* const title = titleFor(e);
                const HRESULT hr = g_objects->CreateObject(title, id);
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
                g_slots[i].physicsFree = isOurCategory(base);
                ++g_shm->objectsCreated;
                if (g_cameraOn && static_cast<int>(e.entityId) == g_cameraEntity) {
                    wchar_t line[512] = L"";
                    swprintf_s(line,
                               L"own ship: entity %u created from '%s' (the view's own aircraft, not drawn)",
                               e.entityId, title);
                    camLog(line);
                }
            }

            // What this object is and what it is doing, for the status word.  A
            // sim object in pause or slew is exactly the state the deployed rig's
            // vendor program used to hold the pose in -- and the state item 4's
            // rotor cannot turn in -- so it is worth reporting whether it is
            // happening at all.
            if (g_slots[i].physicsFree) ++physicsFree;
            if (g_slots[i].base->GetMode() & (SIMOBJECT_MODE_PAUSE | SIMOBJECT_MODE_SLEW)) {
                modePauseOrSlew = true;
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
        ::InterlockedExchange(&g_physicsFreeObjects, static_cast<LONG>(physicsFree));
        ::InterlockedExchange(&g_modePauseOrSlew, modePauseOrSlew ? 1 : 0);
    } else {
        // No manager, or no title: drop whatever objects exist so a config that
        // arrives later starts from a clean sim rather than a mixture.
        if (!g_haveTitle) {
            for (int i = 0; i < simlink::kMaxEntities; ++i) retire(i);
        }
        g_shm->applied   = 0;
        g_shm->heldValid = 0;
        ::InterlockedExchange(&g_physicsFreeObjects, 0);
        ::InterlockedExchange(&g_modePauseOrSlew, 0);
    }

    ::InterlockedExchange(&g_servicing, 0);
}

//-----------------------------------------------------------------------------
// The camera, continued: reading the entity, and the window plugin that drives
// the view from it.
//-----------------------------------------------------------------------------
// The pose the camera should be at this frame: entity `camera=` in the shared
// table, read under its seqlock and only if the HOST is still driving it.  A row
// that is gone, standby, destroyed or being rewritten leaves the camera where it
// was, so a gap in the stream is a held picture and not a jump to nowhere.
bool readCameraEntity(simlink::Entity& e) {
    if (!g_shm || g_cameraEntity < 0) return false;
    const uint32_t id = static_cast<uint32_t>(g_cameraEntity);
    const simlink::Entity& row = g_shm->entities[simlink::slotOf(id)];
    if (!simlink::readEntityRow(row, e)) return false;
    if (e.entityId != id) return false;
    return simlink::entityWanted(e);
}

// The HOST's own eye, read the same way and with the same discipline as the row
// above: only when one has arrived, never from a torn row, and only when it is
// attached to the entity this camera is riding.  A view attached to some *other*
// entity is not this camera's view, and applying its offsets here would put the
// eye where the HOST wants it for a different aircraft.
bool readCameraView(const simlink::Entity& e, simlink::View& v) {
    if (!g_shm) return false;
    if (!simlink::readViewRow(g_shm->view, v)) return false;
    if (!v.valid) return false;
    return v.entityId == e.entityId;
}

// The window plugin.  OnPreCameraUpdate is the documented place for values the
// camera's own update then uses for its position and orientation, which is why
// the pose is applied there and not on the frame event -- set it earlier and the
// camera's update would be free to overwrite it.
class CameraPlugin final : public IWindowPluginV400
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
        if (::IsEqualIID(riid, IID_IUnknown) ||
            ::IsEqualIID(riid, IID_IWindowPluginV400)) {
            *ppv = static_cast<IWindowPluginV400*>(this);
        }
        if (!*ppv) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }

    void OnAdd(IWindowV400* /*pWindow*/, ICameraSystemV400* /*pCamera*/) override {}

    void OnRemove(IWindowV400* /*pWindow*/, ICameraSystemV400* /*pCamera*/) override {}

    void OnPreCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {
        if (!pCamera) return;
        simlink::Entity e;
        if (!readCameraEntity(e)) return;

        // Where the eye goes.  The entity's own reference point is a point on
        // the airframe the HOST chose, not a viewpoint; the HOST's View Control
        // says where its eye is, and in every capture that is the pilot's seat.
        // With no View Control -- or one attached to a different entity -- the
        // camera stays on the reference point, which is what this did before the
        // eye was carried at all.
        simlink::CigiPose eye;
        eye.latDeg    = e.latDeg;
        eye.lonDeg    = e.lonDeg;
        eye.altMeters = e.altMeters;
        eye.rollDeg   = e.rollDeg;
        eye.pitchDeg  = e.pitchDeg;
        eye.yawDeg    = e.yawDeg;

        simlink::View v;
        const bool haveView = readCameraView(e, v);
        if (haveView) eye = simlink::viewEyePose(e, v);

        // SetPBH is on the V400 base, but SetLLA only arrived in V430 -- and a
        // Fixed-origin camera is positioned with SetLLA, so ask for the newer
        // interface rather than assuming it.  If the sim will not give it, the
        // camera simply stays where it is rather than the DLL failing to load.
        ICameraSystemV430* cam = nullptr;
        if (FAILED(pCamera->QueryInterface(IID_ICameraSystemV430,
                                           reinterpret_cast<void**>(&cam))) ||
            !cam) {
            return;
        }

        // CIGI's own units and order: degrees and metres MSL, roll/pitch/yaw --
        // and the PDK's SetPBH is (pitch, bank, heading), so roll and heading
        // swap places.  The bank *also* changes sign: Prepar3D's bank axis is
        // the opposite of CIGI's roll (SimLinkMap.h's toPdkPose has the
        // measurement), so the eye's roll is negated here exactly as an entity's
        // is.  Both have to agree, or a banked turn would draw the model one way
        // and the horizon the other.
        cam->SetLLA(eye.latDeg, eye.lonDeg, eye.altMeters);
        cam->SetPBH(static_cast<float>(eye.pitchDeg),
                    static_cast<float>(-eye.rollDeg),
                    static_cast<float>(eye.yawDeg));

        // The bench trace.  GetLLA/GetPBH read the camera *back* out of the sim,
        // so the log shows the commanded pose and the pose the camera is actually
        // on -- the camera has no status word in the shared region, and a camera
        // that silently refuses SetLLA looks identical to one that has moved.
        // Both poses are printed when there is a view, because the whole of the
        // cockpit fix is the difference between them.  Sampled about once a
        // second; camera_log is off in normal operation.
        //
        // Two orders are printed and labelled, because they are not the same
        // order and one field is not the same sign: `rpy` is CIGI's
        // roll/pitch/yaw, printed in that order, and `pbh` is the PDK's
        // pitch/bank/heading, so the `told pbh` bank reads as the negative of
        // the `eye` roll.  The read-back
        // can only show that the call landed -- it returns whatever number it was
        // given -- so a matching `reads pbh` proves the camera is *on* the told
        // pose, never that the bank leans the right way.  Only looking at the
        // picture settles the sign; that is why it lives in one place
        // (SimLinkMap.h's toPdkPose).
        if (g_camLog) {
            static unsigned tick = 0;
            if ((tick++ % 60u) == 0u) {
                double lla[3] = {0.0, 0.0, 0.0};
                float  pbh[3] = {0.0f, 0.0f, 0.0f};
                pCamera->GetLLA(lla[0], lla[1], lla[2]);
                pCamera->GetPBH(pbh[0], pbh[1], pbh[2]);

                WCHAR name[128] = L"";
                if (pWindow) {
                    const LPCWSTR n = pWindow->GetWindowName();
                    if (n) wcsncpy_s(name, n, _TRUNCATE);
                }
                wchar_t line[640] = L"";
                swprintf_s(line,
                           L"camera '%s' ent %.6f %.6f %.2f rpy %.2f %.2f %.2f "
                           L"| eye %.6f %.6f %.2f rpy %.2f %.2f %.2f "
                           L"| told pbh %.2f %.2f %.2f "
                           L"| reads %.6f %.6f %.2f pbh %.2f %.2f %.2f %s",
                           name, e.latDeg, e.lonDeg, e.altMeters,
                           e.rollDeg, e.pitchDeg, e.yawDeg,
                           eye.latDeg, eye.lonDeg, eye.altMeters,
                           eye.rollDeg, eye.pitchDeg, eye.yawDeg,
                           eye.pitchDeg, -eye.rollDeg, eye.yawDeg,
                           lla[0], lla[1], lla[2], pbh[0], pbh[1], pbh[2],
                           haveView ? L"(view applied)" : L"(no view: on the entity)");
                camLog(line);
            }
        }

        cam->Release();
    }

    void OnPostCameraUpdate(IWindowV400* /*pWindow*/, ICameraSystemV400* /*pCamera*/) override {}

    void OnViewChange(IWindowV400* /*pWindow*/, ICameraSystemV400* /*pCamera*/) override {}

    void OnClose(IWindowV400* /*pWindow*/, ICameraSystemV400* /*pCamera*/) override {}

    bool OnUserInput(HWND /*wnd*/, UINT /*message*/, WPARAM /*wParam*/,
                     LPARAM /*lParam*/) override {
        return false;
    }

private:
    volatile LONG m_ref = 1;
};

// Which windows already carry the plugin, and which have been put onto the
// configured camera definition.
//
// A window is remembered by its *object*, not by its name: putting a window onto
// another camera definition renames it (a view is named after its definition, so
// `Cockpit - View 00` becomes `Cigi View - View 00`), and a name-keyed list would
// read that as a brand-new window and add the plugin a second time.  The pointer
// is only ever compared, never called through, so a window the sim has since
// destroyed is harmless here.
//
// The list is bounded because a sim that opened and closed views forever would
// otherwise grow it forever; past the cap the plugin is not attached to anything
// new, which is the safe direction to fail in.
struct CamWindow {
    IWindowV400* window   = nullptr;  // borrowed; the sim owns it
    std::wstring name;
    bool         switched = false;    // SetCameraDefinition has been called on it
    bool         logged   = false;    // the one-off bench dump has been written
};
std::vector<CamWindow> g_camWindows;
constexpr size_t       kMaxCamWindows = 256;
unsigned               g_camLogWide = 0;  // passes logged with window counts

// One pass over the sim's 3D windows: attach the plugin to any window that has
// not been seen, and put any that is not already on the configured camera
// definition onto it.
//
// The definition switch is done here and not from inside the plugin because
// SetCameraDefinition replaces the window's camera system mid-flight (the
// previous one lives on behind GetPreviousCameraSystem) and re-entering that
// from the pre-camera callback is not something the SDK promises.  On the frame
// event the window is between updates, which is the safe place.
//
// That the camera definition can be *read* is itself versioned -- GetCameraDefinition
// arrived after SetCameraDefinition -- so a sim that will not give V610 is
// handled by switching once per window and trusting it to stay switched.
void serviceCamera() {
    if (!g_cameraOn || !g_windows || !g_camPlugin) return;

    P3D::WindowList windows;
    const HRESULT hr = g_windows->GetWindowList(windows);
    if (g_camLog && g_camLogWide < 3) {
        ++g_camLogWide;
        wchar_t line[256] = L"";
        swprintf_s(line, L"windows: hr=0x%08X count=%u",
                   static_cast<unsigned>(hr),
                   static_cast<unsigned>(windows.Items.size()));
        camLog(line);
    }
    if (FAILED(hr)) return;

    for (CComPtr<IWindowV400>& w : windows.Items) {
        if (!w) continue;

        IWindowV400* raw = w;
        const LPCWSTR name = w->GetWindowName();
        if (!name || !*name) continue;

        CamWindow* entry = nullptr;
        for (CamWindow& c : g_camWindows) {
            if (c.window == raw) { entry = &c; break; }
        }
        if (!entry) {
            if (g_camWindows.size() >= kMaxCamWindows) continue;
            g_camWindows.push_back(CamWindow{raw, name, false, false});
            entry = &g_camWindows.back();
            w->AddPlugin(g_camPlugin);
        } else if (entry->name != name) {
            entry->name = name;   // the same window, renamed after a switch
        }

        // A window with no camera system is a panel or 2D view: there is no
        // picture of the world to fly, and nothing to switch.
        ICameraSystemV400* cam = w->GetCameraSystem();
        if (!cam) continue;

        if (g_camLog && !entry->logged) {
            entry->logged = true;
            WCHAR before[260] = L"";
            bool  saw610 = false;
            bool  saw451 = false;
            int   origin = -1;
            ICameraSystemV610* c610 = nullptr;
            if (SUCCEEDED(cam->QueryInterface(IID_ICameraSystemV610,
                                              reinterpret_cast<void**>(&c610))) &&
                c610) {
                saw610 = true;
                c610->GetCameraDefinition(before);
                c610->Release();
            }
            ICameraSystemV451* c451 = nullptr;
            if (SUCCEEDED(cam->QueryInterface(IID_ICameraSystemV451,
                                              reinterpret_cast<void**>(&c451))) &&
                c451) {
                saw451 = true;
                origin = static_cast<int>(c451->GetCameraOrigin());
                c451->Release();
            }
            wchar_t line[512] = L"";
            swprintf_s(line, L"win '%s' cam=1 v610=%d v451=%d def='%s' origin=%d",
                       name, saw610 ? 1 : 0, saw451 ? 1 : 0, before, origin);
            camLog(line);
        }

        bool onTarget = entry->switched;
        ICameraSystemV610* cam610 = nullptr;
        if (SUCCEEDED(cam->QueryInterface(IID_ICameraSystemV610,
                                          reinterpret_cast<void**>(&cam610))) &&
            cam610) {
            WCHAR def[260] = L"";
            cam610->GetCameraDefinition(def);
            onTarget = (_wcsicmp(def, g_cameraView) == 0);
            cam610->Release();
        }

        if (!onTarget) {
            w->SetCameraDefinition(g_cameraView);
            entry->switched = true;
            if (g_camLog) {
                WCHAR after[260] = L"";
                ICameraSystemV400* cam2 = w->GetCameraSystem();
                ICameraSystemV610* c610 = nullptr;
                if (cam2 &&
                    SUCCEEDED(cam2->QueryInterface(
                        IID_ICameraSystemV610, reinterpret_cast<void**>(&c610))) &&
                    c610) {
                    c610->GetCameraDefinition(after);
                    c610->Release();
                }
                wchar_t line[512] = L"";
                swprintf_s(line, L"switch '%s' -> '%s', reads back '%s'",
                           name, g_cameraView, after);
                camLog(line);
            }
        }
    }
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
    serviceCamera();
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
        if (!frameLive()) {
            serviceTick(true);
            serviceCamera();
        }
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
        s.physicsFree = false;
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
        // The physics-free object: a container whose sim.cfg carries this GUID as
        // [General] CategoryId is routed to CigiEntity, and every entity object
        // is created as one when the configured title is that container.  A stock
        // title simply never matches the GUID and goes on to the sim's own
        // implementation exactly as before, so this registration costs the stock
        // path nothing.  Registered before the callbacks so a container loaded
        // with the config has somewhere to go.
        const HRESULT catHr = g_objects->RegisterSimulationCategory(
            CigiEntity::CategoryId(), L"CigiStarter Entity", &CigiEntity::New);
        if (FAILED(catHr)) publish(catHr);

        g_objects->RegisterOnObjectCreateCallback(onObjectCreate);
        g_objects->RegisterOnObjectRemoveCallback(onObjectRemove);
    }

    // The camera.  Queried only when it is configured, so a deployment that does
    // not want the sim's view touched never even holds the service.
    if (g_cameraOn) {
        if (FAILED(pPdk->QueryService(SID_WindowPluginSystem,
                                      IID_IWindowPluginSystemV440,
                                      reinterpret_cast<void**>(&g_windows))) ||
            !g_windows) {
            g_windows = nullptr;
            publish(E_NOINTERFACE);
            camLog(L"camera: QueryService(SID_WindowPluginSystem) FAILED");
        } else {
            g_camPlugin = new CameraPlugin();
            camLog(L"camera: window plugin system obtained");
            // Windows that exist already get the plugin now; the ones a ViewGroup
            // opens later are picked up by serviceCamera() on the frame event.
            serviceCamera();
        }
    } else {
        camLog(L"camera: off");
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

    // The camera next: the plugin is attached to the sim's own windows, so it
    // has to be taken off every one of them before it is let go.
    if (g_windows) {
        if (g_camPlugin) {
            P3D::WindowList windows;
            if (SUCCEEDED(g_windows->GetWindowList(windows))) {
                for (CComPtr<IWindowV400>& w : windows.Items) {
                    if (w) w->RemovePlugin(g_camPlugin);
                }
            }
        }
        g_windows = nullptr;
    }
    if (g_camPlugin) { g_camPlugin->Release(); g_camPlugin = nullptr; }
    g_camWindows.clear();

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
