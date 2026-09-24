// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
/************************************************************************************************************** 
    ISimObject.Legacy.h
    
    Provides legacy interfaces for backward compatibility.  "0000" is the original base version from
    which all others are derived
***************************************************************************************************************/

#pragma once

#include "..\Types.h"
#include "..\Helpers\ListBuilder.h"

namespace P3D
{

interface IBaseObjectV400;
interface ISimulation;
interface IForceElement;
interface IMassElement;

class WorldConstants;
class WeatherInfoV400;
class SurfaceInfoV400;

/*****************  Add legacy interfaces here *************************************/

/** Handles tasks that are not associated with an instance of a simobject.  This includes:\n
* - Registration of implementation factories and associated properties\n
* - Global application properties (e.g. world constants)\n
* - Object queries */
DECLARE_INTERFACE_(ISimObjectManagerV400, IUnknown)
{
    /** Registers an ISimObject implementation at load time with: unique ID, friendly category name (e.g. "airplane"), and factory function pointer. 
    * The "pszCategoryName" is a high-level categorization used primarily for UI (e.g. "airplane"). Mainly, it is used as a filter to exclude 
    * objects from appearing in the Vehicle Select screen. If you create a unique category name, ensure you add the name to the User Objects key 
    * in the Prepar3D.cfg's [Main] section. */
    STDMETHOD (RegisterSimulationCategory) (__in GUID guidCategory, __in LPCWSTR pszCategoryName, __in __notnull PSimCreateFunc pcbCreateFunction) PURE;

    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallback       pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Vector (DXYZ) */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyVectorCallback pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: String */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PPropertyStringCallback pcbProperty)                                   PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double (with secondary substring input) */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallbackWithSubString    pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallback          pcbEvent, __in EVENTTYPE eType)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event vector */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventVectorCallback)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event string */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PEventStringCallback)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event double (with secondary substring input) */
    STDMETHOD (RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallbackWithSubString  pcbEvent)    PURE;

    /** World Constants are constant values describing the Earth atmosphere and geometry.\n
    * NOTE: While this is accessed through IBaseObject, these values will be constant for all SimObjects, and a single static copy could be shared across multiple instances. */
    STDMETHOD (GetWorldConstants)(__out WorldConstants&) const PURE;
    
    /** Decodes a string units to its integer ID. This can be useful to get at initialization
    * as it is less performanct to query properties using the string version. e.g. "feet per second" to ID. */
    STDMETHOD (GetUnitCode)(__in LPCWSTR pszPropertyUnits, __out int& iUnitCode) const PURE;

    /** Gets another IBaseObject ref for a given ID */
    STDMETHOD (GetObject)(__in UINT idObject, __out IBaseObjectV400** ppObject) const PURE;

    /** Gets an IBaseObject ref for the current user object.\n
    NOTE: If the user object is the Viewer and there is a previous user object, this will return the previous user object. Otherwise, it will return the Viewer. */
    STDMETHOD (GetUserObject)(__out IBaseObjectV400** ppUserObject) const PURE;

    /** Used to register a callback function that is called upon creation of any new object. See types.h for callback definition. */
    STDMETHOD (RegisterOnObjectCreateCallback) (__in __notnull POnObjectCreateCallback  pCb)    PURE;
    /** Used to register a callback function that is called upon destruction of any existing object. The call is just prior to destruction. See types.h for callback definition. */
    STDMETHOD (RegisterOnObjectRemoveCallback) (__in __notnull POnObjectRemoveCallback  pCb)    PURE;
    /** Used to register a callback function that is called whenever the user is moved from one object to another. See types.h for callback definition. */
    STDMETHOD (RegisterOnUserObjectChangedCallback) (__in __notnull POnUserObjectChangedCallback  pCb)    PURE;

    /** Returns a list of object IDs for a given radius.\n
    * @param nObjects IN:  the max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs\n
    * @param nObjects OUT: the actual number of objects found\n
    * @param rgObjectIDs address of array in which object IDs are returned\n
    * NOTE: It is the callers responsibility to allocate the array's required memory */
    STDMETHOD (GetObjectsInRadius)(__in const DXYZ& vLonAltLat, __in float fRadiusFeet, __inout UINT& nObjects, __out UINT* rgObjectIDs) const PURE;

    /** Returns a list of object IDs for a given radius.  Does not include traffic\n
    * @param nObjects IN:  The max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs\n
    * @param nObjects OUT: The actual number of objects found\n
    * @param rgObjectIDs Address of array in which object IDs are returned.\n
    * NOTE: It is the callers responsibility to allocate the array's required memory. */
    STDMETHOD(GetNonTrafficObjectsInRadius)(__in const DXYZ& vLonAltLat, __in float fRadiusFeet, __inout UINT& nObjects, __out UINT* rgObjectIDs) const PURE;

    /**The user-selected general realism scalar, where 0.0 is "easy" and 1.0 is "real". This can be used to scale your implementation as appropriate*/
    STDMETHOD_(float, GetRealismSetting)()            const PURE;     //Percent
    /**The user-selected flag that dictates whether to process a crash or not*/
    STDMETHOD_(BOOL,  IsCrashDetectionOn)()           const PURE;
    /**The user-selected flag for whether to detect crashes between simobjects or not*/
    STDMETHOD_(BOOL,  IsCollisionBetweenObjectsOn)()  const PURE;
    /**The user-selected scalar for determining crash tolerance*/
    STDMETHOD_(float, GetCrashToleranceScalar)()      const PURE;

    /** RemoveObject - Remove any local non-user object 
    * @param idObject The id of the object to remove. */
    STDMETHOD (RemoveObject)(__in UINT idObject) PURE;
};

/** Handles tasks that are not associated with an instance of a simobject.  This includes:\n
* - Registration of implementation factories and associated properties\n
* - Global application properties (e.g. world constants)\n
* - Object queries */
DECLARE_INTERFACE_(ISimObjectManagerV410, ISimObjectManagerV400)
{
    /** Attempts to create an object with the given container title.
    * @param pszTitle   The container title object to be created.
    * @param idObject   The object id of the newly created object.
    * @return           S_OK if the object was successfully created, E_FAIL otherwise.
    */
    STDMETHOD(CreateObject)(__in __notnull LPCWSTR pszTitle, __out UINT& idObject) PURE;
};

