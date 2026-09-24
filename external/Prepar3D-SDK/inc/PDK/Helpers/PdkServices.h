// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// PdkServices.h

#pragma once
#include "IEventService.h"      //From Prepar3D SDK
#include "Pdk.h"                //From Prepar3D SDK
#include "IWindowPluginSystem.h"
#include "IUnknownHelper.h"

#include "IRenderingService.h"  //From Prepar3D SDK
#include "IVisualEffects.h"
#include "IDataLoader.h"
#include "IRenderingPluginSystem.h"
#include "IGlobalData.h"
#include "IWeatherSystem.h"
#include "ISimObject.h"
#include "IReportingService.h"
#include "IPanelSystem.h"
#include "IIconService.h"
#include "IMenuService.h"
#include "INetworkServices.h"
#include "IConfigurationService.h"
#include "ISimPropertyService.h"
#include "IControllableCamera.h"
#include "IVRService.h"
#include "IWorldObjectService.h"
#include "IRecordingService.h"

namespace P3D
{
    /** @addtogroup pdk */ /** @{ */

    /**
     * Provides static access to all of Prepar3D's core PDK services.
     * @remark  To use this in a plugin:
     * - Include initpdk.h from the cpp file where dll start and stop functions are defined
     * - Call Init() in the dll start function
     * - Call Shutdown() from the dll stop function  
     * ```
     *      #include "initpdk.h"
     *     
     *      void DLLStart(__in __notnull IPdk* pPdk)
     *      {
     *          PdkServices::Init(pPdk);
     *          // init code for your plugin
     *      }
     *      
     *      void DLLStop(void)
     *      {
     *          // deinit code for your plugin
     *          PdkServices::Shutdown();
     *      }
     * ```
     */
    class PdkServices
    {
    public:
        PdkServices(IPdk* pPdk) : m_spPdk(pPdk)
        {
            m_spPdk->QueryService(SID_EventService, IID_IEventServiceV600, (void**)&m_spEventService);
            m_spPdk->QueryService(SID_VisualEffectManager, IID_IVisualEffectManagerV530, (void**)&m_spVisualEffectManager);
            m_spPdk->QueryService(SID_DataLoadHelper, IID_IDataLoadHelperV400, (void**)&m_spDataLoadHelper);
            m_spPdk->QueryService(SID_GlobalData, IID_IGlobalDataV430, (void**)&m_spGlobalData);

            m_spPdk->QueryService(SID_ObjectRenderer, IID_IObjectRendererV600, (void**)&m_spObjectRenderer);
            m_spPdk->QueryService(SID_WeatherSystem, IID_IWeatherSystemV400, (void**)&m_spWeatherSystem);
            m_spPdk->QueryService(SID_SimObjectManager, IID_ISimObjectManagerV540, (void**)&m_spSimObjectManager);
            m_spPdk->QueryService(SID_WindowPluginSystem, IID_IWindowPluginSystemV440, (void**)&m_spWindowPluginSystem);
            m_spPdk->QueryService(SID_RenderingPluginSystem, IID_IRenderingPluginSystemV510, (void**)&m_spRenderingPluginSystem);
            m_spPdk->QueryService(SID_ReportingService, IID_IReportingServiceV400, (void**)&m_spReportingService);
            m_spPdk->QueryService(SID_PanelSystem, IID_IPanelSystemV520, (void**)&m_spPanelSystem);
            m_spPdk->QueryService(SID_IconService, IID_IIconServiceV410, (void**)&m_spIconService);
            m_spPdk->QueryService(SID_MenuService, IID_IMenuServiceV410, (void**)&m_spMenuService);
            m_spPdk->QueryService(SID_MultiplayerService, IID_IMultiplayerServiceV540, (void**)&m_spMultiplayerService);
            m_spPdk->QueryService(SID_MultichannelService, IID_IMultichannelServiceV440, (void**)&m_spMultichannelService);
            m_spPdk->QueryService(SID_CigiService, IID_ICigiServiceV430, (void**)&m_spCigiService);
            m_spPdk->QueryService(SID_ConfigurationService, IID_IConfigurationServiceV440, (void**)&m_spConfigurationService);
            m_spPdk->QueryService(SID_SimPropertyService, IID_ISimPropertyServiceV510, (void**)&m_spSimPropertyService);
            m_spPdk->QueryService(SID_ControllableCamera, IID_IControllableCameraV450, (void**)&m_spControllableCamera);
            m_spPdk->QueryService(SID_VRService, IID_IVRServiceV600, (void**)&m_spVRService);
            m_spPdk->QueryService(SID_WorldObjectService, IID_IWorldObjectServiceV510, (void**)&m_spWorldObjectService);
            m_spPdk->QueryService(SID_ScenarioManager, IID_IScenarioManagerV453, (void**)&m_spScenarioManagerService);
            m_spPdk->QueryService(SID_RecordingService, IID_IRecordingServiceV510, (void**)&m_spRecordingService);
        }
        /** 
         * Initialize static instance of PdkServices.  This should be called from the dll start function.
         */
        static void Init(IPdk* pPdk);

