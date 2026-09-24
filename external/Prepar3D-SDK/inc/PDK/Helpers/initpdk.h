// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// initpdk.h

#pragma once
#include <initguid.h>
#include <cguid.h>
#include <atlcomcli.h>
#include "PdkServices.h"

namespace P3D
{
    P3D::PdkServices* P3D::PdkServices::m_pServices = nullptr;

    void PdkServices::Init(IPdk* pPdk)
    {
        m_pServices = new PdkServices(pPdk);
    }

    void PdkServices::Shutdown()
    {
        if (m_pServices)
        {
            delete m_pServices;
            m_pServices = nullptr;
        }
    }

    IPdk* PdkServices::GetPdk() 
    { 
        if (m_pServices)
        {
            return m_pServices->m_spPdk;
        }
        return nullptr;
    }

    IEventServiceV600* PdkServices::GetEventService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spEventService;
        }
        return nullptr;
    }

    IVisualEffectManagerV530* PdkServices::GetVisualEffectManager()
    {
        if (m_pServices)
        {
            return m_pServices->m_spVisualEffectManager; 
        }
        return nullptr;
    }

    IDataLoadHelperV400* PdkServices::GetDataLoadHelper()
    {
        if (m_pServices)
        {
            return m_pServices->m_spDataLoadHelper; 
        }
        return nullptr;
    }

    IRenderingPluginSystemV510* PdkServices::GetRenderingPluginSystem()
    {
        if (m_pServices)
        {
            return m_pServices->m_spRenderingPluginSystem;
        }
        return nullptr;
    }
    
    IWindowPluginSystemV440* PdkServices::GetWindowPluginSystem()
    {
        if (m_pServices)
        {
            return m_pServices->m_spWindowPluginSystem;
        }
        return nullptr;
    }

    IGlobalDataV430* PdkServices::GetGlobalData()
    {
        if (m_pServices)
        {
            return m_pServices->m_spGlobalData;
        }
        return nullptr;
    }

    IObjectRendererV600* PdkServices::GetObjectRenderer()
    {
        if (m_pServices)
        {
            return m_pServices->m_spObjectRenderer;
        }
        return nullptr;
    }

    IWeatherSystemV500* PdkServices::GetWeatherSystem()
    {
        if (m_pServices)
        {
            return m_pServices->m_spWeatherSystem;
        }
        return nullptr;
    }

    ISimObjectManagerV540* PdkServices::GetSimObjectManager()
    {
        if (m_pServices)
        {
            return m_pServices->m_spSimObjectManager;
        }
        return nullptr;
    } 

    IReportingServiceV400* PdkServices::GetReportingService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spReportingService;
        }
        return nullptr;
    }

    IPanelSystemV520* PdkServices::GetPanelSystem()
    {
        if (m_pServices)
        {
            return m_pServices->m_spPanelSystem;
        }
        return nullptr;
    }

    IIconServiceV410* PdkServices::GetIconService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spIconService;
        }
        return nullptr;
    }

    IMenuServiceV410* PdkServices::GetMenuService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spMenuService;
        }
        return nullptr;
    }

    IMultiplayerServiceV540* PdkServices::GetMultiplayerService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spMultiplayerService;
        }
        return nullptr;
    }

    IMultichannelServiceV440 * PdkServices::GetMultichannelService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spMultichannelService;
        }
        return nullptr;
    }

    ICigiServiceV430* PdkServices::GetCigiService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spCigiService;
        }
        return nullptr;
    }

    IConfigurationServiceV440* PdkServices::GetConfigurationService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spConfigurationService;
        }
        return nullptr;
    }

    ISimPropertyServiceV510* PdkServices::GetSimPropertyService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spSimPropertyService;
        }
        return nullptr;
    }

    IControllableCameraV450* PdkServices::GetControllableCamera()
    {
        if (m_pServices)
        {
            return m_pServices->m_spControllableCamera;
        }
        return nullptr;
    }

    IVRServiceV600* PdkServices::GetVRService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spVRService;
        }
        return nullptr;
    }

    IWorldObjectServiceV510* PdkServices::GetWorldObjectService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spWorldObjectService;
        }
        return nullptr;
    }

    IScenarioManagerV453* PdkServices::GetScenarioManagerService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spScenarioManagerService;
        }
        return nullptr;
    }

    IRecordingServiceV510* PdkServices::GetRecordingService()
    {
        if (m_pServices)
        {
            return m_pServices->m_spRecordingService;
        }
        return nullptr;
    }
}
