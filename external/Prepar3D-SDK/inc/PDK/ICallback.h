// Copyright (c) 2010-2020 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// ICallback.h

#pragma once

#include <ObjBase.h>
#include "IParameterList.h"

namespace P3D
{
    /**
    *   Callback interface which must be implemented to register for events.
    *   @param    index   parameter index;
    *   @return   number of parameters in the params list.
    **/
    DECLARE_INTERFACE_(ICallbackV400, IUnknown)
    {
        /**
        *   Called when the registered event is fired.
        *   @param    pParams   Callback parameters interface pointer.
        **/
        virtual void Invoke(IParameterListV400 * pParams) abstract;
    };

    interface __declspec(uuid("{aef008ce-84d1-4a54-b136-04dcb4fafd22}")) ICallbackV400;
    extern __declspec(selectany) REFIID IID_ICallbackV400 = __uuidof(ICallbackV400);
}