/** Handles tasks that are not associated with an instance of a simobject.  This includes:\n
* - Registration of implementation factories and associated properties\n
* - Global application properties (e.g. world constants)\n
* - Object queries */
DECLARE_INTERFACE_(ISimObjectManagerV430, ISimObjectManagerV410)
{
    /** Gets an IBaseObject ref for the current user avatar.
    * @param ppUserAvatar   The IBaseObject ref for the current user avatar.
    * @return               S_OK if the object was successfully found, E_FAIL otherwise.
    * @remarks              The user avatar and the user object may be the same if the user has selected an avatar object.
    */
    STDMETHOD(GetUserAvatar)(__out IBaseObjectV400** ppUserAvatar) const PURE;
};

/** Handles tasks that are not associated with an instance of a simobject.  This includes:\n
* - Registration of implementation factories and associated properties\n
* - Global application properties (e.g. world constants)\n
* - Object queries */
DECLARE_INTERFACE_(ISimObjectManagerV440, ISimObjectManagerV430)
{
    /** Registers an ISimObject implementation at load time with: unique ID, friendly category name (e.g. "airplane"), and factory function pointer.
    * The "pszCategoryName" is a high-level categorization used primarily for UI (e.g. "airplane"). Mainly, it is used as a filter to exclude
    * objects from appearing in the Vehicle Select screen. If you create a unique category name, ensure you add the name to the User Objects key
    * in the Prepar3D.cfg's [Main] section. */
    STDMETHOD(RegisterSimulationCategory) (__in GUID guidCategory, __in LPCWSTR pszCategoryName, __in __notnull PSimCreateFunc pcbCreateFunction) PURE;

    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallback       pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Vector (DXYZ) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyVectorCallback pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: String */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PPropertyStringCallback pcbProperty)                                   PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Double (with secondary substring input) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallbackWithSubString    pcbProperty) PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallback          pcbEvent, __in EVENTTYPE eType)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event vector */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventVectorCallback)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event string */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in __notnull PEventStringCallback)    PURE;
    /** Property "simvar" and "event" registrations. For a specific simobject implementation (guid), associates: property string name, units, and callback pointer (defined above) @note Input: Event double (with secondary substring input) */
    STDMETHOD(RegisterProperty) (__in GUID guidCategory, __in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallbackWithSubString  pcbEvent)    PURE;

    /** World Constants are constant values describing the Earth atmosphere and geometry.\n
    * NOTE: While this is accessed through IBaseObject, these values will be constant for all SimObjects, and a single static copy could be shared across multiple instances. */
    STDMETHOD(GetWorldConstants)(__out WorldConstants&) const PURE;

    /** Decodes a string units to its integer ID. This can be useful to get at initialization
    * as it is less performanct to query properties using the string version. e.g. "feet per second" to ID. */
    STDMETHOD(GetUnitCode)(__in LPCWSTR pszPropertyUnits, __out int& iUnitCode) const PURE;

    /** Gets another IBaseObject ref for a given ID */
    STDMETHOD(GetObject)(__in UINT idObject, __out IBaseObjectV400 * *ppObject) const PURE;

    /** Gets another specific version of an IBaseObject ref for a given ID */
    STDMETHOD(GetObject)(__in UINT idObject, __in REFIID riid, __out void** ppvObject) const PURE;

    /** Gets an IBaseObject ref for the current user object.\n
    NOTE: If the user object is the Viewer and there is a previous user object, this will return the previous user object. Otherwise, it will return the Viewer. */
    STDMETHOD(GetUserObject)(__out IBaseObjectV400 * *ppUserObject) const PURE;

    /** Gets a specific version IBaseObject ref for the current user object.\n
    NOTE: If the user object is the Viewer and there is a previous user object, this will return the previous user object. Otherwise, it will return the Viewer. */
    STDMETHOD(GetUserObject)(__in REFIID riid, __out void** ppvUserObject) const PURE;

    /** Used to register a callback function that is called upon creation of any new object. See types.h for callback definition. */
    STDMETHOD(RegisterOnObjectCreateCallback) (__in __notnull POnObjectCreateCallback  pCb)    PURE;

    /** Used to register a callback function that is called upon destruction of any existing object. The call is just prior to destruction. See types.h for callback definition. */
    STDMETHOD(RegisterOnObjectRemoveCallback) (__in __notnull POnObjectRemoveCallback  pCb)    PURE;

    /** Used to register a callback function that is called whenever the user is moved from one object to another. See types.h for callback definition. */
    STDMETHOD(RegisterOnUserObjectChangedCallback) (__in __notnull POnUserObjectChangedCallback  pCb)    PURE;

    /** Returns a list of object IDs for a given radius.\n
    * @param nObjects IN:  the max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs.\n
    * @param nObjects OUT: the actual number of objects found.\n
    * @param rgObjectIDs address of array in which object IDs are returned.\n
    * NOTE: It is the callers responsibility to allocate the array's required memory */
    STDMETHOD(GetObjectsInRadius)(__in const DXYZ & vLonAltLat, __in float fRadiusFeet, __inout UINT & nObjects, __out UINT * rgObjectIDs) const PURE;

    /** Returns a list of object IDs for a given radius.  Does not include traffic\n
    * @param nObjects IN:  The max number of elements requested.  This must be no smaller than the size of the array pointed to by rgObjectIDs\n
    * @param nObjects OUT: The actual number of objects found.\n
    * @param rgObjectIDs Address of array in which object IDs are returned.\n
    * NOTE: It is the callers responsibility to allocate the array's required memory. */
    STDMETHOD(GetNonTrafficObjectsInRadius)(__in const DXYZ & vLonAltLat, __in float fRadiusFeet, __inout UINT & nObjects, __out UINT * rgObjectIDs) const PURE;

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
    STDMETHOD(GetCategoryId)(__out GUID & guidCategoryId, __out __notnull LPWSTR pszCategoryFriendlyName, __in UINT uNameLen, __out BOOL & bIsNativeSimulation, __in UINT iIndex) const PURE;
};

