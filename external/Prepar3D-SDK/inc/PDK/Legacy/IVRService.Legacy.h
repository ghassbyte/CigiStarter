// Copyright (c) 2010-2019 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IVRService.h

#pragma once
#include "IVRPluginSystem.h"

namespace P3D
{
    interface ICallbackV400;
    interface IStereoCameraV500;

    struct VRServiceMessageIDs;

    //Enumeration of currently supported HMD Devices
    enum HMD_INTERFACES;

    /** \defgroup vrservice   Virtual Reality Service
     *
     * This Service allows the caller to interact with the Virtual Reality Interface, tracked controllers, access Virtual Reality Settings
     * and register for specific VR callbacks. It also gives the ability to Enable/Disable VR.
     *
     * \{
     */

    DECLARE_INTERFACE_(IVRServiceV450, IUnknown)
    {
        virtual void RegisterCallback(ICallbackV400* pCallback) = 0;
        virtual void UnregisterCallback(ICallbackV400* pCallback) = 0;

        /**
        * Returns the Virtual Reality Interface
        */
        virtual IVRInterfaceV450* GetVRInterface() = 0;

        /**
        * Returns the Virtual Reality Settings Interface
        */
        virtual IVRSettingsV450* GetVRSettings() = 0;

        /**
        * Enables Virtual Reality with the indicated HMD Device
        * @note, If Virtual Reality is already enabled this function does nothing
        * @param[in] eInterface          Enum for the HMD Device
        */
        virtual void EnableVR(HMD_INTERFACES eInterface) = 0;

        /**
        * Disables Virtual Reality
        */
        virtual void DisableVR() = 0;
    };

    DECLARE_INTERFACE_(IVRServiceV452, IVRServiceV450)
    {
       virtual void RegisterCallback(ICallbackV400 * pCallback) abstract;
       virtual void UnregisterCallback(ICallbackV400 * pCallback) abstract;
       virtual void UnregisterStereoCamera(IStereoCameraV500 * pCamera) abstract;
       virtual void RegisterStereoCamera(IStereoCameraV500 * pCamera) abstract;

       /**
       * Returns the Virtual Reality Interface
       */
       virtual IVRInterfaceV450* GetVRInterface() = 0;

       /**
       * Returns the Virtual Reality Settings Interface
       */
       virtual IVRSettingsV450* GetVRSettings() = 0;

       /**
       * Enables Virtual Reality with the indicated HMD Device
       * @note, If Virtual Reality is already enabled this function does nothing
       * @param[in] eInterface          Enum for the HMD Device
       */
       virtual void EnableVR(HMD_INTERFACES eInterface) = 0;

       /**
       * Disables Virtual Reality
       */
       virtual void DisableVR() = 0;
    };

    interface __declspec(uuid("{4E1293F6-BD49-4CE4-B308-B99859AD64C8}")) IVRServiceV450;
    extern __declspec(selectany) REFIID IID_IVRServiceV450 = __uuidof(IVRServiceV450);

    interface __declspec(uuid("{81a87b88-90b8-435c-a84d-13418bbe8523}")) IVRServiceV452;
    extern __declspec(selectany) REFIID  IID_IVRServiceV452 = __uuidof(IVRServiceV452);

    /** \} */
};