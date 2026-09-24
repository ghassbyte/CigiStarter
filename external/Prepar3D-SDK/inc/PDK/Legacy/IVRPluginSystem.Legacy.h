// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IVisualEffects.Legacy.h
// Prepar3D SDK interface for visual effects
//----------------------------------------------------------------------------

#pragma once

#include <ObjBase.h>
#include "Helpers/P3DMath.h"

namespace P3D
{
    //The Mode for Rendering the HMD Displays in Prepar3D
    enum HMD_DISPLAY_MODE;

    // Reference Origin for HMD. Default HMD Seated origin, or P3D calculated offset
    enum HMD_REFERENCE_ORIGIN;

    //Virtual Reality Controller Enumeration
    enum VRController;

    // For eye tracking.  Don't use these to refference views
    enum VREye;

    //Virtual Reality View Enumeration
    enum VRView;

    //Virtual Reality Eye Data Structure
    struct P3DEyeData;

    //Interface for accessing Virtual Reality Tracked Controller Information
    interface IVRTrackedControllerV450;

    /**
     *  Interface for accessing the Virtual Reality Settings
     **/
    DECLARE_INTERFACE_(IVRInterfaceV450, IUnknown)
    {
        /**
        * Check if Portals are being edited
        */
        virtual bool GetIsPortalEditMode() abstract;

        /**
        * Returns the Window Interface of the indicated View
        * @param[in] eView          Enum for the Virtual Reality View
        */
        virtual IWindowV440* GetWindow(VRView eView) abstract;

        /**
        * Returns the name of the Headset that is currently active
        */
        virtual LPCWSTR GetNameOfHmd() abstract;

        /**
        * Get the current Z offset of the Eyes
        */
        virtual float GetEyepointZOffset() abstract;

        /**
        * Get the World Transform of the headset origin
        * @param[in\out] HMDWorld       World Location to store the origin
        */
        virtual void GetHMDOrigin(ObjectWorldTransform &HMDWorld) abstract;

        /**
        * Returns the Camera System Interface for the indicated Virtual Reality View
        * @param[in] eView          Enum for the Virtual Reality View
        */
        virtual ICameraSystemV450* GetCameraSystem(VRView eView) abstract;

        /**
        * Get the World Transform of the indicted Virtual Reality View
        * @param[in] eView              The desired view
        * @param[in\out] worldTrans     World location to store the views world transform
        */
        virtual void GetWorldTransform(VRView eView, ObjectWorldTransform& worldTrans) abstract;

        /**
        * Get the Local Transform of the indicated Virtual Reality View
        * @param[in] eView              The desired view
        * @param[in\out] localTrans     Local postiion to store the views local position
        */
        virtual void GetLocalTransform(VRView eView, ObjectLocalTransform& localTrans) abstract;

        /**
        * Sets the Portal editing mode
        * @param[in] bEdit          Boolean for the portal edit mode
        */
        virtual void SetIsPortalEditMode(bool bEdit) abstract;

        /**
        * Set the World Transform of the headset origin
        * @param[in] HMDWorld       World Location to set the origin
        */
        virtual void SetHMDOrigin(ObjectWorldTransform &HMDWorld) abstract;

        /**
        * Returns the Tracked Controller Interface for the indicated controller
        * @param[in] controller          Enum for the controller
        */
        virtual IVRTrackedControllerV450* GetController(VRController controller) abstract;

        /**
        * Returns the Eye Data of the indicated Eye including focus, openness, pupil diameter and gaze information
        * @param[in] eye          Enum for which eye Left or Right
        */
        virtual P3DEyeData& GetEyeData(VREye eye) abstract;
    };

    DECLARE_INTERFACE_(IVRSettingsV450, IUnknown)
    {
        /**
        * Get the current render mode setting
        */
        virtual HMD_DISPLAY_MODE GetHMDDisplayMode() abstract;

        /**
        * Get the current reference origin setting
        */
        virtual HMD_REFERENCE_ORIGIN GetHMDReferenceOrigin() abstract;

        /**
        * Returns the Button of the indicated button type
        */
        virtual float GetEyePointZOffset() abstract;

        /**
        * Returns if zoom is enabled on the headset
        */
        virtual bool GetEnableZoom() abstract;

        /**
        * Returns whether eye tracking is enabled or not
        */
        virtual bool GetEnableEyeTracking() abstract;

        /**
        * Returns if Gaze Selection is enabled
        */
        virtual bool GetEnableGazeSelection() abstract;

        /**
        * Returns if tooltips are enabled
        */
        virtual bool GetEnableToolTips() abstract;
    };
	
