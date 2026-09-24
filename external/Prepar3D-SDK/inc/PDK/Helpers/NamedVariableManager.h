// Copyright (c) 2010-2022 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// NamedVariableManager.h

#pragma once

#include "NamedVariable.h"
#include "PdkPlugin.h"
#include <set>
#include <string>

namespace P3D
{
    /** @addtogroup panelsystem */ /** @{ */

    class NamedVariableManager : PdkPlugin
    {
    public:

        static void Init();
        static void Shutdown();

        static void RegisterVariable(LPCSTR pszName)
        {
            if (m_pInstance == nullptr)
            {
                Init();
            }

            m_pInstance->RegisterVariablePrivate(pszName);
        }

        NamedVariableManager() : PdkPlugin()
        {
        }

        ~NamedVariableManager()
        {
            m_ManagedVariableNames.clear();
        }

        virtual void OnLoadComplete(IParameterListV400* pParams) 
        {
            ReRegisterManageVariables();
        }

        virtual void OnVehicleChanged(IParameterListV400* pParams) 
        {
            ReRegisterManageVariables();
        }

    private:

        void RegisterVariablePrivate(LPCSTR pszName)
        {
            m_ManagedVariableNames.insert(pszName);
        }

        void ReRegisterManageVariables()
        {
            for (const auto& name : m_ManagedVariableNames)
            {
                P3D::PdkServices::GetPanelSystem()->RegisterNamedVariable(name.c_str());
            }
        }

        static NamedVariableManager* m_pInstance;

        std::set<std::string> m_ManagedVariableNames;
    };

    class ManagedNamedVariable : public NamedVariable
    {
    public:
    
        ManagedNamedVariable(LPCSTR szName, FLOAT64 fInitialValue) :
            NamedVariable(szName, fInitialValue)
        {
            NamedVariable::RegisterVariable();
            NamedVariableManager::RegisterVariable(szName);
        }
    };
}
