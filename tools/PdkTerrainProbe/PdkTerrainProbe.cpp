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
// It runs in-process (loaded from an add-on.xml as a PDK DLL) and is driven by
// the sim's own update loop, so no extra thread ever touches the terrain
// engine.  The shape below - DLLStart/DLLStop plus an ISimulation registered on
// an object - is the same one the SDK's own RadarAltimeter sample uses.
//
// See TerrainProbe.h for the shared-memory contract, and README.md in this
// folder for how it is installed on the IG PC.
//-----------------------------------------------------------------------------

#include <windows.h>

#include <Pdk.h>
#include <ISimObject.h>

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
// Prepar3D calls Update() on the sim's thread at the rate registered below.
// That is the only place the terrain engine is touched: the readers on the
// other side of the mapping never call into Prepar3D at all.
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

        ++g_shm->ticks;
        g_shm->alive = (g_surface != nullptr) ? 1 : 0;

        if (!g_surface || g_shm->reqSeq == 0) return S_OK;

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
        sim->Release();
        return;
    }

    obj->AddRef();
    g_host = obj;
    g_sim  = sim;
    publish(0);

    // Record the rate we asked the sim for, so the reader can tell a probe that
    // is merely running from one the sim is actually ticking.
    if (g_shm) g_shm->tickRateHz = static_cast<uint32_t>(kTickHz);
}

void detach() {
    if (g_host) { g_host->Release(); g_host = nullptr; }
    if (g_sim)  { g_sim->Release();  g_sim  = nullptr; }
    if (g_shm)  { g_shm->alive = 0; }
}

HRESULT STDMETHODCALLTYPE onObjectCreate(__in IUnknown& obj) {
    if (g_sim) return S_OK;

    IBaseObjectV400* base = nullptr;
    if (FAILED(obj.QueryInterface(IID_IBaseObjectV400, reinterpret_cast<void**>(&base))) || !base) {
        return S_OK;
    }

    if (base->IsUser()) attachTo(base);
    base->Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE onObjectRemove(__in IUnknown& obj) {
    if (!g_host) return S_OK;

    IBaseObjectV400* base = nullptr;
    if (SUCCEEDED(obj.QueryInterface(IID_IBaseObjectV400, reinterpret_cast<void**>(&base))) && base) {
        if (static_cast<IUnknown*>(base) == static_cast<IUnknown*>(g_host)) detach();
        base->Release();
    }
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
    if (g_objects) {
        g_objects->UnRegisterOnObjectCreateCallback(onObjectCreate);
        g_objects->UnRegisterOnObjectRemoveCallback(onObjectRemove);
        g_objects = nullptr;
    }
    detach();
    closeShared();
    g_surface = nullptr;
}