DECLARE_INTERFACE_(ISimObjectManagerV500, ISimObjectManagerV440)
{
    /** Used to unregister a callback function previously registered with RegisterOnObjectCreateCallback() */
    STDMETHOD(UnRegisterOnObjectCreateCallback) (__in __notnull POnObjectCreateCallback  pCb)    PURE;
    /** Used to unregister a callback function previously registered with RegisterOnObjectRemoveCallback() */
    STDMETHOD(UnRegisterOnObjectRemoveCallback) (__in __notnull POnObjectRemoveCallback  pCb)    PURE;
    /** Used to unregister a callback function previously registered with RegisterOnUserObjectChangedCallback() */
    STDMETHOD(UnRegisterOnUserObjectChangedCallback) (__in __notnull POnUserObjectChangedCallback  pCb)    PURE;
};

DECLARE_INTERFACE_(ISimObjectManagerV520, ISimObjectManagerV500)
{
    /** Attempts to set the user object to the given container title.
    * @param pszTitle   The container title object to be created.
    * @return           S_OK if the object was successfully created, E_FAIL otherwise.
    */
    STDMETHOD(ChangeUserObject)(__in __notnull LPCWSTR pszTitle) PURE;
};

/*************************************************************************************************
*************************************************************************************************/
DECLARE_INTERFACE_ (ISimObject, IServiceProvider)
{
    STDMETHOD (LoadConstantData)(__inout void** ppConstantData)     PURE;       //on-disk data.  return loaded data
    STDMETHOD (UnloadConstantData)(__inout void** ppConstantData)   PURE;       //on-disk data.  should point to loaded data, and return NULL
    STDMETHOD (LoadDynamicData)()                                   PURE;       //e.g. create runtime subsystems based on disk data
    STDMETHOD (Init)()                                              PURE;       //e.g. init relationships between subsystems
    STDMETHOD (DeInit)()                                            PURE;       //e.g. release references between subsystems
    STDMETHOD_(BOOL, SupportsLabels)()                      const   PURE;       //Does this simobject support label display?
    STDMETHOD_(void, OnModeChange)(int bfNewModes)                  PURE;       //Called upon change in modes (pause, slew, etc...)
    STDMETHOD_(void, OnPositionInit)()                              PURE;       //Called upon change in non-realtime object positions (e.g. UI) call GetPosition() to get new position
    STDMETHOD_(void, OnSpeedInit)(float fSpeed)                     PURE;       //Called upon change in speed   (e.g. UI)
};

DECLARE_INTERFACE_ (ISimObjectV01, ISimObject)
{
    //Identical to ISimObject
};

DECLARE_INTERFACE_ (ISimObjectV02, ISimObjectV01)
{
    STDMETHOD (SetSupportsLabels)(BOOL bOn)                         PURE;       //Set if this simobject should support label display?
};

DECLARE_INTERFACE_(ISimObjectV03, ISimObjectV02)
{
    STDMETHOD(QueryBaseObject)(REFIID riid, void** ppv)     PURE;       //Accessor to get the base object from the ISimObject
};

DECLARE_INTERFACE_(ISimObjectV400, ISimObjectV03)
{
    STDMETHOD(GetMainSimRate)(__out float& fSimRate)         const PURE;       /**< Provide the main rate (Hz).  Typically the world position update rate. */
};

/*************************************************************************************************
*************************************************************************************************/
DECLARE_INTERFACE_ (ISimulation, IUnknown)
{
    //Real-time "simulate" call
    STDMETHOD (Update)(double dDeltaT)           PURE;    
    //State saving / loading
    STDMETHOD (SaveLoadState)(__in __notnull PSaveLoadCallback pfnCallback, __in const BOOL bSave) PURE;  
};

