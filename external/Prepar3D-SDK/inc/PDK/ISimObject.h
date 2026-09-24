// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ISimObject.h
// Provides public interfaces for dll implementations
//-----------------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include "Legacy\ISimObject.Legacy.h"  //Include previous version
#include "IScenarioManagerService.h"

/** @defgroup isim ISimObject
*   @{
* The SimObject API utilizes a service-based methodology for building simulation behaviors to be visualized in Prepar3D. The API enables a solution developer to
* create a simulation object (SimObject) complete with customized behaviors, input properties (also referred to as events or triggers), and state properties (also
* referred to as simvars or simply properties). These properties can be referenced in content such as SimObject gauges, animations, and scenario scripts which are
* discussed in more detail in other parts of the SDK. The properties are text-based, and can be referenced in the same way as the stock simvars and events are referred
* to in other parts of the Prepar3D SDK.
*
* ## Types.h
* _______________________________________________
* Includes common data types found throughout the ISimObject Samples.\n\n
*
* PSaveLoadCallback() - Function pointer for state save/load (.FXML files). This pointer is passed to each ISimulation when it's time for save/load Section names will automatically be constructed as: [SectionName.Instance.SimObjectID] allowing for multiple instances of a system (e.g. multi-engines) Constructed section names are limited to a maximum of 128 characters. SIM_DATA_TYPE allows saving either numeric or string data.
*
* @code
* typedef HRESULT (STDMETHODCALLTYPE *PSaveLoadCallback)(__in LPCTSTR pszSection, __in unsigned int uInstance, __in LPCTSTR pszKeyword, __inout void* pvVal, __in const SAVED_DATA_TYPE eDataType);\n\n
* @endcode
*
* The property type enum used in the property string->ID lookup. See IBaseObject.
*
*    @code
*    typedef enum
*    {
*       PROPERTY_TYPE_EVENT,
*       PROPERTY_TYPE_EVENT_WITH_SUBSTRING_INPUT,
*       PROPERTY_TYPE_EVENT_VECTOR,
*       PROPERTY_TYPE_EVENT_STRING,
*       PROPERTY_TYPE_DOUBLE,
*       PROPERTY_TYPE_DOUBLE_WITH_SUBSTRING_INPUT,
*       PROPERTY_TYPE_STRING,
*       PROPERTY_TYPE_VECTOR,
*       NUM_PROPERTY_TYPES,
*    }  PROPERTY_TYPE;
*    @endcode
*
* Enum definition for data types used when saving / loading. See ISimulation and PSaveLoadCallback.\n\n
*
*   @code
*   typedef enum
*   {
*       SAVED_DATA_TYPE_DOUBLE,
*       SAVED_DATA_TYPE_STRING,
*       NUM_SAVED_DATA_TYPES
*   }   SAVED_DATA_TYPE;
*   @endcode
*
* Enum definition for the various modes in which an Artificially Intelligent (AI) object can be.\n\n
*
* @code
* typedef enum
* {
*     UNITMODE_SLEEP,
*     UNITMODE_ZOMBIE,
*     UNITMODE_WAYPOINT,
*     UNITMODE_TAKEOFF,
*     UNITMODE_LANDING,
*     UNITMODE_TAXI,
*     UNITMODE_WORKING,
*     UNITMODE_WAITING,
* }   UNITMODE_TYPE;
* @endcode
*
* A BasicWaypoint is generally used to define a point along a path. It is used primarily by the AI system.\n\n
*
* @code
* class BasicWaypoint
* {
* public:
*     DXYZ vLonAltLat; //Lat/Lon (Radians), Alt (Feet)
*     double dHeading; //Radians
* };
* @endcode
*
* Enum definition for various network modes. See Object Mode Monitoring.
*
* @code
* typedef enum
* {
*     NET_MODE_TYPE_NORMAL,   // The object is owned by the current client
*     NET_MODE_TYPE_REMOTE,   // The object is owned by another client
*     NET_MODE_TYPE_MASTER,   // Shared Cockpit: The object is owned by the current client
*     NET_MODE_TYPE_SLAVE,    // Shared Cockpit: The object is owned by another client
* } NET_MODE_TYPE;
* @endcode
* \n
* ## Versioning Code
* _______________________________________________
* Compiled with the PDK and ISimObject interfaces will be expected to function in subsequent versions of the SDK.
* To do so, each interface name is appended with the version number, and is derived from the preceding version. The
* preceding versions will be maintained intact in the Legacy subfolder. It is recommended that all new code utilizes
* the latest version when defining your objects and each QueryInterface supports all versions. Internal to Prepar3D,
* the earliest version possible will be used.
*
*
*
*
* */

// Outside the P3D namespace.
// Must include NetInOutPublic.h to use.
interface NetInPublic;
interface NetOutPublic;

namespace P3D
{

interface IBaseObjectV400;
interface ISimObject;
interface ISimulation;
interface IForceElement;
interface IMassElement;

class WorldConstants;
class WeatherInfoV400;
class SurfaceInfoV400;


/** Handles tasks that are not associated with an instance of a simobject.  This includes:\n
* - Registration of implementation factories and associated properties\n
* - Global application properties (e.g. world constants)\n
* - Object queries */
DECLARE_INTERFACE_(ISimObjectManagerV540, ISimObjectManagerV520)
{
    /** Registers an ISimObject implementation at load time with: unique ID, friendly category name (e.g. "airplane"), and factory function pointer.
    * The "pszCategoryName" is a high-level categorization used primarily for UI (e.g. "airplane"). Mainly, it is used as a filter to exclude
    * objects from appearing in the Vehicle Select screen. If you create a unique category name, ensure you add the name to the User Objects key
    * in the Prepar3D.cfg's [Main] section. */
    STDMETHOD(RegisterSimulationCategory) (__in GUID guidCategory, __in LPCWSTR pszCategoryName, __in __notnull PSimCreateFunc pcbCreateFunction) PURE;

    /** Property "simvar" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallback       pcbProperty) PURE;
    /** Property "simvar" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Vector (DXYZ) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyVectorCallback pcbProperty) PURE;
    /** Property "simvar" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: String */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PPropertyStringCallback pcbProperty)                                   PURE;
    /** Property "simvar" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double (with secondary substring input) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallbackWithSubString    pcbProperty) PURE;
    /** Property "event" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallback          pcbEvent, __in EVENTTYPE eType)    PURE;
    /** Property "event" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event vector */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventVectorCallback)    PURE;
    /** Property "event" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event string */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PEventStringCallback)    PURE;
    /** Property "event" registration. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event double (with secondary substring input) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallbackWithSubString  pcbEvent)    PURE;

    /** Disables a specified PEventCallback registration from being triggered by a client machine in a Shared Cockpit scenario.  Example: disable a client joystick from interfering with the pilot flying**/
    STDMETHOD(DisableSharedCockpitClientInputEvent)(__in GUID guidCategory, __in LPCWSTR pszPropertyName)    PURE;

    /** World Constants are constant values describing the Earth atmosphere and geometry.\n
    * NOTE: While this is accessed through IBaseObject, these values will be constant for all SimObjects, and a single static copy could be shared across multiple instances. */
    STDMETHOD(GetWorldConstants)(__out WorldConstants&) const PURE;

    /** Decodes a string units to its integer ID. This can be useful to get at initialization
    * as it is less performanct to query properties using the string version. e.g. "feet per second" to ID. */
    STDMETHOD(GetUnitCode)(__in LPCWSTR pszPropertyUnits, __out int& iUnitCode) const PURE;

    /** Gets another IBaseObject ref for a given ID */
    STDMETHOD(GetObject)(__in UINT idObject, __out IBaseObjectV400** ppObject) const PURE;

    /** Gets another specific version of an IBaseObject ref for a given ID */
    STDMETHOD(GetObject)(__in UINT idObject, __in REFIID riid, __out void** ppvObject) const PURE;

    /** Gets an IBaseObject ref for the current user object.\n
    NOTE: If the user object is the Viewer and there is a previous user object, this will return the previous user object. Otherwise, it will return the Viewer. */
    STDMETHOD(GetUserObject)(__out IBaseObjectV400** ppUserObject) const PURE;

    /** Gets a specific version IBaseObject ref for the current user object.\n
    NOTE: If the user object is the Viewer and there is a previous user object, this will return the previous user object. Otherwise, it will return the Viewer. */
    STDMETHOD(GetUserObject)(__in REFIID riid, __out void** ppvUserObject) const PURE;

    /** Used to register a callback function that is called upon creation of any new object. See types.h for callback definition. */
    STDMETHOD(RegisterOnObjectCreateCallback) (__in __notnull POnObjectCreateCallback  pCb)    PURE;
    /** Used to unregister a callback function previously registered with RegisterOnObjectCreateCallback() */
    STDMETHOD(UnRegisterOnObjectCreateCallback) (__in __notnull POnObjectCreateCallback  pCb)    PURE;
    /** Used to register a callback function that is called upon destruction of any existing object. The call is just prior to destruction. See types.h for callback definition. */
    STDMETHOD(RegisterOnObjectRemoveCallback) (__in __notnull POnObjectRemoveCallback  pCb)    PURE;
    /** Used to unregister a callback function previously registered with RegisterOnObjectRemoveCallback() */
    STDMETHOD(UnRegisterOnObjectRemoveCallback) (__in __notnull POnObjectRemoveCallback  pCb)    PURE;
    /** Used to register a callback function that is called whenever the user is moved from one object to another. See types.h for callback definition. */
    STDMETHOD(RegisterOnUserObjectChangedCallback) (__in __notnull POnUserObjectChangedCallback  pCb)    PURE;
    /** Used to unregister a callback function previously registered with RegisterOnUserObjectChangedCallback() */
    STDMETHOD(UnRegisterOnUserObjectChangedCallback) (__in __notnull POnUserObjectChangedCallback  pCb)    PURE;

    /** Returns a list of object IDs for a given radius.\n
    * @param nObjects IN:  the max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs.\n
    * @param nObjects OUT: the actual number of objects found.\n
    * @param rgObjectIDs address of array in which object IDs are returned.\n
    * NOTE: It is the callers responsibility to allocate the array's required memory */
    STDMETHOD(GetObjectsInRadius)(__in const DXYZ& vLonAltLat, __in float fRadiusFeet, __inout UINT& nObjects, __out UINT* rgObjectIDs) const PURE;

    /** Returns a list of object IDs for a given radius.  Does not include traffic\n
    * @param nObjects IN:  The max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs\n
    * @param nObjects OUT: The actual number of objects found.\n
    * @param rgObjectIDs Address of array in which object IDs are returned.\n
    * NOTE: It is the callers responsibility to allocate the array's required memory. */
    STDMETHOD(GetNonTrafficObjectsInRadius)(__in const DXYZ& vLonAltLat, __in float fRadiusFeet, __inout UINT& nObjects, __out UINT* rgObjectIDs) const PURE;

    /**The user-selected general realism scalar, where 0.0 is "easy" and 1.0 is "real". This can be used to scale your implementation as appropriate*/
    STDMETHOD_(float, GetRealismSetting)()            const PURE;     //Percent
                                                                      /**The user-selected flag that dictates whether to process a crash or not*/
    STDMETHOD_(BOOL, IsCrashDetectionOn)()            const PURE;
    /**The user-selected flag for whether to detect crashes between simobjects or not*/
    STDMETHOD_(BOOL, IsCollisionBetweenObjectsOn)()   const PURE;
    /**The user-selected scalar for determining crash tolerance*/
    STDMETHOD_(float, GetCrashToleranceScalar)()      const PURE;

    /** RemoveObject - Remove any local non-user object
    * @param idObject The id of the object to remove. */
    STDMETHOD(RemoveObject)(__in UINT idObject) PURE;

    /** Attempts to create an object with the given container title.
    * @param pszTitle   The container title object to be created.
    * @param idObject   The object id of the newly created object.
    * @return           S_OK if the object was successfully created, E_FAIL otherwise.
    */
    STDMETHOD(CreateObject)(__in __notnull LPCWSTR pszTitle, __out UINT& idObject) PURE;

    /** Gets an IBaseObject ref for the current user avatar.
    * @param ppUserAvatar   The IBaseObject ref for the current user avatar.
    * @return               S_OK if the object was successfully found, E_FAIL otherwise.
    * @remarks              The user avatar and the user object may be the same if the user has selected an avatar object.
    */
    STDMETHOD(GetUserAvatar)(__out IBaseObjectV400** ppUserAvatar) const PURE;

    /** Gets an IBaseObject ref for the current user avatar.
    * @param ppvUserAvatar  The IBaseObject ref for the current user avatar.
    * @param riid           Interface ID.
    * @return               S_OK if the object was successfully found, E_FAIL otherwise.
    * @remarks              The user avatar and the user object may be the same if the user has selected an avatar object.
    */
    STDMETHOD(GetUserAvatar)(__in REFIID riid, __out void** ppvUserAvatar) const PURE;

    /** Gets the number of registered simulations.
    * @return       Number of registered simulation categories
    * @remarks      Note that if this queried at startup during DLL loading, some externally developed categories may not yet be registered.  Core Prepar3D native simulations will be registered by that time.
    */
    STDMETHOD_(UINT, GetNumberOfCategories)() const PURE;

    /** Gets the simulation category unique GUID and friendly string name.
    * @param    iIndex                      The unique (0-based) index into the list of registered categories. 
    * @param    guidCategoryId              The GUID id unique to this simulation category.
    * @param    pszCategoryFriendlyName     The friendly string name for the category.  These are guaranteed to be unique only for native simulations in core Prepar3D.  
    * @param    uNameLen                    The maximum length of the allocated string friendly name.
    * @param    bIsNativeSimulation         Indicates if this simulation is natively implemented in core Prepar3D (TRUE) or externally developed (FALSE).
    * @return   S_OK  if the category is successfully found.  E_INVALIDARG if the index exceeds the maximum index of the registered simulation list.
    * @remarks  Indexes are gauaranteed to remain unique for the lifetime of a Prepar3D instance.  
    */
    STDMETHOD(GetCategoryId)(__out GUID& guidCategoryId, __out __notnull LPWSTR pszCategoryFriendlyName, __in UINT uNameLen, __out BOOL& bIsNativeSimulation, __in UINT iIndex) const PURE;

    /** Attempts to set the user object to the given container title.
    * @param pszTitle   The container title object to be created.
    * @return           S_OK if the object was successfully created, E_FAIL otherwise.
    */
    STDMETHOD(ChangeUserObject)(__in __notnull LPCWSTR pszTitle) PURE;
};

