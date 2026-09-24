// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IWindowPluginSystem.h

#pragma once

#include <atlcomcli.h>
#include <vector>
#include "Helpers\ListBuilder.h"
#include "IRenderingService.h"

#include "Legacy\IWindowPluginSystem.legacy.h"

namespace P3D
{
    /** @defgroup windows Window and Camera Services
    *  The window and camera services enables external applications to control basic camera functions as well as adding/removing
    *  post-process effects and sensor modes using window plug-ins.
    *  @{
    */

    interface IWindowPluginSystemV400;
    interface IWindowPluginV400;
    interface IWindowV400;
    interface ICameraV400;
    interface IMouseRectListenerCallback;

    enum MOUSE_CLICK_TYPE;

    //typedef IWindowPluginSystemV400     IWindowPluginSystem;
    //typedef IWindowPluginV400           IWindowPlugin;
    //typedef IWindowV400              IWindow;

    interface ICameraSystemV400;
    interface IWindowV400;

    using IWindowList = IListBuilder<IWindowV400>;
    using WindowList = CComPtrVecBuilder<IWindowV400>;

    enum MOUSE_CLICK_TYPE
    {
        MOUSE_CLICK_NONE,           //None
        MOUSE_CLICK_RIGHT_SINGLE,   //RightSingle
        MOUSE_CLICK_MIDDLE_SINGLE,  //MiddleSingle
        MOUSE_CLICK_LEFT_SINGLE,    //LeftSingle
        MOUSE_CLICK_RIGHT_DOUBLE,   //RightDouble
        MOUSE_CLICK_MIDDLE_DOUBLE,  //MiddleDouble
        MOUSE_CLICK_LEFT_DOUBLE,    //LeftDouble
        MOUSE_CLICK_RIGHT_DRAG,     //RightDrag
        MOUSE_CLICK_MIDDLE_DRAG,    //MiddleDrag
        MOUSE_CLICK_LEFT_DRAG,      //LeftDrag
        MOUSE_CLICK_MOVE,           //Move
        MOUSE_CLICK_RIGHT_RELEASE,  //RightRelease
        MOUSE_CLICK_MIDDLE_RELEASE, //MiddleRelease
        MOUSE_CLICK_LEFT_RELEASE,   //LeftRelease
        MOUSE_CLICK_WHEEL_UP,       //WheelUp
        MOUSE_CLICK_WHEEL_DOWN,     //WheelDown
        MOUSE_CLICK_LEAVE,          //Leave
        MOUSE_CLICK_COUNT,          //Count
    };

    enum HMD_MODE
    {
        HMD_MODE_NONE,
        HMD_MODE_VR,
        HMD_MODE_AR
    };

    enum CAMERA_TRANFORM_REFERENCE
    {
        CAMERA_REFERENCE_ORIGIN = 0,
        CAMERA_REFERENCE_OFFSET,
        CAMERA_REFERENCE_EYE,
        CAMERA_REFERENCE_NONE
    };

    struct PickResult
    {
        PickResult() noexcept : bWasHit(false), uMessage(0), fTextureU(0.0f), fTextureV(0.0f), pWindow(nullptr) {}
        bool         bWasHit;
        ScreenCoord  Location;
        unsigned int uMessage;
        float        fTextureU;
        float        fTextureV;
        IWindowV400* pWindow;
    };

    /*
    * These enums behave similarly to the ways described in the camera definition docs.
    */
    enum CAMERA_SYSTEM_ORIGIN
    {
        CAMERA_SYSTEM_ORIGIN_UNKNOWN = 0,
        CAMERA_SYSTEM_ORIGIN_OBJECT_COCKPIT,
        CAMERA_SYSTEM_ORIGIN_OBJECT_VIRTUAL_COCKPIT,
        CAMERA_SYSTEM_ORIGIN_OBJECT_CENTER,
        CAMERA_SYSTEM_ORIGIN_OBJECT_CENTER_NO_ORIENT,
        CAMERA_SYSTEM_ORIGIN_OBJECT_PILOT,
        CAMERA_SYSTEM_ORIGIN_TOWER,
        CAMERA_SYSTEM_ORIGIN_FIXED,
        CAMERA_SYSTEM_ORIGIN_WORLD_OBJECT,
        CAMERA_SYSTEM_ORIGIN_SCENARIO,
        CAMERA_SYSTEM_ORIGIN_LATLONALT_ORTHOGONAL,
        CAMERA_SYSTEM_ORIGIN_OBSERVER,
        CAMERA_SYSTEM_ORIGIN_OBJECT_AI_VIRTUAL_COCKPIT,
        CAMERA_SYSTEM_ORIGIN_OBJECT_AI_CENTER,
        CAMERA_SYSTEM_ORIGIN_ATTACH_POINT,
        CAMERA_SYSTEM_ORIGIN_WORLD_OBJECT_VIRTUAL_COCKPIT,
        CAMERA_SYSTEM_ORIGIN_COUNT, // Keep this at the bottom.
    };

    /*
    * These enums behave similarly to the ways described in the camera definition docs.
    */
    enum CAMERA_SYSTEM_CATEGORY
    {
        CAMERA_SYSTEM_CATEGORY_UNKNOWN = 0,
        CAMERA_SYSTEM_CATEGORY_COCKPIT,
        CAMERA_SYSTEM_CATEGORY_OUTSIDE,
        CAMERA_SYSTEM_CATEGORY_TOWER,
        CAMERA_SYSTEM_CATEGORY_RUNWAY,
        CAMERA_SYSTEM_CATEGORY_AIRCRAFT,
        CAMERA_SYSTEM_CATEGORY_AIR_TRAFFIC,
        CAMERA_SYSTEM_CATEGORY_MULTI_PLAYER,
        CAMERA_SYSTEM_CATEGORY_SCENERY,
        CAMERA_SYSTEM_CATEGORY_IG,
        CAMERA_SYSTEM_CATEGORY_OBSERVER,
        CAMERA_SYSTEM_CATEGORY_SENSOR,
        CAMERA_SYSTEM_CATEGORY_WEAPON,
        CAMERA_SYSTEM_CATEGORY_VR,
        CAMERA_SYSTEM_CATEGORY_CUSTOM,
        CAMERA_SYSTEM_CATEGORY_DIS_ENTITIES,
        CAMERA_SYSTEM_CATEGORY_HLA_OBJECTS,
        CAMERA_SYSTEM_CATEGORY_EQUIPMENT,
        CAMERA_SYSTEM_CATEGORY_FIRST_PERSON,
        CAMERA_SYSTEM_CATEGORY_THIRD_PERSON,
        CAMERA_SYSTEM_CATEGORY_SCENARIO_OBJECTS,
        CAMERA_SYSTEM_CATEGORY_COUNT, // Keep this at the bottom.
    };

    enum AUTO_EXPOSURE
    {
        AUTO_EXPOSURE_DEFAULT,
        AUTO_EXPOSURE_OFF,
        AUTO_EXPOSURE_ON
    };

    enum DEPTH_OF_FIELD
    {
        DEPTH_OF_FIELD_DEFAULT,
        DEPTH_OF_FIELD_OFF,
        DEPTH_OF_FIELD_ON
    };