DECLARE_INTERFACE_(ISimulationV01, ISimulation)
{
    //Identical to ISimulation
};

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_ (IBaseObjectV400, IServiceProvider)
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

    /** Returns bitwise flags for the current modes of the SimObject. */
    STDMETHOD_(int, GetMode)() const PURE;
    /** Should be called when it is desired to put Prepar3D into "crash" mode. By default, the application will go through it's 
    * crash cycle and reset. It is the developer's responsibility to program the behavior of the object when crash in crash mode. */
    STDMETHOD (SetCrashMode)(double dDeltaT)  PURE;

    /** Gets the current world relative position and velocity from the Prepar3D-side SimObject. This will provide the 
    * valid state upon initialization, as well as when another system such as a UI element or slew changes the position. 
    * @param vLonAltLat Longitude, altitude, latitude (radians) 
    * @param vPHB Pitch, heading, bank (radians) 
    * @param vLonAltLatVel Longitude, altitude, latitude velocity (feet / second) 
    * @param vPHBVel Pitch, heading, bank velocity (radians / second) */
    STDMETHOD (GetPosition)(__out      DXYZ& vLonAltLat, __out      DXYZ& vPHB, __out      DXYZ& vLonAltLatVel, __out      DXYZ& vPHBVel) const PURE;
    /** Sets the current world relative position and velocity to the Prepar3D-side SimObject. 
    * @param vLonAltLat Longitude, altitude, latitude (radians) 
    * @param vPHB Pitch, heading, bank (radians) 
    * @param vLonAltLatVel Longitude, altitude, latitude velocity (feet / second) 
    * @param vPHBVel Pitch, heading, bank velocity (radians / second) 
    * @param bIsOnGround Flag indicating if the object is on the ground. This is important during terrain updates. 
    * @param dDeltaT The time between object updates used to track how much time has accumulated between camera frames */
    STDMETHOD (SetPosition)(__in const DXYZ& vLonAltLat, __in const DXYZ& vPHB, __in const DXYZ& vLonAltLatVel, __in const DXYZ& vPHBVel, __in BOOL bIsOnGround, __in double dDeltaT) PURE;
    /** IsOnGround Returns the on-ground flag value currently in the core base object. This can be useful for determining 
    * if the object has been placed on the ground through a non-simulated means such as the UI. */
    STDMETHOD_(BOOL, IsOnGround)() const PURE;

    /** Rotates a vector from the world frame of reference to the body frame of reference. */
    STDMETHOD (RotateWorldToBody)(__in const DXYZ& vWorld, __out DXYZ& vBody)  const PURE;
    /** Rotates a vector from the body frame of reference to the world frame of reference. */
    STDMETHOD (RotateBodyToWorld)(__in const DXYZ& vBody,  __out DXYZ& vWorld) const PURE;

    /** Registers an ISimulation callback for real-time updates (discussed in Creating Behaviors.) ISimulation registration will 
    * be locked after the ISimObject Init() function has been called. All ISimulation objects must be registered before this point. 
    * @param pSimulation Address of simulation system 
    * @param fRateHz Specified iteration rate */
    STDMETHOD (RegisterSimulation)(__in __notnull ISimulation* pSimulation, float fRateHz) PURE;

    /** Registers a service that can be queried for on this object. A service should be an IUnknown-derived object and registered with a unique GUID. 
    * @param guidService Unique GUID to identify this service. 
    * @param punkService Reference to an instance of this service. */
    STDMETHOD (RegisterService)(__in REFGUID guidService, __in __notnull IUnknown* punkService) PURE;
    /** Removes a service that has been register with RegisterService(). 
    * @param guidService Unique GUID to identify this service. */
    STDMETHOD (UnregisterService)(__in REFGUID guidService) PURE;

    STDMETHOD (GetPropertyCodeAndIndex)(__in PROPERTY_TYPE eType, __in LPCWSTR pszPropertyName, __out int& iPropertyCode, __inout int& iIndex) const PURE; /**< Get Properties */
    
    STDMETHOD (GetProperty)(__in int     iPropertyCode,   __in int      iUnitCode,   __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in int      iUnitCode,   __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __out double& dProperty, __in int index = 0) const PURE;   /**< Get Property - Doubles */
    
    STDMETHOD (GetProperty)(__in int     iPropertyCode,   __in int      iUnitCode,   __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in int      iUnitCode,   __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode, __out DXYZ&   dProperty, __in int index = 0) const PURE;   /**< Get Property - Vectors */
    
    STDMETHOD (GetProperty)(__in int     iPropertyCode,   __out LPWSTR pszProperty, __in UINT uLength,       __in int index = 0) const PURE;   /**< Get Property - Strings */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __out LPWSTR pszProperty, __in UINT uLength,       __in int index = 0) const PURE;   /**< Get Property - Strings */
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD (GetProperty)(__in int     iPropertyCode,   __in LPCWSTR pszSecondarySubstring, __in int      iUnitCode,   __out double& dProperty, __in int index = 0) const PURE;
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in int      iUnitCode,   __out double& dProperty, __in int index = 0) const PURE;
    /** Get Property - Doubles (with secondary substring input) */
    STDMETHOD (GetProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in LPCWSTR  pszUnitCode, __out double& dProperty, __in int index = 0) const PURE;

    STDMETHOD (TriggerProperty)(__in  int     iPropertyCode,    __in int iUnitCode,         __in double dData,   __in  int  index) const PURE;  /**< Numeric trigger */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName,  __in int iUnitCode,         __in double dData,   __in  int  index) const PURE;  /**< Numeric trigger */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName,  __in LPCWSTR  pszUnitCode,  __in double dData,   __in  int  index) const PURE;  /**< Numeric trigger */
    
    STDMETHOD (TriggerProperty)(__in  int     iPropertyCode,   __in int iUnitCode,         __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName, __in int iUnitCode,         __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR  pszUnitCode,  __in const DXYZ&   vData, __in  int  index) const PURE;  /**< Vector trigger */
    
    STDMETHOD (TriggerProperty)(__in int     iPropertyCode,   __in LPCWSTR pszData, __in  int  index) const PURE;   /**< String strigger */
    STDMETHOD (TriggerProperty)(__in LPCWSTR pszPropertyName, __in LPCWSTR pszData, __in  int  index) const PURE;   /**< String strigger */
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD (TriggerProperty)(__in  int       iPropertyCode, __in LPCWSTR pszSecondarySubstring, __in int iUnitCode,         __in double dData, __in  int  index) const PURE; 
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in int iUnitCode,         __in double dData, __in  int  index) const PURE;
    /** Trigger - Doubles (with secondary substring input) */
    STDMETHOD (TriggerProperty)(__in  LPCWSTR pszPropertyName, __in LPCWSTR pszSecondarySubstring, __in LPCWSTR  pszUnitCode,  __in double dData, __in  int  index) const PURE;

    /** Registers a simulation property for this instance of the object. This differs from the RegisterProperty method in the ISimObjectManager 
    * interface which registers a property that exists for all instances associated with the given guid. Properties registered here, as well as their ID, exist 
    * only for the lifetime of this object. Otherwise they function the same when used for gauges, animations, and missions. */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyCallback       pcbProperty)                      PURE;   /**< @note Double "get" */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventCallback          pcbEvent, __in EVENTTYPE eType)   PURE;   /**< @note Double "set" */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PPropertyVectorCallback pcbProperty)                      PURE;   /**< @note Vector "get" */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in LPCWSTR pszPropertyBaseUnits, __in __notnull PEventVectorCallback)                                     PURE;   /**< @note Vector "set" */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in __notnull PPropertyStringCallback pcbProperty)                                                         PURE;   /**< @note String "get" */
    STDMETHOD (RegisterProperty) (__in LPCWSTR pszPropertyName, __in __notnull PEventStringCallback)                                                                        PURE;   /**< @note String "set" */

    // System malfunctions.  The successful handle (greater than 0) returned on registration is used to later reference health (0 - 1.0).\n
    // e.g. pszName would include index, such as Engine 1 */
    /** Registers a specific malfunction that can be set through the UI, scenarios, or missions. 
    * @param guidMalfunction Unique malfunction ID 
    * @param pszType UI Type. Choices: Instruments, Systems, Radios, Engines, Controls, Structural, Miscellaneous 
    * @param pszBaseName Name used for mission file reference. Should be generic (no index), such as "Engine" 
    * @param pszInstanceName Specific malfunction name for the UI, such as "Total Failure Engine 1" 
    * @param nSubIndex Sub-index. For example, engine 0, 1, etc... */
    STDMETHOD_(HANDLE, RegisterSystemMalfunction)(__in REFGUID guidMalfunction, __in LPCWSTR pszType,  __in LPCWSTR pszBaseName, __in LPCWSTR pszInstanceName, __in int nSubIndex) PURE;
    /** Returns the health percentage (0.0 - 1.0) for a given malfunction. */
    STDMETHOD_(float,  GetSystemHealth)(HANDLE hSystem) const PURE;

    /** Apply damage points.  Positive points passed in will be decremented from current health points, to a limit of zero. */
    STDMETHOD (DecrementHealthPoints)(__in float fDamagePoints)      PURE;
    /** Retrieves the current health of the object. */
    STDMETHOD_(float, GetHealthPoints)()                       const PURE;
    /** Sets current health of the object. */
    STDMETHOD_(void, SetHealthPoints)(float fHealthPoints)     PURE;

    /** Provides current surface information for the requested offset from the model center. See the ISimObject.h for the definition of the SurfaceInfo data 
    * structure. A return value of E_FAIL means that Prepar3D's terrain system failed to process the request properly. This could happen if it is not initialized fully.
    * @param SurfaceInfo Reference to local SurfaceInfo data structure
    * @param pvOffsetFeet The offset from model enter.A value of NULL will use the model's center */
    STDMETHOD (GetSurfaceInformation)(__out SurfaceInfoV400& SurfaceInfo, __in const FXYZ* pvOffsetFeet) PURE;  //Pass in NULL for center of object
    /** Provides current surface elevation (above Mean Sea Level) for the requested offset from the model center. This will be more efficient than 
    * GetSurfaceInformation when only the elevation is needed. A return value of E_FAIL means that Prepar3D's terrain system failed to process the 
    * request properly. This could happen if it is not initialized fully. 
    * @param fElevationFeet Reference to the elevation variable (Feet) 
    * @param pvOffsetFeet The offset from model enter. A value of NULL will use the model's center */
    STDMETHOD(GetSurfaceElevation)(__out float& fElevationFeet, __in const FXYZ* pvOffsetFeet) PURE;       //Pass in NULL for center of object 
    /** Provides current depth for the requested offset from the model center.
    * @param fDepthFeet Reference to the depth variable (Feet)
    * @param pvOffsetFeet The offset from model enter. A value of NULL will use the model's center */
    STDMETHOD(GetBathymetryElevation)(__out float& fDepthFeet,  __in const FXYZ* pvOffsetFeet) PURE;    //Pass in NULL for center of object 

    /** Provides current weather information for the object's current position. See the ISimObject.h for the definition of the WeatherInfo data structure. 
    * A return value of E_FAIL means that Prepar3D's weather system failed to process the request properly. This could happen if it is not initialized fully. 
    * @param WeatherInfo Reference to local WeatherInfo data structure */
    STDMETHOD (GetWeatherInformation)(__out WeatherInfoV400& WeatherInfo) PURE;
    /** Returns the object's current magnetic variation in radians. A positive is value is "east". */
    STDMETHOD_(float, GetMagneticVariation)() const PURE;

    /** Turns a visual effect on. The out parameter allows you to hold a reference to a visual effect to subsequently turn off. 
    * @param pszEffectName File name for requested visual effect 
    * @param pvOffsetFeet The offset from model center. A value of NULL will use the model's center 
    * @param ppEffect Reference pointer for turning the visual effect off with VisualEffectOff() */
    STDMETHOD (VisualEffectOn) (__in __notnull LPCWSTR pszEffectName, __in const FXYZ* pvOffsetFeet, __out void** ppEffect) PURE;
    /** Turns a visual effect off. 
    * @param pEffect Reference pointer obtained in out parameter of VisualEffectOn() */
    STDMETHOD (VisualEffectOff)(__in __notnull void* pEffect) PURE;

    /** Triggers a sound configured in the object's sound.cfg. 
    * @param pszName Sound reference name from Sound.cfg */
    STDMETHOD (TriggerSound)(__in __notnull LPCWSTR pszName, BOOL bOn) PURE;
    /** Triggers a sound specifically for a ground contact point. 
    * @param pszName Sound reference name from Sound.cfg
    * @param  pvOffset, The offset from model center. A value of NULL will use the model's center 
    * @param fImpactSpeed Speed used by sound system to scale sound */
    STDMETHOD (TriggerContactSound)(__in __notnull LPCWSTR pszName, __in const FXYZ* pvOffset, float fImpactSpeed /*, surface type*/) PURE;

    
    //Proffered services - A simulation service that is made available by Prepar3D
    //for use in your simobject implementation.  The SDK will provide any available services

    /** Invokes the loading of the relevant constant data. This should be called from your SimObject's LoadConstantData(); */
    STDMETHOD (LoadServiceConstantData)  (__in REFGUID guidService)                 PURE;   
    /** Causes Prepar3D to unload the relevant constant data. This should be called from your SimObject's UnLoadConstantData(); */
    STDMETHOD (UnloadServiceConstantData)(__in REFGUID guidService)                 PURE;
    /** Invokes the instantiation of the service, based on the loaded constant data. This should be called from your SimObject's LoadDynamicData(); */
    STDMETHOD (CreateServiceInstance)    (__in REFGUID guidService)                 PURE;
    /** Causes Prepar3D to destroy the instance of the service. This should be called from your SimObject's Deinit(); */
    STDMETHOD (DestroyServiceInstance)   (__in REFGUID guidService)                 PURE;
    /** The real-time update of the service. Your SimObject is responsible for calling it with an accurate delta time. */
    STDMETHOD (UpdateServiceInstance)    (__in REFGUID guidService, double dDeltaT) PURE;

    
    //Config items refer to the configuration of the content "container".
    //Title   is unique string identifier defined in sim.cfg
    //CfgFile is the sim.cfg or aircraft.cfg file path
    //CfgDir  is the top level folder path for the content
    //GetCfgSectionName is the name of the section loaded.  For example [FLTSIM.1]
    //NOTE:  A minimum length of 260 is required for a fully qualified path

    /** Returns the unique string that identifies this object. */
    STDMETHOD (GetTitle)         (__out LPWSTR pszCfgTitle,__in unsigned int uLength)   const PURE;
    /** Returns a fully qualified path to the object's content path. This is generally the folder where the sim.cfg (or aircraft.cfg) lives. */
    STDMETHOD (GetCfgDir)        (__out LPWSTR pszCfgDir,  __in unsigned int uLength)   const PURE;
    /** Returns a fully qualified path to the sim.cfg file. */
    STDMETHOD (GetCfgFilePath)   (__out LPWSTR pszCfgFile, __in unsigned int uLength)   const PURE;
    /** Returns the relevant section name in the sim.cfg. e.g. [fltsim.1]. */
    STDMETHOD (GetCfgSectionName)(__out LPWSTR pszCfgFile, __in unsigned int uLength)   const PURE;

    /** Destroy self.  This will not be immediate, so it can be called from within 
    * itself.  It will be destroyed as soon as the current simulation finishes. */
    STDMETHOD (Destroy)()     PURE;

    /** Check for this object colliding with building or other simobject.  
    * Note: A return of S_OK does not mean there is a collision, only that the query operation encountered no errors
    * The eCollision should be checked for a positive collision, and ppUnkHitObject for whether it involved an (IUnknown) object */
    STDMETHOD (CheckCollision)(float fRadiusFeet, __out COLLISIONTYPE& eCollision,__out IUnknown** ppUnkHitObject) const PURE;

    /** Returns the current network mode for this object. */
    STDMETHOD_(NET_MODE_TYPE, GetNetworkMode)() const PURE;
};

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_(IBaseObjectV410, IBaseObjectV400)
{
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
};

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_(IBaseObjectV430, IBaseObjectV410)
{
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
};

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_(IBaseObjectV440, IBaseObjectV430)
{
    /** Registers an ISimulation callback for real-time updates (discussed in Creating Behaviors.) ISimulation registration will 
    * be locked after the ISimObject Init() function has been called. All ISimulation objects must be registered before this point. 
    * @param pSimulation Address of simulation system 
    * @param fRateHz Specified iteration rate */
    STDMETHOD (RegisterSimulation)(__in __notnull ISimulation* pSimulation, float fRateHz) PURE;
    /** Registers an ISimulation callback for real-time updates (discussed in Creating Behaviors.) ISimulation registration will
    * be locked after the ISimObject Init() function has been called. All ISimulation objects must be registered before this point.
    * @param pSimulation Address of simulation system
    * @param fMinRateHz Specified minimum iteration rate
    * @param fMaxRateHz Specified maximum iteration rate */
    STDMETHOD (RegisterSimulation)(__in __notnull ISimulation* pSimulation, float fMinRateHz, float fMaxRateHz) PURE;

    /**  Provides the minimum and maximum main simulation rate (Hz).  Typically the world position update rate.
    * An ISimObject implementation will be called by this if it exists. */
    STDMETHOD (GetMainMinMaxSimRates)(__out float& fMinHz, __out float& fMaxHz) const PURE;
    /** Sets minimum and maximum main simulation rate (Hz).  Typically the world position update rate.
    * This is typically called if there is a desire to synchronize the rates of two or more objects.  For example an aircraft and an aircraft carrier, or to prevent perceived jitter
    * when viewing an object from a camera attached to another object.
    * An ISimObject implementation will be called by this if it exists. */
    STDMETHOD (SetMainMinMaxSimRates)(__in  float  fMinHz, __in  float  fMaxHz) PURE;

    /** Stops a sound configured in the object's sound.cfg. This function will stop a looping or a one shot sound.
    * @param pszName Sound reference name from Sound.cfg */
    STDMETHOD (StopSound)(__in __notnull LPCWSTR pszName) PURE;
};

