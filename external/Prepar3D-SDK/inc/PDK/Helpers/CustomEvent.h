// Copyright (c) 2010-2022 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// CustomEvent.h

#pragma once
#include <functional>

namespace P3D
{
    //EventCallback for Custom Events
    class CustomEventCallback : public P3dCallback
    {
    public:

        CustomEventCallback(const GUID& eventID, std::function<void(IParameterListV400*)> func) :
            m_eventID(eventID),
            m_func(func)
        {
        }

        virtual void Invoke(IParameterListV400* pParams)
        {
            if (m_func)
            {
                m_func(pParams);
            }
        }

        const GUID& GetID() const { return m_eventID; }

    private:

        std::function<void(IParameterListV400*)> m_func;
        GUID m_eventID;
    };

}