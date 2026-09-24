// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IControllableCamera.h

#pragma once

#include <atlcomcli.h>
#include <Unknwn.h>

namespace P3D
{
    /** \defgroup   controllableCamera     Controllable Camera
    *  This service is provided through the PDK interface and provides a wrapper interface 
    *  to control a camera using XML via a GaugeCallback plugin.  While XML is the primary use case,
    *  a PDK service is provided to make it easier to extend the camera control capabilities via C++.
    *  The controllable camera service was developed entirely through the SDK to showcase the power 
    *  and flexibility that 3rd party developers have.
    *  \{
    */

    /**
        Interface for ControllableCamera PDK service
    */
    DECLARE_INTERFACE_(IControllableCameraV400, IUnknown)
    {
    public:
        /**
            Initialize the Controllable Camera system with the window name
            @param sWindowName Name of window
        */
        virtual HRESULT Init(LPCWSTR sWindowName) abstract;
        /**
            Deinit the Controllable Camera 
        */
        virtual HRESULT DeInit(void) abstract;
        /**
            Returns true if camera is initialized
        */
        virtual bool IsInitialized() const abstract;
        /** 
           Provides way for external implementations to keep track of when view should be displayed or not, 
           setting this does not modify the P3D camera in any way.
           @param visible determines if view should be visible or not.
        */
        virtual void SetVisible(bool visible) abstract;
        /**
            Is view being displayed
            setting this does not modify the P3D camera in any way.
            @return true if view is visible
        */
        virtual bool IsVisible() abstract;
        /**  
            Register view   
        */
        virtual HRESULT Register() abstract;
        /**
           Check if view is registered
        */
        virtual bool IsRegistered() abstract;
        /**
            Set camera pitch bank and heading in degrees
            @param pitch  Camera pitch in degrees
            @param bank  Camera bank in degrees
            @param heading  Camera heading in degrees
        */
        virtual void SetPbh(float pitch, float bank, float heading) abstract;
        /**
            Set camera pitch in degrees
            @param pitch  Camera pitch in degrees
        */
        virtual void SetPitch(float pitch) abstract;
        /**
            Set camera bank in degrees
            @param bank  Camera bank in degrees
        */
        virtual void SetBank(float bank) abstract;
        /**
            Set camera heading in degrees
            @param heading  Camera heading in degrees
        */
        virtual void SetHeading(float heading) abstract;
        /**
            Set camera xyz offset in meters
            @param x  Camera x offset in meters
            @param y  Camera y offset in meters
            @param z  Camera z offset in meters
        */
        virtual void SetXyz(float x, float y, float z) abstract;
        /**
            Set camera x offset in meters
            @param x  Camera x offset in meters
        */
        virtual void SetX(float x) abstract;
        /**
            Set camera y offset in meters
            @param y  Camera y offset in meters
        */
        virtual void SetY(float y) abstract;
        /**
            Set camera z offset in meters
            @param z  Camera z offset in meters
        */
        virtual void SetZ(float z) abstract;
        /** 
            Zoom camera in  
        */
        virtual void ZoomIn() abstract;
        /** 
            Zoom camera out  
        */
        virtual void ZoomOut() abstract;
        /**
            Set camera zoom
            @param zoom camera zoom
        */
        virtual void SetZoom(float zoom) abstract;
        /**
            Set camera's horizontal and vertical field of view in degrees
            @param hFov  Camera horizontal FOV in degrees
        */
        virtual void SetFov(float hFov, float vFov) abstract;
        /**
            Set camera's vertical field of view in degrees
            @param hFov  Camera horizontal FOV in degrees
        */
        virtual void SetHFov(float hFov) abstract;
        /**
            Set camera's vertical field of view in degrees
            @param vFov  Camera vertical FOV in degrees
        */
        virtual void SetVFov(float vFov) abstract;
        /** 
            Activate Position Tracking 
        */
        virtual void ActivatePositionTracking() abstract;
        /** 
            Deactivate Position Tracking 
        */
        virtual void DeactivatePositionTracking() abstract;
        /**
            Set target latitude longitude and altitude in degrees/meters
            @param lat  target latitude in degrees
            @param lon  target longitude in degrees
            @param alt  target altitude in meters
        */
        virtual void SetTargetLla(double lat, double lon, double alt) abstract;
        /**
            Set target latitude longitude and altitude in degrees/meters
            @param lat  target latitude in degrees
        */
        virtual void SetTargetLat(double lat) abstract;
        /**
            Set target longitude in degrees/meters
            @param lon target longitude in degrees
        */
        virtual void SetTargetLon(double lon) abstract;
        /**
            Set target altitude in degrees/meters
            @param alt target altitude in meters
        */
        virtual void SetTargetAlt(double alt) abstract;
        /**
            Activate Entity Tracking
        */
        virtual void ActivateEntityTracking() abstract;
        /**
            Deactivate Entity Tracking
        */
        virtual void DeactivateEntityTracking() abstract;
        /**
            Set target container id
            @param targetID container id of target
        */
        virtual void SetTargetContainerId(unsigned int targetId) abstract;
        /**
            Add post process
            @param postProcess name of post process
        */
        virtual void AddPostProcess(const WCHAR* postProcess) abstract;
        /**
            Remove post process
            @param postProcess name of post process
        */
        virtual void RemovePostProcess(const WCHAR* postProcess) abstract;
        /**  Reset post processes  */
        virtual void ResetPostProcess() abstract;
        /**  Clear post processes  */
        virtual void ClearPostProcess() abstract;
        /**
            Set camera sensor mode
            @param sensorMode sensor mode
        */
        virtual void SetSensorMode(unsigned int sensorMode) abstract;
        /**
            Set Continuous heading
            @param pitch heading in meters
        */
        virtual void SetContinuousPitch(float pitch) abstract;
        /**
            Set Continuous bank
            @param pitch bank in meters
        */
        virtual void SetContinuousBank(float bank) abstract;
        /**
            Set Continuous Pitch
            @param pitch pitch in meters
        */
        virtual void SetContinuousHeading(float heading) abstract;
        /**
            Set global rotate
            @param bGlobalRotate enables global rotation
        */
        virtual void SetGlobalRotate(bool bGlobalRotate) abstract;
        /**
            Set camera pitch bank and heading in degrees with global values
            @param pitch  Camera pitch in degrees
            @param bank  Camera bank in degrees
            @param heading  Camera heading in degrees
        */
        virtual void SetGlobalPbh(float pitch, float bank, float heading) abstract;
        /**
            Get camera pitch bank and heading in degrees 
            @param pitch[out]    Camera pitch in degrees
            @param bank[out]     Camera bank in degrees
            @param heading[out]  Camera heading in degrees
        */
        virtual void GetPbh(float& pitch, float& bank, float& heading) abstract;
        /**
            Get camera pitch bank and heading in degrees 
            @return Camera pitch in degrees
        */
        virtual float GetPitch() abstract;
        /**
            Get camera bank in degrees
            @return Camera bank in degrees
        */
        virtual float GetBank() abstract;
        /**
            Get camera heading bank and heading in degrees
            @return Camera heading in degrees
        */
        virtual float GetHeading() abstract;
        /**
            Get camera xyz offset in meters
            @param[out] x  Camera x offset in meters
            @param[out] y  Camera y offset in meters
            @param[out] z  Camera z offset in meters
        */
        virtual void GetXyz(float& x, float& y, float& z) abstract;
        /**
            Get camera x offset in meters
            @return Camera x offset in meters
        */
        virtual float GetX() abstract;
        /**
            Get camera y offset in meters
            @return Camera y offset in meters
        */
        virtual float GetY() abstract;
        /**
            Get camera z offset in meters
            @return Camera z offset in meters
        */
        virtual float GetZ() abstract;
        /**
            Get camera zoom
            @return Camera zoom
        */
        virtual float GetZoom() abstract;
        /**
            Get camera's horizontal and vertical field of view in degrees
            @param[out] hFov  Camera's horizontal FOV in degrees
            @param[out] vFov  Camera's vertical FOV in degrees
        */
        virtual void GetFov(float& hFov, float& vFov) abstract;
        /**
            Get camera's horizontal field of view in degrees
            @return  Camera's horizontal FOV in degrees
        */
        virtual float GetHFov() abstract;
        /**
            Get camera's vertical field of view in degrees
            @return  Camera's vertical FOV in degrees
        */
        virtual float GetVFov() abstract;
        /**
            Is position tracking enabled
            @return  true if position tracking is enabled
        */
        virtual bool IsPositionTracking() abstract;
        /**
            Get target latitude longitude and altitude in degrees/meters
            @param[out] lat  Camera latitude in degrees
            @param[out] lon  Camera longitude in degrees
            @param[out] alt  Camera altitude in meters
        */
        virtual void GetTargetLla(double& lat, double& lon, double& alt) abstract;
        /**
            Get target latitude degrees
            @return  target latitude in degrees
        */
        virtual double GetTargetLat() abstract;
        /**
            Get target longitude degrees
            @return  target longitude in degrees
        */
        virtual double GetTargetLon() abstract;
        /**
            Get target altitude in meters
            @return  target altitude in meters
        */
        virtual double GetTargetAlt() abstract;
        /**
            Get target latitude as string
            @return  target altitude in meters
        */
        virtual const WCHAR* GetTargetLatString() abstract;
        /**
            Get target longitude as string
            @return  target longitude in meters
        */
        virtual const WCHAR* GetTargetLonString() abstract;
        /**
            Check if entity tracking is enabled
            @return  true if entity tracking is enabled
        */
        virtual bool IsEntityTracking() abstract;
        /**
            Get target container id
            @return  target container id
        */
        virtual unsigned int GetTargetContainerId() abstract;
        /**
            Get sensor mode
            @return  sensor mode
        */
        virtual unsigned int GetSensorMode() abstract;
        /**
            Get camera lookat location LLA in degrees/meters
            @param[out] lat   latitude in degrees
            @param[out] lon   longitude in degrees
            @param[out] alt   altitude in meters
        */
        virtual bool GetCameraLookAtLLA(double& lat, double& lon, double& alt) abstract;
        /**
            Check if global rotation is enabled
            @return  true if global rotation is enabled
        */
        virtual bool IsGlobalRotate() abstract;
        /**
            Get camera's global pitch bank and heading in degrees
            @param pitch[out]    Camera pitch in degrees
            @param bank[out]     Camera bank in degrees
            @param heading[out]  Camera heading in degrees
        */
        virtual void GetGlobalPbh(float& pitch, float& bank, float& heading) abstract;
    };

    //{0340940A-D8DA-4878-A397-F1C814B75C80}
    interface __declspec(uuid("{0340940A-D8DA-4878-A397-F1C814B75C80}")) IControllableCameraV400;
    extern __declspec(selectany) REFIID  IID_IControllableCameraV400 = __uuidof(IControllableCameraV400);
}