/** Object interface on the host side for providing platform information and services for the object */
DECLARE_INTERFACE_(IBaseObjectV450, IBaseObjectV440)
{
    /** Group ObjectFoeList Hosts a list of ID's that are considered foe's to the current entity\n */
    STDMETHOD_(BOOL, InObjectFoeList)(UINT id)                         const PURE;
    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n */
    STDMETHOD_(void, SetObjectFoeList)(UINT* uEnteredFoeID, UINT size)            PURE;

    /** Group ObjectFoeList Hosts a list of ID's that are considered friends's to the current entity\n */
    STDMETHOD_(BOOL, InObjectFriendList)(UINT id)                         const PURE;
    /** Group Association ID can be used to set/get IDs for Friend/Foe or other types of groupings.\n */
    STDMETHOD_(void, SetObjectFriendList)(UINT* uEnteredFriendID, UINT size)            PURE;

    /** Sets the current world relative position and velocity to the Prepar3D-side SimObject.
    All parameters are optional.  Any parameter set to NULL will be ignored, and current object values will be retained.
    * @param pvLonAltLat Longitude, altitude, latitude (radians)
    * @param pvPHB Pitch, heading, bank (radians)
    * @param pvLonAltLatVel Longitude, altitude, latitude velocity (feet / second)
    * @param pvPHBVel Pitch, heading, bank velocity (radians / second)
    * @param bSetOnGround Flag indicating if the object is to be set on the ground, in which case the on-ground height and pitch attitude will be set.  */
    STDMETHOD(InitPosition)(__in const DXYZ* pvLonAltLat, __in const DXYZ* pvPHB, __in const DXYZ* pvLonAltLatVel, __in const DXYZ* pvPHBVel, __in BOOL bSetOnGround) PURE;

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

/** Factory class interface for creating supplemental subsystems on an existing simobject implementation
*/
DECLARE_INTERFACE_(ISubSystemFactoryV440, IUnknown)
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
    STDMETHOD(Create)(__in IBaseObjectV400* pBaseObject, __in LPCWSTR pszSecondaryData) PURE;
};

