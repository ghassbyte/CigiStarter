// Copyright (c) 2010-2019 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IVRPluginSystem.h

#pragma once

#include <atlcomcli.h>
#include "IRenderingService.h"
#include "Legacy/IVRPluginSystem.Legacy.h"


namespace P3D
{
    /** @addtogroup vrservice
    * @{
    */

    interface ICameraSystemV450;
    interface IWindowV440;

    //Virtual Reality Controller Enumeration
    enum VRController
    {
        VR_CONTROLLER_LEFT, //Left Controller
        VR_CONTROLLER_RIGHT //Right Controller
    };

    //Virtual Reality Controller Button Enumeration
    enum VRControllerButtonType {
        BUTTON_TRIGGER,         //Trigger
        BUTTON_DPAD_RIGHT,      //D-Pad Right
        BUTTON_DPAD_LEFT,       //D-Pad Left
        BUTTON_DPAD_DOWN,       //D-Pad Down
        BUTTON_DPAD_UP,         //D-Pad Up
        BUTTON_MENU,            //Menu Button
        BUTTON_GRIP,            //Grip Button
        BUTTON_ANALOG,          //Analog Button
        MAX_BUTTON_COUNT //MUST BE LAST!!
    };

    //Virtual Reality Controller Axis Enumeration
    enum VRControllerAxisType {
        AXIS_NONE,              //No Axis
        AXIS_TOUCHPAD,          //Touchpad Axis
        AXIS_ANALOG,            //Analog Axis
        AXIS_TRIGGER,           //Trigger Axis
        AXIS_DPAD,              //D-Pad Axis
        MAX_AXIS_COUNT //MUST BE LAST!!
    };

    //Struct for Virtual Reality Controller Buttons
    struct VRControllerButton
    {
    public:
        enum VRControllerButtonType m_eButtonType;
        bool m_bPressedState; // 1 for pressed
        bool m_bPreviousPressedState;
        inline bool WasPressed()
        {
            return m_bPressedState == true && m_bPreviousPressedState == false;
        }
        inline bool WasReleased()
        {
            return m_bPressedState == false && m_bPreviousPressedState == true;
        }
    };

    //Struct for Virtual Reality Controller Axis
    struct VRControllerAxis
    {
    public:
        enum VRControllerAxisType m_eAxisType;
        float m_fAxisX;
        float m_fAxisY;
        bool m_bPressedState; // 1 for pressed
        bool m_bPreviousPressedState;
        inline bool WasPressed()
        {
            return m_bPressedState == true && m_bPreviousPressedState == false;
        }
        inline bool WasReleased()
        {
            return m_bPressedState == false && m_bPreviousPressedState == true;
        }
    };

    //The Mode for Rendering the HMD Displays in Prepar3D
    enum HMD_DISPLAY_MODE
    {
        MONO = 0,
        STEREO = 1,
        SINGLEPASS = 2
    };

    // Reference Origin for HMD. Default HMD Seated origin, or P3D calculated offset
    enum HMD_REFERENCE_ORIGIN
    {
        REF_ORIGIN_HMD_ZERO = 0,  // Calibrate using HMD API (usually resets the origin to the HMD)
        REF_ORIGIN_P3D_CUSTOM,    // Recenter HMD 0 to wherever the HMD is now by inverting the offset from current 0
        REF_ORIGIN_P3D_TRACKED,   // Calibrate by applying saved offset from a tracker (currently left controller). Also opens option to save offset.
        REF_ORIGIN_MAX            // MUST BE LAST
    };

    // For eye tracking.  Don't use these to refference views
    enum VREye
    {
        VR_EYE_LEFT, //Left Eye
        VR_EYE_RIGHT //Right Eye
    };

    //Virtual Reality View Enumeration
    enum VRView
    {
        VR_VIEW_LEFT,           //Left View
        VR_VIEW_RIGHT,          //Right View
        VR_VIEW_LEFT_FOCUS,     //Left Focus View only supported by Varjo
        VR_VIEW_RIGHT_FOCUS,    //Right Focus View only supported by Varjo
        VR_VIEW_CENTER,         
        VR_VIEW_COUNT           //MUST BE LAST
    };

    //Virtual Reality Eye Data Structure
    struct P3DEyeData
    {
        float            fPupilDiameter;
        float            fOpenness;
        ScreenCoord      ScreenFocus;
        LLADegreesMeters WorldFocus;
        XYZMeters        LocalFocus;
        XYZMeters        fGazeDirection;
        ARGBColor        DisplayColor;
    };

    /**
     *  Interface for accessing Virtual Reality Tracked Controller Information
     **/
    DECLARE_INTERFACE_(IVRTrackedControllerV450, IUnknown)
    {
        /**
        * Get the Local Transform of the Tracked Controller
        */
        virtual ObjectLocalTransform& GetLocalTransform() abstract;

        /**
        * Get the World Transform of the Tracked Controller
        */
        virtual ObjectWorldTransform& GetWorldTransform() abstract;

        /**
        * Returns the Button of the indicated button type
        * @param[in] eType      Enum for the Button Type
        */
        virtual VRControllerButton& GetButtonByType(VRControllerButtonType eType) abstract;

        /**
        * Returns the axis of the indicated axis type
        * @param[in] eType      Enum for the Axis Type
        */
        virtual VRControllerAxis& GetAxisByType(VRControllerAxisType eType) abstract;
    };

    /**
     *  Interface for Virtual Reality
     **/
    DECLARE_INTERFACE_(IVRInterfaceV451, IVRInterfaceV450)
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
        * Get the World Transform of the indicated Virtual Reality View
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

        /**
        * Returns the Projection Matrix for the indicated Virtual Reality View
        * @param[in] eView          Enum for the desired view 
        */
        virtual P3DMath::float4x4& GetProjectionMatrix(VRView eView) abstract;

        /**
        * Returns the View Matrix for the indicated Virtual Reality View
        * @param[in] eView          Enum for the desired view
        */
        virtual P3DMath::float4x4& GetViewMatrix(VRView eView) abstract;
    };

    /**
     *  Interface for accessing the Virtual Reality Settings
     **/
    DECLARE_INTERFACE_(IVRSettingsV530, IVRSettingsV500)
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
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs

    interface __declspec(uuid("{dcc11520-555a-40a5-a7ff-1409247b60fd}")) IVRInterfaceV451;
    extern __declspec(selectany) REFIID  IID_IVRInterfaceV451 = __uuidof(IVRInterfaceV451);

    interface __declspec(uuid("{EFBE48A4-785F-41B4-BEB9-FB2DD9CC7568}")) IVRSettingsV530;
    extern __declspec(selectany) REFIID  IID_IVRSettingsV530 = __uuidof(IVRSettingsV530);

    interface __declspec(uuid("{e0a9969d-976b-4923-b853-17be1e392bc7}")) IVRTrackedControllerV450;
    extern __declspec(selectany) REFIID  IID_IVRTrackedControllerV450 = __uuidof(IVRTrackedControllerV450);

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID  IID_IVRInterface = IID_IVRInterfaceV451;
    extern __declspec(selectany) REFIID  IID_IVRSettings = IID_IVRSettingsV530;
    extern __declspec(selectany) REFIID  IID_IVRTrackedController = IID_IVRTrackedControllerV450;

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    typedef IVRInterfaceV451 IVRInterface;
    typedef IVRSettingsV530 IVRSettings;
    typedef IVRTrackedControllerV450 IVRTrackedController;

    /** @} */ // end of group
}
