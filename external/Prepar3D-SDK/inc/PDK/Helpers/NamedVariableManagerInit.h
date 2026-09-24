#pragma once
#include "NamedVariableManager.h"

#pragma once
namespace P3D
{
    /** @use this once in your DLL to initialize NamedVariableManager */ /** @{ */

    NamedVariableManager* NamedVariableManager::m_pInstance = nullptr;

    void NamedVariableManager::Init()
    {
        if (m_pInstance == nullptr)
        {
            m_pInstance = new NamedVariableManager();
        }
    }

    void NamedVariableManager::Shutdown()
    {
        if (m_pInstance)
        {
            delete m_pInstance;
            m_pInstance = nullptr;
        }
    }
}