/** Provides the current scenario (also known as "flight") and mission files. */
DECLARE_INTERFACE_(IScenarioManagerV400, IUnknown)
{
    /** Provides the fully qualified path to the scenario file. */
    STDMETHOD(GetScenarioFilePath)(__out LPWSTR pszFilePath, __in UINT uLength) const    PURE;
    /** Provides the fully qualified path to the associated object file. If one does not exist a zero-length string is returned. */
    STDMETHOD(GetObjectFilePath) (__out LPWSTR pszFilePath, __in UINT uLength)  const    PURE;
    /** Provides the file name of the associated object file. If one does not exist a zero-length string is returned. */
    STDMETHOD(GetObjectFileName) (__out LPWSTR pszFileName, __in UINT uLength)  const    PURE;
    /** Loads the specified scenario. */
    STDMETHOD(LoadScenario)(__in LPWSTR pszFilePath)                            const    PURE;

    //  Function callback registration for notification that a new mission or scenario (also known as a "flight") is loaded or saved.
    /** This allows registering a callback function for notification when the current scenario has been loaded or changed. See Types.h for the function profile of PNewScenarioNotify. */
    STDMETHOD(RegisterForNewScenarioNotification)(__in PNewScenarioNotify pcbNotifyFunction)      PURE;
};

