// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// InterfaceServiceWrapper.h

#include <ObjBase.h>
#include <atlcomcli.h>
#include <IUnknownHelper.h>
#pragma once
namespace P3D
{
    /** Small service provider wrapper that provides access to an IUnknown object by
     *  returning it as service when queried using an interface.
     */ 
    class InterfaceServiceWrapper : public IServiceProvider
    {
    public:
        InterfaceServiceWrapper(IUnknown* pInterface) : m_RefCount(1), m_pInterface(pInterface)
        {
        }

        HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
        {
            HRESULT hr = E_NOINTERFACE;
            if (m_pInterface != nullptr)
            {
                hr = m_pInterface->QueryInterface(riid, ppvObject);
            }
            return hr;
        }

        CComPtr<IUnknown> m_pInterface;
        DEFAULT_REFCOUNT_INLINE_IMPL();

        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == nullptr)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_IServiceProvider))
            {
                *ppv = static_cast<IServiceProvider*>(this);
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
    };
}