/** Interface from which object implementations must derive. */
DECLARE_INTERFACE_(ISimObjectV440, ISimObjectV400)
{
    STDMETHOD(LoadConstantData)(__inout void** ppConstantData)     PURE;       /**< Where your object class should load data from the disk. The return data is cached for subsequent instances of this same object. */
    STDMETHOD(UnloadConstantData)(__inout void** ppConstantData)   PURE;       /**< Where your object class should unload data from the disk. The return data is cached for subsequent instances of this same object. */
    STDMETHOD(LoadDynamicData)()                                   PURE;       /**< Called on each object instance. This would be an appropriate place to create the object's runtime subsystems. */
    STDMETHOD(Init)()                                              PURE;       /**< An appropriate place to initialize data and establish references between subsystems. */
    STDMETHOD(DeInit)()                                            PURE;       /**< Can be used to release inter-system references prior to the object being destroyed. */
    STDMETHOD_(BOOL, SupportsLabels)()                      const  PURE;       /**< Defines if the SimObject will or will not support labels to be displayed. */
                                                                               /** Requests this SimObject to support labels. Return S_OK if the new setting is accepted. If not, return an error code, such as E_FAIL. This value should maintain by this
                                                                               * class and returned when requested by SupportsLabel(). You may choose for your class to not support labels. This setting will not override settings in the Traffic Settings. */
    STDMETHOD(SetSupportsLabels)(BOOL bOn)                         PURE;
    STDMETHOD_(void, OnModeChange)(int bfNewModes)                 PURE;       /**< Called upon change in modes (pause, slew, etc...) */
                                                                               /** Called whenever Prepar3D has changed the position of this object outside of its own simulation implementation.
                                                                               * Examples of this would be positioning from the User Interface, Slew Mode, or terrain resolution changing. */
    STDMETHOD_(void, OnPositionInit)()                             PURE;
    /** Called whenever Prepar3D has changed the speed of this object outside of its own simulation implementation. This would occur normally if positioning from the User Interface. */
    STDMETHOD_(void, OnSpeedInit)(float fSpeed)                    PURE;
    /**< Accessor to get the base object from the ISimObject.
    * NOTE: This previously required a return type of IBaseObjectV400**.  Existing implementations will downcast to void** automatically.  */
    STDMETHOD(QueryBaseObject)(REFIID riid, void** ppv)            PURE;

    /**  Provide the main simulation rate (Hz).  Typically the world position update rate. 
    * This assumes the simulation is registered at a constant simulation rate.  Beginning with V440 of this interface, a min and max rate can be used if desired,
    * so the following GetMainSimRate may give more accurate information.  */
    STDMETHOD(GetMainSimRate)(__out float& fSimRate)                              const PURE;

    /** Provide the minimum and maximum main simulation rate (Hz).  Typically the world position update rate. */
    STDMETHOD(GetMainMinMaxSimRates)(__out float& fMinSimRate, __out float& fMaxSimRate) const PURE;
    
    /** Sets minimum and maximum main simulation rate (Hz).  Typically the world position update rate. 
    * This is typically called if there is a desire to synchronize the rates of two or more objects.  For example an aircraft and an aircraft carrier, or to prevent perceived jitter
    * when viewing an object from a camera attached to another object. 
    * Note: It is the responsibility of the ISimObject developer to determine if the rates are appropriate for the implementation, and subsequently call IBaseObject::RegisterSimulation() (again)
    * with the new rates with the relevant ISimulations. */
    STDMETHOD(SetMainMinMaxSimRates)(__in  float  fMinSimRate, __in  float  fMaxSimRate)       PURE;
};

/** Interface to individual simulation subsystems. */
DECLARE_INTERFACE_(ISimulationV310, ISimulationV01)
{
    /** Called by Prepar3D at the iteration rate specified when this ISimulation interface is registered using RegisterSimulation() in the IBaseObject interface. */
    STDMETHOD(Update)(double dDeltaT)           PURE;

    /** Called when either saving or loading a Prepar3D scenario. The function pointer allows your code to save and load "name - value"
    * pairs in the Prepar3D .fxml file. See the definition for PSaveLoadCallback and the supported data type enum SAVED_DATA_TYPE. */
    STDMETHOD(SaveLoadState)(__in __notnull PSaveLoadCallback pfnCallback, __in const BOOL bSave) PURE;

    // Multiplayer serialization
    /** Only called when in an active multiplayer session. This function can be implemented to create network packets that are then broadcast
    * to other clients for this ISimulation instance. The NetOutPublic interface is defined in NetInOutPublic.h */
    STDMETHOD(Serialize)(__in NetOutPublic& netOut) PURE;
    /** Only called when in an active multiplayer session. This function can be implemented to deserialize network packets that have been sent
    * by other clients for this ISimulation instance. The NetInPublic interface is defined in NetInOutPublic.h. */
    STDMETHOD(Deserialize)(__in NetInPublic& netIn) PURE;
};

