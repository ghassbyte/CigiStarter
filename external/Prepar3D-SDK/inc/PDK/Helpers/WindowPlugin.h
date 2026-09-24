// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// WindowPlugin.h

#pragma once 

#include "IWindowPluginSystem.h"
#include "IUnknownHelper.h"

namespace P3D
{
    /** @addtogroup windows */ /** @{ */

    /**
    * Base implementation of the IWindowPluginV400 which defines
    * default implementations of all IWindowPluginV400 functions and
    * a default IUnknown implementation.
    */
    class WindowPlugin : public IWindowPluginV400
    {
    public:
        WindowPlugin() noexcept : m_RefCount(1)
        {

        }

        virtual ~WindowPlugin()
        {

        }

        virtual void OnAdd(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}
        virtual void OnRemove(IWindowV400* pWindow, ICameraSystemV400* pCamera) override {}
        virtual void OnPreCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnPostCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnViewChange(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual void OnClose(IWindowV400* pWindow, ICameraSystemV400* pCamera)  override {}
        virtual bool OnUserInput(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override { return false; };

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
        }
    };
    /** @} */
}