    interface __declspec(novtable) IWindowPluginV400 : public IUnknown
    {
        /**
        * Called when the plugin is added to a window.
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnAdd(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /**
        * Called when the plugin is removed from a window.
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnRemove(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /** 
        * Called before the camera updates. This is the best place to set most camera values
        * because many of them are used to update the camera's position and orientation in the world
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnPreCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /** 
        * Called after the camera updates. This is the best place to set values that override a
        * camera update.  SetCameraLODOriginLLA for example must be called here because the lod origin
        * is reset to the camera's lla each frame in the camer's update.
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnPostCameraUpdate(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /**
        * Called after window changes cameras.  
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnViewChange(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /**
        * Called when the window closes.
        * @param   pWindow   Window the plugin is currently operating on
        * @param   pCamera   Camera the plugin is currently operating on
        **/
        virtual void OnClose(IWindowV400* pWindow, ICameraSystemV400* pCamera) abstract;
        /**
        * Called when mouse of keyboard input is passed to the window.
        * @param   wnd      HWIND for the windows message
        * @param   message  Message ID for the windows message
        * @param   wParam   WPARAM for the windows message
        * @param   lParam   LPARAM for the windows message
        * @return  true if the message was handled and should not be passed through
        * to any windows underneath.
        **/
        virtual bool OnUserInput(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) abstract;
    };

    /**
    * Interface used for registering and unregistering for mouse rectangle click event callbacks.
    * Below is an example implementation of this interface:
    *```
            class MyMouseRectListenerCallback : public P3D::IMouseRectListenerCallback
            {
                DEFAULT_REFCOUNT_INLINE_IMPL();

                STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv)
                {
                    HRESULT hr = E_NOINTERFACE;

                    if (ppv == NULL)
                    {
                        return E_POINTER;
                    }

                    *ppv = NULL;

                    if (IsEqualIID(riid, IID_IMouseListenerCallback))
                    {
                        *ppv = static_cast<IMouseListenerCallback*>(this);
                    }
                    else if (IsEqualIID(riid, IID_IUnknown))
                    {
                        *ppv = static_cast<IUnknown*>(this);
                    }
                    if (*ppv)
                    {
                        hr = S_OK;
                        AddRef();
                    }

                    return hr;
                };

            public:

                MyMouseRectListenerCallback() :
                    m_RefCount(1)
                {
                }

                virtual void MouseRectListenerProc(UINT id, MOUSE_CLICK_TYPE clickType) override;
            };

            // override
            void MyMouseRectListenerCallback::MouseRectListenerProc(UINT id, MOUSE_CLICK_TYPE clickType)
            {
                OutputDebugString(_T("Click received!"));
            }
    *```
    **/
    DECLARE_INTERFACE_(IMouseRectListenerCallback, IUnknown)
    {
        /**
        * This function is called when a mouse rectangle is clicked.
        * @param   id            id of the mouse rect hit
        * @param   clickType     click type 
        **/
        virtual void MouseRectListenerProc(UINT id, MOUSE_CLICK_TYPE clickType) abstract;
    };

    /**
     *  Service for accessing windows and registering window plugins.
     **/
    DECLARE_INTERFACE_(IWindowPluginSystemV440, IWindowPluginSystemV420)
    {
        /**
        * Register a callback to be hit when a mouse rectangle is clicked.
        * @param   callback  The callback to register
        **/
        virtual HRESULT RegisterMouseRectListenerCallback(IMouseRectListenerCallback* callback) abstract;
        /**
        * UnRegister a callback that would be hit when a mouse rectangle is clicked.
        * @param   callback  The callback to unregister
        **/
        virtual HRESULT UnRegisterMouseRectListenerCallback(IMouseRectListenerCallback* callback) abstract;
        /**
        * Used to fire a mouse rectangle with the given ID and click type.
        * @param   id            id of the mouse rect hit
        * @param   clickType     click type
        **/
        virtual HRESULT FireMouseRectClick(UINT id, MOUSE_CLICK_TYPE clickType) abstract;

        /**
        * Check if a window containing a 3d view exists
        * @return name Name of the window
        **/
        virtual BOOL HasWindow(LPCWSTR name) abstract;

        /**
        * Get list of windows containing a 3D view that are currently open
        * @param[out] windows   list of windows
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT GetWindowList(IWindowList& windows) abstract;

        /**
        * Get a IWindowReaderPdk interface for a 3D view window with the inpute name
        * @param[in]  name    Name of the window
        * @param[out] window  The window with the requested name
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT GetWindow(LPCWSTR name, IWindowV400*& window) abstract;

        /**
        * Register a callback to an existing window.  Return error if window does not exist
        * @param[in] name      Name of the window
        * @param[in] plugin    Plugin that will be registered to the window
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        * @remark While the name is used to find the window, the plugin is added to the window
        * and will continue to get called even if the camera changes or the window name changes.
        **/
        virtual HRESULT RegisterInternalWindow(LPCWSTR name, IWindowPluginV400* plugin) abstract;

        /**
        * Unregister a callback from an existing window.  Return error if window does not exist
        * @param[in] name      Name of the window
        * @param[in] plugin    Plugin that will be unregistered
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT UnRegisterInternalWindow(LPCWSTR name, IWindowPluginV400* callback) abstract;

        /**
        * Create a new window and register a callback.  Return error if window already exist
        * @param[in] name,              Name of the window
        * @param[in] x,y                Window position relative to parent window. (origin is top left corner)
        * @param[in] width,height       Window dimensions in pixels
        * @param[in] plugin             Plugin that will be registered at creation time
        * @param[in] bRTT               Determines if this window is a render-to-texture view.
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT CreateInternalWindow(LPCWSTR name, UINT32 x, UINT32 y, UINT32 width, UINT32 height, IWindowPluginV400* pPlugin, bool bRTT = false) abstract;

        /**
        * Close an existing window  Return error if window does not exist
        * @param[in] name   Name of the window
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT CloseInternalWindow(LPCWSTR name) abstract;

        /**
        * This will return the window currently being rendered or updated.  If no windows are being
        * rendered, or updated, this will return the active 3d window.  If no 3d view windows are active,
        * this will return the default 3d view.
        * @return Current window pointer.  Will return nullptr if it request fails.
        **/
        virtual IWindowV400* GetCurrentWindow() abstract;

        /**
        * This function is used to create a new camera instance on the given object based on the given camera definition.
        * @param[in]    guidCameraDefinition    The guid of the camera definition to be used to create the camera.
        * @param[in]    pszName                 The name of the camera.
        * @param[in]    uObjectId               The id of the object that this camera instance will be associated with.
        * @param[out]   uInstanceId             The instance id of the created camera.
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails.
        **/
        virtual HRESULT CreateCameraInstance(const GUID& guidCameraDefinition, LPCWSTR pszName, UINT uObjectId, UINT& uInstanceId) abstract;

        /**
        * This function is used to create a new camera instance at the given location based on the given camera definition.
        * @param[in]    guidCameraDefinition    The guid of the camera definition to be used to create the camera.
        * @param[in]    pszName                 The name of the camera.
        * @param[in]    dLatDegrees             The latitude of the camera in degrees.
        * @param[in]    dLonDegrees             The longitude of the camera in degrees.
        * @param[in]    dAltMeters              The altitude of the camera in meters.
        * @param[out]   uInstanceId             The instance id of the created camera.
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails.
        **/
        virtual HRESULT CreateCameraInstance(const GUID& guidCameraDefinition, LPCWSTR pszName, double dLatDegrees, double dLonDegrees, double dAltMeters, UINT& uInstanceId) abstract;
        
        /**
        * This function is used to delete a camera instance.
        * @param[in]    guidCameraDefinition    The guid of the camera definition to be used to delete the camera instance.
        * @param[in]    uInstanceId             The instance id of the camera to be deleted.
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails.
        **/
        virtual HRESULT DeleteCameraInstance(const GUID& guidCameraDefinition, UINT uInstanceId) abstract;

        /**
        * This will return the default window.
        * @return default window pointer.  Will return nullptr if it request fails.
        **/
        virtual IWindowV400* GetDefaultWindow() abstract;
    };


