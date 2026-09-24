// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// RenderingPlugin.h

#pragma once

#include "IRenderingPluginSystem.h"
#include "IUnknownHelper.h"
namespace P3D
{
    /** @addtogroup rendering */ /** @{ */

   /**
    * Base implementation of the IRenderingPluginV500 which defines
    * default implementations of all IRenderingPluginV500 functions and
    * a default IUnknown implementation.
    */
    class RenderingPlugin : public IRenderingPluginV500
    {
    public:
        RenderingPlugin() noexcept : m_RefCount(1) {  }

        virtual ~RenderingPlugin() {  }

        /**
        *  Enables rendering if the current render pass is RenderPassDefault
        */ 
        virtual void PreRender(IRenderDataV500* pRenderData)
        {
            // only want Render() called during the default render pass
            mRenderFlags.RenderingIsEnabled = (pRenderData->GetRenderPass() == RenderPassDefault);
            pRenderData->SetRenderFlags(mRenderFlags);
        }

        virtual void Render(IRenderDataV500* pRenderData) abstract;


        DEFAULT_REFCOUNT_INLINE_IMPL();
        
        STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
        {
            HRESULT hr = E_NOINTERFACE;

            if (ppv == NULL)
            {
                return E_POINTER;
            }

            *ppv = NULL;

            if (IsEqualIID(riid, IID_IRenderingPluginV500))
            {
                *ppv = static_cast<IRenderingPluginV500*>(this);
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

    protected:
        PdkRenderFlags mRenderFlags;
    };
    /** @} */ // end of group
}