	DECLARE_INTERFACE_(IVRSettingsV451, IVRSettingsV450)
    {
        /**
        * Get the current render mode setting
        */
        virtual HMD_DISPLAY_MODE GetHMDDisplayMode() abstract;

        /**
        * Get the current reference origin setting
        */
        virtual HMD_REFERENCE_ORIGIN GetHMDReferenceOrigin() abstract;

        /**
        * Returns the Button of the indicated button type
        */
        virtual float GetEyePointZOffset() abstract;

        /**
        * Returns if zoom is enabled on the headset
        */
        virtual bool GetEnableZoom() abstract;

        /**
        * Returns whether eye tracking is enabled or not
        */
        virtual bool GetEnableEyeTracking() abstract;

        /**
        * Returns if Gaze Selection is enabled
        */
        virtual bool GetEnableGazeSelection() abstract;

        /**
        * Returns if tooltips are enabled
        */
        virtual bool GetEnableToolTips() abstract;

        /**
        * Returns if Gaze Detection is enabled
        */
        virtual bool GetEnableGazeDetection() abstract;

        /**
        * Returns if Eye Data Panel is enabled
        */
        virtual bool GetEnableEyeData() abstract;
    };

    DECLARE_INTERFACE_(IVRSettingsV452, IVRSettingsV451)
    {
        /**
        * Get the current render mode setting
        */
        virtual HMD_DISPLAY_MODE GetHMDDisplayMode() abstract;

        /**
        * Get the current reference origin setting
        */
        virtual HMD_REFERENCE_ORIGIN GetHMDReferenceOrigin() abstract;

        /**
        * Returns the Button of the indicated button type
        */
        virtual float GetEyePointZOffset() abstract;

        /**
        * Returns if zoom is enabled on the headset
        */
        virtual bool GetEnableZoom() abstract;

        /**
        * Returns whether eye tracking is enabled or not
        */
        virtual bool GetEnableEyeTracking() abstract;

        /**
        * Returns if Gaze Selection is enabled
        */
        virtual bool GetEnableGazeSelection() abstract;

        /**
        * Returns if tooltips are enabled
        */
        virtual bool GetEnableToolTips() abstract;

        /**
        * Returns if Gaze Detection is enabled
        */
        virtual bool GetEnableGazeDetection() abstract;

        /**
        * Returns if Eye Data Panel is enabled
        */
        virtual bool GetEnableEyeData() abstract;

        /// PassThroughCamera settings ///

        /**
        * Returns: The bightness setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraBrightness() abstract;

        /**
        * Returns: The contrast setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraContrast() abstract;

        /**
        * Returns: The hue setting
        * Range: 0 - 11
        * Default: 0
        */
        virtual int GetPassThroughCameraHue() abstract;

        /**
        * Returns: The saturation setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraSaturation() abstract;

        /**
        * Returns: The gain setting
        * Range: 0 - 100
        * Default: 50
        */
        virtual int GetPassThroughCameraGain() abstract;

        /**
        * Returns: The exposure setting
        * Range: 0 - 100
        * Default: 50
        */
        virtual int GetPassThroughCameraExposure() abstract;

        /**
        * Returns: The white balance setting
        * Range: 2800 - 6500 or cool - warm
        * Default: 4800
        */
        virtual int GetPassThroughCameraWhiteBalance() abstract;

        /**
        * Returns: The camera resolution setting
        * Resolutions: 2K, 1080p, 720p, VGA
        * Default: 720p
        */
        virtual int GetPassThroughCameraResolution() abstract;

        /**
        * Returns: The camera mode
        * Modes: Camera only, Portals, Inverted Portals, Green Screen, Green Screen and Portals, Green Screen and Inverted Portals
        * Default: 0 or Camera only
        */
        virtual int GetPassThroughCameraMode() abstract;

        /**
        * Returns: The field of view
        * Default: 85, 60
        * Default aspect ratio: 17:12
        */
        virtual P3DMath::float2& GetPassThroughCameraFOV() abstract;

        /**
        * Returns: The sensor mode
        * Modes: Left eye, Right eye, Both
        * Default: 2 or Both
        */
        virtual int GetPassThroughCameraSensorMode() abstract;

        /**
        * Returns: If the auto exposure is activated
        * Default: 1
        */
        virtual bool GetPassThroughCameraAutoExposure() abstract;

        /**
        * Returns: If the auto white balance is activated
        * Default: 1
        */
        virtual bool GetPassThroughCameraAutoWhiteBalance() abstract;

        /**
        * Returns: The exposure time
        * Default: 0
        */
        virtual float GetPassThroughCameraExposureTime() abstract;

        /**
        * Returns: The global gain
        * Range: 0 - 1
        * Default: 1
        */
        virtual float GetPassThroughCameraGlobalGain() abstract;

        /**
        * Returns: The color of the global gain
        * Default: {1, 1, 1}
        */
        virtual P3DMath::float3& GetPassThroughCameraColorGain() abstract;

        /**
        * Returns: A string representing the texture
        */
        virtual PCWSTR GetCameraEmulatorTexture() abstract;
    };


