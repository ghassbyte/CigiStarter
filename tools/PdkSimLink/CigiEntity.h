//-----------------------------------------------------------------------------
// CigiEntity.h
//
// The physics-free sim object PdkSimLink creates for the HOST's entities.
//
// Why it exists
// -------------
// A container title created with ISimObjectManager::CreateObject() is the sim's
// own: whatever the container describes runs between our SetPosition calls. For
// an aircraft that is a flight model, and two things follow from it that the
// writer cannot live with (both measured on the bench, see README.md):
//
//   * a pose far below the ground under it is refused, and the object is held
//     above it (the hold altitude is a property of the place, not the terrain
//     surface, and a command only a little below the ground is followed);
//   * it wants the sim paused or slewed to hold a pose, and a paused sim freezes
//     the rotor (see TODO.md item 4).
//
// A custom ISimObject is the other way: Prepar3D hands the object's behaviour to
// this DLL, and this implementation registers *no* ISimulation, so nothing moves
// the object between writes.  The pose SetPosition is given is the pose that
// stays, and a command below the ground the sim would have refused is simply
// where the object goes.  The same registration also removes the need to pause:
// the sim can run at its own rate while the HOST owns the pose.
//
// How a container reaches this class
// ----------------------------------
// A container is routed here by the CategoryId in its sim.cfg's [General]
// section -- the GUID below must match it exactly, and the add-on registers the
// same GUID with RegisterSimulationCategory() at load.  Ship the two out of step
// and every CreateObject() fails (the e2e test catches that as create failures).
// See SimObjects\CigiEntity\sim.cfg next to this file.
//
// See ../../SimLinkIpc.h for the shared-memory contract and README.md for how it
// is installed.
//-----------------------------------------------------------------------------

#pragma once

#include <windows.h>

#include <ISimObject.h>

// A physics-free ISimObject: it owns a base object and nothing else.
//
// ISimObjectV400 is the interface the SDK's own samples implement, and the one
// Prepar3D will pick up.  Only the methods Prepar3D actually calls are overridden
// to do anything; the rest are deliberately empty, because a no-op is exactly
// what a physics-free object wants.
class __declspec(uuid("{7B1E4A2C-9F03-4D5B-8E6A-1C2D3E4F5A6B}")) CigiEntity final
    : public P3D::ISimObjectV400
{
public:
    CigiEntity(const CigiEntity&) = delete;
    CigiEntity& operator=(const CigiEntity&) = delete;

    // The category this class registers, and the GUID a container's
    // [General] CategoryId must carry to be routed to it.  One source for both,
    // so the code and the content cannot drift apart silently.
    static REFGUID CategoryId() { return __uuidof(CigiEntity); }

    // PSimCreateFunc: called by Prepar3D when a container with our CategoryId is
    // created.  Deliberately does *not* call IBaseObject::RegisterSimulation():
    // registering nothing is what makes the object physics-free.
    static HRESULT STDMETHODCALLTYPE New(__in P3D::IBaseObjectV400* pBaseObject,
                                         __out P3D::ISimObject** ppThisSim);

    ULONG STDMETHODCALLTYPE AddRef() override { return ::InterlockedIncrement(&m_ref); }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG n = ::InterlockedDecrement(&m_ref);
        if (n == 0) delete this;
        return n;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid,
                                           void** ppv) override;

    // ISimObject.  Nothing to load: the object *is* the pose the HOST commands.
    HRESULT STDMETHODCALLTYPE LoadConstantData(void** /*ppConstantData*/) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE UnloadConstantData(void** /*ppConstantData*/) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE LoadDynamicData() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE Init() override { return S_OK; }
    HRESULT STDMETHODCALLTYPE DeInit() override { return S_OK; }
    STDMETHOD_(BOOL, SupportsLabels)() const override { return m_labels; }
    STDMETHOD_(void, OnModeChange)(int /*bfNewModes*/) override {}
    // Prepar3D has placed us outside our own simulation (the UI, slew, a terrain
    // change).  There is no simulation to resync, so this is deliberately empty.
    STDMETHOD_(void, OnPositionInit)() override {}
    STDMETHOD_(void, OnSpeedInit)(float /*fSpeed*/) override {}

    STDMETHOD(SetSupportsLabels)(BOOL bOn) override {
        m_labels = (bOn != FALSE);
        return S_OK;
    }

    STDMETHOD(QueryBaseObject)(REFIID riid, void** ppv) override;

    // The world position update rate Prepar3D asks for.  The HOST commands the
    // pose; this is only the rate the base object is asked to refresh at, and is
    // the usual 60 Hz.
    STDMETHOD(GetMainSimRate)(__out float& fSimRate) const override {
        fSimRate = 60.0f;
        return S_OK;
    }

private:
    // Held as a reference, exactly as the SDK's samples do: the base object's
    // lifetime belongs to Prepar3D, not to us.
    explicit CigiEntity(P3D::IBaseObjectV400& baseObject) : m_baseObject(baseObject) {}
    ~CigiEntity() = default;

    volatile LONG          m_ref = 1;
    BOOL                   m_labels = TRUE;
    P3D::IBaseObjectV400&  m_baseObject;
};

inline HRESULT STDMETHODCALLTYPE CigiEntity::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;

    if (::IsEqualIID(riid, IID_IUnknown) ||
        ::IsEqualIID(riid, IID_IServiceProvider)) {
        *ppv = static_cast<IServiceProvider*>(this);
    } else if (::IsEqualIID(riid, P3D::IID_ISimObject)) {
        *ppv = static_cast<P3D::ISimObject*>(this);
    } else if (::IsEqualIID(riid, P3D::IID_ISimObjectV01)) {
        *ppv = static_cast<P3D::ISimObjectV01*>(this);
    } else if (::IsEqualIID(riid, P3D::IID_ISimObjectV02)) {
        *ppv = static_cast<P3D::ISimObjectV02*>(this);
    } else if (::IsEqualIID(riid, P3D::IID_ISimObjectV03)) {
        *ppv = static_cast<P3D::ISimObjectV03*>(this);
    } else if (::IsEqualIID(riid, P3D::IID_ISimObjectV400)) {
        *ppv = static_cast<P3D::ISimObjectV400*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

inline HRESULT STDMETHODCALLTYPE CigiEntity::QueryService(REFGUID guidService,
                                                          REFIID riid, void** ppv) {
    if (::IsEqualIID(guidService, P3D::SID_SimObject)) {
        return QueryInterface(riid, ppv);
    }
    if (ppv) *ppv = nullptr;
    return E_NOINTERFACE;
}

inline HRESULT STDMETHODCALLTYPE CigiEntity::QueryBaseObject(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    *ppv = nullptr;
    return m_baseObject.QueryInterface(riid, ppv);
}

inline HRESULT STDMETHODCALLTYPE CigiEntity::New(P3D::IBaseObjectV400* pBaseObject,
                                                 P3D::ISimObject** ppThisSim) {
    if (!ppThisSim) return E_POINTER;
    *ppThisSim = nullptr;
    if (!pBaseObject) return E_POINTER;

    *ppThisSim = new CigiEntity(*pBaseObject);
    if (!*ppThisSim) return E_OUTOFMEMORY;
    return S_OK;
}