    /**
    * Camera system interface that can be used to get or set camera states such as 
    * position, orientation, and field of view.  
    * @remark   The reader/writer interfaces usind in Prepar3D v2 and v3 have been
    * have been combined for ease of use and the camera and window functionality have
    * been put in separate interfaces. All 3d views will have a camera system.  Panel
    * windows do not have a camera system.
    **/
    DECLARE_INTERFACE_(ICameraSystemV610, ICameraSystemV600)
    {
        /**
        * Get the window's camera definition
        * @remark camera definitions are defined in Cameras.cfg or in aircraft.cfg
        **/
        virtual void GetCameraDefinition(WCHAR* cameraDefinition) abstract;

        /**
        * Set the camera position and orientation relative to its origin in 3D space
        * @param fDeltaX,fDeltaY,fDeltaZ         3D position relative to origin
        * @param fPitchDeg,fBankDeg,fHeadingDeg  Relative orientation pitch bank and heading
        **/
        virtual void SetRelative6DOF(float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg, float fBankDeg, float fHeadingDeg) abstract;

        /**
        * Change the relative position and orientation of camera using a smooth transition
        * @param fDeltaX,fDeltaY,fDeltaZ         3D position relative to origin
        * @param fPitchDeg,fBankDeg,fHeadingDeg  Relative orientation pitch bank and heading
        * @param smoothPanTime                   Time-frame for the camera to pan to the new position
        **/
        virtual void SetSmoothRelative6DOF(float fDeltaX, float fDeltaY, float fDeltaZ, float fPitchDeg, float fBankDeg, float fHeadingDeg, float smoothPanTime = 0) abstract;

        /**
        * Set the field of view of the camera
        * @param hFov   Horizontal field of view
        * @param vFov   Vertical field of view
        **/
        virtual void SetFov(float hFov, float vFov) abstract;

        /**
        * Set the Zoom goal for the camera
        * @param zoomGoal   Zoom goal to set
        **/
        virtual void SetZoomGoal(float zoomGoal) abstract;

        /**
        * Set the Zoom
        * @param zoom
        **/
        virtual void SetZoom(float zoom) abstract;

        /**
        * Pan the camera to a saved location
        * @param name of saved custom camera to pan to
        **/
        virtual void PanToView(const WCHAR* name) abstract;

        /**
        * Add a post process effect to the window
        * @param  name   Name of post process to add
        **/
        virtual void AddPostProcess(const WCHAR* name) abstract;

        /**
        * Remove a post process effect to the window
        * @param  name  Name of post process to remove
        **/
        virtual void RemovePostProcess(const WCHAR* name) abstract;

        /**
        * Reset all post process effects to those defined in camera definition
        **/
        virtual void ResetPostProcess() abstract;

        /**
        * Clear all post process effects from the camera
        **/
        virtual void ClearPostProcess() abstract;

        /**
        * Reset camera settings to those defined in camera definition
        **/
        virtual void Reset() abstract;

        /**
        * Zoom in one zoom level increment
        **/
        virtual void ZoomIn(void) abstract;

        /**
        * Zoom out one zoom level increment
        **/
        virtual void ZoomOut(void) abstract;

        /**
        * Sets a cockpit transparency level
        * @param level value between [0-100] to indicate the transparent level. 100 is Fully transparent.
        **/
        virtual void SetVirtualCockpitTransparentValue(unsigned int level) abstract;

        /**
        * Sets the chase distance of a camera
        * @param fDistance Chase distance in meters
        **/
        virtual void SetChaseDistance(float fDistance) abstract;

        /**
        * Sets the chase altitude of a camera
        * @param fAltitude Chase altitude in meters
        **/
        virtual void SetChaseAltitude(float fAltitude) abstract;

        /**
        * Sets the sensor mode of a camera
        * @param mode Sensor mode (0: None, 1: IR White Hot, 2: IR Black Hot, 3: GData
        **/
        virtual void SetSensorMode(unsigned int mode) abstract;

        /**
        * Excludes entire VC from being post processed (if post process has PreVC enabled)
        * @param exclude boolean value to set exclusion state
        **/
        virtual void SetExcludeVcPostProcess(bool exclude) abstract;

        /**
        * Excludes VC panels from being sensorized
        * @param exclude boolean value to set exclusion state
        **/
        virtual void SetExcludeVcPanelsSensor(bool exclude) abstract;

        /**
        * Set the far clip.
        * @param far Far clip distance in meters
        **/
        virtual void SetFarClip(float far) abstract;

        /**
        * Set the near clip.
        * @param near Near clip distance in meters
        **/
        virtual void SetNearClip(float near) abstract;

        /**
        * Set the LLA of where the center of the terrain LOD radius for this camera will go.  
        * @param lat Latitude of LOD ring center in radians
        * @param lon Longitude of LOD ring center in radians
        * @param alt Altitude of LOD ring center in meters
        **/
        virtual void SetTerrainLODOriginLLA(double lat, double lon, double alt) abstract;

        /**
        * Set the LLA of where the center of the scenery LOD radius for this camera will go.  This determines
        * where models and builds load and display.
        * @param lat Latitude of LOD ring center in radians
        * @param lon Longitude of LOD ring center in radians
        * @param alt Altitude of LOD ring center in meters
        **/
        virtual void SetSceneryLODOriginLLA(double lat, double lon, double alt) abstract;

        /**
        * Set the a scalar to amplify all light, and or a boost to the base ambient level. defaults are 1.0 and 0.0
        * @param ambientBoost boost to base ambient level
        * @param lightAmplificationLevel scalar multiplied into all light sources
        **/
        virtual void SetAmbientBoostAndLightAmplificationLevel(const float ambientBoost, const float lightAmplificationLevel) abstract;

        /**
        * Set the target frame rate, a rate of 0 is unlimited
        * @param targetFrameRate target frame rate in seconds
        **/
        virtual void SetTargetFrameRate(const float targetFrameRate) abstract;

        /**
        * Request a Pick test at a given screen coordinate.  An ID is returned which can later be used to request the results of the test.
        * @param x  x screen coordinate
        * @param y  y screen coordinate
        * @return ID later used to request the pick result
        **/
        virtual int AddPickRequest(int x, int y) abstract;

        /**
        * Activate position tracking enabling camera to look at a specified LLA
        **/
        virtual void ActivatePositionTracking() abstract;

        /**
        * Deactivate position tracking
        **/
        virtual void DeactivatePositionTracking() abstract;

        /**
        * Activate entity tracking enabling camera to look at a specified entity
        **/
        virtual void ActivateEntityTracking() abstract;

        /**
        * Deactivate entity tracking
        **/
        virtual void DeactivateEntityTracking() abstract;

        /**
        * Set the target location for the camera to track.
        * @param lat Target latitude in degrees
        * @param lon Target longitude in degrees
        * @param alt Target altitude in feet
        **/
        virtual void SetTargetLatLonAltDegrees(double lat, double lon, double alt) abstract;

        /**
        * Set the target container ID for the camera to track.
        * @param containerId The ID of the container to track
        **/
        virtual void SetTargetContainerId(UINT containerId) abstract;

        /**
        * Set the pbh of the camera.
        * @param fPitchDeg Pitch in degrees
        * @param fPitchDeg Bank, in degrees
        * @param fPitchDeg Heading in degrees
        **/
        virtual void SetPBH(float fPitchDeg, float fBankDeg, float fHeadingDeg) abstract;

        /**
        * Set the location of the camera. Currently only applies to cameras with a Fixed Origin type.
        * @param dLat Latitude in degrees
        * @param dLon Longitude in degrees
        * @param dAlt Altitude in meters
        **/
        virtual void SetLLA(double dLat, double dLon, double dAlt) abstract;

        /**
        * Set the xyz offset of the camera.
        * @param fDeltaX X offset in meters
        * @param fDeltaY Y offset in meters
        * @param fDeltaZ Z offset in meters
        **/
        virtual void SetOffsetXYZ(float fDeltaX, float fDeltaY, float fDeltaZ) abstract;

        /**
        * Sets whether the camera will rotate independent of attached object
        * @param bGlobalRotate Whether or not the camera will rotate independent of attached object
        **/
        virtual void SetGlobalRotate(bool bGlobalRotate) abstract;

        /**
        * Sets the global rotation value of the camera
        * @param fPitchDeg The pitch
        * @param fBankDeg The bank
        * @param fHeadingDeg The heading
        **/
        virtual void SetGlobalPBH(float fPitchDeg, float fBankDeg, float fHeadingDeg) abstract;

        /**
        * Automatically sets the camera to target ground at center of view
        **/
        virtual void TargetCameraLookAt() abstract;

        /**
        * Set Side Angles for a potentially off-axis projection.  Positive direction is Right and Down.
        * @param fLeft Angle in radians from view direction to the left edge of camera's view
        * @param fRight Angle in radians from view direction to the right edge of camera's view
        * @param fTop Angle in radians from view direction to the top edge of camera's view
        * @param fBottom Angle in radians from view direction to bottom left edge of camera's view
        **/
        virtual void SetSideAngles(float fLeft, float fRight, float fTop, float fBottom) abstract;

        /**
        * Gets the Right Side Angle of the camera view
        **/
        virtual float GetRightAngle() abstract;

        /**
        * Gets the Left Side Angle of the camera view
        **/
        virtual float GetLeftAngle() abstract;

        /**
        * Gets the Bottom Side Angle of the camera view
        **/
        virtual float GetBottomAngle() abstract;

        /**
        * Gets the Top Side Angle of the camera view
        **/
        virtual float GetTopAngle() abstract;

        /**
        * Set the offset from the camera group origin for this camera
        * @param fOffsetX       Offset frustum from group origin in x direction relative to origin
        * @param fOffsetY       Offset frustum from group origin in y direction relative to origin
        * @param fOffsetZ       Offset frustum from group origin in z direction relative to origin
        **/
        virtual void SetFrustumOffsetXYZ(float fOffsetX, float fOffsetY, float fOffsetZ) abstract;

        /**
        * Set the offset PBH rotation relative to frustum group parent rotation
        * @param fOffsetP       Offset frustum Pitch relative to parent of group
        * @param fOffsetB       Offset frustum Bank relative to parent of group
        * @param fOffsetH       Offset frustum Heading relative to parent of group
        **/
        virtual void SetFrustumOffsetPBH(float fOffsetP, float fOffsetB, float fOffsetH) abstract;

        /**
        * Set the HMD mode for this camera
        * @param eMode the HDM mode to assign to this camera
        **/
        virtual void SetHmdMode(HMD_MODE eMode) abstract;

        /**
        * Get active View Group
        **/
        virtual int GetActiveViewGroup() abstract;

        /**
        * Get the field of view of the camera
        * @param[out] hFov   Horizontal field of view
        * @param[out] vFov   Vertical field of view
        **/
        virtual void GetFov(float& hFov, float& vFov) const abstract;

        /**
        * Get the Zoom
        * @param[out] fZoom   zoom value
        **/
        virtual void GetZoom(float& fZoom) const abstract;

        /**
        * Get the orientation of the camera (Pitch, Bank, and Heading)
        * @param[out] p Pitch
        * @param[out] b Bank
        * @param[out] h Heading
        **/
        virtual void GetPBH(float& p, float& b, float& h) const abstract;

        /**
        * Get the offset of the pbh relative to the origin
        * @param[out] p Pitch
        * @param[out] b Bank
        * @param[out] h Heading
        **/
        virtual void GetBiasPBH(float& p, float& b, float& h) const abstract;

        /**
        * Get the offset of the latitude longitude and altitude of the camera
        * @param[out] lat Latitude
        * @param[out] lon Longitude
        * @param[out] alt Altitude
        **/
        virtual void GetLLA(double& lat, double& lon, double& alt) const abstract;

        /**
        * Get the camera offset from the origin
        * @param[out] x,y,z   camera's 3d offset from origin
        **/
        virtual void GetCameraOffset(float& x, float& y, float& z) const abstract;

        /**
        * Get View Matrix for this Camera/Window
        * @param[out] output4x4  4x4 vidw matrix
        */
        virtual void GetViewMatrix(float output4x4[4][4]) const abstract;

        /**
        * Get the standard projection matrix which is used by the Camera/Window
        * for rendering everything but the virtual cockpit
        * @param[out] output4x4  4x4 vidw matrix
        */
        virtual void GetStandardProjectionMatrix(float output4x4[4][4]) const abstract;

        /**
        * Get the standard projection matrix which is used by the Camera/Window
        * for rendering the virtual cockpit.  The VC projection has a different
        * near and far clip to reduce z-fighting in the cockpit.
        * @param[out] output4x4  4x4 vidw matrix
        */
        virtual void GetVirtualCockpitProjectionMatrix(float output4x4[4][4]) const abstract;

        /**
        * Gets near clip distance in meters
        * @return near clip distance in meters
        **/
        virtual float GetNearClip() const abstract;

        /**
        * Gets far clip distance in meters
        * @return far clip distance in meters
        **/
        virtual float GetFarClip() const abstract;

        /**
        * Get hit result.  A pick request must first be requested using AddPickRequest.
        * The ID passed in should be the ID that was returned from the AddPickRequest call.
        * Passing in 0 will return the last result from the native mouse handling.
        * @param      ID        ID of the hit request.
        * @param[out] wasHit    true if hit test passed
        * @param[out] distance  distance to hit test intersection
        */
        virtual void GetPickResult(int ID, bool& wasHit, float& distanceToHit) const abstract;

        /**
        * Get hit result.  A pick request must first be requested using AddPickRequest.
        * The ID passed in should be the ID that was returned from the AddPickRequest call.
        * Passing in 0 will return the last result from the native mouse handling.
        * @param      ID        ID of the hit request.
        * @param[out] result    The hit result
        */
        virtual void GetPickResult(int ID, PickResult& result) const abstract;

        /**
        * Get the AvatarMode for the current window.
        * @return The AvatarMode of the current window. (0 = None, 1 = First, 2 = Third)
        */
        virtual int GetAvatarMode() const abstract;

        /**
        * Get the offset of the latitude longitude and altitude of the camera in radians
        * @param[out] lat Latitude
        * @param[out] lon Longitude
        * @param[out] alt Altitude
        **/
        virtual void GetLLARadians(double& lat, double& lon, double& alt) const abstract;

        /**
        * Get the orientation of the camera (Pitch, Bank, and Heading) in radians
        * @param[out] p Pitch
        * @param[out] b Bank
        * @param[out] h Heading
        **/
        virtual void GetPBHRadians(float& pitch, float& bank, float& heading) const abstract;

        /**
        * Get the target location for the camera to track.
        * @param[out] lat Target latitude in degrees
        * @param[out] lon Target longitude in degrees
        * @param[out] alt Target altitude in feet
        **/
        virtual void GetTargetLatLonAltDegrees(double& lat, double& lon, double& alt) const abstract;

        /**
        * Get the target container ID for the camera to track.
        * @param[out] containerId The ID of the container to track
        **/
        virtual void GetTargetContainerId(UINT32& containerId) const abstract;

        /**
        * Get the sensor mode of the camera.
        * @param[out] sensorMode The sensor mode of the camera
        **/
        virtual void GetSensorMode(UINT32& sensorMode) const abstract;

        /**
        * Gets the LLA the camera is looking at
        * @param[out] lat The target latitude
        * @param[out] lon The target longitude
        * @param[out] alt The target altitude
        *
        * @return If the LLA is valid
        **/
        virtual bool GetCameraLookAtLLA(double& lat, double& lon, double& alt) const abstract;

        /**
        * Gets if the camera is rotating relative to the world or vehicle.
        * @return Whether or not the camera is globally rotating
        **/
        virtual bool IsGlobalRotate() const abstract;

        /**
        * Gets the global rotation PBH of the camera
        * @param[out] p The pitch in degrees
        * @param[out] b The bank in degrees
        * @param[out] h The heading in degrees
        *
        **/
        virtual void GetGlobalPBH(float &p, float &b, float &h) const abstract;

        /**
        * Gets the target latitude in string format
        * @param[out] targetLat Latitude
        *
        **/
        virtual void GetTargetLatString(WCHAR* targetLat) const abstract;

        /**
        * Gets the target longitude in string format
        * @param[out] targetLon Longitude
        *
        **/
        virtual void GetTargetLonString(WCHAR* targetLon) const abstract;

        /**
        * Get the texture coords that the pick hit on the texture in the specified window.
        * A pick request must first be requested using AddPickRequest
        * on a WindowPdk interface from within a window callback.  The ID passed in should
        * be the ID that was returned from the AddPickRequest call.
        *
        * @param      ID            ID of the hit request.
        * @param      windowName    Name of window texture resides within.
        * @param[out] wasHit        true if hit test passed
        * @param[out] u             u location in texture
        * @param[out] v             v location in texture
        * @param[out] distanceToHit distance to texture hit
        */
        virtual void GetPickTextureLocation(int ID, WCHAR* windowName, bool& wasHit, float& u, float& v, float& distanceToHit) const abstract;

        /**
        * Get the screen coord of a point LLA + xyz offset in world space (meters)
        * @param[in]  lla       Lat lon alt origin of local coordinate system
        * @param[in]  pbh       Orientation of local coordinate system
        * @param[in]  xyzOffset List of 3d offsets in local coordinates.
        * @param[out] xyOutput  List of screenspace position of the requested points
        * @param[in]  count     Number of points to request.  Must be at least as big as input array sizes.
        **/
        virtual void GetScreenCoord(const double lla[3],
            const float pbh[3],
            const float xyzOffset[][3],
            float xyOutput[][3],
            const int count) const abstract;

        /**
        * Get the screen space coordinates of a list of points relative to user sim object
        * @param[in]  xyzOffset  list of 3d offsets in local coordinates.
        * @param[out] xyOutput   list of screenspace position of the requested points
        * @param[in]  count      Number of points to request.  Must be at least as big as input array sizes.
        **/
        virtual void GetScreenCoord(const float xyzOffset[][3],
            float xyOutput[][3],
            const int count) const abstract;

        /**
        * Get the screen space coordinates of a point at the given LLA
        * @param[in]  lla          world point to query 
        * @param[out] screenCoord  ScreenCoord of output
        **/
        virtual void GetScreenCoord(const LLADegreesMeters& lla, ScreenCoord& screenCoord) const abstract;
        
        /**
        * Get the world lla coordinates of a point given a ScreenCoord
        * @param[in]  screenCoord    ScreenCoord point to query
        * @param[out] lla            LLA  output
        **/
        virtual void GetWorldCoord(const ScreenCoord& screenCoord, LLADegreesMeters& lla) const abstract;

        /**
        * Get transparency level
        * @return value between [0-100] to indicate the transparency level. 100 is fully transparent
        **/
        virtual unsigned int GetVirtualCockpitTransparentValue() const abstract;

        /**
        * Gets the chase distance in meters
        * @return Chase distance in meters
        **/
        virtual float GetChaseDistance() const abstract;

        /**
        * Gets the chase altitude in meters
        * @return Chase altitude in meters
        **/
        virtual float GetChaseAltitude() const abstract;

        /**
        * Gets the target frame rate in seconds
        * @return Target frame rate in seconds
        **/
        virtual float GetTargetFrameRate() const abstract;

        /**
        * Gets list of the names of post processes assigned to this window
        * @param[out] nameList  array of cstrings to hold the list of names (size should be 32)
        * @param[out] size      of list returned (will not be larger than 32)
        **/
        virtual void GetPostProcesses(INameList& names) const abstract;

        /**
        * Check if camera is a virtual cockpit
        * @return true if window's active camera is a virtual cockpit view
        **/
        virtual bool IsVirtualCockpit(void) const abstract;

        /**
        * Check if camera is Top Down view
        * @return true if window's active camera is a Top Down view
        **/
        virtual bool IsTopDown(void) const abstract;

        /**
        * Get the requested zoom from the camera
        * @param[out] the requested zoom of the camera
        **/
        virtual void GetRequestedZoom(float& zoom) const abstract;

        /**
        * Check if camera is an HMD view
        * @return true if camera is an HDM view
        **/
        virtual bool IsHMDView() const abstract;

        /**
        * Get the world transform and transform reference for the camera
        * @param[out] world transform of the camera
        * @param[out] camera referece transform
        **/
        virtual void GetWorldTransform(ObjectWorldTransform& transform, CAMERA_TRANFORM_REFERENCE referece) const abstract;

        /**
        * Get the side angles of the camera view in degrees
        * @param[out] fLeft  left side angle
        * @param[out] fRight right side angle
        * @param[out] fTop top side angle
        * @param[out] fBottom  bottom side angle
        **/
        virtual void GetSideAngles(float& fLeft, float& fRight, float& fTop, float& fBottom) const abstract;

        /**
        * Gets whether or not the camera has crosshairs
        * @return true if camera is an HDM view
        **/
        virtual bool GetTargetCrosshairs() const abstract;

        /**
        * Gets whether or not the camera has a designator
        * @return true if camera has designator
        **/
        virtual bool GetRenderDesignators() const abstract;

        /*
        * Get tool tip text.  
        * @return The tool tip text. If no tool tip is showing, the string will be empty
        */
        virtual LPCWSTR GetToolTipText() const abstract;

        /*
        * Get the Ground Altitude of the Cameras current Lat and Lon
        * @return current ground altitude of camera
        */
        virtual float GetGroundAlt() abstract;

        /**
        * Sets whether the camera needs it's own terrain view or can use global
        * @param[in] useGlobalTerrainView whether or not the camera will use global terrain view
        **/
        virtual void SetUseGlobalTerrainView(bool useGlobalTerrainView) abstract;

        /**
        * Sets if the camera view should be inside a single pass group
        * @param[in] id for this group, set to 0 to remove single pass grouping
        * @param[in] drawOrder for this camera view, should be set left to right
        * @note Current limit of 2 camera views. DrawOrder example: Left eye view would be 0, right eye view would be 1
        * @note Needs to be configured on any type of camera add, remove or view change for the window
        **/
        virtual void SetSinglePassGroupID(unsigned int id, unsigned int drawOrder) abstract;

        /**
        * Gets the single pass groud id for the camera view
        * @return current single pass group ID, 0 means not assigned to a group
        **/
        virtual unsigned int GetSinglePassGroupID() const abstract;

        /*
        * Gets the camera origin
        * @return CAMERA_SYSTEM_ORIGIN_UNKNOWN if the origin was not found, otherwise returns the correct origin type.
        */
        virtual CAMERA_SYSTEM_ORIGIN GetCameraOrigin() const abstract;

        /*
        * Gets the camera category
        * @return CAMERA_SYSYEM_CATEGORY_UNKNOWN if the category was not found, otherwise returns the correct category type.
        */
        virtual CAMERA_SYSTEM_CATEGORY GetCameraCategory() const abstract;

        /**
        * Gets whether the mouse cursor is currently hidden
        * @return true if the mouse cursor is hidden, false if the mouse cursor is being shown
        */
        virtual bool MouseGetCursorIsHidden() abstract;

        /**
        * Resets the timer to hide the mouse cursor
        */
        virtual void ResetMouseCursorCountdown() abstract;
        
        /**
        * Returns the tooltip for an element intersected by a specific external pick request, based on pick ID
        * @return L"" if no valid tooltip, otherwise the tooltip of the element hit by the external pick request
        */
        virtual LPCWSTR GetToolTipText(int ePickID) const abstract;

        //----- V520

        /**
        * Set custom extents for an othogonal projeciton.  Not all visual elements work properly using an othogonal
        * projection.  This is experimental and is primarily provided for top down terrain rendering of maps.
        * @param fLeftMeters Distance in meters from view origin to the left edge of camera's view
        * @param fRightMeters Distance in meters from view origin to the left edge of camera's view
        * @param fTopMeters Distance in meters from view origin to the left edge of camera's view
        * @param fBottomMeters Distance in meters from view origin to the left edge of camera's view
        **/
        virtual void SetOrthogonalExtents(float fLeftMeters, float fRightMeters, float fBottomMeters, float fTopMeters) abstract;

        /**
        * Get custom extents for an othogonal projeciton.  Values will be 0 if the camera is not using orthogonal projection.
        * @param[out] fLeftMeters Distance in meters from view origin to the left edge of camera's view
        * @param[out] fRightMeters Distance in meters from view origin to the left edge of camera's view
        * @param[out] fTopMeters Distance in meters from view origin to the left edge of camera's view
        * @param[out] fBottomMeters Distance in meters from view origin to the left edge of camera's view
        **/
        virtual void GetOrthogonalExtents(float& fLeftMeters, float& fRightMeters, float& fBottomMeters, float& fTopMeters) const abstract;

        //----- V530

        /**
        * Get the pbh frustom offset of the camera relative to its origin after bias offsets have been applied
        * @param[out] p Pitch in degrees
        * @param[out] b Bank in degrees 
        * @param[out] h Heading in degrees
        **/
        virtual void GetFrustumOffsetXYZ(float& x, float& y, float& z) const abstract;

        /**
        * Get the xyz frustom offset of the camera relative to its origin after bias offsets have been applied
        * @param[out] p Pitch
        * @param[out] b Bank
        * @param[out] h Heading
        **/
        virtual void GetFrustumOffsetPBH(float& p, float& b, float& h) const abstract;

        //----- V600

        /**
        * Sets the aperture setting for this camera used during day, dawn, and dusk.
        * @param aperture
        **/
        virtual void SetApertureDay(float aperture) abstract;

        /**
        * Returns the aperture setting for this camera used during day, dawn, and dusk.
        * @return Daytime aperture setting
        */
        virtual float GetApertureDay() const abstract;

        /**
        * Sets the aperture setting for this camera used during night.
        * @param aperture
        **/
        virtual void SetApertureNight(float aperture) abstract;

        /**
        * Returns the aperture setting for this camera used during night.
        * @return Nighttime aperture setting
        */
        virtual float GetApertureNight() const abstract;

        /**
        * Sets the shutter speed setting for this camera used during day, dawn, and dusk.
        * @param shutterSpeed
        **/
        virtual void SetShutterSpeedDay(float shutterSpeed) abstract;

        /**
        * Returns the shutter speed setting for this camera used during day, dawn, and dusk.
        * @return Daytime shutter speed setting
        */
        virtual float GetShutterSpeedDay() const abstract;

        /**
        * Sets the shutter speed setting for this camera used during night.
        * @param shutterSpeed
        **/
        virtual void SetShutterSpeedNight(float shutterSpeed) abstract;

        /**
        * Returns the shutter speed setting for this camera used night.
        * @return Nighttime shutter speed setting
        */
        virtual float GetShutterSpeedNight() const abstract;

        /**
        * Sets the ISO setting for this camera used during day, dawn, and dusk.
        * @param iso
        **/
        virtual void SetISODay(float iso) abstract;

        /**
        * Returns the ISO setting for this camera used during day, dawn, and dusk.
        * @return Daytime ISO setting
        */
        virtual float GetISODay() const abstract;

        /**
        * Sets the ISO setting for this camera used during night.
        * @param iso
        **/
        virtual void SetISONight(float iso) abstract;

        /**
        * Returns the ISO setting for this camera used night.
        * @return Nighttime ISO setting
        */
        virtual float GetISONight() const abstract;

        /**
        * Sets the auto exposure setting for this camera.
        * @param autoExposure
        **/
        virtual void SetAutoExposure(AUTO_EXPOSURE autoExposure) abstract;

        /**
        * Returns the auto exposure setting for this camera.
        * @return The auto exposure setting
        */
        virtual AUTO_EXPOSURE GetAutoExposure() const abstract;

        /**
         * Request a Pick test at a given screen coordinate.  An ID is returned which can later be used to request the results of the test.
         * @param x  x screen coordinate
         * @param y  y screen coordinate
         * @param mouseEvent WM_* Mouse Event Flags (see win32 system defined messages)
         * @return ID later used to request the pick result
         **/
        virtual int AddPickRequest(int x, int y, UINT mouseEvent) abstract;

        /**
        * Enable or disable pre-multiplied alpha.  Note that the final alpha value is only written by special
        * materials used for mixed reality portals.  Some XR cameras require that alpha be pre-multiplied so that
        * the color fades to black.
        * @param bEnable
        **/
        virtual void SetPreMultiplyFinalAlpha(bool bEnable) abstract;

        /**
        * Returns pre-multiplied alpha setting.  Note that the final alpha value is only written by special
        * materials used for mixed reality portals.  Some XR cameras require that alpha be pre-multiplied so that
        * the color fades to black.
        * @return true is pre-multiply alpha is enabled.  False if not. 
        **/
        virtual bool GetPreMultiplyFinalAlpha() const abstract;

        //----- V610

        /**
        * Sets the sensor size for this camera
        * @param sensorSize size of the sensor in mm
        **/
        virtual void SetSensorSize(float sensorSize) abstract;

        /**
        * Returns the sensor size setting for this camera.
        * @return The sensor size setting in mm
        **/
        virtual float GetSensorSize() const abstract;

        /**
        * Sets the depth of field focus distance for this camera
        * @param focusDistance focus distance of the camera in meters
        **/
        virtual void SetDoFFocusDistance(float focusDistance) abstract;

        /**
        * Returns the depth of field focus distance setting for this camera
        * @return the depth of field focus distance setting in meters
        **/
        virtual float GetDoFFocusDistance() const abstract;

        /**
        * Sets the depth of field blur size limit settings for this camera
        * @param blurSizeLimit blur size limit as a scale of y resolution
        **/
        virtual void SetDoFBlurSizeLimit(float blurSizeLimit) abstract;

        /**
        * Returns the depth of field blur size limit setting for this camera
        * @return depth of field blur size limit as a scale of y resolution
        **/
        virtual float GetDoFBlurSizeLimit() const abstract;

        /**
        * Set custom mask properties for the DoF effect
        * @param maskOffsetX u offset from the center of the screen
        * @param maskOffsetY v offset from the center of the screen
        * @param maskRadius radius of the mask in screen coordinates
        * @param maskFadeSize fade size of the mask in screen coordinates
        **/
        virtual void SetDoFMask(float maskOffsetX, float maskOffsetY, float maskRadius, float maskFadeSize) abstract;

        /**
        * Get custom mask properties for the DoF effect
        * @param[out] maskOffsetX u offset from the center of the screen
        * @param[out] maskOffsetY v offset from the center of the screen
        * @param[out] maskRadius radius of the mask in screen coordinates
        * @param[out] maskFadeSize fade size of the mask in screen coordinates
        **/
        virtual void GetDoFMask(float& maskOffsetX, float& maskOffsetY, float& maskRadius, float& maskFadeSize) const abstract;

        /**
        * Sets the depth of field setting for this camera.
        * @param dof enum state of depth of field
        **/
        virtual void SetDepthOfField(DEPTH_OF_FIELD dof) abstract;

        /**
        * Returns the depth of field setting for this camera.
        * @return The depth of field setting
        */
        virtual DEPTH_OF_FIELD GetDepthOfField() const abstract;

        /**
        * Get ambient light boost scalar
        * @return ambientBoost scalar used to boost base ambient level
        **/
        virtual float GetAmbientBoost() const abstract;

        /**
        * Get light amplication level
        * @return LightAmplificationLevel scalar which is multiplied into all light sources
        **/
        virtual float GetLightAmplificationLevel() const abstract;

    };