    /**
     *  Interface for accessing the Virtual Reality Settings
     **/
    DECLARE_INTERFACE_(IVRSettingsV500, IVRSettingsV452)
    {
        /**
        * Get the current render mode setting
        */
        virtual HMD_DISPLAY_MODE GetHMDDisplayMode() abstract;

        /**
        * Get the current reference origin setting
        */
        virtual HMD_REFERENCE_ORIGIN GetHMDReferenceOrigin() abstract;

        /**
        * Returns the Button of the indicated button type
        */
        virtual float GetEyePointZOffset() abstract;

        /**
        * Returns if zoom is enabled on the headset
        */
        virtual bool GetEnableZoom() abstract;

        /**
        * Returns whether eye tracking is enabled or not
        */
        virtual bool GetEnableEyeTracking() abstract;

        /**
        * Returns if Gaze Selection is enabled
        */
        virtual bool GetEnableGazeSelection() abstract;

        /**
        * Returns if tooltips are enabled
        */
        virtual bool GetEnableToolTips() abstract;

        /**
        * Returns if Gaze Detection is enabled
        */
        virtual bool GetEnableGazeDetection() abstract;

        /**
        * Returns if Eye Data Panel is enabled
        */
        virtual bool GetEnableEyeData() abstract;

        /// PassThroughCamera settings ///

        /**
        * Returns: The bightness setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraBrightness() abstract;

        /**
        * Returns: The contrast setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraContrast() abstract;

        /**
        * Returns: The hue setting
        * Range: 0 - 11
        * Default: 0
        */
        virtual int GetPassThroughCameraHue() abstract;

        /**
        * Returns: The saturation setting
        * Range: 0 - 8
        * Default: 4
        */
        virtual int GetPassThroughCameraSaturation() abstract;

        /**
        * Returns: The gain setting
        * Range: 0 - 100
        * Default: 50
        */
        virtual int GetPassThroughCameraGain() abstract;

        /**
        * Returns: The exposure setting
        * Range: 0 - 100
        * Default: 50
        */
        virtual int GetPassThroughCameraExposure() abstract;

        /**
        * Returns: The white balance setting
        * Range: 2800 - 6500 or cool - warm
        * Default: 4800
        */
        virtual int GetPassThroughCameraWhiteBalance() abstract;

        /**
        * Returns: The camera resolution setting
        * Resolutions: 2K, 1080p, 720p, VGA
        * Default: 720p
        */
        virtual int GetPassThroughCameraResolution() abstract;

        /**
        * Returns: The camera mode
        * Modes: Camera only, Portals, Inverted Portals, Green Screen, Green Screen and Portals, Green Screen and Inverted Portals
        * Default: 0 or Camera only
        */
        virtual int GetPassThroughCameraMode() abstract;

        /**
        * Returns: If green screen is enabled
        * Default: 0
        */
        virtual int GetEnableGreenScreen() abstract;

        /**
        * Returns: The field of view
        * Default: 85, 60
        * Default aspect ratio: 17:12
        */
        virtual P3DMath::float2& GetPassThroughCameraFOV() abstract;

        /**
        * Returns: The sensor mode
        * Modes: Left eye, Right eye, Both
        * Default: 2 or Both
        */
        virtual int GetPassThroughCameraSensorMode() abstract;

        /**
        * Returns: If the auto exposure is activated
        * Default: 1
        */
        virtual bool GetPassThroughCameraAutoExposure() abstract;

        /**
        * Returns: If the auto white balance is activated
        * Default: 1
        */
        virtual bool GetPassThroughCameraAutoWhiteBalance() abstract;

        /**
        * Returns: The exposure time
        * Default: 0
        */
        virtual float GetPassThroughCameraExposureTime() abstract;

        /**
        * Returns: The global gain
        * Range: 0 - 1
        * Default: 1
        */
        virtual float GetPassThroughCameraGlobalGain() abstract;

        /**
        * Returns: The color of the global gain
        * Default: {1, 1, 1}
        */
        virtual P3DMath::float3& GetPassThroughCameraColorGain() abstract;

        /**
        * Returns: A string representing the texture
        */
        virtual PCWSTR GetCameraEmulatorTexture() abstract;

        /*
        * Returns: An boolean representing if the varjo camera is enabled
        * Default: False
        */
        virtual bool GetVarjoEnableCamera() abstract;

        /*
        * Returns: An integer representing the Varjo view mode (Stereo for two views is less expensive)
        * Modes: Stereo, Four-Views
        * Default: Four-Views
        */
        virtual int GetVarjoViewMode() abstract;

        /*
        * Returns: An integer representing the HMD sync mode.
        * Modes: Pre-Render, Update
        * Default: Update
        */
        virtual int GetSyncMode() abstract;

        /*
        * Returns: A float represneting the radius of the virtual reality cursor
        * Default: 0.5
        */
        virtual float GetMouseCursorRadius() abstract;

        /*
        * Returns: An integer represneting the opacity of the virtual reality cursor
        * Default: 200
        */
        virtual int GetMouseCursorOpacity() abstract;

        /*
        * Returns: A vector of three floats representing the RGB color of the virtual reality cursor
        * Default: (200, 200, 255)
        */
        virtual P3DMath::uint3 GetMouseCursorColor() abstract;

        /*
        * Returns: An boolean representing if the varjo camera is enabled
        * Default: False
        */
        virtual bool GetVarjoEnableChromaKey() abstract;

        /*
        * Returns: A vector of three floats represneting the HSB (Hue, Saturation, Brightness / Value) color of the greenscreen
        * Default: (12, 100, 100)
        */
        virtual P3DMath::float3& GetVarjoChromaKeyColor() abstract;

        /*
        * Returns: A vector of three floats represneting the tolerance for balancing chroma leak, reflections, and shadows
        * Default: (15, 60, 92)
        */
        virtual P3DMath::float3& GetVarjoChromaKeyTolerance() abstract;

        /*
        * Returns: A vector of three floats represneting the falloff for gradual fade out of reflections and shadows
        * Default: (3, 3, 3)
        */
        virtual P3DMath::float3& GetVarjoChromaKeyFalloff() abstract;

        virtual bool GetEnableHiddenAreaMesh() abstract;

        virtual bool GetEnableFoveatedRendering() abstract;

        virtual bool GetEnableVariableRateShading() abstract;

        virtual float GetResolutionScalar() abstract;

        virtual int GetMaxViewCount() abstract;

        virtual bool GetShowAppMenu() abstract;

        virtual bool GetShowInfoGen() abstract;

        virtual bool GetShowPortalEditControls() abstract;
    };

    interface __declspec(uuid("{f617a011-4016-4742-8794-5a07758e4952}")) IVRInterfaceV450;
    extern __declspec(selectany) REFIID  IID_IVRInterfaceV450 = __uuidof(IVRInterfaceV450);

    interface __declspec(uuid("{03f92bbf-5a66-434f-a298-8e181094a90e}")) IVRSettingsV450;
    extern __declspec(selectany) REFIID  IID_IVRSettingsV450 = __uuidof(IVRSettingsV450);
	
	    interface __declspec(uuid("{75d69bf7-09de-4758-8c1c-b387e1180632}")) IVRSettingsV451;
    extern __declspec(selectany) REFIID  IID_IVRSettingsV451 = __uuidof(IVRSettingsV451);

    interface __declspec(uuid("{32f9ae00-8622-43e6-9a01-417b29069fc4}")) IVRSettingsV452;
    extern __declspec(selectany) REFIID  IID_IVRSettingsV452 = __uuidof(IVRSettingsV452);

    interface __declspec(uuid("{8C99BDE0-092A-4D83-806F-72DF3C19E91E}")) IVRSettingsV500;
    extern __declspec(selectany) REFIID  IID_IVRSettingsV500 = __uuidof(IVRSettingsV500);
}