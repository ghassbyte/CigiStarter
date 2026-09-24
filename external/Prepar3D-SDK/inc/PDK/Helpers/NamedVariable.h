#include "PdkServices.h"

#include <string>

#pragma once
namespace P3D
{
    /** @addtogroup panelsystem */ /** @{ */

    /**
    * Basic wrapper class for binding to L: named variables.
    */
    class NamedVariable
    {
    public:

        NamedVariable(LPCSTR szName, FLOAT64 fInitialValue) :
            m_szName(szName),
            m_fValue(fInitialValue),
            m_fInitialValue(fInitialValue)
        {
        }

        void SetValue(FLOAT64 fValue, bool bUpdate = true)
        {
            m_fValue = fValue;
            if (bUpdate)
            {
                RegisterVariable();
                P3D::PdkServices::GetPanelSystem()->SetNamedVariableValue(m_ID, m_fValue);
            }
        }

        FLOAT64 GetValue(bool bUpdate = true)
        {
            if (bUpdate)
            {
                RegisterVariable();
                m_fValue = P3D::PdkServices::GetPanelSystem()->GetNamedVariableValue(m_ID);
            }
            return m_fValue;
        }

        void RegisterVariable()
        {
            if (m_ID < 0)
            {
                m_ID = P3D::PdkServices::GetPanelSystem()->CheckNamedVariable(m_szName.c_str());
                if (m_ID < 0)
                {
                    m_ID = P3D::PdkServices::GetPanelSystem()->RegisterNamedVariable(m_szName.c_str());
                    P3D::PdkServices::GetPanelSystem()->SetNamedVariableValue(m_ID, m_fValue);
                }
            }
        }

        void Reset()
        {
            m_ID = -1;
            SetValue(m_fInitialValue);
        }

        ID m_ID = -1;
        std::string m_szName = "";
        FLOAT64 m_fValue = 0.0;
        FLOAT64 m_fInitialValue = 0.0;
    };
}