#define SIMOBJECT_MODE_PAUSE                    0x00000001
#define SIMOBJECT_MODE_SLEW                     0x00000002
#define SIMOBJECT_MODE_DISABLED                 0x00000004
#define SIMOBJECT_MODE_CRASH                    0x00000008
#define SIMOBJECT_MODE_FREEZE_POSITION          0x00000010
#define SIMOBJECT_MODE_FREEZE_ATTITUDE          0x00000020
#define SIMOBJECT_MODE_FREEZE_ALTITUDE          0x00000040
#define SIMOBJECT_MODE_FREEZE_LATLON            0x00000080
#define SIMOBJECT_MODE_SIMDIRECTOR              0x00000100
#define SIMOBJECT_MODE_SIMDIRECTOR_WORLD_EDITOR 0x00000200
#define SIMOBJECT_RECORDING_ACTIVE              0x00000400
#define SIMOBJECT_RECORDING_PLAYBACK            0x00000800
#define SIMOBJECT_INSTANT_REPLAY                0x00001000
#define SIMOBJECT_MODE_ATTACHED                 0x00002000
#define SIMOBJECT_MODE_DEACTIVATED              0x00004000
#define SIMOBJECT_MODE_INVISIBLE                0x00008000
#define SIMOBJECT_MODE_PREVIEW_WINDOW           0x00010000
#define SIMOBJECT_MODE_STAGING_OBJECT           0x00020000

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_(IBaseObjectV520, IBaseObjectV450)
{
    /** The ID of this object:  NOTE: "0" is an invalid id */
    STDMETHOD_(UINT, GetId)() const PURE;

    /** The guid ID of the object defined in an object file. NOTE: If the object is not spawned by a scenario, the return will be E_FAIL and the ID will be GUID_NULL. */
    STDMETHOD(GetMissionId)(__out GUID& guid) const PURE;
    /** Returns if the object is the user or not. */
    STDMETHOD_(BOOL, IsUser)() const PURE;

    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n
    * Note: Group association is arbitrary. It could be used for things like alliances or squadrons. Default is 0, which signifies a neutral grouping. */
    STDMETHOD_(UINT, GetObjectGroupAssociationId)()                      const PURE;
    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n
    * Note: Group association is arbitrary. It could be used for things like alliances or squadrons. Default is 0, which signifies a neutral grouping. */
    STDMETHOD_(void, SetObjectGroupAssociationId)(UINT uAssociationId)         PURE;

    /** Group ObjectFoeList Hosts a list of ID's that are considered foe's to the current entity\n */
    STDMETHOD_(BOOL, InObjectFoeList)(UINT id)                         const PURE;
    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n */
    STDMETHOD_(void, SetObjectFoeList)(UINT* uEnteredFoeID, UINT size)            PURE;

    /** Group ObjectFoeList Hosts a list of ID's that are considered friends's to the current entity\n */
    STDMETHOD_(BOOL, InObjectFriendList)(UINT id)                         const PURE;
    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n */
    STDMETHOD_(void, SetObjectFriendList)(UINT* uEnteredFriendID, UINT size)            PURE;

    /** Returns bitwise flags for the current modes of the SimObject. */
    STDMETHOD_(int, GetMode)() const PURE;
    /** Should be called when it is desired to put Prepar3D into "crash" mode. By default, the application will go through it's
    * crash cycle and reset. It is the developer's responsibility to program the behavior of the object when crash in crash mode. */
    STDMETHOD(SetCrashMode)(double dDeltaT)  PURE;

    /** Gets the current world relative position and velocity from the Prepar3D-side SimObject. This will provide the
    * valid state upon initialization, as well as when another system such as a UI element or slew changes the position.
    * @param vLonAltLat Longitude, altitude, latitude (radians)
    * @param vPHB Pitch, heading, bank (radians)
    * @param vLonAltLatVel Longitude, altitude, latitude velocity (feet / second)
    * @param vPHBVel Pitch, heading, bank velocity (radians / second) */
    STDMETHOD(GetPosition)(__out      DXYZ& vLonAltLat, __out      DXYZ& vPHB, __out      DXYZ& vLonAltLatVel, __out      DXYZ& vPHBVel) const PURE;
    /** Sets the current world relative position and velocity to the Prepar3D-side SimObject.
    * @param vLonAltLat Longitude, altitude, latitude (radians)
    * @param vPHB Pitch, heading, bank (radians)
    * @param vLonAltLatVel Longitude, altitude, latitude velocity (feet / second)
    * @param vPHBVel Pitch, heading, bank velocity (radians / second)
    * @param bIsOnGround Flag indicating if the object is on the ground. This is important during terrain updates.
    * @param dDeltaT The time between object updates used to track how much time has accumulated between camera frames */
    STDMETHOD(SetPosition)(__in const DXYZ& vLonAltLat, __in const DXYZ& vPHB, __in const DXYZ& vLonAltLatVel, __in const DXYZ& vPHBVel, __in BOOL bIsOnGround, __in double dDeltaT) PURE;
    /** Sets the current world relative position and velocity to the Prepar3D-side SimObject.
    All parameters are optional.  Any parameter set to NULL will be ignored, and current object values will be retained.
    * @param pvLonAltLat Longitude, altitude, latitude (radians)
    * @param pvPHB Pitch, heading, bank (radians)
    * @param pvLonAltLatVel Longitude, altitude, latitude velocity (feet / second)
    * @param pvPHBVel Pitch, heading, bank velocity (radians / second)
    * @param bSetOnGround Flag indicating if the object is to be set on the ground, in which case the on-ground height and pitch attitude will be set.  */
    STDMETHOD(InitPosition)(__in const DXYZ* pvLonAltLat, __in const DXYZ* pvPHB, __in const DXYZ* pvLonAltLatVel, __in const DXYZ* pvPHBVel, __in BOOL bSetOnGround) PURE;
    /** IsOnGround Returns the on-ground flag value currently in the core base object. This can be useful for determining
    * if the object has been placed on the ground through a non-simulated means such as the UI. */
    STDMETHOD_(BOOL, IsOnGround)() const PURE;

    /** Rotates a vector from the world frame of reference to the body frame of reference. */
    STDMETHOD(RotateWorldToBody)(__in const DXYZ& vWorld, __out DXYZ& vBody)  const PURE;
    /** Rotates a vector from the body frame of reference to the world frame of reference. */
    STDMETHOD(RotateBodyToWorld)(__in const DXYZ& vBody, __out DXYZ& vWorld) const PURE;

    /** Registers an ISimulation callback for real-time updates (discussed in Creating Behaviors.) ISimulation registration will
    * be locked after the ISimObject Init() function has been called. All ISimulation objects must be registered before this point.
    * @param pSimulation Address of simulation system
    * @param fRateHz Specified iteration rate */
    STDMETHOD(RegisterSimulation)(__in __notnull ISimulation* pSimulation, float fRateHz) PURE;

    /** Registers an ISimulation callback for real-time updates (discussed in Creating Behaviors.) ISimulation registration will
    * be locked after the ISimObject Init() function has been called. All ISimulation objects must be registered before this point.
    * @param pSimulation Address of simulation system
    * @param fMinRateHz Specified minimum iteration rate
    * @param fMaxRateHz Specified maximum iteration rate */
    STDMETHOD(RegisterSimulation)(__in __notnull ISimulation* pSimulation, float fMinRateHz, float fMaxRateHz) PURE;

    /**  Provides the minimum and maximum main simulation rate (Hz).  Typically the world position update rate. 
    * An ISimObject implementation will be called by this if it exists. */
    STDMETHOD(GetMainMinMaxSimRates)(__out float& fMinHz, __out float& fMaxHz) const PURE;
    /** Sets minimum and maximum main simulation rate (Hz).  Typically the world position update rate. 
    * This is typically called if there is a desire to synchronize the rates of two or more objects.  For example an aircraft and an aircraft carrier, or to prevent perceived jitter
    * when viewing an object from a camera attached to another object. 
    * An ISimObject implementation will be called by this if it exists. */
    STDMETHOD(SetMainMinMaxSimRates)(__in  float  fMinHz, __in  float  fMaxHz) PURE;
    
    /** Registers a service that can be queried for on this object. A service should be an IUnknown-derived object and registered with a unique GUID.
    * @param guidService Unique GUID to identify this service.
    * @param punkService Reference to an instance of this service. */
    STDMETHOD(RegisterService)(__in REFGUID guidService, __in __notnull IUnknown* punkService) PURE;
    /** Removes a service that has been register with RegisterService().
    * @param guidService Unique GUID to identify this service. */
    STDMETHOD(UnregisterService)(__in REFGUID guidService) PURE;

    STDMETHOD(GetPropertyCodeAndIndex)(__in PROPERTY_TYPE eType, __in LPCWSTR pszPropertyName, __out int& iPropertyCode, __inout int& iIndex) const PURE; /**< Get Properties */

    STDMETHOD(GetProperty)(__in int     iPropertyCode, __in int      iUnitCode, __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in int      iUnitCode, __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */

    STDMETHOD(GetProperty)(__in int     iPropertyCode, __in int      iUnitCode, __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in int      iUnitCode, __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */

    STDMETHOD(GetProperty)(__in int     iPropertyCode, __out LPWSTR pszProperty, __in UINT uLength, __in int index = 0) const PURE;   /**< Get Property - Strings */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __out LPWSTR pszProperty, __in UINT uLength, __in int index = 0) const PURE;   /**< Get Property - Strings */
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD(GetProperty)(__in int     iPropertyCode, __in LPCWSTR pszSecondarySubstring, __in int      iUnitCode, __out double& dProperty, __in int index = 0) const PURE;
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in int      iUnitCode, __out double& dProperty, __in int index = 0) const PURE;
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD(GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in LPCWSTR  pszUnitCode, __out double& dProperty, __in int index = 0) const PURE;

    STDMETHOD(TriggerProperty)(__in  int     iPropertyCode, __in int iUnitCode, __in double dData, __in  int  index) const PURE;  /**< Numeric trigger */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in int iUnitCode, __in double dData, __in  int  index) const PURE;  /**< Numeric trigger */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __in double dData, __in  int  index) const PURE;  /**< Numeric trigger */

    STDMETHOD(TriggerProperty)(__in  int     iPropertyCode, __in int iUnitCode, __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in int iUnitCode, __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */

    STDMETHOD(TriggerProperty)(__in int     iPropertyCode, __in LPCWSTR pszData, __in  int  index) const PURE;   /**< String strigger */
    STDMETHOD(TriggerProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszData, __in  int  index) const PURE;   /**< String strigger */
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD(TriggerProperty)(__in  int       iPropertyCode, __in LPCWSTR pszSecondarySubstring, __in int iUnitCode, __in double dData, __in  int  index) const PURE;
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in int iUnitCode, __in double dData, __in  int  index) const PURE;
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD(TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in LPCWSTR  pszUnitCode, __in double dData, __in  int  index) const PURE;

    /** Property "simvar" registration. For this specific object, associates: property string name, units, and callback pointer (defined above) 
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    @note Input: Double */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallback       pcbProperty)                      PURE;
    /** Property "event" registration. For this specific object, associates: property string name, units, and callback pointer (defined above).
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    *@note Input: Event */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallback          pcbEvent, __in EVENTTYPE eType)   PURE;
    /** Property "simvar" registration. For this specific object, associates: property string name, units, and callback pointer (defined above).
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    *@note Input: Vector (DXYZ) */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyVectorCallback pcbProperty)                      PURE;
    /** Property "event" registration. For this specific object, associates: property string name, units, and callback pointer (defined above).
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    *@note Input: Event vector */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventVectorCallback)                                     PURE;
    /** Property "simvar" registration. For this specific object, associates: property string name, units, and callback pointer (defined above).
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    @note Input: String */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in __notnull PPropertyStringCallback pcbProperty)                                                         PURE;
    /** Property "event" registration. For this specific object, associates: property string name, units, and callback pointer (defined above).
    * This differs from the RegisterProperty method in the ISimObjectManager interface which registers a property that exists for all instances 
    * associated with the given guid. Properties registered here, as well as their ID, exist only for the lifetime of this object. Otherwise 
    * they function the same when used for gauges, animations, and missions. Properties must also be declared in Properties.xml for the object. See the PDK samples.
    @note Input: Event string */
    STDMETHOD(RegisterProperty) (__in LPCWSTR pszPropertyName, __in __notnull PEventStringCallback)                                                                        PURE;

    // System malfunctions.  The successful handle (greater than 0) returned on registration is used to later reference health (0 - 1.0).\n
    // e.g. pszName would include index, such as Engine 1 */
    /** Registers a specific malfunction that can be set through the UI, scenarios, or missions.
    * @param guidMalfunction Unique malfunction ID
    * @param pszType UI Type. Choices: Instruments, Systems, Radios, Engines, Controls, Structural, Miscellaneous
    * @param pszBaseName Name used for mission file reference. Should be generic (no index), such as "Engine"
    * @param pszInstanceName Specific malfunction name for the UI, such as "Total Failure Engine 1"
    * @param nSubIndex Sub-index. For example, engine 0, 1, etc... */
    STDMETHOD_(HANDLE, RegisterSystemMalfunction)(__in REFGUID guidMalfunction, __in LPCWSTR pszType, __in LPCWSTR pszBaseName, __in LPCWSTR pszInstanceName, __in int nSubIndex) PURE;
    /** Returns the health percentage (0.0 - 1.0) for a given malfunction. */
    STDMETHOD_(float, GetSystemHealth)(HANDLE hSystem) const PURE;
    /** Sets the health percentage (0.0 - 1.0) for a given malfunction. */
    STDMETHOD(SetSystemHealth)(float fHealth, HANDLE hSystem) PURE;

    /** Apply damage points.  Positive points passed in will be decremented from current health points, to a limit of zero. */
    STDMETHOD(DecrementHealthPoints)(__in float fDamagePoints)      PURE;
    /** Retrieves the current health of the object. */
    STDMETHOD_(float, GetHealthPoints)()                      const PURE;
    /** Sets current health of the object. */
    STDMETHOD_(void, SetHealthPoints)(float fHealthPoints)          PURE;

    /** Provides current surface information for the requested offset from the model center. See the ISimObject.h for the definition of the SurfaceInfo data
    * structure. A return value of E_FAIL means that Prepar3D's terrain system failed to process the request properly. This could happen if it is not initialized fully.
    * @param SurfaceInfo Reference to local SurfaceInfo data structure
    * @param pvOffsetFeet The offset from model enter.A value of NULL will use the model's center */
    STDMETHOD(GetSurfaceInformation)(__out SurfaceInfoV400& SurfaceInfo, __in_opt const FXYZ* pvOffsetFeet) PURE;  //Pass in NULL for center of object
    /** Provides current surface elevation (above Mean Sea Level) for the requested offset from the model center. This will be more efficient than
    * GetSurfaceInformation when only the elevation is needed. A return value of E_FAIL means that Prepar3D's terrain system failed to process the
    * request properly. This could happen if it is not initialized fully.
    * @param fElevationFeet Reference to the elevation variable (Feet)
    * @param pvOffsetFeet The offset from model enter. A value of NULL will use the model's center */
    STDMETHOD(GetSurfaceElevation)(__out float& fElevationFeet, __in_opt const FXYZ* pvOffsetFeet) PURE;       //Pass in NULL for center of object 
    /** Provides current depth for the requested offset from the model center.
    * @param fDepthFeet Reference to the depth variable (Feet)
    * @param pvOffsetFeet The offset from model enter. A value of NULL will use the model's center */
    STDMETHOD(GetBathymetryElevation)(__out float& fDepthFeet, __in_opt const FXYZ* pvOffsetFeet) PURE;    //Pass in NULL for center of object 

    /** Provides current weather information for the object's current position. See the ISimObject.h for the definition of the WeatherInfo data structure.
    * A return value of E_FAIL means that Prepar3D's weather system failed to process the request properly. This could happen if it is not initialized fully.
    * @param WeatherInfo Reference to local WeatherInfo data structure */
    STDMETHOD(GetWeatherInformation)(__out WeatherInfoV400& WeatherInfo) PURE;
    /** Returns the object's current magnetic variation in radians. A positive is value is "east". */
    STDMETHOD_(float, GetMagneticVariation)() const PURE;

    /** Turns a visual effect on. The out parameter allows you to hold a reference to a visual effect to subsequently turn off.
    * @param pszEffectName File name for requested visual effect
    * @param pvOffsetFeet The offset from model center. A value of NULL will use the model's center
    * @param ppEffect Reference pointer for turning the visual effect off with VisualEffectOff() */
    STDMETHOD(VisualEffectOn) (__in __notnull LPCWSTR pszEffectName, __in_opt const FXYZ* pvOffsetFeet, __out void** ppEffect) PURE;
    /** Turns a visual effect off.
    * @param pEffect Reference pointer obtained in out parameter of VisualEffectOn() */
    STDMETHOD(VisualEffectOff)(__in __notnull void* pEffect) PURE;

    /** Triggers a sound configured in the object's sound.cfg.
    * @param pszName    Sound reference name from Sound.cfg
    * @param bOn        Turns on/off a looping sound. This value has no effect on one shot sounds.
    * @remarks          To turn off a one shot sound, use the StopSound function. */
    STDMETHOD(TriggerSound)(__in __notnull LPCWSTR pszName, BOOL bOn) PURE;
    /** Triggers a sound specifically for a ground contact point.
    * @param pszName Sound reference name from Sound.cfg
    * @param  pvOffset, The offset from model center. A value of NULL will use the model's center
    * @param fImpactSpeed Speed used by sound system to scale sound */
    STDMETHOD(TriggerContactSound)(__in __notnull LPCWSTR pszName, __in const FXYZ* pvOffset, float fImpactSpeed /*, surface type*/) PURE;

    /** Stops a sound configured in the object's sound.cfg. This function will stop a looping or a one shot sound.
    * @param pszName Sound reference name from Sound.cfg */
    STDMETHOD(StopSound)(__in __notnull LPCWSTR pszName) PURE;

    //Proffered services - A simulation service that is made available by Prepar3D
    //for use in your simobject implementation.  The SDK will provide any available services

    /** Invokes the loading of the relevant constant data. This should be called from your SimObject's LoadConstantData(); */
    STDMETHOD(LoadServiceConstantData)  (__in REFGUID guidService)                 PURE;
    /** Causes Prepar3D to unload the relevant constant data. This should be called from your SimObject's UnLoadConstantData(); */
    STDMETHOD(UnloadServiceConstantData)(__in REFGUID guidService)                 PURE;
    /** Invokes the instantiation of the service, based on the loaded constant data. This should be called from your SimObject's LoadDynamicData(); */
    STDMETHOD(CreateServiceInstance)    (__in REFGUID guidService)                 PURE;
    /** Causes Prepar3D to destroy the instance of the service. This should be called from your SimObject's Deinit(); */
    STDMETHOD(DestroyServiceInstance)   (__in REFGUID guidService)                 PURE;
    /** The real-time update of the service. Your SimObject is responsible for calling it with an accurate delta time. */
    STDMETHOD(UpdateServiceInstance)    (__in REFGUID guidService, double dDeltaT) PURE;


    //Config items refer to the configuration of the content "container".
    //Title   is unique string identifier defined in sim.cfg
    //CfgFile is the sim.cfg or aircraft.cfg file path
    //CfgDir  is the top level folder path for the content
    //GetCfgSectionName is the name of the section loaded.  For example [FLTSIM.1]
    //NOTE:  A minimum length of 260 is required for a fully qualified path

    /** Returns the unique string that identifies this object. */
    STDMETHOD(GetTitle)         (__out LPWSTR pszCfgTitle, __in unsigned int uLength)   const PURE;
    /** Returns a fully qualified path to the object's content path. This is generally the folder where the sim.cfg (or aircraft.cfg) lives. */
    STDMETHOD(GetCfgDir)        (__out LPWSTR pszCfgDir, __in unsigned int uLength)     const PURE;
    /** Returns a fully qualified path to the sim.cfg file. */
    STDMETHOD(GetCfgFilePath)   (__out LPWSTR pszCfgFile, __in unsigned int uLength)    const PURE;
    /** Returns the relevant section name in the sim.cfg. e.g. [fltsim.1]. */
    STDMETHOD(GetCfgSectionName)(__out LPWSTR pszCfgFile, __in unsigned int uLength)    const PURE;

    /** Destroy self.  This will not be immediate, so it can be called from within
    * itself.  It will be destroyed as soon as the current simulation finishes. */
    STDMETHOD(Destroy)()     PURE;

    /** Checks if this object's center point is colliding with a building or another SimObject.
    * @param    fRadiusFeet     The radius around the object to check for collisions (feet).
    * @param    eCollision      The resulting collision type.
    * @param    ppUnkHitObject  The object the collision happened with.
    * @return   S_OK if successful, E_FAIL otherwise.
    * Note: A return of S_OK does not mean there is a collision, only that the query operation encountered no errors.
    * The eCollision should be checked for a positive collision, and ppUnkHitObject for whether it involved an (IUnknown) object */
    STDMETHOD(CheckCollision)(__in float fRadiusFeet, __out COLLISIONTYPE& eCollision, __out IUnknown** ppUnkHitObject) const PURE;

    /** Checks if any of the given points are colliding with a building or another SimObject.
    * @param    fRadiusFeet     The radius around the object to check for collisions (feet).
    * @param    pdxyzPoints     Body relative offset points used for collision detection (feet).
    * @param    uPointCount     The number of points in pdxyzPoints.
    * @param    eCollision      The resulting collision type.
    * @param    ppUnkHitObject  The object the collision happened with.
    * @return   S_OK if successful, E_FAIL otherwise.
    * Note: A return of S_OK does not mean there is a collision, only that the query operation encountered no errors.
    * The eCollision should be checked for a positive collision, and ppUnkHitObject for whether it involved an (IUnknown) object */
    STDMETHOD(CheckCollision)(__in float fRadiusFeet, __in const DXYZ* pdxyzPoints, __in UINT32 uPointCount, __out COLLISIONTYPE& eCollision, __out IUnknown** ppUnkHitObject) const PURE;

    /** Returns the current network mode for this object. */
    STDMETHOD_(NET_MODE_TYPE, GetNetworkMode)() const PURE;

    /** Attaches the given object via offsets.
    * @param vOffsetFeetParent      The offset in feet from the parent model center.
    * @param vOffsetRadiansParent   The orientation offset in radians from the parent model center.
    * @param idChild                The child object id to be attached to the parent.
    * @param vOffsetFeetChild       The offset in feet from the parent attach point.
    * @param vOffsetRadiansChild    The orientation offset in radians from the parent attach point.
    * @return                       S_OK if the objects were successfully attached, E_FAIL otherwise.
    */
    STDMETHOD(AttachObject)(__in const DXYZ& vOffsetFeetParent, __in const DXYZ& vOffsetRadiansParent,
                            __in UINT idChild, __in const DXYZ& vOffsetFeetChild, __in const DXYZ& vOffsetRadiansChild) PURE;

    /** Attaches the given object via attach point name and offsets.
    * @param pszAttachPointName     The name of the parent attach point.
    * @param vOffsetRadiansParent   The orientation offset in radians from the parent attach point.
    * @param idChild                The child object id to be attached to the parent attach point.
    * @param vOffsetFeetChild       The offset in feet from the parent attach point.
    * @param vOffsetRadiansChild    The orientation offset in radians from the parent attach point.
    * @return                       S_OK if the objects were successfully attached, E_FAIL otherwise.
    */
    STDMETHOD(AttachObject)(__in LPCSTR pszAttachPointName, __in const DXYZ& vOffsetRadiansParent,
                            __in UINT idChild, __in const DXYZ& vOffsetFeetChild, __in const DXYZ& vOffsetRadiansChild) PURE;

    /** Detaches the given object.
    * @param idChild    The child object id to be detached from the parent.
    * @return           S_OK if the object was successfully detached,
    *                   S_FALSE if the object to be removed was not attached to the parent,
    *                   E_FAIL otherwise.
    */
    STDMETHOD(DetachObject)(__in UINT idChild) PURE;

    /** Returns the given simobject's category name.
    * @param pszCategoryName    The buffer to store the category name of the object
    * @param uLength            The length of the buffer to store the category name in characters
    * @return                   S_OK if the object's category name was returned
    */
    STDMETHOD(GetCategoryName)(__out LPWSTR pszCategoryName, __in unsigned int uLength) const PURE;

    /** Returns the given simobject's category ID.
    * @param guidCategory       The category ID of the object
    * @return                   S_OK if the object's category ID was returned
    */
    STDMETHOD(GetCategoryId)(__out GUID& guidCategory) const PURE;

    /** Gets the damage state
    * @return               The damage state
    * @remarks              0 = No Damage, 1 = Light, 2 = Moderate, 3 = Destroyed, 4-n = User Defined
    */
    STDMETHOD_(UINT, GetDamageState)() const PURE;

    /** Sets the damage state
    * @param eDamageState   The damage state
    * @remarks              0 = No Damage, 1 = Light, 2 = Moderate, 3 = Destroyed, 4-n = User Defined
    */
    STDMETHOD_(void, SetDamageState)(UINT uDamageState) PURE;

    /** Gets the bounding box of the object.
    * @param dxyzMin        The minimum x, y, z values of the box in feet.
    * @param dxyzMax        The maximum x, y, z values of the box in feet.
    * @return               E_FAIL on failure, S_OK on success.
    */
    STDMETHOD(GetBoundingBox)(__out DXYZ& dxyzMin, __out DXYZ& dxyzMax) const PURE;

    /** Gets the crash tree boxes of the object via index.
    * @param index          The index of the crash tree box.
    * @param dxyzMin        The minimum x, y, z values of the box in feet.
    * @param dxyzMax        The maximum x, y, z values of the box in feet.
    * @return               E_FAIL on failure, S_OK on success.
    */
    STDMETHOD(GetCrashTreeBox)(__in UINT index, __out DXYZ& dxyzMin, __out DXYZ& dxyzMax) const PURE;

    /** Gets the crash tree box count.
    * @return               The number of crash tree boxes.
    * @remarks              Returns 0 when no crash tree is found.
    */
    STDMETHOD_(UINT, GetCrashTreeBoxCount)() const PURE;
};

