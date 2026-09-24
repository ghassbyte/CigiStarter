// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IEventService.Legacy.h

#pragma once

#include <ObjBase.h>
#include "ICallback.h"
#include "IParameterList.h"

namespace P3D
{
    /** \defgroup    eventservice      Event Service
     This service is provided through the PDK interface and allows callers to register for events
     which are signaled by Prepar3D. Event GUIDs are defined in the interface header. Currently
     supported events are:

     - EVENTID_Frame: This event is signaled once per frame.
     - EVENTID_1Hz: This event is signaled once per second.
     - EVENTID_CustomObjectDraw: This event is signaled once per view each frame.

     See PdkPlugin for an example of registering for multiple events and messages.
     \{
     */

    /** Service used to register for Prepar3D event callbacks
     *
     */
    DECLARE_INTERFACE_(IEventServiceV400, IUnknown)
    {
        /**
        *   Register an event callback.
        *   @param    eventID      GUID id of the event to register for
        *   @param    pCallback    Callback to register
        *   @return   S_OK if succeeded and E_FAIL if it failed.
        **/
        virtual HRESULT RegisterCallback(const GUID & eventID, ICallbackV400 * pCallback) abstract;
        /**
        *   Unregister an event callback.
        *   @param    eventID      GUID id of the event to register for
        *   @param    pCallback    Callback to register
        *   @return   S_OK if succeeded and E_FAIL if it failed.
        **/
        virtual HRESULT UnregisterCallback(const GUID & eventID, ICallbackV400 * pCallback) abstract;
        /**
        *   Send a message event.
        *   @param    messageID  message ID
        *   @param    PVOID      message parameter
        **/
        virtual HRESULT SendMessageEvent(UINT32 messageID, PVOID messageParam) abstract;
    };

    DECLARE_INTERFACE_(IEventServiceV510, IEventServiceV400)
    {
        /**
        *   Register an event callback.
        *   @param    eventID      GUID id of the event to register for
        *   @param    pCallback    Callback to register
        *   @return   S_OK if succeeded and E_FAIL if it failed.
        **/
        virtual HRESULT RegisterCallback(const GUID & eventID, ICallbackV400 * pCallback) abstract;

        /**
        *   Unregister an event callback.
        *   @param    eventID      GUID id of the event to register for
        *   @param    pCallback    Callback to register
        *   @return   S_OK if succeeded and E_FAIL if it failed.
        **/
        virtual HRESULT UnregisterCallback(const GUID & eventID, ICallbackV400 * pCallback) abstract;

        /**
        *   Send a message event.
        *   @param    messageID  message ID
        *   @param    PVOID      message parameter
        **/
        virtual HRESULT SendMessageEvent(UINT32 messageID, PVOID messageParam) abstract;

        /**
        *   Shutdown Prepar3D.
        *   @return   S_OK if succeeded and E_FAIL if it failed.
        **/
        virtual HRESULT ShutdownApplication() abstract;
    };

    interface __declspec(uuid("{5b3bac47-d372-4a51-a666-d0bc839484c7}")) IEventServiceV400;
    extern __declspec(selectany) REFIID IID_IEventServiceV400 = __uuidof(IEventServiceV400);

    interface __declspec(uuid("{4d119933-e0f6-434c-aea8-8cb0df164bc7}")) IEventServiceV510;
    extern __declspec(selectany) REFIID IID_IEventServiceV510 = __uuidof(IEventServiceV510);

/** \} */ // end doc group
}