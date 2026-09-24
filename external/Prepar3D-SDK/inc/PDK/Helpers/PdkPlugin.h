// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// PdkPlugin.h

#pragma once
#include "PdkServices.h"
namespace P3D
{
    /** @addtogroup pdk */ /** @{ */

    /**
     * Base plugin implementation.  This class registers for common Prepar3D events and messages,
     * and provides easy access via a set of virtual functions.
     */
    class PdkPlugin
    {
    public:
        PdkPlugin() noexcept
        {
            RegisterEvents(true,true,true,true);
        }

        PdkPlugin(bool bOneHz, bool bFrame, bool bCustomRender, bool bMessage) noexcept
        {
            RegisterEvents(bOneHz, bFrame, bCustomRender, bMessage);
        }

        virtual ~PdkPlugin()
        {
            UnregisterEvents();
        }
        /** 
         *  Called once per second  
         */
        virtual void OnOneHz(IParameterListV400* pParams) {}
        /** 
         *  Called once per frame  
         */
        virtual void OnFrame(IParameterListV400* pParams) {}
        /** 
         *  Called when loading of a scenario is complete  
         */
        virtual void OnLoadComplete(IParameterListV400* pParams) {}
        /** 
         *  Called after all dll-based plugins have loaded.  This is a 
         *  good place to query for pdk services registered by other plugins.
         */
        virtual void OnPluginsLoaded(IParameterListV400* pParams) {}
        /** 
         *  Called whenever a window changes views or sets a new camera definition
         */
        virtual void OnViewChanged(IParameterListV400* pParams) {}
        /**
        *  Called whenever a view is created
        */
        virtual void OnViewCreated(IParameterListV400* pParams) {}
        /**
        *  Called whenever a view is destroyed
        */
        virtual void OnViewDestroyed(IParameterListV400* pParams) {}
        /**
        *  Called whenever the user vehicle is changed
        */
        virtual void OnVehicleChanged(IParameterListV400* pParams) {}
        /** 
         *  Called every frame for each 3d view.  At this point, Prepar3D is still building
         *  the 3d scene, and it is safe to add new objects using the IObjectRenderer service
         */
        virtual void OnCustomRender(IParameterListV400* pParams) {}
        /** 
         *  Called when application shutdown has been requested.  No pdk services or
         *  plugins have shut down yet, so it should be safe to use all services to
         *  unregister for callbacks and clean up local resources.
         */
        virtual void OnApplicationShutdown(IParameterListV400* pParams) {}
        /** 
         *  Called when a message event is fired.  This function calls 
         *  OnLoadComplete, OnPluginsLoaded, OnApplicationShutdown, 
         *  OnViewChanged, OnViewCreated, OnViewDestroyed, and OnVehicleChanged.
         *  Any overrides of this function should call back
         *  to the base to ensure the callbacks continue to function.
         */
        virtual void OnMessage(IParameterListV400* pParams)
        {
            UINT64 messageID = pParams->GetParameter(0).Value;
            switch (messageID)
            {
            case EVENT_MESSAGE_LOADING_COMPLETE:
                OnLoadComplete(pParams);
                break;
            case EVENT_MESSAGE_APPLICATION_DLLS_LOADED:
                OnPluginsLoaded(pParams);
                break;
            case EVENT_MESSAGE_APPLICATION_SHUTDOWN:
                OnApplicationShutdown(pParams);
                break;
            case EVENT_MESSAGE_VIEW_MODE_CHANGED:
                OnViewChanged(pParams);
                break;
            case EVENT_MESSAGE_VIEW_CREATE:
                OnViewCreated(pParams);
                break;
            case EVENT_MESSAGE_VIEW_DESTROY:
                OnViewDestroyed(pParams);
                break;
            case EVENT_MESSAGE_CHANGE_USER_VEHICLE_FINISH:
                OnVehicleChanged(pParams);
                break;
            }
        }
        /** 
         *  Register event callbacks for this plugin.
         */
        virtual void RegisterEvents(bool bOneHz = true, bool bFrame = true, bool bCustomRender = true, bool bMessage = true)
        {
            //Get the Prepar3D Event Service from the PDK
            CComPtr<IEventServiceV400> spEventService = PdkServices::GetEventService();
            if (spEventService != nullptr) 
            {
                // Register event callbacks
                if (bOneHz)
                {
                    m_spOneHzCallback.Attach(new OneHzCallback(this));
                    m_spOneHzCallback->Register(spEventService);
                }
                if (bFrame)
                {
                    m_spFrameCallback.Attach(new FrameCallback(this));
                    m_spFrameCallback->Register(spEventService);
                }
                if (bCustomRender)
                {
                    m_spCustomRenderCallback.Attach(new CustomRenderCallback(this));
                    m_spCustomRenderCallback->Register(spEventService);
                }
                if (bMessage)
                {
                    m_spMessageCallback.Attach(new MessageCallback(this));
                    m_spMessageCallback->Register(spEventService);
                }
            }
        }
        /** 
         *  Unegister event callbacks for this plugin.
         */
        virtual void UnregisterEvents()
        {
            //Get the Prepar3D Event Service from the PDK
            CComPtr<IEventServiceV400> spEventService = PdkServices::GetEventService();
            if (spEventService != nullptr)
            {
                // Unregister event callbacks
                if (m_spCustomRenderCallback != nullptr)
                {
                    m_spCustomRenderCallback->Unregister(spEventService);
                }
                if (m_spOneHzCallback != nullptr)
                {
                    m_spOneHzCallback->Unregister(spEventService);
                }
                if (m_spMessageCallback != nullptr)
                {
                    m_spMessageCallback->Unregister(spEventService);
                }
                if (m_spFrameCallback != nullptr)
                {
                    m_spFrameCallback->Unregister(spEventService);
                }
            }
        }