/** Factory class interface for creating supplemental subsystems on an existing simobject implementation\n\n
*
*  There are two separate methods for creating a system with registered factory.\n
*  - Create() - This creates the object and registers it directly with the simulation manager for real-time callbacks.
*  - CreateEx() - Returns an IUnknown reference to the created object, in which the caller will be responsible for
*                 all real-time callbacks.\n
*  \n
*  Create() will be called first, and CreateEx() will only be called if the former does not succeed.  
*/
DECLARE_INTERFACE_(ISubSystemFactoryV500, ISubSystemFactoryV440)
{
    /** Creates a new subsystem during object loading
    * @param pBaseObject                  The object on which the subsytem is being attached
    * @param pszSecondaryData (optional)  This allows unique subsystems to be specified using the same factory.
    * @remarks   The SubSystemFactory should be registered through the IPdk interface at DLL load time.
    * @remarks   Supplemental subsystems can be specifed in the aircraft.cfg/sim.cfg file.  For example:
    * @remarks      [SupplementalSystems]
    * @remarks      System.0 = {bc95b363-1d22-42aa-82b1-f10905b22c40}, Engine
    * @remarks      System.1 = {bc95b363-1d22-42aa-82b1-f10905b22c40}, Propeller
    * @remarks   where the guid is the registered Servide ID (SID)
    */
    STDMETHOD(Create)(  __in IBaseObjectV400* pBaseObject, __in LPCWSTR pszSecondaryData) PURE;
    /** Creates a new subsystem during object loading
    * @param pBaseObject                  The object on which the subsytem is being attached
    * @param pszSecondaryData (optional)  This allows unique subsystems to be specified using the same factory.
    * @param punkSystem                   Returns reference to the object created.
    * @remarks   The SubSystemFactory should be registered through the IPdk interface at DLL load time.
    * @remarks   Supplemental subsystems can be specifed in the aircraft.cfg/sim.cfg file.  For example:
    * @remarks      [SupplementalSystems]
    * @remarks      System.0 = {bc95b363-1d22-42aa-82b1-f10905b22c40}, Engine
    * @remarks      System.1 = {bc95b363-1d22-42aa-82b1-f10905b22c40}, Propeller
    * @remarks   where the guid is the registered Servide ID (SID)
    */
    STDMETHOD(CreateEx)(__in IBaseObjectV400* pBaseObject, __in LPCWSTR pszSecondaryData, __out IUnknown** punkSystem) PURE;
};

/** Constant values describing the Earth atmosphere and geometry
* @note While this is accessed through IBaseObject, these values will be
* constant for all simobjects, and a single static copy could be
* shared across multiple instances. */
class WorldConstants
{
public:
    //Atmosphere
    /** R */
    float m_fSpecificGasConstant;
    /** Gamma (Cp/Cv for air (specific heat ratio) */
    float m_fSpecificHeatRatio;
    /** slugs/ft^3 */
    float m_fStandardSeaLevelDensity;
    /** Rankine */
    float m_fStandardSeaLevelTemperature;
    /** Lbs/SqFt */
    float m_fStandardSeaLevelPressure;
    //Gravity
    /** ft/s^2 */
    float m_fGravitySeaLevel;
    //Radii
    /** Feet */
    double m_dPolarRadius;
    /** Feet */
    double m_dEquatorialRadius;
};


/** Contains terrain/surface information for a given object's offset */
class SurfaceInfoV400
{
public:
    /** Surface categories */
    enum SURFACE_TYPE_CATEGORY
    {
        SURFACE_TYPE_CATEGORY_INVALID = -1,
        SURFACE_TYPE_CATEGORY_HARD,
        SURFACE_TYPE_CATEGORY_SOFT,
        SURFACE_TYPE_CATEGORY_WATER,
        SURFACE_TYPE_NUM_CATEGORIES,
    };


    /** Surface conditions */
    enum SURFACE_CONDITION_CATEGORY
    {
        SURFACE_CONDITION_CATEGORY_INVALID = -1,
        SURFACE_CONDITION_CATEGORY_NORMAL,
        SURFACE_CONDITION_CATEGORY_WET,
        SURFACE_CONDITION_CATEGORY_ICY,
        SURFACE_CONDITION_CATEGORY_SNOW,
        SURFACE_CONDITION_CATEGORY_LOOSE,
        SURFACE_CONDITION_CATEGORY_LOOSE_SNOW,
        SURFACE_CONDITION_NUM_CATEGORIES,
    };

    /** Feet */
    float                       m_fElevation;
    /** Feet */
    float                       m_fWaveHeight;
    /** Unit vector */
    FXYZ                        m_vNormal;
    /** Feet per Second */
    FXYZ                        m_vVelocity;
    /** Radians per Second */
    FXYZ                        m_vRotVelocity;
    /** TRUE or FALSE */
    BOOL                        m_bOnPlatform;
    /** SURFACE_TYPE_CATEGORY */
    SURFACE_TYPE_CATEGORY       m_eSurfaceCategory;
    /** SURFACE_CONDITION_CATEGORY */
    SURFACE_CONDITION_CATEGORY  m_eSurfaceCondition;
};


/** Contains terrain/surface information for a given object's offset */
class WeatherInfoV400
{
public:
    float m_fTemperature;            ///<Degrees Rankine
    float m_fWindSpeed;              ///<Feet per Second
    float m_fWindDirection;          ///<Radians True
    FXYZ  m_vWind;                   ///<Feet per Second
    float m_fAmbientPressure;        ///<PSF
    float m_fSeaLevelPressure;       ///<PSF
    float m_fVisibility;             ///<Feet
    BOOL  m_bIsInCloud;              ///<TRUE or FALSE
    float m_fIcingIntensityPercent;  ///<0.0 = none, 1.0 = max
    float m_fPrecipIntensityPercent; ///<0.0 = min, 1.0 = max
    BOOL  m_bUnsteadyWind;           ///<TRUE if gusting
    BOOL  m_bIsSnowing;              ///<TRUE or FALSE
    BOOL  m_bIsRaining;              ///<TRUE or FALSE
    float m_fTurbulencePercent;      ///<0.0 = none, 1.0 = max
};


/** Interface for getting mass properties from SimObject implementation */
DECLARE_INTERFACE(IMassElement)
{
    STDMETHOD_(float, GetWeight())                                       const PURE; /**< Returns weight of SimObject in pounds. */
    STDMETHOD(GetOffset)(float& fXFeet, float& fYFeet, float& fZFeet)    const PURE; /**< Returns offset of SimObject */
};

/** Interface for getting mass properties from SimObject implementation */
DECLARE_INTERFACE_(IMassPropertiesV533, IMassPropertiesV01)
{
    STDMETHOD_(float, GetWeight())                                        const PURE; /**< Returns weight of SimObject in pounds. */
    STDMETHOD_(float, GetEmptyWeight())                                   const PURE; /**< Returns "empty" weight of SimObject in pounds. i.e. no fuel, passengers, weapons, or payload */
    STDMETHOD_(float, GetMaxGrossWeight())                                const PURE; /**< Returns maximum allowable weight of SimObject in pounds. */
    STDMETHOD_(UINT,  GetPayloadStationCount())                           const PURE; /**< Returns number of payload stations. */
    STDMETHOD(GetPayloadStationName)(__in UINT iPayloadIndex, __out LPWSTR pszName, __in UINT uLength)   const PURE; /**< Returns string name used in User Interface for specified station. */    
    STDMETHOD_(float, GetPayloadStationWeight(UINT iPayloadIndex))        const PURE; /**< Returns weight of specified payload in pounds. */
    STDMETHOD(SetPayloadStationWeight(float fWeight, UINT iPayloadIndex))       PURE; /**< Sets weight of specified payload in pounds. */
    STDMETHOD(SetDefaultPayload())                                              PURE; /**< Sets default payloads in pounds. */
    STDMETHOD_(BOOL, RegisterMass(__in const IMassElement* pElement))           PURE; /**< Registers mass element on SimObject. */
    STDMETHOD_(BOOL, UnRegisterMass(__in const IMassElement* pElement))         PURE; /**< Unregisters mass element on SimObject */
    STDMETHOD_(void, ForceUpdate())                                             PURE; /**< Force an update of properties */

    STDMETHOD_(float, GetPercentCenterOfGravityLongitudinal())            const PURE; /**< Center of Gravity Percetage (typically Percent of Mean Aerodynamic Chord in airplanes */
    STDMETHOD_(float, GetPercentCenterOfGravityLateral())                 const PURE; /**< Center of Gravity Percetage (typically Percent of Mean Aerodynamic Chord in airplanes */
};

/** Interface for getting physical forces from a SimObject implementation. */
DECLARE_INTERFACE(IForceElement)
{
    STDMETHOD_(void, GetForceMoment(__out DXYZ& vForces, __out DXYZ& vMoment))  const PURE; /**< Gets a force moment */
    STDMETHOD(GetOffset(float& fXFeet, float& fYFeet, float& fZFeet))           const PURE; /**< Gets the body offset of the force on the host.  If return S_OK, force will be applied to the moment at this offset */
    STDMETHOD(SetForceMultiplier(float scalar))                                       PURE; /**< Sets a force multiplier */
};

/** Interface for getting physical forces from a SimObject implementation. By default, a registered force and moment is assumed to be world-relative. Specifying body-relative 
changes the assumption that the force is already in the body-frame of reference of the object in which it is registering.*/
DECLARE_INTERFACE_(IForceMomentsV510, IForceMomentsV01)
{
    STDMETHOD_(BOOL, RegisterElement(__in IForceElement* pElement, BOOL bBodyRelative))                    PURE; /**< Registers a force element. TRUE = Force is relative to the body being applied to, FALSE = world relative */
    STDMETHOD_(BOOL, RegisterElement(__in IForceElement* pElement))                                        PURE; /**<DEPRECATED Registers a force element - ASSUMES WORLD-RELATIVE FORCE*/
    STDMETHOD_(BOOL, UnRegisterElement(__in IForceElement* pElement))                                      PURE; /**< Unregisters a force element */
    STDMETHOD_(UINT, ElementCount())                                                                 const PURE; /**< Returns the total number of force elements */
    STDMETHOD_(IForceElement*, GetElement(int iIndex))                                               const PURE; /**< Gets a force element */
    STDMETHOD_(BOOL, IsElementBodyRelative(int iIndex))                                              const PURE; /**< TRUE if registered force element is Body-relative, otherwise World-relative */
};