DECLARE_INTERFACE_(IScenarioManagerV430, IScenarioManagerV400)
{
    /** Provides the fully qualified path to the scenario file. */
    STDMETHOD(GetScenarioFilePath)(__out LPWSTR pszFilePath, __in UINT uLength) const    PURE;
    /** Provides the fully qualified path to the associated object file. If one does not exist a zero-length string is returned. */
    STDMETHOD(GetObjectFilePath) (__out LPWSTR pszFilePath, __in UINT uLength)  const    PURE;
    /** Provides the file name of the associated object file. If one does not exist a zero-length string is returned. */
    STDMETHOD(GetObjectFileName) (__out LPWSTR pszFileName, __in UINT uLength)  const    PURE;
    /** Provides a list of scenario paths. */
    STDMETHOD(GetScenarioFilePathList)(__out INameList& names)                  const    PURE;
    /** Loads the specified scenario. */
    STDMETHOD(LoadScenario)(__in LPWSTR pszFilePath)                            const    PURE;

    //  Function callback registration for notification that a new mission or scenario (also known as a "flight") is loaded or saved.
    /** This allows registering a callback function for notification when the current scenario has been loaded or changed. See Types.h for the function profile of PNewScenarioNotify. */
    STDMETHOD(RegisterForNewScenarioNotification)(__in PNewScenarioNotify pcbNotifyFunction)      PURE;
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IMassProperties, IUnknown)
{
    STDMETHOD_ (float,  GetWeight())                                        const PURE;
    STDMETHOD_ (BOOL,   RegisterMass(__in const IMassElement* pElement))          PURE;
    STDMETHOD_ (BOOL,   UnRegisterMass(__in const IMassElement* pElement))        PURE;
    STDMETHOD_ (void,   ForceUpdate())                                            PURE;
};

/** Interface for getting mass properties from SimObject implementation */
DECLARE_INTERFACE_(IMassPropertiesV01, IMassProperties)
{
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IForceMoments, IUnknown)
{
    STDMETHOD_ (BOOL, RegisterElement(__in IForceElement* pElement))        PURE;
    STDMETHOD_ (BOOL, UnRegisterElement(__in IForceElement* pElement))      PURE;
    STDMETHOD_ (UINT, ElementCount())                                 const PURE;
    STDMETHOD_ (IForceElement*, GetElement(int index))                const PURE;
};

DECLARE_INTERFACE_(IForceMomentsV01, IForceMoments)
{
    STDMETHOD_(BOOL, RegisterElement(__in IForceElement* pElement))                                     PURE; /**<DEPRECATED Registers a force element - ASSUMES WORLD-RELATIVE FORCE*/
    STDMETHOD_(BOOL, UnRegisterElement(__in IForceElement* pElement))                                   PURE; /**< Unregisters a force element */
    STDMETHOD_(UINT, ElementCount())                                                              const PURE; /**< Returns the total number of force elements */
    STDMETHOD_(IForceElement*, GetElement(int index))                                             const PURE; /**< Gets a force element */
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (ICollisionService, IUnknown)
{
    STDMETHOD_ (BOOL, InvokesCrashOnOtherObjects())                             PURE;
    STDMETHOD_ (void, SetInvokesCrashOnOtherObjects(__in BOOL invokesCrash))    PURE;
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IAircraftService, IUnknown)
{
    STDMETHOD_(float, GetIndicatedAirspeed)() const PURE;  //Feet per second
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IBoatService, IUnknown)
{
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IGroundVehicleService, IUnknown)
{
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IAtcService, IUnknown)
{
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IRadarSignatureService, IUnknown)
{
};

/**********************************************************************************************************
**********************************************************************************************************/
DECLARE_INTERFACE_ (IDoorService, IUnknown)
{
    STDMETHOD_(float, GetDoorPercentOpen)(__in int doorIndex)                             const PURE;
};

/**********************************************************************************************************
**********************************************************************************************************/

DECLARE_INTERFACE_ (IMarkerManagerV01, IUnknown)
{
    STDMETHOD (CreateObjectMarker)      (__in UINT idObject, __out HANDLE& hHandle)            PURE;
    STDMETHOD (UpdateObjectMarkerOffset)(__in const HANDLE hHandle, __in const FXYZ& vOffset)  PURE;
    STDMETHOD (RemoveMarker)            (__inout HANDLE& hHandle)                              PURE;
};

/**********************************************************************************************************
**********************************************************************************************************/

DECLARE_INTERFACE_(IRadioSystemV400, IUnknown)
{
    STDMETHOD_(BOOL, AreRadiosActive)()    const PURE;
};

/**********************************************************************************************************
**********************************************************************************************************/

DECLARE_INTERFACE_(IAttachmentServiceV420, IUnknown)
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
};

//---------------------------------------------------------------------------
// Predefined UUIDs

interface __declspec(uuid("{8a645399-229d-4e7c-8def-5b0912810f10}")) ISimObjectManagerV400;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV400 = __uuidof(ISimObjectManagerV400);

interface __declspec(uuid("{4ee8eb41-3cc6-4d2f-b0c7-b63887298ecd}")) ISimObjectManagerV410;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV410 = __uuidof(ISimObjectManagerV410);

interface __declspec(uuid("{0ad42b42-8549-49a6-b738-a4b14bc3c893}")) ISimObjectManagerV430;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV430 = __uuidof(ISimObjectManagerV430);

interface __declspec(uuid("{34354443-98cb-4cd3-be2c-544780a3d5ed}")) ISimObjectManagerV440;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV440 = __uuidof(ISimObjectManagerV440);

interface __declspec(uuid("{B56046A5-C789-4DC6-A5A9-850AE5DC45E4}")) ISimObjectManagerV500;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV500 = __uuidof(ISimObjectManagerV500);

interface __declspec(uuid("{1A0207EE-09BA-4EB3-89FA-5084EE6C4F0A}")) ISimObjectManagerV520;
extern __declspec(selectany) REFIID IID_ISimObjectManagerV520 = __uuidof(ISimObjectManagerV520);

interface __declspec(uuid("{5c467f71-3817-4fff-95a8-447c2bb5eb56}")) ISimObject;
extern __declspec(selectany) REFIID IID_ISimObject = __uuidof(ISimObject);

interface __declspec(uuid("{a5177ac6-84a8-494b-a2fc-e29f6ddfa787}")) ISimObjectV01;
extern __declspec(selectany) REFIID IID_ISimObjectV01 = __uuidof(ISimObjectV01);