    /**
     * Window writer interface for setting window states
     **/
    DECLARE_INTERFACE_(IWindowV610, IWindowV510)
    {
        /**
         * Set Docked/Undocked state
         * @param isDocked   Set TRUE to dock or FALSE to undock a window
         **/
        virtual void SetDocking(BOOL isDocked) abstract;

        /**
        * Get the position of a window in pixels
        * @param width,height   Window size in pxiels
        **/
        virtual void GetPosition(UINT32 & topLeftX, UINT32 &topLeftY) abstract;

        /**
         * Set the position of the window in screen space
         * @param x,y    Screenspace position of window
         **/
        virtual void SetPosition(UINT32 x, UINT32 y) abstract;

        /**
         * Set the size of a window in pixels
         * @param width,height   Window size in pxiels
         **/
        virtual void SetSize(UINT32 w, UINT32 h) abstract;

        /**
        * Set this window to panel only mode
        * @param bPanelOnly panel-only mode on or off
        **/
        virtual void SetPanelOnly(bool bPanelOnly) abstract;

        /**
        * Get the size of a window in pixels
        * @param width,height   Window size in pxiels
        **/
        virtual void GetSize(UINT32& width, UINT32& height) abstract;

        /**
        * Set the size render target resolution in pixels. This can be used to super-sample or
        * subsample on-screen views by setting the resolution different than the window size.
        * @param width,height   Window size in pixels
        **/
        virtual void SetResolution(UINT32 width, UINT32 height) abstract;

        /**
        * Send a message to the window.
        * @param uMsg   The message for the window
        * @param wParam w param of the message
        * @param lParam l param of the message
        **/
        virtual void SendWindowMessage(UINT uMsg, long wParam, long lParam) abstract;

        /**
        * Add a plugin to the window.
        * @param  pPlugin the plguin to add
        **/
        virtual void AddPlugin(IWindowPluginV400* pPlugin) abstract;

        /**
        * Remove a plugin from the window.
        * @param  pPlugin the plguin to remove
        **/
        virtual void RemovePlugin(IWindowPluginV400* pPlugin) abstract;

        /**
        * Get window's docking state
        * @return bool value of true if window is docked and false if not
        **/
        virtual bool IsDocked() const abstract;

        /**
        * Get the parent window
        * @return  IWindowV400 parent to this window or nullptr if none exists.
        **/
        virtual IWindowV400* GetParentWindow() const abstract;

        /**
        * Get window's panel state
        * @param[out] pPanelIdent   optional panel id if this window is a panel
        * @return bool value of true if window is a panel and false if not
        **/
        virtual bool IsPanelWindow(UINT32* pPanelIdent = nullptr) const abstract;

        /**
        * Get the position of the window in screen space
        * @param[out] topLeftX,topLeftY   position of the window in screen space
        **/
        virtual void GetPosition(UINT32& topLeftX, UINT32& topLeftY) const abstract;

        /**
        * Get the size of the window in screen space
        * @param[out] width,height  size of the window in screen space
        **/
        virtual void GetSize(UINT32& width, UINT32& height) const abstract;

        /**
        * Get the window name
        * @return window name
        **/
        virtual const LPCWSTR GetWindowName(void) const abstract;

        /**
        * Check if window's active camera is in focus
        * @return true if window's active camera is in focus
        **/
        virtual bool IsActiveWindow(void) const abstract;

        /**
        Is this window the main Application Window?
        @return true if this window is the main window
        */
        virtual bool IsMainAppWindow() const abstract;

        /**
        * Get the screen coords for the given client coords.
        * @param[in/out] x          Client x coordinate
        * @param[in/out] y          Client y coordinate
        * @return If the function executed successfully
        */
        virtual bool GetClientToScreen(long& x, long& y) const abstract;

        /**
        * Get camera system attached to this window.
        * @return  Camera System attached to this window or nullptr if none exists.
        */
        virtual ICameraSystemV400* GetCameraSystem() abstract;

        /**
        * Get previous camera system attached to this window.  This can be useful when handling a view 
        * changes triggered by the user or by calls to SetCameraDefinition.
        * @return  Camera System previously attached to this window or nullptr if none exists.
        */
        virtual ICameraSystemV400* GetPreviousCameraSystem() abstract;

        /**
        * Set the window to use the specified camera.  This will get or create an instance of a camera
        * using the specified camera defintion name.
        * @param name    Name of camera definition
        * @remark camera definitions are defined in Cameras.cfg or in aircraft.cfg.
        * This will cause the camera to change, so calling GetCameraSystem()
        * window will return a different value.  The current camera instance will still be
        * maintained until the window that created it has been closed, and it can be accessed by
        * calling GetPreviousCameraSystem after the changed.
        **/
        virtual void SetCameraDefinition(const WCHAR* name) abstract;

        /**
        * Setup a world space draw object for the window, the object is a double sided plane. 
        * This is useful for displaying windows in world space locations.
        * @param location   World location to place the draw object.
        * @param width,height   World plane size in meters.
        * @return  If the creation of the world space draw object was successful.
        */
        virtual bool InitWorldSpaceDraw(const ObjectWorldTransform& location, float width, float height) abstract;

        /**
        * Shutdown the world space draw object, this will cause the window to no longer draw in world space.
        */
        virtual void DeinitWorldSpaceDraw() abstract;

        /**
        * Set the world space location of the draw object if one exist for this window.
        * @param location   World location to place the draw object.
        */
        virtual void SetWorldSpacePosition(const ObjectWorldTransform& location) abstract;

        /**
        * Get the world space location of the draw object if one exist for this window.
        * @param location   World location to store the world space location.
        */
        virtual void GetWorldSpacePosition(ObjectWorldTransform& location) abstract;

        /**
        * Set the world space size of the draw object if one exist for this window.
        * @param width,height   World plane size in meters.
        */
        virtual void SetWorldSpaceSize(UINT32 width, UINT32 height) abstract;

        /**
        * Set the world space size of the draw object if one exist for this window.
        * @param width,height   World plane size in meters.
        */
        virtual void SetWorldSpaceSize(float width, float height) abstract;

        /**
        * Set the world space size of the draw object if one exist for this window.
        * @param pitch,bank,heading   Flag for setting facing status.
        */
        virtual void SetWorldSpaceFacingFlags(bool pitch, bool bank, bool heading) abstract;

        /**
        * Set the world space depth mode of the draw object if one exist for this window.
        * @param readDepth,writeDepth   Flag for setting depth read write status.
        */
        virtual void SetWorldSpaceDepthMode(bool readDepth, bool writeDepth) abstract;

        /**
        * Get the local transfrom of the object used to calculate the world space offset
        */
        virtual void GetWorldspaceOffset(ObjectLocalTransform& location) abstract;

        /**
        * Set the local transfrom of the object used to calculate the world space offset
        */
        virtual void SetWorldspaceOffset(ObjectLocalTransform location) abstract;

        /**
        * Get the size of the world space draw object.
        * @param[in/out] x          Draw Objects Size X
        * @param[in/out] y          Draw Objects Size Y
        */
        virtual void GetWorldSpaceSize(float& x, float& y) abstract;

        /**
        * Get the reference the world space draw object uses to offset its position and PBH
        */
        virtual void GetWorldspaceOffsetReference(CAMERA_TRANFORM_REFERENCE& reference) abstract;

        /**
        * Set the reference the world space draw object uses to offset its position and PBH
        */
        virtual void SetWorldspaceOffsetReference(CAMERA_TRANFORM_REFERENCE reference) abstract;

        /**
        * Enable Variable Rate Shading.  Prepar3D will create a shading rate image which can be
        * modified using D3D12 in a rendering plugin.
        */
        virtual void EnableVariableRateShading(bool bEnable) abstract;

        /**
        * Check if variable rate shading is enabled. 
        */
        virtual bool IsVariableRateShadingEnabled() abstract;

        /**
        * Get the render target resolution in pixels.
        * @param[in/out] width Window size in pixels
        * @param[in/out] height Window size in pixels
        **/
        virtual void GetResolution(UINT32& width, UINT32& height) abstract;
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs

    interface __declspec(uuid("{0740820A-C0CC-4BCF-9876-BE937ED8EE23}")) IWindowPluginSystemV440;
    extern __declspec(selectany) REFIID  IID_IWindowPluginSystemV440 = __uuidof(IWindowPluginSystemV440);

    interface __declspec(uuid("{ea4acd0b-09b6-47ac-a4a0-b7bbeecce371}")) IWindowPluginV400;
    extern __declspec(selectany) REFIID  IID_IWindowPluginV400 = __uuidof(IWindowPluginV400);
  
    interface __declspec(uuid("{82D6A7F9-E48C-4DBD-B84E-F5DC61C90689}")) IWindowV610;
    extern __declspec(selectany) REFIID  IID_IWindowV610 = __uuidof(IWindowV610);

    interface __declspec(uuid("{3A7840E7-5E60-4620-8921-EBA46606692B}")) ICameraSystemV610;
    extern __declspec(selectany) REFIID IID_ICameraSystemV610 = __uuidof(ICameraSystemV610);

    extern __declspec(selectany) REFIID  SID_WindowPluginSystem = __uuidof(IWindowPluginSystemV400);

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID  IID_IWindowPluginSystem = IID_IWindowPluginSystemV440;

    extern __declspec(selectany) REFIID  IID_IWindowPlugin = IID_IWindowPluginV400;

    extern __declspec(selectany) REFIID  IID_ICameraSystem = IID_ICameraSystemV600;

    extern __declspec(selectany) REFIID  IID_IWindow = IID_IWindowV440;


    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    typedef IWindowPluginSystemV440     IWindowPluginSystem;
    typedef IWindowPluginV400           IWindowPlugin;
    typedef IWindowV610                 IWindow;
    typedef ICameraSystemV610           ICameraSystem;

    /** @} */ // end of group
}
