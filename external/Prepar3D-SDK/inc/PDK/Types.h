// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// Types.h
// Provides types used by the ISimObject Samples

#pragma once

#include <Pdk.h>

namespace P3D
{

    /** @defgroup types PDK Types
    *  The window and camera services enables external applications to control basic camera functions as well as adding/removing
    *  post-process effects and sensor modes using window plug-ins.
    *  @{
    */

interface IBaseObjectV400;
interface ISimObject;

///Simple vector structures for passing between P3D host and implementation
#define DXYZ P3DDXYZ
#define FXYZ P3DFXYZ


///COLLISIONTYPE - Object-to-object collision results
///- Returned in IBaseObject::CheckCollision
enum COLLISIONTYPE
{
    COLLISIONTYPE_NONE,
    COLLISIONTYPE_SIMOBJECT,
    COLLISIONTYPE_BUILDING,
    COLLISIONTYPE_TERRAIN,
    COLLISIONTYPE_OTHER,
};

///Type of keyboard or joystick mapping associated with an event being registered.
enum EVENTTYPE
{
    EVENTTYPE_NORMAL,
    EVENTTYPE_AXIS,
    EVENTTYPE_POV,
    //The following are used specifically for subsystem addons on P3D native simobject implementations
    // when the addon requires overriding the native events.
    EVENTTYPE_NORMAL_NATIVE_OVERRIDE,
    EVENTTYPE_AXIS_NATIVE_OVERRIDE,
    EVENTTYPE_POV_NATIVE_OVERRIDE,
    EVENTTYPE_NOT_MAPPABLE,     //The event will not appear in the Controls Assignment UI
};

///Types of properties
enum PROPERTY_TYPE
{
    PROPERTY_TYPE_EVENT,
    PROPERTY_TYPE_EVENT_WITH_SUBSTRING_INPUT,
    PROPERTY_TYPE_EVENT_VECTOR,
    PROPERTY_TYPE_EVENT_STRING,
    PROPERTY_TYPE_DOUBLE,
    PROPERTY_TYPE_DOUBLE_WITH_SUBSTRING_INPUT,
    PROPERTY_TYPE_STRING,
    PROPERTY_TYPE_VECTOR,
    NUM_PROPERTY_TYPES,
};

///Types of data saved via the PSaveLoadCallback
typedef enum 
{
    SAVED_DATA_TYPE_DOUBLE,
    SAVED_DATA_TYPE_STRING,
    NUM_SAVED_DATA_TYPES
}SAVED_DATA_TYPE;

///Types of network modes for a given object
typedef enum
{
    NET_MODE_TYPE_NORMAL,   // The object is owned by the current client
    NET_MODE_TYPE_REMOTE,   // The object is owned by another client
    NET_MODE_TYPE_MASTER,   // Shared Cockpit: The object is owned by the current client
    NET_MODE_TYPE_SLAVE,    // Shared Cockpit: The object is owned by another client
}NET_MODE_TYPE;

///Flag based types of ray trace interogation
typedef enum
{
    INTEROGATIONTYPE_NONE = 0,
    INTEROGATIONTYPE_TERRAIN = 1,
    INTEROGATIONTYPE_OBJECTS = 2,
}INTEROGATIONTYPE;

///General time of day
enum TIMEOFDAY
{
    TIMEOFDAY_INVALID = -1,
    TIMEOFDAY_DAWN,
    TIMEOFDAY_DAY,
    TIMEOFDAY_DUSK,
    TIMEOFDAY_NIGHT,
    NUM_PERIODS_OF_DAY,
};

/// Used in time queries to reference Zulu (UTC) or Local time.
enum TIMEREF
{
    TIMEREF_INVALID = -1,
    TIMEREF_LOCAL,
    TIMEREF_ZULU,
    NUM_TIMEREF,
};

// TACAN band
typedef enum
{
    TACAN_BAND_X,
    TACAN_BAND_Y,
    TACAN_BAND_INVALID
} TACAN_BAND, *PTACAN_BAND, **PPTACAN_BAND;

///Function pointer prototype for object implementation creation function.  This function is called for 
/// each instance of the respective simobject class
typedef HRESULT (STDMETHODCALLTYPE *PSimCreateFunc)(__in __notnull IBaseObjectV400*, __out __notnull ISimObject**);

///Function pointer for state save/load (.FXML files)  This pointer is passed to each ISimulation when it's time for save/load
///Section names will automatically be constructed as: [SectionName.Instance.SimObjectID] allowing for multiple instances of a system (e.g. multi-engines)
///Constructed section names are limited to a maximum of 128 characters.  SAVED_DATA_TYPE allows you to distinguish your data between numeric and string values
typedef HRESULT (STDMETHODCALLTYPE *PSaveLoadCallback)(__in LPCWSTR pszSection, __in unsigned int uInstance, __in LPCWSTR pszKeyword, __inout  void* pvVal, __in const SAVED_DATA_TYPE eDataType);

///Function callback for notification that a new scenario is loaded or saved.  
///These are registered through the IScenarioManager interface
typedef void (STDMETHODCALLTYPE *PNewScenarioNotify)(BOOL bOnSave);


///   Property callback function pointers

///Max string length for property names
static const unsigned int MAX_PROPERTY_NAME = 64;    
///Double property callback
typedef HRESULT (STDMETHODCALLTYPE *PPropertyCallback)      (__in const ISimObject& Sim, __out double& dProperty, __in int iIndex);     //Returns a 64-bit property value
///Vector (double x,y,z) property callback
typedef HRESULT (STDMETHODCALLTYPE *PPropertyVectorCallback)(__in const ISimObject& Sim, __out DXYZ&   vProperty, __in int iIndex);     //Returns a 64-bit vector value
///String property callback
typedef HRESULT (STDMETHODCALLTYPE *PPropertyStringCallback)(__in const ISimObject& Sim, __out LPWSTR  pszProperty, __in UINT uStringLength, __in int iIndex);     //Returns string
///Double property callback (with secondary string input)
typedef HRESULT (STDMETHODCALLTYPE *PPropertyCallbackWithSubString)(__in const ISimObject& Sim, __out double& dProperty, __in LPCWSTR pszSecondarySubstring, __in int iIndex);     //Returns a 64-bit property value

///Event input callback
typedef HRESULT (STDMETHODCALLTYPE *PEventCallback)         (__in       ISimObject& Sim, __in  double  dProperty, __in int iIndex);     //Triggers an implemented event
///Event input callback
typedef HRESULT (STDMETHODCALLTYPE *PEventVectorCallback)   (__in       ISimObject& Sim, __in  const DXYZ& vProperty, __in int iIndex);  //Triggers an implemented event for vectors
///Event input callback
typedef HRESULT (STDMETHODCALLTYPE *PEventStringCallback)   (__in       ISimObject& Sim, __in  LPCWSTR pszProperty, __in int iIndex);  //Triggers an implemented event for vectors
///Event input callback for double (with secondary string input)
typedef HRESULT (STDMETHODCALLTYPE *PEventCallbackWithSubString)(__in   ISimObject& Sim, __in  double dProperty, __in LPCWSTR pszSecondarySecondarySubstring, __in int iIndex);  //Triggers an implemented event for string based properties

///Register callback on object creation. This could be a simobject or a library object
typedef HRESULT (STDMETHODCALLTYPE *POnObjectCreateCallback)(__in IUnknown& Obj );  
///Register callback on object removal. This could be a simobject or a library object
typedef HRESULT (STDMETHODCALLTYPE *POnObjectRemoveCallback)(__in IUnknown& Obj );  
///Register callback on user object changing from one user to another (during runtime, not scenario load or vehicle change via UI)
typedef HRESULT (STDMETHODCALLTYPE *POnUserObjectChangedCallback)(__in IUnknown& NewObj, __in IUnknown& OldObj );  

///AI Unit modes
typedef enum
{
    UNITMODE_SLEEP,
    UNITMODE_ZOMBIE,
    UNITMODE_WAYPOINT,
    UNITMODE_TAKEOFF,
    UNITMODE_LANDING,
    UNITMODE_TAXI,
    UNITMODE_WORKING,
    UNITMODE_WAITING,
} UNITMODE;


///BasicWaypoint
class BasicWaypoint
{
public:
    DXYZ vLonAltLat;        //Lat/Lon (Radians), Alt (Feet)
    double dHeading;        //Radians
};

/**************************************************************************************************************
EntityType - A class used to represent a given entity's EntityType.
**************************************************************************************************************/
class EntityType
{
public:

	EntityType() noexcept :
		m_byKind(0),
		m_byDomain(0),
		m_usCountry(0),
		m_byCategory(0),
		m_bySubcategory(0),
		m_bySpecific(0),
		m_byExtra(0)
	{
	}

	unsigned char   GetKind()                                       const { return m_byKind; } /**< Gets DIS entity kind */
	void            SetKind(unsigned char byKind) { m_byKind = byKind; } /**< Sets DIS entity kind */

	unsigned char   GetDomain()                                     const { return m_byDomain; } /**< Gets DIS entity domain */
	void            SetDomain(unsigned char byDomain) { m_byDomain = byDomain; } /**< Sets DIS entity domain */

	unsigned short  GetCountry()                                    const { return m_usCountry; } /**< Gets DIS entity country */
	void            SetCountry(unsigned short usCountry) { m_usCountry = usCountry; } /**< Sets DIS entity country */

	unsigned char   GetCategory()                                   const { return m_byCategory; } /**< Gets DIS entity category */
	void            SetCategory(unsigned char byCategory) { m_byCategory = byCategory; } /**< Sets DIS entity category */

	unsigned char   GetSubcategory()                                const { return m_bySubcategory; } /**< Gets DIS entity subcategory */
	void            SetSubcategory(unsigned char bySubcategory) { m_bySubcategory = bySubcategory; } /**< Sets DIS entity subcategory */

	unsigned char   GetSpecific()                                   const { return m_bySpecific; } /**< Gets DIS entity specific */
	void            SetSpecific(unsigned char bySpecific) { m_bySpecific = bySpecific; } /**< Sets DIS entity specific */

	unsigned char   GetExtra()                                      const { return m_byExtra; } /**< Gets DIS entity extra */
	void            SetExtra(unsigned char byExtra) { m_byExtra = byExtra; } /**< Sets DIS entity extra */

private:

	unsigned char   m_byKind; /**< DIS entity kind */
	unsigned char   m_byDomain; /**< DIS entity domain */
	unsigned short  m_usCountry; /**< DIS entity country */
	unsigned char   m_byCategory; /**< DIS entity category */
	unsigned char   m_bySubcategory; /**< DIS entity subcategory */
	unsigned char   m_bySpecific; /**< DIS entity specific */
	unsigned char   m_byExtra; /**< DIS entity extra */
};
 /** @} */ // end of group
} //End namespace P3D