    protected:

        /** 
         *  Helper class that defines a callback for a specific event ID
         */
        class P3dPluginCallback : public ICallbackV400
        {
        public:
            P3dPluginCallback(PdkPlugin* pPlugin, const GUID& eventID)
                : m_RefCount(1), m_pPlugin(pPlugin), m_EventID(eventID)
            {

            }
            /** 
             *  Register this callback
             */
            void Register(IEventServiceV400* pEventService)
            {
                pEventService->RegisterCallback(m_EventID, this);
            }
            /** 
             *  Unregister this callback
             */
            void Unregister(IEventServiceV400* pEventService)
            {
                pEventService->UnregisterCallback(m_EventID, this);
            }

            virtual void Invoke(IParameterListV400* pParams) abstract;

            const GUID& m_EventID;
            PdkPlugin* m_pPlugin;

            DEFAULT_REFCOUNT_INLINE_IMPL();

            STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
            {
                HRESULT hr = E_NOINTERFACE;

                if (ppv == NULL)
                {
                    return E_POINTER;
                }

                *ppv = NULL;

                if (IsEqualIID(riid, IID_ICallbackV400))
                {
                    *ppv = static_cast<ICallbackV400*>(this);
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
       /** 
         *  Callback class that calls OnOneHz
         */
        class OneHzCallback : public P3dPluginCallback
        {
        public:
            OneHzCallback(PdkPlugin* pPlugin) : P3dPluginCallback(pPlugin, EVENTID_1Hz) {}

            virtual void Invoke(IParameterListV400* pParams) override
            {
                m_pPlugin->OnOneHz(pParams);
            }
        };
       /** 
         *  Callback class that calls OnFrame
         */
        class FrameCallback : public P3dPluginCallback
        {
        public:
            FrameCallback(PdkPlugin* pPlugin) : P3dPluginCallback(pPlugin, EVENTID_Frame) {}

            virtual void Invoke(IParameterListV400* pParams) override
            {
                m_pPlugin->OnFrame(pParams);
            }
        };
       /** 
         *  Callback class that calls OnCustomRender
         */
        class CustomRenderCallback : public P3dPluginCallback
        {
        public:
            CustomRenderCallback(PdkPlugin* pPlugin) : P3dPluginCallback(pPlugin, EVENTID_CustomObjectDraw) {}

            virtual void Invoke(IParameterListV400* pParams) override
            {
                m_pPlugin->OnCustomRender(pParams);
            }
        };
       /** 
         *  Callback class that calls OnMessage
         */
        class MessageCallback : public P3dPluginCallback
        {
        public:
            MessageCallback(PdkPlugin* pPlugin) : P3dPluginCallback(pPlugin, EVENTID_Message) {}

            virtual void Invoke(IParameterListV400* pParams) override
            {
                m_pPlugin->OnMessage(pParams);
            }
        };

        CComPtr<P3dPluginCallback>   m_spCustomRenderCallback;
        CComPtr<P3dPluginCallback>   m_spOneHzCallback;
        CComPtr<P3dPluginCallback>   m_spMessageCallback;
        CComPtr<P3dPluginCallback>   m_spFrameCallback;
    };
    /** @} */
}
