// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// WindowRenderingPlugin.h

#include "IWindowPluginSystem.h"
#include "IRenderingPluginSystem.h"
#include "IUnknownHelper.h"


namespace P3D
{
    class WindowRenderingPlugin : public IWindowPluginV400, public IRenderingPluginV400
    {
    public:
        WindowRenderingPlugin() : m_RefCount(1)
        {

        }

        virtual ~WindowRenderingPlugin()
        {

        }

        // Window Plugin Interface
        virtual void OnAdd(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}
        virtual void OnRemove(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}
        virtual void OnPreCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnPostCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnViewChange(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnClose(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual bool OnUserInput(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override { return false; };
        
        // Rendering plugin interface
        virtual void PreRender(IRenderDataV400* pRenderData)
        {
            // only want Render() called during the default render pass
            mRenderFlags.RenderingIsEnabled = (pRenderData->GetRenderPass() == RenderPassDefault);
            pRenderData->SetRenderFlags(mRenderFlags);
        }

        virtual void Render(IRenderDataV400* pRenderData) abstract;
        virtual bool HasUpdate() { return true; }
        virtual bool RequiredDoubleBuffer() { return false; }
        virtual bool IsPreVc() { return false; }
        virtual void OnAdd(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}
        virtual void OnRemove(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}

    protected:
        PdkRenderFlags mRenderFlags;
        DEFAULT_REFCOUNT_INLINE_IMPL();

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == NULL)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_IWindowPluginV400))
            {
                *ppv = static_cast<IWindowPluginV400*>(this);
            }
            else if (IsEqualIID(riid, IID_IUnknown))
            {
                *ppv = static_cast<IUnknown*>(this);
            }
            if (*ppv)
            {
                hr = S_OK;
                AddRef();
            }

            return hr;
        };
}