/** Interface for getting crash parameters for this object */
DECLARE_INTERFACE_(ICollisionServiceV01, ICollisionService)
{
    STDMETHOD_(BOOL, InvokesCrashOnOtherObjects())                             PURE; /**< Whether crash will invoke on other objects */
    STDMETHOD_(void, SetInvokesCrashOnOtherObjects(__in BOOL invokesCrash))    PURE; /**< Sets whether to invoke crash on other objects */
};


//IAircraftService
DECLARE_INTERFACE_(IAircraftServiceV01, IAircraftService)
{
    STDMETHOD_(float, GetIndicatedAirspeed)() const PURE; /**< Gets the indicated airspeed. (feet per second)*/
};

//IAirplaneService
DECLARE_INTERFACE_(IAirplaneServiceV01, IAircraftServiceV01)
{
};

//IRotorcraftService
DECLARE_INTERFACE_(IRotorcraftServiceV01, IAircraftServiceV01)
{
};

//IBoatService
DECLARE_INTERFACE_(IBoatServiceV01, IBoatService) // {190D16C5-DB9E-44B1-87AB-858632C08F5E}
{
};

//IGroundVehicleService
DECLARE_INTERFACE_(IGroundVehicleServiceV01, IGroundVehicleService)
{
};

/** Interface for getting ATC parameters from this object */
DECLARE_INTERFACE_(IAtcServiceV01, IAtcService)
{
};

/** Interface for getting the radar signature of this object */
DECLARE_INTERFACE_(IRadarSignatureServiceV01, IRadarSignatureService)
{
};


/** Interface for getting door parameters for this object */
DECLARE_INTERFACE_(IDoorServiceV01, IDoorService)
{
    /** Returns percentage of how open a door currently is
    * @param doorIndex Index that associated with a door */
    STDMETHOD_(float, GetDoorPercentOpen)(__in int doorIndex)                              const PURE;
};

/** Interface for setting/getting fuel levels, such as in the UI */
DECLARE_INTERFACE_(IFuelServiceV400, IUnknown)
{
    STDMETHOD_(float, GetWeightPerGallon)()                                                const PURE; /**< Pounds per gallon */
    STDMETHOD_(UINT, GetNumberOfTanks)()                                                   const PURE; /**< Number of tanks with capacity greater than zero. */

    STDMETHOD_(float, GetTotalCapacityGallons)()                                           const PURE; /**< Total capacity for all tanks */
    STDMETHOD_(float, GetTotalLevelPercent)()                                              const PURE; /**< Total percentage (0-1) for all tanks combined. */
    STDMETHOD(SetTotalLevelPercent)(float fPct)                                                  PURE; /**< Sets total percentage (0-1) for all tanks combined. */

    STDMETHOD(GetTankName)(__in int iTankIndex, __out LPWSTR pszName, __in UINT uLength)   const PURE; /**< Returns string name used in the Fuel User Interface for specified tank. */
    STDMETHOD_(float, GetTankCapacityGallons)(__in int iTankIndex)                         const PURE; /**< Gallons for specified tank. */
    STDMETHOD_(float, GetTankLevelPercent)   (__in int iTankIndex)                         const PURE; /**< Percentage (0 - 1) for specified tank. */
    STDMETHOD(SetTankLevelPercent)          (__in int iTankIndex, float fPct)                    PURE; /**< Sets the percentage (0 - 1) for specified tank. */

};

/**********************************************************************************************************
The following interfaces and services are not associated with a specific object.  The service can be
obtained from the PDK ServiceProvider
**********************************************************************************************************/


/** General surface queries, not associated with a specific object. This service interface is available from the PDK. */
DECLARE_INTERFACE_(ISurfaceQueryManagerV400, IUnknown)
{
    /** Provides surface information for a given world-relative position. (X = Longitude in radians, Y = Altitude in feet, Z = Latitude in radians). Returns E_FAIL if query fails. */
    STDMETHOD(QuerySurfaceInformation) (__out SurfaceInfoV400& SurfaceInfo, __in const DXYZ& vWorldPosRadiansFeet)  const PURE;
    /** Provides surface elevation (in feet) for a given world-relative position. (X = Longitude in radians, Y = Altitude in feet, Z = Latitude in radians). Returns E_FAIL if query fails. */
    STDMETHOD(QuerySurfaceElevation)   (__out float& fElevationFeet, __in const DXYZ& vWorldPosRadiansFeet)  const PURE;
    /** Provides bathymetry elevation (in feet)  for a given world-relative position. (X = Longitude in radians, Y = Altitude in feet, Z = Latitude in radians). Returns E_FAIL if query fails. */
    STDMETHOD(QueryBathymetryElevation)(__out float& fDepthFeet, __in const DXYZ& vWorldPosRadiansFeet)  const PURE;
};

/** Interface that provides waypoint information specifically for the user in a mission. This service interface is available from the PDK. */
DECLARE_INTERFACE_(IWaypointQueryManagerV400, IUnknown)
{
    //  Manager queries
    /** Returns the number of waypoint lists the manager is holding. */
    STDMETHOD_(UINT, GetNumberOfWaypointLists())                                                                                   const    PURE;
    /** Provides the index (0 - based) of the waypoint list with the description string passed in. Returns E_FAIL if waypoint list not found. */
    STDMETHOD(GetWaypointListIndexFromDescription(__in LPCWSTR pszDescription, __out UINT& iWaypointList))                        const    PURE;

    //  Waypoint list queries
    /** Returns the number of waypoints for the list specified by the list index input. */
    STDMETHOD_(UINT, GetNumberOfWaypoints) (__in UINT iWaypointList)                                                               const    PURE;
    /** Provides the description string for the list specified by the list index input.  Returns E_FAIL if waypoint list not found. */
    STDMETHOD(GetWaypointListDescription) (__in UINT iWaypointList, __out LPWSTR pszDesc, __in UINT uLength)                      const    PURE;

    //  Waypoint queries
    /** Returns the specified integer ID of the waypoint. */
    STDMETHOD_(int, GetWaypointID)    (__in UINT iWaypointList, __in UINT iWaypoint)                                                  const    PURE;
    /** Returns the specified description of the waypoint. */
    STDMETHOD(GetWaypointDescription)(__in UINT iWaypointList, __in UINT iWaypoint, __out LPWSTR pszDescription, __in UINT uLength)  const    PURE;
    /** Provides the world-relative position for the waypoint specified by the list and waypoint index inputs.
    * (X = Longitude in radians, Y = Altitude in feet, Z = Latitude in radians).  Returns E_FAIL if waypoint not found. */
    STDMETHOD(GetWaypointPosition)   (__in UINT iWaypointList, __in UINT iWaypoint, __out P3D::DXYZ& vWorldPosition)                 const    PURE;
    /** Provides the orientation for the waypoint specified by the list and waypoint index inputs.
    * (X = Pitch in radians, Y = Heading in radians, Z = Bank in radians).  Returns E_FAIL if waypoint not found. */
    STDMETHOD(GetWaypointOrientation)(__in UINT iWaypointList, __in UINT iWaypoint, __out P3D::DXYZ& vOrientation)                   const    PURE;
    /** Returns whether the specified altitude of the waypoint is Above Ground Level or Mean Sea Level. */
    STDMETHOD_(BOOL, IsAltitudeAGL)   (__in UINT iWaypointList, __in UINT iWaypoint)                                                  const    PURE;
    /** Provides the text string specified in the Custom Value field for the waypoint specified by the list and waypoint index inputs.  Returns E_FAIL if waypoint not found. */
    STDMETHOD(GetWaypointCustomValue)(__in UINT iWaypointList, __in UINT iWaypoint, __out LPWSTR pszCustomVal, __in UINT uLength)  const    PURE;
};

/** This interface is to be implemented on any simulation object that is used for the user's avatar. */
DECLARE_INTERFACE_(IAvatarSimV01, IUnknown)
{
    STDMETHOD(OnAttach()) PURE;    /**< Called when the avatar attaches to the user's object being controlled. */
    STDMETHOD(OnDetach()) PURE;    /**< Called when the avatar detaches to the user's object being controlled. */
};

/** This service can queried for on a simulation object to play pre-defined animations built into the 3-D visual model. For example, a walking animation. */
DECLARE_INTERFACE_(IAnimationControllerV01, IUnknown)
{
    STDMETHOD(Play)(const GUID& guidAnimationID, BOOL bLoop) PURE;    /**< Called to invoke a specified animation. */
    STDMETHOD(TransitionAndPlay)(const GUID& guidCurrentAnimationID, const GUID& guidNextAnimationID, BOOL bLoop, double fBlendDuration) PURE;    /**< Called to transition from one animation to another. */
};

/** This service allows configuring conditions or constraints in which the avatar can be attached and
* detached on this object. For example, implement this on an airplane to prevent detaching at high speeds. */
DECLARE_INTERFACE_(IAvatarAttachServiceV01, IUnknown)
{
    STDMETHOD_(BOOL, CanAvatarAttach())  const    PURE;    /**< Return if conditions are appropriate for attaching. */
    STDMETHOD_(BOOL, CanAvatarDetach())  const    PURE;    /**< Return if conditions are appropriate for detaching. */
};

/** This service allows for placing a graphical orthogonal marker on a simobject at a specified offset. This is useful for visualizing physical offsets relative to the visual model.
* For example, wheel and engine positions. By default, the marker consists of red 30 meter orthogonal lines in both the positive and negative directions of the X,Y, and Z axis.
* This service is available through the PDK service provider. */
DECLARE_INTERFACE_(IMarkerManagerV310, IMarkerManagerV01)
{
    /** Creates a new marker with the manager. It is advised to pass "0" for the handle, as that will verify with the manager that this is a new marker.
    * A valid handle will be returned when successfully created. idObject is the Object ID in which to attach the marker. */
    STDMETHOD(CreateObjectMarker)                    (__in UINT idObject, __out HANDLE& hHandle)                                          PURE;
    /** This is called to update the offset from the object's center in which to draw the marker. */
    STDMETHOD(UpdateObjectMarkerOffset)              (__in const HANDLE hHandle, __in const FXYZ& vOffset)                                PURE;
    /** This is called to update the orientation relative to the object's body axis in which to draw the marker. */
    STDMETHOD(UpdateObjectMarkerOrientation)         (__in const HANDLE hHandle, __in const FXYZ& vOrientation)                           PURE;
    /** This is called to update both the offset and orientation from the object's center and body axis in which to draw the marker. */
    STDMETHOD(UpdateObjectMarkerOffsetAndOrientation)(__in const HANDLE hHandle, __in const FXYZ& vOffset, __in const FXYZ& vOrientation) PURE;
    /** Called to remove the marker. This will unregister the marker. The handle will be returned to a value of "0", and use of the original should be avoided. */
    STDMETHOD(RemoveMarker)                          (__inout HANDLE& hHandle)                                                            PURE;
};

/**************************************************************************************************************
**************************************************************************************************************/

/** Interface implemented on a SimObject in order for core Prepar3D to interface with it for the purposes of broadcasting DIS related PDUs.
* This interface may also be queried by an ISimObject to gather designator related information. */
DECLARE_INTERFACE_(IDesignatorServiceV340, IUnknown)
{
    STDMETHOD_(UINT, GetDesignatorCount())                                                                       const PURE;    /**< The number of designators */
    STDMETHOD_(BOOL, IsActive)                   (__in UINT iDesignator)                                         const PURE;    /**< The active state of the given designator */
    STDMETHOD_(USHORT, GetCodeName)                (__in UINT iDesignator)                                       const PURE;    /**< The DIS code name */
    STDMETHOD_(UINT, GetDesignatedObjectId)      (__in UINT iDesignator)                                         const PURE;    /**< The object ID of the designated target */
    STDMETHOD_(USHORT, GetDesignatorCode)          (__in UINT iDesignator)                                       const PURE;    /**< The DIS designator code */
    STDMETHOD_(float, GetDesignatorPower)         (__in UINT iDesignator)                                        const PURE;    /**< The power of the designator in watts */
    STDMETHOD_(float, GetDesignatorWaveLength)    (__in UINT iDesignator)                                        const PURE;    /**< The wavelength of the designator in microns */
    STDMETHOD(GetDesignatorSpotLocation)           (__in UINT iDesignator, __out DXYZ& vWorldPosRadiansFeet)     const PURE;    /**< The world location of the spot designation in radians/feet */
    STDMETHOD(GetDesignatorSpotAcceleration)       (__in UINT iDesignator, __out DXYZ& vWorldAccelerationFpss)   const PURE;    /**< The world acceleration of the spot designation in feet/sec */

};

/**************************************************************************************************************
**************************************************************************************************************/

/** This service allows the user to perform collision based ray tracing. Ray tracing can be performed on either objects,
* terrain, or both based on the given interrogation type. When casting from an object location, that object's object id
* should be in the ignore field to prevent it from casting on itself.\n\n
*
* Object interrogation is typically more expensive than terrain interrogation. Ray length and granularity can be used to
* help control performance depending on the need of the ray trace. Ray trace calls should typically be done on both objects
* and terrain at a shorted ray length and a more precise granularity first. If no collision is detected, a higher ray length
* and less precise terrain based ray trace should be performed.
* @note Ray tracing is an expensive operation. Ray trace calls should be limited whenever possible. */
DECLARE_INTERFACE_(IRayTraceManagerV340, IUnknown)
{
    /** Performs a world space based collision ray trace. */
    STDMETHOD(InterrogateWorldRay)(
        /** INTEROGATIONTYPE flags. See Types.h */
        __in DWORD dwInterrogationTypes,
        /** This object will be ignored when performing the ray trace, likely the casting object (Optional) */
        __in_opt UINT iIgnoreObjectId,
        /** The initial LonAltLat of the ray trace in world radians/feet */
        __in const DXYZ& vWorldRadiansFeet,
        /** A unit vector representing the orientation in world space */
        __in const DXYZ& xyzWorldUnitRayDir,
        /** The maximum length of the ray trace in meters */
        __in float fRayLengthMax,
        /** The minimum step distance of the ray trace in meters */
        __in float fGranularityMin,
        /** The resulting object id of the collision (Optional) */
        __out_opt UINT* pResultObjectId,
        /** The resulting LonAltLat of the collision in world radians/feet (Optional) */
        __out_opt DXYZ* pResultWorldRadiansFeet,
        /** The resulting INTEROGATIONTYPE flags. See Types.h */
        __inout DWORD& dwInterogationResults) const PURE;
};