interface __declspec(uuid("{c4c4f969-e2c7-48cb-abfe-53030da2b0e8}")) ISimObjectV02;
extern __declspec(selectany) REFIID IID_ISimObjectV02 = __uuidof(ISimObjectV02);

interface __declspec(uuid("{ef0dda2e-3d35-4b4b-b112-fa5c0be48410}")) ISimObjectV03;
extern __declspec(selectany) REFIID IID_ISimObjectV03 = __uuidof(ISimObjectV03);

interface __declspec(uuid("{98f656db-80e9-4577-86d4-4778961deb0a}")) ISimObjectV400;
extern __declspec(selectany) REFIID IID_ISimObjectV400 = __uuidof(ISimObjectV400);

interface __declspec(uuid("{5612b28f-04f7-4090-ab84-b186a3060163}")) ISimulation;
extern __declspec(selectany) REFIID IID_ISimulation = __uuidof(ISimulation);

interface __declspec(uuid("{e950867e-fb49-4d0f-8331-e80917509c35}")) ISimulationV01;
extern __declspec(selectany) REFIID IID_ISimulationV01 = __uuidof(ISimulationV01);

interface __declspec(uuid("{65ba5c17-6e25-4d2f-926c-084e5b9876af}")) IBaseObjectV400;
extern __declspec(selectany) REFIID IID_IBaseObjectV400 = __uuidof(IBaseObjectV400);

interface __declspec(uuid("{474cdb85-5288-45e6-8907-2e3b0bcfac1f}")) IBaseObjectV410;
extern __declspec(selectany) REFIID IID_IBaseObjectV410 = __uuidof(IBaseObjectV410);

interface __declspec(uuid("{adb58368-3075-43a5-b860-c13367a7dd1e}")) IBaseObjectV430;
extern __declspec(selectany) REFIID IID_IBaseObjectV430 = __uuidof(IBaseObjectV430);

interface __declspec(uuid("{8e882182-9ee2-418d-ae1d-8ab31db4d060}")) IBaseObjectV440;
extern __declspec(selectany) REFIID IID_IBaseObjectV440 = __uuidof(IBaseObjectV440);

interface __declspec(uuid("{a5a2d08d-4cff-49ab-9b28-2523cfeb153c}")) IBaseObjectV450;
extern __declspec(selectany) REFIID IID_IBaseObjectV450 = __uuidof(IBaseObjectV450);

interface __declspec(uuid("{3687f401-c339-4f41-af43-afbfc70778b5}")) ISubSystemFactoryv440;
extern __declspec(selectany) REFIID IID_ISubSystemFactoryV440 = __uuidof(ISubSystemFactoryv440);

interface __declspec(uuid("{a2207a3b-5ea1-4187-acca-2fc1a9a1f4a3}")) IScenarioManagerV400;
extern __declspec(selectany) REFIID IID_IScenarioManagerV400 = __uuidof(IScenarioManagerV400);

interface __declspec(uuid("{7b601866-c316-46bc-abe2-fe9968cdc7ff}")) IScenarioManagerV430;
extern __declspec(selectany) REFIID IID_IScenarioManagerV430 = __uuidof(IScenarioManagerV430);

interface __declspec(uuid("{3220df91-6200-4f68-ae5e-eee812695b8b}")) IMassProperties;
extern __declspec(selectany) REFIID IID_IMassProperties = __uuidof(IMassProperties);

interface __declspec(uuid("{81392dc1-94d5-4a42-a96b-de37d9de8524}")) IMassPropertiesV01;
extern __declspec(selectany) REFIID IID_IMassPropertiesV01 = __uuidof(IMassPropertiesV01);

interface __declspec(uuid("{663a2f20-189d-4797-a293-b8451d078e00}")) IForceMoments;
extern __declspec(selectany) REFIID IID_IForceMoments = __uuidof(IForceMoments);

interface __declspec(uuid("{a20b378f-376f-4a8c-b5d3-521a5a7e100a}")) IForceMomentsV01;
extern __declspec(selectany) REFIID IID_IForceMomentsV01 = __uuidof(IForceMomentsV01);

interface __declspec(uuid("{231dfb4a-f68f-4b0e-88af-bd97308dd012}")) ICollisionService;
extern __declspec(selectany) REFIID IID_ICollisionService = __uuidof(ICollisionService);

interface __declspec(uuid("{402d68a6-156a-45ed-9222-db8bd001bd03}")) IAircraftService;
extern __declspec(selectany) REFIID IID_IAircraftService = __uuidof(IAircraftService);

interface __declspec(uuid("{6505f704-f983-4ccc-b1ed-f4a403f4b98a}")) IBoatService;
extern __declspec(selectany) REFIID IID_IBoatService = __uuidof(IBoatService);

interface __declspec(uuid("{c6661c56-aa27-4199-a397-1cb948fe8465}")) IGroundVehicleService;
extern __declspec(selectany) REFIID IID_IGroundVehicleService = __uuidof(IGroundVehicleService);

interface __declspec(uuid("{08f23e25-f169-4d4e-a345-e7b0a3df461f}")) IAtcService;
extern __declspec(selectany) REFIID IID_IAtcService = __uuidof(IAtcService);

interface __declspec(uuid("{77dab198-df1d-4fa8-9818-db8041387129}")) IRadarSignatureService;
extern __declspec(selectany) REFIID IID_IRadarSignatureService = __uuidof(IRadarSignatureService);

interface __declspec(uuid("{4dcb87d6-eea7-4562-9481-a3948b500172}")) IDoorService;
extern __declspec(selectany) REFIID IID_IDoorService = __uuidof(IDoorService);

interface __declspec(uuid("{c2982c23-5403-4b8e-87b6-f7171ba3efd0}")) IMarkerManagerV01;
extern __declspec(selectany) REFIID IID_IMarkerManagerV01 = __uuidof(IMarkerManagerV01);

interface __declspec(uuid("{2f1901b7-60e3-418b-9d9d-ce0b87a8bff9}")) IRadioSystemV400;
extern __declspec(selectany) REFIID IID_IRadioSystemV400 = __uuidof(IRadioSystemV400);

interface __declspec(uuid("{358adf81-3ed2-42dc-8288-67ce145978b8}")) IAttachmentServiceV420;
extern __declspec(selectany) REFIID IID_IAttachmentServiceV420 = __uuidof(IAttachmentServiceV420);

/**********************************************************************************************************
**********************************************************************************************************/

} //End namespace P3D