        /** 
         * Shutdown static instance of PdkServices.  This should be called from the dll stop function.
         */
        static void Shutdown();

        ~PdkServices()
        {
            m_spPdk = nullptr;
            m_spDataLoadHelper = nullptr;
            m_spGlobalData = nullptr;
            m_spEventService = nullptr;
            m_spVisualEffectManager = nullptr;         
            m_spRenderingPluginSystem = nullptr;
            m_spWindowPluginSystem = nullptr;     
            m_spObjectRenderer = nullptr;
            m_spWeatherSystem = nullptr;
            m_spSimObjectManager = nullptr;
            m_spReportingService = nullptr;
            m_spPanelSystem = nullptr;
            m_spIconService = nullptr;
            m_spMenuService = nullptr;
            m_spMultiplayerService = nullptr;
            m_spCigiService = nullptr;
            m_spConfigurationService = nullptr;
            m_spSimPropertyService = nullptr;
            m_spControllableCamera = nullptr;
            m_spVRService = nullptr;
            m_spWorldObjectService = nullptr;
            m_spScenarioManagerService = nullptr;
            m_spRecordingService = nullptr;
        }
        /// Get Pdk
        static IPdk*                        GetPdk();
        /// Get Event Service
        static IEventServiceV600*           GetEventService();
        /// Get Visual Effect Manager
        static IVisualEffectManagerV530*    GetVisualEffectManager();
        /// Get Data Load Helper
        static IDataLoadHelperV400*         GetDataLoadHelper();
        /// Get Rendering Plugin System
        static IRenderingPluginSystemV510*  GetRenderingPluginSystem();
        /// Get Window Plugin System
        static IWindowPluginSystemV440*     GetWindowPluginSystem();
        /// Get Global Data
        static IGlobalDataV430*             GetGlobalData();
        /// Get Object Renderer
        static IObjectRendererV600*         GetObjectRenderer();
        /// Get Weather System
        static IWeatherSystemV500*          GetWeatherSystem();
        /// Get Simulation Object Manager
        static ISimObjectManagerV540*       GetSimObjectManager();
        /// Get Reporting Service
        static IReportingServiceV400*       GetReportingService();
        /// Get Panel System
        static IPanelSystemV520*            GetPanelSystem();
        /// Get Icon Service
        static IIconServiceV410*            GetIconService();
        /// Get Menu Service
        static IMenuServiceV410*            GetMenuService();
        /// Get Multiplayer Service
        static IMultiplayerServiceV540*     GetMultiplayerService();
        /// Get Multichannel Service
        static IMultichannelServiceV440*    GetMultichannelService();
        /// Get CIGI Service
        static ICigiServiceV430*            GetCigiService();
        /// Get Configuration Service
        static IConfigurationServiceV440*   GetConfigurationService();
        /// Get Sim Property Service
        static ISimPropertyServiceV510*     GetSimPropertyService();
        /// Get Controllable Camera
        static IControllableCameraV450*     GetControllableCamera();
        /// Get VR Service
        static IVRServiceV600*              GetVRService();
        /// Get World object Service
        static IWorldObjectServiceV510*     GetWorldObjectService();
        /// Get Scenario Manager Service
        static IScenarioManagerV453*        GetScenarioManagerService();
        /// Get Recording Service
        static IRecordingServiceV510*       GetRecordingService();



    protected:
        static PdkServices*                 m_pServices;
        CComPtr<IPdk>                       m_spPdk;
        CComPtr<IEventServiceV600>          m_spEventService;
        CComPtr<IVisualEffectManagerV530>   m_spVisualEffectManager;
        CComPtr<IDataLoadHelperV400>        m_spDataLoadHelper;
        CComPtr<IRenderingPluginSystemV510> m_spRenderingPluginSystem;
        CComPtr<IWindowPluginSystemV440>    m_spWindowPluginSystem;
        CComPtr<IGlobalDataV430>            m_spGlobalData;
        CComPtr<IObjectRendererV600>        m_spObjectRenderer;
        CComPtr<IWeatherSystemV500>         m_spWeatherSystem;
        CComPtr<ISimObjectManagerV540>      m_spSimObjectManager;
        CComPtr<IReportingServiceV400>      m_spReportingService;
        CComPtr<IPanelSystemV520>           m_spPanelSystem;
        CComPtr<IIconServiceV410>           m_spIconService;
        CComPtr<IMenuServiceV410>           m_spMenuService;
        CComPtr<IMultiplayerServiceV540>    m_spMultiplayerService;
        CComPtr<IMultichannelServiceV440>   m_spMultichannelService;
        CComPtr<ICigiServiceV430>           m_spCigiService;
        CComPtr<IConfigurationServiceV440>  m_spConfigurationService;
        CComPtr<ISimPropertyServiceV510>    m_spSimPropertyService;
        CComPtr<IControllableCameraV450>    m_spControllableCamera;
        CComPtr<IVRServiceV600>             m_spVRService;
        CComPtr<IWorldObjectServiceV510>    m_spWorldObjectService;
        CComPtr<IScenarioManagerV453>       m_spScenarioManagerService;
        CComPtr<IRecordingServiceV510>      m_spRecordingService;
    };
    /** @} */
}