/**************************************************************************************************************
**************************************************************************************************************/

/** Interface implemented on a SimObject in order for core Prepar3D to interface with it for the purposes of broadcasting DIS related PDUs.
* This interface may also be queried by an ISimObject to gather electromagnetic emission related information. */
DECLARE_INTERFACE_(IEmissionsServiceV340, IUnknown)
{
    // Top Level
    /** The number of emission systems */
    STDMETHOD_(UINT, GetEmissionSystemCount())                                                                   const PURE;

    // Emission System Level
    /** The number of emitter beam in the given emission system */
    STDMETHOD_(UINT, GetEmitterBeamCount(__in UINT iSystem))                                                     const PURE;
    /** The DIS emission system name */
    STDMETHOD_(UINT, GetEmissionSystemName(__in UINT iSystem))                                                   const PURE;
    /** The DIS emission system function */
    STDMETHOD_(UINT, GetEmissionSystemFunction(__in UINT iSystem))                                               const PURE;
    /** The emitter identification number */
    STDMETHOD_(UINT, GetEmitterNumber(__in UINT iSystem))                                                        const PURE;
    /** The offset of the beam source in body coordinates (feet) */
    STDMETHOD(GetEmissionSystemBodyOffset(__in UINT iSystem, __out P3D::DXYZ& vBodyOffsetFeet))                  const PURE;

    // Beam Level
    /** The number targets being tracked/jammed */
    STDMETHOD_(UINT, GetTrackJamCount(__in UINT iSystem, __in UINT iBeam))                                       const PURE;
    /** The beam identification number */
    STDMETHOD_(UINT, GetEmitterBeamNumber(__in UINT iSystem, __in UINT iBeam))                                   const PURE;
    /** The DIS beam parameter */
    STDMETHOD_(UINT, GetEmitterBeamParameter(__in UINT iSystem, __in UINT iBeam))                                const PURE;
    /** The frequency of the beam in hertz */
    STDMETHOD_(float, GetEmitterBeamFrequency(__in UINT iSystem, __in UINT iBeam))                               const PURE;
    /** The frequency range of the beam in hertz */
    STDMETHOD_(float, GetEmitterBeamFrequencyRange(__in UINT iSystem, __in UINT iBeam))                          const PURE;
    /** The average effective radiated power of the beam in dBm */
    STDMETHOD_(float, GetEmitterBeamEffectiveRadiatedPower(__in UINT iSystem, __in UINT iBeam))                  const PURE;
    /** The average pulse repetition frequency of the beam in hertz */
    STDMETHOD_(float, GetEmitterBeamPulseRepetitionFrequency(__in UINT iSystem, __in UINT iBeam))                const PURE;
    /** The average pulse width of the beam in microseconds */
    STDMETHOD_(float, GetEmitterBeamPulseWidth(__in UINT iSystem, __in UINT iBeam))                              const PURE;
    /** The center azimuth of the beam in radians relative to the emitter */
    STDMETHOD_(float, GetEmitterBeamAzimuthCenter(__in UINT iSystem, __in UINT iBeam))                           const PURE;
    /** The half-angle sweep of the azimuth in radians relative to the center azimuth */
    STDMETHOD_(float, GetEmitterBeamAzimuthSweep(__in UINT iSystem, __in UINT iBeam))                            const PURE;
    /** The center elevation of the beam in radians relative to the emitter */
    STDMETHOD_(float, GetEmitterBeamElevationCenter(__in UINT iSystem, __in UINT iBeam))                         const PURE;
    /** The half-angle sweep of the elevation in radians relative to the center elevation */
    STDMETHOD_(float, GetEmitterBeamElevationSweep(__in UINT iSystem, __in UINT iBeam))                          const PURE;
    /** The range from 0.0 to 100.0 representing the percentage of the pattern scanned */
    STDMETHOD_(float, GetEmitterBeamSweepSync(__in UINT iSystem, __in UINT iBeam))                               const PURE;
    /** The DIS beam function */
    STDMETHOD_(UINT, GetEmitterBeamFunction(__in UINT iSystem, __in UINT iBeam))                                 const PURE;
    /** True if all targets in the scan pattern are to be considered tracked or jammed */
    STDMETHOD_(BOOL, GetEmitterBeamIsHighDensityTrack(__in UINT iSystem, __in UINT iBeam))                       const PURE;
    /** The DIS jamming mode sequence for the given emitter in string representation */
    STDMETHOD_(UINT, GetEmitterBeamJammingMode(__in UINT iSystem, __in UINT iBeam))                              const PURE;

    // Track/Jam Level
    /** The object id of the target being tracked/jammed */
    STDMETHOD_(UINT, GetTrackJamObjectId(__in UINT iSystem, __in UINT iBeam, __in UINT iTrackJam))               const PURE;
    /** The emitter identification number associated with the target */
    STDMETHOD_(UINT, GetTrackJamEmitterNumber(__in UINT iSystem, __in UINT iBeam, __in UINT iTrackJam))          const PURE;
    /** The beam identification number associated with the target */
    STDMETHOD_(UINT, GetTrackJamBeamNumber(__in UINT iSystem, __in UINT iBeam, __in UINT iTrackJam))             const PURE;
};

/**
*   This interface can be queried for on any object to determine if a radio system is available.
**/
DECLARE_INTERFACE_(IRadioSystemV540, IRadioSystemV400)
{
    /**
    * Queries if any radios are currently active
    * @return  TRUE if any radios active
    **/
    STDMETHOD_(BOOL, AreRadiosActive)() const PURE;

    /**
    * Queries a specific radio
    * @param    riid        The IID of the radio interface being queried
    * @param    uUnit       The radio unit being queried (i.e. 1 for COM1, 2 for COM2)
    * @param    ppvRadio    The returned radio pointer
    * @return               S_OK if successfull, failure otherwise
    * @remarks              All radio unit's are 1-based and do not start at 0.
    **/
    STDMETHOD(QueryRadio)(__in REFIID riid, __in unsigned int uUnit, __out void** ppvRadio) PURE;

    /**
    * Queries the radio system for the number of radios with the given IID.
    * @param    riid        The IID of the radio interface being counted
    * @return               S_OK if successfull, failure otherwise
    **/
    STDMETHOD_(unsigned int, QueryRadioCount)(__in REFIID riid) PURE;
};

/**
*   Audio panel interface. Can be queried from IRadioSystem using IID and a unit value of 1 or more.
**/
DECLARE_INTERFACE_(IAudioPanelV540, IUnknown)
{
    /** Returns the panels unit (1 or more). */
    STDMETHOD_(unsigned int, GetUnit)() const PURE;
    /** Selects COM to transmit. */
    STDMETHOD(SetComTransmit)(unsigned int uUnit) PURE;
    /** Sets whether to receive on all COM radios. */
    STDMETHOD(SetComReceiveAll)(bool bReceiveAll) PURE;
    /** Toggles all COM radios to receive on. */
    STDMETHOD(ToggleComReceiveAll)() PURE;
    /** COM transmit state. Index of 1 or 2. */
    STDMETHOD_(bool, GetComTransmit)(unsigned int uUnit) const PURE;
    /** Flag if all COMs receiving. */
    STDMETHOD_(bool, GetComReceiveAll)() const PURE;
    /** Sets NAV ident (on/off). Index of 1 or 2. */
    STDMETHOD(SetNavSound)(unsigned int uUnit, bool bOn) PURE;
    /** Toggles NAV ident on/off. Index of 1 or 2. */
    STDMETHOD(ToggleNavSound)(unsigned int uUnit) PURE;
    /** NAV audio flag. Index of 1 or 2. */
    STDMETHOD_(bool, GetNavSound)(unsigned int uUnit) const PURE;
    /** Sets DME ident (on/off). Index of 1 or 2. */
    STDMETHOD(SetDmeSound)(unsigned int uUnit, bool bOn) PURE;
    /** Toggles DME ident on/off. Index of 1 or 2. */
    STDMETHOD(ToggleDmeSound)(unsigned int uUnit) PURE;
    /** DME audio flag. Index of 1 or 2. */
    STDMETHOD_(bool, GetDmeSound)(unsigned int uUnit) const PURE;
    /** Sets the DME identification sound on/off for the selected DME. */
    STDMETHOD(SetDmeSelectedSound)(bool bOn) PURE;
    /** Turns on/off the identification sound for the selected DME. */
    STDMETHOD(ToggleDmeSelectedSound)() PURE;
    /** DME selected audio flag. */
    STDMETHOD_(bool, GetDmeSelectedSound)() const PURE;
    /** Sets TACAN DME ident (on/off). Index of 1 or 2. */
    STDMETHOD(SetTacanDmeSound)(unsigned int uUnit, bool bOn) PURE;
    /** Toggles TACAN DME ident on/off. Index of 1 or 2. */
    STDMETHOD(ToggleTacanDmeSound)(unsigned int uUnit) PURE;
    /** TACAN DME audio flag. Index of 1 or 2. */
    STDMETHOD_(bool, GetTacanDmeSound)(unsigned int uUnit) const PURE;
    /** Sets ADF ident (on/off). Index of 1 or 2. */
    STDMETHOD(SetAdfSound)(unsigned int uUnit, bool bOn) PURE;
    /** Toggles ADF ident on/off. Index of 1 or 2. */
    STDMETHOD(ToggleAdfSound)(unsigned int uUnit) PURE;
    /** ADF audio flag. Index of 1 or 2. */
    STDMETHOD_(bool, GetAdfSound)(unsigned int uUnit) const PURE;
    /** Sets TACAN ident (on/off). Index of 1 or 2. */
    STDMETHOD(SetTacanSound)(unsigned int uUnit, bool bOn) PURE;
    /** Toggles TACAN ident on/off. Index of 1 or 2. */
    STDMETHOD(ToggleTacanSound)(unsigned int uUnit) PURE;
    /** TACAN audio flag. Index of 1 or 2. */
    STDMETHOD_(bool, GetTacanSound)(unsigned int uUnit) const PURE;
    /** Sets marker ident (on/off). */
    STDMETHOD(SetMarkerSound)(bool bOn) PURE;
    /** Toggles marker ident on/off. */
    STDMETHOD(ToggleMarkerSound)() PURE;
    /** Marker audio flag. */
    STDMETHOD_(bool, GetMarkerSound)() const PURE;
    /** Sets the intercom (on/off). */
    STDMETHOD(SetIntercomSound)(bool bOn) PURE;
    /** Toggles intercom on/off. */
    STDMETHOD(ToggleIntercomSound)() PURE;
    /** Intercom audio flag. */
    STDMETHOD_(bool, GetIntercomSound)() const PURE;
};

