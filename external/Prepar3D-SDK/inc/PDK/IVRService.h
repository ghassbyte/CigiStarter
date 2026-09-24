// Copyright (c) 2010-2019 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IVRService.h

#pragma once
#include "IVRPluginSystem.h"
#include "Legacy/IVRService.Legacy.h"

namespace P3D
{
    interface ICallbackV400;
    interface IStereoCameraV500;

    struct VRServiceMessageIDs
    {
        static const UINT64 VR_ENABLED = 0;
        static const UINT64 VR_DISABLED = 1;
        static const UINT64 VR_SETTINGS_SAVED = 2;
        static const UINT64 VR_UPDATE = 3;
    };

    // Message IDs for the the EVENT_MESSAGE_HMD_EVENT P3D event
    struct EVENT_MESSAGE_HMD_EVENT_IDs
    {
        static const UINT64 XR_DISABLE = 0;
        static const UINT64 XR_ENABLE = 1;
    };

    //Enumeration of currently supported HMD Devices
    enum HMD_INTERFACES
    {
        STEAM,
        OCULUS,
        VARJO,
        HMD_EMULATOR,
        VRG,
        HMD_INVALID_INTERFACE
    };

    /** \defgroup vrservice   Virtual Reality Service
     *
     * This Service allows the caller to interact with the Virtual Reality Interface, tracked controllers, access Virtual Reality Settings
     * and register for specific VR callbacks. It also gives the ability to Enable/Disable VR.
     *
     * \{
     */

    DECLARE_INTERFACE_(IVRServiceV600, IVRServiceV452)
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

       /**
       * Check if Virtual Reality is enabled
       */
       virtual bool IsVREnabled() = 0;

       /**
       * Check if any XR camera is active
       */
       virtual bool GetIsXRCameraActive() = 0;
    };

    DECLARE_INTERFACE_(IVarjoPostProcessServiceV610, IUnknown)
    {
        virtual void SetShaderFileName(PCWSTR fileName) abstract;
        virtual PCWSTR GetShaderFileName() abstract;
        virtual void SetShaderBlockSize(int nBlockSize) abstract;
        virtual int GetShaderBlockSize() abstract;
        virtual void SetSamplingMargin(int nSamplingMargin) abstract;
        virtual int GetSamplingMargin() abstract;
        virtual void SetConstantBufferSize(int nConstantBufferSize) abstract;
        virtual int GetConstantBufferSize() abstract;
        virtual bool IsEnabled() abstract;
        virtual void SetIsEnabled(bool bIsEnabled) abstract;
        virtual void SetConstantBufferData(char* pData, int nConstantBufferSize) abstract;
        virtual char* GetConstantBufferData() abstract;
        virtual bool IsAvailable() abstract;
    };

    //---------------------------------------------------------------------------
    // Predefined UUIDs

    interface __declspec(uuid("{56EA45FE-A91F-4BDD-B70C-8574361221C5}")) IVRServiceV600;
    extern __declspec(selectany) REFIID IID_IVRServiceV600 = __uuidof(IVRServiceV600);
    extern __declspec(selectany) REFGUID SID_VRService = __uuidof(IVRServiceV450);

    interface __declspec(uuid("{C8E5DFD7-905C-4C01-92A5-1540739759F6}")) IVarjoPostProcessServiceV610;
    extern __declspec(selectany) REFIID IID_IVarjoPostProcessServiceV610 = __uuidof(IVarjoPostProcessServiceV610);
    extern __declspec(selectany) REFGUID SID_VarjoPostProcessService = __uuidof(IVarjoPostProcessServiceV610);

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.
    extern __declspec(selectany) REFIID  IID_IVRService = IID_IVRServiceV600;

    typedef IVRServiceV600 IVRService;

    /** \} */
};