/**
*   ADF radio interface. Can be queried from IRadioSystem using IID and a unit value of 1 or 2.
**/
DECLARE_INTERFACE_(IAdfRadioV540, IUnknown)
{
    /** Returns the radios unit (1 or 2). */
    STDMETHOD_(unsigned int, GetUnit)() const PURE;
    /** Returns whether or not the radio has a standby frequency. */
    STDMETHOD_(bool, HasStandby)() const PURE;
    /** ICAO code. */
    STDMETHOD(GetIdent)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Descriptive name. */
    STDMETHOD(GetName)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Returns the ADF latitude, longitude and altitude of the station the radio equipment is currently tuned to, or zeros if the radio is not tuned to any ADF station. */
    STDMETHOD(GetPosition)(DXYZ& vLonAltLat) PURE;
    /** Sets ADF primary frequency (Hz). */
    STDMETHOD(SetPrimaryFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets ADF primary frequency (Hz). */
    STDMETHOD_(unsigned int, GetPrimaryFrequency)() PURE;
    /** Sets ADF standby frequency (Hz). */
    STDMETHOD(SetStandbyFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets ADF standby frequency (Hz). */
    STDMETHOD_(unsigned int, GetStandbyFrequency)() PURE;
    /** Swaps the primary and standby channels. */
    STDMETHOD(SwapFrequencies)() PURE;
    /** Magnetic variation of tuned station. */
    STDMETHOD_(float, GetMagVar)() PURE;
};

/**
*   COM radio interface. Can be queried from IRadioSystem using IID and a unit value of 1 or 2.
**/
DECLARE_INTERFACE_(IComRadioV540, IUnknown)
{
    /** Returns the radios unit (1 or 2). */
    STDMETHOD_(unsigned int, GetUnit)() const PURE;
    /** Returns whether or not the radio has a standby frequency. */
    STDMETHOD_(bool, HasStandby)() const PURE;
    /** Sets COM primary frequency (Hz). */
    STDMETHOD(SetPrimaryFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets COM primary frequency (Hz). */
    STDMETHOD_(unsigned int, GetPrimaryFrequency)() PURE;
    /** Sets COM standby frequency (Hz). */
    STDMETHOD(SetStandbyFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets COM standby frequency (Hz). */
    STDMETHOD_(unsigned int, GetStandbyFrequency)() PURE;
    /** Swaps the primary and standby channels. */
    STDMETHOD(SwapFrequencies)() PURE;
    /** Sets COM radio tuning mode to 8.33 kHz tuning if passed true and 25 kHz if passed false. */
    STDMETHOD(Set8333HzMode)(bool b8333HzMode) PURE;
    /** Toggle Com 1 radio tuning between 25 kHz and 8.33 kHz tuning. */
    STDMETHOD(Toggle8333HzMode)() PURE;
    /** Returns true if this COM is in 8.33 kHz tuning mode, false if 25 kHz tuning mode. */
    STDMETHOD_(bool, Get8333HzMode)() PURE;
    /** Returns true if this COM supports 8.33 kHz tuning. */
    STDMETHOD_(bool, Is8333HzSupported)() PURE;
};

/**
*   Marker radio interface. Can be queried from IRadioSystem using IID and a unit value of 1.
**/
DECLARE_INTERFACE_(IMarkerRadioV540, IUnknown)
{
    /** Returns the latitude, longitude and altitude of the inner marker of an approach to a runway, if the aircraft is within the required proximity, otherwise it will return zeros. */
    STDMETHOD(GetInnerPosition)(DXYZ& vLonAltLat) PURE;
    /** Returns the latitude, longitude and altitude of the middle marker of an approach to a runway, if the aircraft is within the required proximity, otherwise it will return zeros. */
    STDMETHOD(GetMiddlePosition)(DXYZ& vLonAltLat) PURE;
    /** Returns the latitude, longitude and altitude of the outer marker of an approach to a runway, if the aircraft is within the required proximity, otherwise it will return zeros. */
    STDMETHOD(GetOuterPosition)(DXYZ& vLonAltLat) PURE;
};

/**
*   NAV radio interface. Can be queried from IRadioSystem using IID and a unit value of 1 or 2.
**/
DECLARE_INTERFACE_(INavRadioV540, IUnknown)
{
    /** Returns the radios unit (1 or 2). */
    STDMETHOD_(unsigned int, GetUnit)() const PURE;
    /** Returns whether or not the radio has a standby frequency. */
    STDMETHOD_(bool, HasStandby)() const PURE;
    /** ICAO code. */
    STDMETHOD(GetIdent)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Descriptive name. */
    STDMETHOD(GetName)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Returns the VOR station latitude, longitude and altitude. */
    STDMETHOD(GetVorPosition)(DXYZ& vLonAltLat) PURE;
    /** Returns the glide slope latitude, longitude and altitude. */
    STDMETHOD(GetGlideSlopePosition)(DXYZ& vLonAltLat) PURE;
    /** Returns the DME station latitude, longitude and altitude. */
    STDMETHOD(GetDmePosition)(DXYZ& vLonAltLat) PURE;
    /** Sets NAV primary frequency (Hz). */
    STDMETHOD(SetPrimaryFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets NAV primary frequency (Hz). */
    STDMETHOD_(unsigned int, GetPrimaryFrequency)() PURE;
    /** Sets NAV standby frequency (Hz). */
    STDMETHOD(SetStandbyFrequency)(unsigned int uFrequencyHz) PURE;
    /** Gets NAV standby frequency (Hz). */
    STDMETHOD_(unsigned int, GetStandbyFrequency)() PURE;
    /** Swaps the primary and standby channels. */
    STDMETHOD(SwapFrequencies)() PURE;
    /** Magnetic variation of tuned station. */
    STDMETHOD_(float, GetMagVar)() PURE;
    /** Localizer course heading. */
    STDMETHOD_(float, GetLocalizer)() PURE;
    /** Glide slope angle. */
    STDMETHOD_(float, GetGlideSlope)() PURE;
};

/**
*   TACAN radio interface. Can be queried from IRadioSystem using IID and a unit value of 1 or 2.
**/
DECLARE_INTERFACE_(ITacanRadioV540, IUnknown)
{
    /** Returns the radios unit (1 or 2). */
    STDMETHOD_(unsigned int, GetUnit)() const PURE;
    /** Returns whether or not the radio has a standby frequency. */
    STDMETHOD_(bool, HasStandby)() const PURE;
    /** ICAO code. */
    STDMETHOD(GetIdent)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Descriptive name. */
    STDMETHOD(GetName)(wchar_t* pszBuffer, unsigned int uLength) PURE;
    /** Returns the TACAN latitude, longitude and altitude of the station the radio equipment is currently tuned to, or zeros if the radio is not tuned to any Tacan station. */
    STDMETHOD(GetTacanPosition)(DXYZ& vLonAltLat) PURE;
    /** Returns the TACAN DME station latitude, longitude and altitude. */
    STDMETHOD(GetDmePosition)(DXYZ& vLonAltLat) PURE;
    /** Swaps the primary and standby channels. */
    STDMETHOD(SwapStandbyChannel)() PURE;
    /** Swaps the primary and standby bands. */
    STDMETHOD(SwapStandbyXYBand)() PURE;
    /** Swaps the primary and standby channels and bands. */
    STDMETHOD(SwapStandby)() PURE;
    /** Sets the primary channel. */
    STDMETHOD(SetPrimaryChannel)(unsigned int uChannel) PURE;
    /** Gets the primary channel. */
    STDMETHOD_(unsigned int, GetPrimaryChannel)() PURE;
    /** Sets the standby channel. */
    STDMETHOD(SetStandbyChannel)(unsigned int uChannel) PURE;
    /** Gets the standby channel. */
    STDMETHOD_(unsigned int, GetStandbyChannel)() PURE;
    /** Sets the X-Y band selector. */
    STDMETHOD(SetXYBand)(TACAN_BAND eTacanBand) PURE;
    /** Gets the X-Y band selector. */
    STDMETHOD_(TACAN_BAND, GetXYBand)() PURE;
    /** Sets the standby X-Y band selector. */
    STDMETHOD(SetStandbyXYBand)(TACAN_BAND eTacanBand) PURE;
    /** Gets the standby X-Y band selector. */
    STDMETHOD_(TACAN_BAND, GetStandbyXYBand)() PURE;
    /** Toggles X-Y band selector. */
    STDMETHOD(ToggleXYBand)() PURE;
    /** Toggles standby X-Y band selector. */
    STDMETHOD(ToggleStandbyXYBand)() PURE;
    /** Magnetic variation of tuned station. */
    STDMETHOD_(float, GetMagVar)() PURE;
};

/**
*   Transponder radio interface. Can be queried from IRadioSystem using IID and a unit value of 1.
**/
DECLARE_INTERFACE_(ITransponderRadioV540, IUnknown)
{
    /** Gets transponder code. */
    STDMETHOD_(unsigned int, GetCode)() PURE;
    /** Sets transponder code. */
    STDMETHOD(SetCode)(unsigned int uCode) PURE;
};

/**
*   IAttachmentServiceV430
*   This interface can be queried for on an object to obtain attachment data.
**/
DECLARE_INTERFACE_(IAttachmentServiceV430, IAttachmentServiceV420)
{
    /** Gets the attach point index from the given name.
    * @param pszAttachPointName The name of the attach point.
    * @param uIndex             The index of the given attach point.
    * @return                   S_OK if the attach point index was successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachPointIndex)(__in __notnull LPCSTR pszAttachPointName, __out UINT& uIndex) const PURE;
    /** Gets the attach point count.
    * @param uCount             The number of attach points.
    * @return                   S_OK if the attach point count was successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachPointCount)(__out UINT& uCount) const PURE;
    /** Gets the offset of the attach point with the given index.
    * @param uIndex             The attach point index.
    * @param vOffsetMeters      The body offset of the attach point in meters.
    * @return                   S_OK if the attach point offset was successfully found, E_FAIL otherwise.
    * @note                     Units for this function are in meters, not feet.
    */
    STDMETHOD(GetAttachPointOffset)(__in UINT uIndex, __out DXYZ& vOffsetMeters) const PURE;
    /** Gets the orientation of the attach point with the given index.
    * @param uIndex                 The attach point index.
    * @param vOrientationRadians    The body orientation offset of the attach point in radians.
    * @return                       S_OK if the attach point orientation was successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachPointOrientation)(__in UINT uIndex, __out DXYZ& vOrientationRadians) const PURE;
    /** Gets the number of attached objects.
    * @param nObjects       The number of attached objects.
    * @return               S_OK if the attached objects were successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachedObjectCount)(__out UINT& nObjects) const PURE;
    /** Gets a list of all attached objects.
    * @param nObjects IN:   The max number of objects requested. This must be no smaller than the size of the array pointed to by rgObjectIDs.
    * @param nObjects OUT:  The actual number of objects found.
    * @param rgObjectIDs    Address of array in which object IDs are returned.
    * @return               S_OK if the attached objects were successfully found, E_FAIL otherwise.
    * @note                 It is the callers responsibility to allocate the array's required memory.
    */
    STDMETHOD(GetAttachedObjects)(__inout UINT& nObjects, __out UINT* rgObjectIDs) const PURE;
    /** Gets the attach point index for the given attached object id.
    * @param uObjectId  The attached object's id.
    * @param uIndex     The attached object's attach point index.
    * @return           S_OK if the attached object's attach point index is successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachedObjectIndex)(__in UINT uObjectId, __out UINT& uIndex) const PURE;
    /** Gets the attached object id for the given attach point index.
    * @param uIndex     The attach point index.
    * @param uObjectId  The attached object's id.
    * @return           S_OK if the attach point index's attached object id is successfully found, E_FAIL otherwise.
    */
    STDMETHOD(GetAttachedObjectId)(__in UINT uIndex, __out UINT& uObjectId) const PURE;
    /** Updates the location and orientation of all attach points. Should be called after an
    *   object has simulated to ensure attached objects are in the correct location.
    * @return                   S_OK if at least on attach point was updated, S_FALSE otherwise.
    */
    STDMETHOD(UpdateAttachments)() PURE;

    /** If this object is attached to another object, this function will return the parent's object ID.
    *   If this object is not attached to another object, this function will return zero.
    * @return   The parent's object id if attached, zero otherwise.
    */
    STDMETHOD_(UINT, GetParentId)() PURE;

    /** If this object is attached to another object, this function will set the offset in feet relative to the parent object and return S_OK.
    *   If this object is not attached to another object (or this object is attached via an attachpoint), this function will return E_FAIL.
    * @param vOffsetFeet    The offset relative to the parent object in feet.
    * @return               S_OK if attached and the offset was correctly set, E_FAIL otherwise.
    */
    STDMETHOD(SetOffsetFeet)(__in const DXYZ& vOffsetFeet) PURE;

    /** If this object is attached to another object, this function will provide the offset in feet relative to the parent object and return S_OK.
    *   If this object is not attached to another object, this function will return E_FAIL.
    * @param vOffsetFeet    The offset relative to the parent object in feet.
    * @return               S_OK if attached and the offset is valid, E_FAIL otherwise.
    */
    STDMETHOD(GetOffsetFeet)(__out DXYZ& vOffsetFeet) PURE;

    /** If this object is attached to another object, this function will set the orientation offset in radians relative to the parent object and return S_OK.
    *   If this object is not attached to another object (or this object is attached via an attachpoint), this function will return E_FAIL.
    * @param vOffsetRadians The orientation offset relative to the parent object in radians.
    * @return               S_OK if attached and the offset was correctly set, E_FAIL otherwise.
    */
    STDMETHOD(SetOffsetRadians)(__in const DXYZ& vOffsetRadians) PURE;

    /** If this object is attached to another object, this function will provide the orientation offset in radians relative to the parent object and return S_OK.
    *   If this object is not attached to another object, this function will return E_FAIL.
    * @param vOffsetRadians The orientation offset relative to the parent object in radians.
    * @return               S_OK if attached and the offset is valid, E_FAIL otherwise.
    */
    STDMETHOD(GetOffsetRadians)(__out DXYZ& vOffsetRadians) PURE;
};

/**
*   IPrecipSimulationV600
*   This interface can be queried to obtain precipitation accumulation data.
**/
DECLARE_INTERFACE_(IPrecipSimulationV600, IUnknown)
{
    /** Returns the current precipitation accumulation scale.
    * @return       Precipitaion scale from 0.0 to 1.0.
    */
    STDMETHOD_(float, GetPrecipScale)() const PURE;

    /** Returns the current precipitation type.
    * @return       Precipitaion type from 0.0 to 1.0, where 0.0 represents rain and 1.0 represents snow.
    */
    STDMETHOD_(float, GetPrecipType)() const PURE;

    /** Returns the current precipitation rate.
    * @return       Precipitaion rate from 0.0 to 1.0, where 0.0 represents no precipitation and 1.0 represents max precipitation rate.
    */
    STDMETHOD_(float, GetPrecipRate)() const PURE;
};

//---------------------------------------------------------------------------
// Predefined UUIDs
// 
interface __declspec(uuid("{23499FBC-6226-4537-9760-80647AF4E401}")) ISimObjectManagerV540;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV540 = __uuidof(ISimObjectManagerV540);

interface __declspec(uuid("{58cb02b9-2dba-4423-856d-ae66d2f6201a}")) ISimObjectV440;
extern __declspec(selectany) REFIID IID_ISimObjectV440 = __uuidof(ISimObjectV440);

interface __declspec(uuid("{ef3f6189-aabf-45b5-a2d7-a42737137f52}")) ISimulationV310;
extern __declspec(selectany) REFIID IID_ISimulationV310 = __uuidof(ISimulationV310);

interface __declspec(uuid("{ED69333F-094F-46DA-A614-19E15D9A0596}")) IBaseObjectV520;
extern __declspec(selectany) REFIID IID_IBaseObjectV520 = __uuidof(IBaseObjectV520);

interface __declspec(uuid("{1aed41c9-3ece-4703-93f6-cc705cbeb5f0}")) ISubSystemFactoryV500;
extern __declspec(selectany) REFIID IID_ISubSystemFactoryV500 = __uuidof(ISubSystemFactoryV500);

interface __declspec(uuid("{BA9FDD46-1D33-4B51-8CF7-5A1E446258A9}")) IMassPropertiesV533;
extern __declspec(selectany) REFIID IID_IMassPropertiesV533 = __uuidof(IMassPropertiesV533);

interface __declspec(uuid("{1A313C18-3EA0-4DCB-A960-A8370319123D}")) IForceMomentsV510;
extern __declspec(selectany) REFIID IID_IForceMomentsV510 = __uuidof(IForceMomentsV510);

interface __declspec(uuid("{93719210-0f03-409c-81db-c03ba96d3db2}")) ICollisionServiceV01;
extern __declspec(selectany) REFIID IID_ICollisionServiceV01 = __uuidof(ICollisionServiceV01);

interface __declspec(uuid("{23841739-49fd-408b-b0c8-ad81ea1840a7}")) IAircraftServiceV01;
extern __declspec(selectany) REFIID IID_IAircraftServiceV01 = __uuidof(IAircraftServiceV01);

interface __declspec(uuid("{c4966ae6-2e2f-40b7-b971-98c502cba9b1}")) IAirplaneServiceV01;
extern __declspec(selectany) REFIID IID_IAirplaneServiceV01 = __uuidof(IAirplaneServiceV01);

interface __declspec(uuid("{f00852b9-e57a-4f79-a16d-3e3d4746da47}")) IRotorcraftServiceV01;
extern __declspec(selectany) REFIID IID_IRotorcraftServiceV01 = __uuidof(IRotorcraftServiceV01);

interface __declspec(uuid("{74d67daa-0d29-4af1-9263-991e0b430b89}")) IBoatServiceV01;
extern __declspec(selectany) REFIID IID_IBoatServiceV01 = __uuidof(IBoatServiceV01);

interface __declspec(uuid("{f4b5b522-3519-471a-8b81-7165a7dd59c3}")) IGroundVehicleServiceV01;
extern __declspec(selectany) REFIID IID_IGroundVehicleServiceV01 = __uuidof(IGroundVehicleServiceV01);

interface __declspec(uuid("{d0b4caba-a5bb-4747-8d27-5ad3fcc4d14b}")) IAtcServiceV01;
extern __declspec(selectany) REFIID IID_IAtcServiceV01 = __uuidof(IAtcServiceV01);

interface __declspec(uuid("{717ae5d0-c918-4847-94b7-2a4534a1f884}")) IAnimationControllerV01;
extern __declspec(selectany) REFIID IID_IAnimationControllerV01 = __uuidof(IAnimationControllerV01);

interface __declspec(uuid("{2abd755e-469b-4a93-a52f-6d8d27f50ce2}")) IRadarSignatureServiceV01;
extern __declspec(selectany) REFIID IID_IRadarSignatureServiceV01 = __uuidof(IRadarSignatureServiceV01);

interface __declspec(uuid("{88e51fb4-4548-4e45-a467-a3ca247db029}")) IDoorServiceV01;
extern __declspec(selectany) REFIID IID_IDoorServiceV01 = __uuidof(IDoorServiceV01);

interface __declspec(uuid("{d8e04302-ffa5-46a2-adaf-97b98f4b4ace}")) IFuelServiceV400;
extern __declspec(selectany) REFIID IID_IFuelServiceV400 = __uuidof(IFuelServiceV400);

interface __declspec(uuid("{87fff968-3362-43c1-898d-1375cea57dbc}")) ISurfaceQueryManagerV400;
extern __declspec(selectany) REFIID IID_ISurfaceQueryManagerV400 = __uuidof(ISurfaceQueryManagerV400);

interface __declspec(uuid("{d6fbeb73-348c-4881-8a80-9ad6807228e9}")) IWaypointQueryManagerV400;
extern __declspec(selectany) REFIID IID_IWaypointQueryManagerV400 = __uuidof(IWaypointQueryManagerV400);

interface __declspec(uuid("{ade7e944-4a36-4add-a321-31a02e771564}")) IScenarioManagerV453;
extern __declspec(selectany) REFIID IID_IScenarioManagerV453 = __uuidof(IScenarioManagerV453);

interface __declspec(uuid("{115a2efa-1ff3-422b-9fa4-ba75590f279d}")) IAvatarSimV01;
extern __declspec(selectany) REFIID IID_IAvatarSimV01 = __uuidof(IAvatarSimV01);

interface __declspec(uuid("{422f8864-f75f-4b76-9059-fac0238d9995}")) IMarkerManagerV310;
extern __declspec(selectany) REFIID IID_IMarkerManagerV310 = __uuidof(IMarkerManagerV310);

interface __declspec(uuid("{4da51b78-cda6-44b9-8814-e928886ceb60}")) IAvatarAttachServiceV01;
extern __declspec(selectany) REFIID IID_IAvatarAttachServiceV01 = __uuidof(IAvatarAttachServiceV01);

interface __declspec(uuid("{e48cad0b-69bd-48a0-9a41-5b6c87933379}")) IDesignatorServiceV340;
extern __declspec(selectany) REFIID IID_IDesignatorServiceV340 = __uuidof(IDesignatorServiceV340);

interface __declspec(uuid("{3009df72-1a78-4cf1-8b29-58720685618a}")) IRayTraceManagerV340;
extern __declspec(selectany) REFIID IID_IRayTraceManagerV340 = __uuidof(IRayTraceManagerV340);

interface __declspec(uuid("{8346029b-00c4-4f3d-9eb6-5b7d784f9ee7}")) IEmissionsServiceV340;
extern __declspec(selectany) REFIID IID_IEmissionsServiceV340 = __uuidof(IEmissionsServiceV340);

interface __declspec(uuid("{1b49aead-a226-450b-b6ac-a15445030859}")) IRadioSystemV540;
extern __declspec(selectany) REFIID IID_IRadioSystemV540 = __uuidof(IRadioSystemV540);

interface __declspec(uuid("{c5debe75-86ed-4006-92de-c2c97237b09e}")) IAttachmentServiceV430;
extern __declspec(selectany) REFIID IID_IAttachmentServiceV430 = __uuidof(IAttachmentServiceV430);

interface __declspec(uuid("{814ec34d-0bc1-487a-b76f-8dbff95d0684}")) IPrecipSimulationV600;
extern __declspec(selectany) REFIID IID_IPrecipSimulationV600 = __uuidof(IPrecipSimulationV600);

interface __declspec(uuid("{1670853d-4b37-4911-b235-0abb0b77ed1d}")) IAudioPanelV540;
extern __declspec(selectany) REFIID IID_IAudioPanelV540 = __uuidof(IAudioPanelV540);

interface __declspec(uuid("{8ee64e26-d74a-48c4-9193-1a3a21103676}")) IAdfRadioV540;
extern __declspec(selectany) REFIID IID_IAdfRadioV540 = __uuidof(IAdfRadioV540);

interface __declspec(uuid("{c5aa3f8f-6c62-4c3c-a58b-d550a983d84a}")) IComRadioV540;
extern __declspec(selectany) REFIID IID_IComRadioV540 = __uuidof(IComRadioV540);

interface __declspec(uuid("{e96381dc-a6e0-4429-b086-22a1b02548ec}")) IMarkerRadioV540;
extern __declspec(selectany) REFIID IID_IMarkerRadioV540 = __uuidof(IMarkerRadioV540);

interface __declspec(uuid("{f224f219-c057-4753-9f43-4f8fab2550a3}")) INavRadioV540;
extern __declspec(selectany) REFIID IID_INavRadioV540 = __uuidof(INavRadioV540);

interface __declspec(uuid("{3c23b7af-fd8f-4cbb-8c68-669caf4f174a}")) ITacanRadioV540;
extern __declspec(selectany) REFIID IID_ITacanRadioV540 = __uuidof(ITacanRadioV540);

interface __declspec(uuid("{4fd0177a-3001-4b34-b7c1-99784432a507}")) ITransponderRadioV540;
extern __declspec(selectany) REFIID IID_ITransponderRadioV540 = __uuidof(ITransponderRadioV540);

extern __declspec(selectany) REFGUID SID_SimObjectManager = { 0x6fc0e4e9, 0x50f9, 0x4716, 0xa4, 0x91, 0xa0, 0xb5, 0x91, 0x20, 0xa0, 0x67 };
extern __declspec(selectany) REFGUID SID_SimObject = { 0x7a8708fc, 0xc598, 0x491e, 0xa2, 0x1c, 0xfa, 0xaf, 0x46, 0x35, 0x89, 0xb4 };
extern __declspec(selectany) REFGUID SID_Simulation = { 0x5e4e8e5d, 0x71ac, 0x4d41, 0xac, 0xdc, 0x7a, 0xfc, 0x1c, 0x8d, 0x66, 0x8e };
extern __declspec(selectany) REFGUID SID_MassProperties = { 0xf29d1e3a, 0x12cf, 0x4bbb, 0xb7, 0xf5, 0xff, 0xdf, 0x8, 0xf, 0x2a, 0x3a };
extern __declspec(selectany) REFGUID SID_ForceMoments = { 0xe5f8a234, 0xf85a, 0x4f41, 0xa6, 0x84, 0x4a, 0x5c, 0x1a, 0x1a, 0x55, 0x45 };
extern __declspec(selectany) REFGUID SID_CollisionService = { 0x99ea56fb, 0x78d6, 0x4248, 0xa3, 0x23, 0x28, 0x71, 0x12, 0x22, 0xf4, 0x3d };
extern __declspec(selectany) REFGUID SID_AircraftService = { 0xdcdba3e0, 0x13e0, 0x4ec7, 0xac, 0xfc, 0x5c, 0xfb, 0x21, 0x53, 0xa0, 0xe6 };
extern __declspec(selectany) REFGUID SID_AirplaneService = { 0x2438864f, 0x9a02, 0x4be3, 0xb2, 0x77, 0x64, 0x7, 0xee, 0x3b, 0x20, 0x55 };
extern __declspec(selectany) REFGUID SID_RotorcraftService = { 0x4691442a, 0xf36d, 0x498d, 0x96, 0xd5, 0x39, 0xbb, 0x1d, 0x7, 0xac, 0xbb };
extern __declspec(selectany) REFGUID SID_BoatService = { 0x190d16c5, 0xdb9e, 0x44b1, 0x87, 0xab, 0x85, 0x86, 0x32, 0xc0, 0x8f, 0x5e };
extern __declspec(selectany) REFGUID SID_GroundVehicleService = { 0x759fdb9f, 0x7ef4, 0x4c8c, 0x91, 0xe6, 0x6a, 0x50, 0x6, 0x0, 0xb8, 0x0 };
extern __declspec(selectany) REFGUID SID_AtcService = { 0x73610523, 0xdc86, 0x4bec, 0x98, 0x72, 0x34, 0xca, 0x76, 0x2e, 0x98, 0xe3 };
extern __declspec(selectany) REFGUID SID_RadarSignatureService = { 0x15d122eb, 0xb4b3, 0x45eb, 0xb1, 0x5c, 0x6b, 0x5f, 0x11, 0x1c, 0xb0, 0x6a };
extern __declspec(selectany) REFGUID SID_DoorService = { 0xf00be407, 0xf795, 0x42c7, 0xa7, 0xab, 0x63, 0x70, 0xff, 0xff, 0x82, 0xb5 };
extern __declspec(selectany) REFGUID SID_FuelService = { 0xfe71618f, 0x2cc1, 0x46bf, 0x83, 0xca, 0x60, 0x62, 0x42, 0x4a, 0xfe, 0xd };
extern __declspec(selectany) REFGUID SID_SurfaceQueryManager = { 0x99b8b54, 0x583a, 0x4cf6, 0x97, 0x69, 0x74, 0xb1, 0x1c, 0xe9, 0x4c, 0xd7 };
extern __declspec(selectany) REFGUID SID_WaypointQueryManager = { 0x597ad8d5, 0xb479, 0x4115, 0x98, 0xbc, 0xd4, 0x80, 0x42, 0x24, 0x5e, 0x4 };
extern __declspec(selectany) REFGUID SID_ScenarioManager = { 0x2f0e740, 0x5276, 0x494e, 0xa7, 0x42, 0x67, 0x6b, 0x7e, 0xc1, 0x3b, 0x84 };
extern __declspec(selectany) REFGUID SID_AvatarSim = { 0x5939e102, 0xf5af, 0x4301, 0xb4, 0x2e, 0x29, 0xd9, 0x4a, 0x1d, 0x59, 0x16 };
extern __declspec(selectany) REFGUID SID_AnimationController = { 0x32b19a8e, 0x5d35, 0x4292, 0x80, 0x4, 0xc6, 0xfe, 0x86, 0x1a, 0xfa, 0x2 };
extern __declspec(selectany) REFGUID SID_AvatarAttachService = { 0x21c3f176, 0xa007, 0x481e, 0x8d, 0xbb, 0x12, 0x50, 0xae, 0xf7, 0xbd, 0xa0 };
extern __declspec(selectany) REFGUID SID_MarkerManager = { 0x1b55d667, 0x8b98, 0x4ed4, 0xb2, 0x43, 0xfb, 0x85, 0x53, 0x52, 0xf6, 0x7f };
extern __declspec(selectany) REFGUID SID_DesignatorService = { 0x59222281, 0x90c2, 0x437e, 0xb3, 0x3e, 0x40, 0x16, 0xd7, 0x8c, 0x53, 0x67 };
extern __declspec(selectany) REFGUID SID_RayTraceManager = { 0x9720abd0, 0x2c7, 0x4b40, 0xb6, 0x89, 0x85, 0xc0, 0x47, 0x72, 0x6a, 0x32 };
extern __declspec(selectany) REFGUID SID_EmissionsService = { 0x8d1b430, 0xe9e3, 0x4ef1, 0xac, 0x27, 0x53, 0x57, 0x8c, 0x56, 0x30, 0x76 };
extern __declspec(selectany) REFGUID SID_RadioSystem = { 0xfd23cd7a, 0xca95, 0x4e2f, 0xb0, 0xe4, 0x7, 0x79, 0x4d, 0x6d, 0xb6, 0xbc };
extern __declspec(selectany) REFGUID SID_AttachmentService = { 0xc863672, 0x229b, 0x46ca, 0xa5, 0x13, 0x5a, 0x52, 0x4c, 0xe5, 0x76, 0x19 };
extern __declspec(selectany) REFGUID SID_PrecipSimulation = { 0x6d33e88e, 0xb42b, 0x4d5a, 0x8f, 0x96, 0x95, 0x2a, 0xf3, 0x92, 0xd9, 0x37 };

} //End namespace P3D

/** @} */ // end of ISimObject group
