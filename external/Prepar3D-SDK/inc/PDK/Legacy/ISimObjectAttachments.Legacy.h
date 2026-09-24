// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
/************************************************************************************************************** 
    ISimObjectAttachments.h    
    Provides public interfaces for ISimObject Attachment Systems implementations
***************************************************************************************************************/

#pragma once

namespace P3D
{
interface ICountermeasureService;
interface IWeaponServiceV400;
interface IPylonService;
interface IGunV400;

/** ## *Professional Plus Only*
* Interface for getting gun system parameters for this object */
DECLARE_INTERFACE_(IGunSystemV400, ISimulation)
{
    /** Adds an IGun implementation to the internal GunSystem at the given station. Returns TRUE if successfully added, and FALSE otherwise.
    * If successfully added, the IGun reference count will be increased by one. */
    STDMETHOD_(BOOL, AddGun(__in IGunV400* pGun, __in UINT stationIndex))                                       PURE;
    /** Removes an IGun implementation from the internal GunSystem at the given station. Returns TRUE if successfully removed, and FALSE
    * otherwise. If successfully removed, the IGun reference count will be decreased by one. */
    STDMETHOD_(BOOL, RemoveGun(__in UINT stationIndex))                                                         PURE;

    /** Gets a pointer to an IGun implementation at the given station if it exists, otherwise returns NULL. */
    STDMETHOD_(const IGunV400*, GetGun(__in UINT stationIndex))                                         const   PURE;

    STDMETHOD_(void, SetIsActive(__in BOOL isActive))                                                           PURE; /**< Sets a gun system to active (true) or inactive (false) */
    STDMETHOD_(BOOL, GetIsActive())                                                                     const   PURE; /**< Gets the active state of the gun system */
    STDMETHOD_(BOOL, GetIsFiring())                                                                     const   PURE; /**< Gets if a gun system is currently firing */

    STDMETHOD_(void, SetIsAutomatedGunsEnabled(__in BOOL isAutomatedGunsEnabled))                               PURE; /**< Sets the state of the automatic gun targeting. */
    STDMETHOD_(BOOL, GetIsAutomatedGunsEnabled())                                                       const   PURE; /**< Gets the state of the automatic gun targeting. */

    STDMETHOD_(void, EngageTrigger())                                                                           PURE; /**< Engages the trigger of a gun system */
    STDMETHOD_(void, DisengageTrigger())                                                                        PURE; /**< Disengages the trigger of a gun system */

    STDMETHOD_(UINT, GetNumberOfStations())                                                             const   PURE; /**< Returns the number of stations available to the gun system */
    STDMETHOD_(UINT, GetNumberOfGuns())                                                                 const   PURE; /**< Returns the number of guns loaded by the gun system */

    STDMETHOD_(BOOL, IsStationIndexValid(__in UINT stationIndex))                                       const   PURE; /**< Checks if an index refers to an existing station */

    STDMETHOD_(BOOL, GetIsGunPresentAtStation(__in UINT stationIndex))                                  const   PURE; /**< Checks if a station contains a gun */
    STDMETHOD_(BOOL, GetIsGunSelectedAtStation(__in UINT stationIndex))                                 const   PURE; /**< Checks if a station contains a selected gun */

    STDMETHOD_(BOOL, IsSystemOn)()                                                                      const   PURE; /**< Returns whether or not the system is on. */
    STDMETHOD_(void, ToggleSystem())                                                                            PURE; /**< Toggles a gun system on and off */

    STDMETHOD_(void, ToggleStation(__in UINT stationIndex))                                                     PURE; /**< Toggles a station on and off */

    STDMETHOD_(void, SelectStationOn) (UINT iStationIndex, BOOL bExclusiveOn)                                   PURE; /**< Turns selected station on @param bExclusiveOn Turns all other stations off */
    STDMETHOD_(void, SelectStationOff)(UINT iStationIndex, BOOL bAllOff)                                        PURE; /**< Turns selected station off @param bAllOff Turns all stations off */

                                                                                                                      /** Creates a tracer visual effect at the given location. Longitude, altitude, and latitude units are radians and feet. Pitch, heading, and bank units are radians */
    STDMETHOD_(void, CreateTracerEffect(__in __notnull const WCHAR* pszEffectName, __in const DXYZ* pvLonAltLat, __in const DXYZ* pvPHB, __out void** ppEffect)) PURE;
    /** Moves an existing visual effect to the given location. Same position and rotation values above apply */
    STDMETHOD_(void, MoveTracerEffect(__in const DXYZ* pvLonAltLat, __in const DXYZ* pvPHB, __in void* pEffect)) PURE;
    STDMETHOD_(void, DestroyTracerEffect(__in void* pEffect)) PURE; /**< Removes a tracer effect if it is still alive */

                                                                    /**< Performs collision detection at a location in the form longitude, altitude, and latitude, where the units are radians and feet. The delta offset should be given from this location in feet */
    STDMETHOD_(BOOL, CheckBulletCollision(__in const DXYZ* pvLonAltLat, __in const DXYZ* pvDeltaOffset, __out COLLISIONTYPE& eCollision, __out IUnknown** ppUnkObjectHit)) PURE;

    STDMETHOD_(void, ToggleAutomaticGuns)()             PURE; /**< Toggles the state of automatic gun targeting on and off */
    STDMETHOD_(void, SetPitchPercent)(float fPercent)   PURE; /**< Change the pitch of guns based on percentage. [-1, 1] */
    STDMETHOD_(void, SetHeadingPercent)(float fPercent) PURE; /**< Change the heading of guns based on percentage. [-1, 1] */
    STDMETHOD_(void, IncrementLeft)()                   PURE; /**< Moves gun left */
    STDMETHOD_(void, IncrementRight)()                  PURE; /**< Moves gun right */
    STDMETHOD_(void, IncrementUp)()                     PURE; /**< Moves gun up */
    STDMETHOD_(void, IncrementDown)()                   PURE; /**< Moves gun down */
    STDMETHOD_(void, IncrementLeftAndUp)()              PURE; /**< Moves gun left and up */
    STDMETHOD_(void, IncrementLeftAndDown)()            PURE; /**< Moves gun left and down */
    STDMETHOD_(void, IncrementRightAndUp)()             PURE; /**< Moves gun right and up */
    STDMETHOD_(void, IncrementRightAndDown)()           PURE; /**< Moves gun right and down */

    STDMETHOD_(void, ResetGuns)()                       PURE; /**< Resets the gun loadout to its original state. This does not change station/pylon point selection. */
};

DEFINE_GUID(IID_IGunSystemV400, 0xb4ceb0c3, 0x18ce, 0x430b, 0x90, 0x78, 0xa4, 0x71, 0x78, 0x2e, 0x4a, 0x28);

/** ## *Professional Plus Only*
* Interface to the Prepar3D native weapon system.
* Can also be used to implement a custom weapon system */
DECLARE_INTERFACE_(IWeaponsSystemV400, ISimulation)
{
	STDMETHOD_(BOOL, GetIgnoreAttachmentForces)()                const PURE; /**< Gets the Global Attachment Setting for ignoring forces */
	STDMETHOD_(void, SetIgnoreAttachmentForces)(BOOL enabled)          PURE; /**< Sets the Global Attachment Setting for ignoring forces */

	STDMETHOD_(BOOL, GetIgnoreAttachmentWeight)()                const PURE; /**< Gets the Global Attachment Setting for ignoring weight */
	STDMETHOD_(void, SetIgnoreAttachmentWeight)(BOOL enabled)          PURE; /**< Sets the Global Attachment Setting for ignoring forces */

	STDMETHOD_(UINT, GetNumberOfStations)()                      const PURE; /**< Gets the number of stations available to the weapon system */
	STDMETHOD_(UINT, GetStationQuantity)(UINT iStationIndex)     const PURE; /**< Gets the the total number of weapons loaded at the given */
	STDMETHOD_(BOOL, HasPylon)(UINT iStationIndex)               const PURE; /**< Returns true if a station contains a pylon, false otherwise */
	STDMETHOD_(UINT, GetNumberOfPylonPoints)(UINT iStationIndex) const PURE; /**< Gets the total number of pylon points that a station contains */

	STDMETHOD(GetWeapon)(__in UINT iStationIndex, __in UINT iPylonIndex, __out UINT& uObjectId, __out IWeaponServiceV400** ppWeapon)  const PURE; /**< Gets a weapon */

	STDMETHOD_(BOOL, IsSystemOn)()                                                                  const PURE; /**< Returns whether or not the system is on. */
	STDMETHOD_(BOOL, IsSystemArmed)()                                                               const PURE; /**< Returns whether or not the system is armed. */
	STDMETHOD_(BOOL, IsSafetyOn)()                                                                  const PURE; /**< Returns whether or not system's safety is on. */

																												// User inputs
	STDMETHOD_(void, ToggleSystem)()                                                                      PURE; /**< Toggles a weapon system on and off */
	STDMETHOD_(void, ToggleArmed)()                                                                       PURE; /**< Arms and disarms a weapon system */
	STDMETHOD_(void, ToggleSafety)()                                                                      PURE; /**< Toggles a weapon system safety on and off */
																												/** Engages a weapon system trigger @param bSingleShot Sets max of one shot per trigger engage
																												* @note In order to work properly, EngageTrigger() cannot be called every frame. At least one frame must pass without a call to EngageTrigger() to allow the trigger state to reset. */
	STDMETHOD_(void, EngageTrigger)(BOOL bSingleShot)                                                     PURE;
	STDMETHOD_(void, DisengageTrigger)()                                                                  PURE; /**< Disengages a weapon system trigger */
																												/** Triggers jettison of a currently selected weapon system
																												* @note In order to work properly, TriggerJettison() cannot be called every frame.At least one frame must pass without a call to TriggerJettison() to allow the trigger state to reset. */
	STDMETHOD_(void, TriggerJettison)()                                                                   PURE;
	STDMETHOD_(BOOL, IsStationSelected)(UINT iStationIndex)                                         const PURE; /**< Returns true if a station if selected */
	STDMETHOD_(void, SelectNextStation)()                                                                 PURE; /**< Selects the next station, even if empty */
	STDMETHOD_(void, SelectPreviousStation)()                                                             PURE; /**< Selects the previous station, even if empty */
																												/** Selects the next station for the weapon type corresponding to the index defined in the WeaponSelectorTypes list in attachments.xml */
	STDMETHOD_(void, SetSelectedWeaponTypeIndex)(UINT uData)                                              PURE;
	STDMETHOD_(void, ToggleStation)(UINT iStationIndex)                                                   PURE; /**< Toggles the selection of the given station on and off */
	STDMETHOD_(void, SelectStationOn) (UINT iStationIndex, BOOL bExclusiveOn)                             PURE; /**< Turns selected station on @param bExclusiveOn Turns all other stations off */
	STDMETHOD_(void, SelectStationOff)(UINT iStationIndex, BOOL bAllOff)                                  PURE; /**< Turns selected station off @param bAllOff Turns all stations off */
	STDMETHOD_(void, SelectPylonPointOn)(UINT iStationIndex, UINT iPylonPoint, BOOL bExclusiveOn)         PURE; /**< Turns selection pylon point on @param bExclusiveOn Turns all other pylon points off at station */
	STDMETHOD_(void, SelectPylonPointOff)(UINT iStationIndex, UINT iPylonPoint, BOOL bAllOff)             PURE; /**< Turns selection pylon point off @param bAllOff Turns all pylon points off at station */
	STDMETHOD_(void, SelectNextWeapon)()                                                                  PURE; /**< Select the next available weapon regardless of type */
	STDMETHOD_(void, SelectPreviousWeapon)()                                                              PURE; /**< Select the previous available weapon regardless of type */
	STDMETHOD_(void, ResetWeapons)()                                                                      PURE; /**< Reset the weapon loadouts to their original state. This does not change station/pylon point selection. */
};

DEFINE_GUID(IID_IWeaponsSystemV400, 0x32c700dd, 0xdc36, 0x42bf, 0xac, 0xbe, 0x30, 0xca, 0xe2, 0x57, 0x22, 0x86);

/** ## *Professional Plus Only*
* Interface to the Prepar3D native weapon system.
* Can also be used to implement a custom weapon system */
DECLARE_INTERFACE_(IWeaponsSystemV430, IWeaponsSystemV400)
{
    STDMETHOD (GetPylon)(__in UINT iStationIndex, __out UINT& uObjectId, __out IPylonService** ppPylon)  const PURE; /**< Gets a pylon */
};

DEFINE_GUID(IID_IWeaponsSystemV430, 0x5f323180, 0x87d6, 0x4c7d, 0xb0, 0xb6, 0x21, 0xcf, 0xc6, 0x80, 0x6a, 0x32);

/** ## *Professional Plus Only*
* Interface for getting weapon parameters for this object */
DECLARE_INTERFACE_(IWeaponServiceV400, IUnknown)
{
    STDMETHOD(SetIsAttachedToOwner)(BOOL bAttached, UINT uOwnerId, BOOL bJettisoned)        PURE;     /**< Called from weapon system when attached, jettisoned, or fired (0 = invalid id) */
    STDMETHOD_(BOOL, IsAttachedToOwner)()                                              const PURE;     /**< Is weapon currently attached to parent object */
    STDMETHOD_(UINT, GetOwnerId)()                                                     const PURE;     /**< ID of object in which weapon is attached (should remain valid even after detached) */
    STDMETHOD(GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)                          const PURE;     /**< Gets the offset on the weapon in which it is attached to the parent */
    STDMETHOD_(BOOL, CanWeaponBeReleased)()                                            const PURE;     /**< Called upon firing of weapon.  The weapon implementation can block being released */

    STDMETHOD_(float, GetAerodynamicsDragCoefficient)(float fMach)  const PURE; /**< Gets the aerodynamic drag for the weapon loadout UI in SimDirector */

    /** Gets the string type of weapon (e.g. "AAM", "SAM"). These are defined for native weapons in sim.cfg. It is dependent on the weapon implementation, but can be used for arbitrary categorization */
    STDMETHOD(GetType)(__out LPWSTR pszType, __in unsigned int uLength)   const PURE;
};

DEFINE_GUID(IID_IWeaponServiceV400, 0x7d59d798, 0x5c9, 0x4001, 0xbe, 0xf8, 0x3a, 0xff, 0x5f, 0x36, 0x23, 0xd0);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
ICounterMeasureSystem - Used to implement or query  for a countermeasure system
**********************************************************************************************************/
DECLARE_INTERFACE_(ICountermeasureSystem, ISimulation)
{
    STDMETHOD_(UINT, GetNumberOfStations)()                               const PURE;
    STDMETHOD_(UINT, GetStationQuantity)(UINT iStationIndex)              const PURE;

    STDMETHOD (GetCountermeasure)(__in UINT iStationIndex, __in UINT iPylonIndex, __out UINT& uObjectId, __out ICountermeasureService** ppCM)  const PURE;

    STDMETHOD_(void, ToggleSystem)()                                            PURE;
    STDMETHOD_(void, ToggleArmed)()                                             PURE;
    STDMETHOD_(void, EngageTrigger)(BOOL bSingleShot)                           PURE;
    STDMETHOD_(void, DisengageTrigger)()                                        PURE;

    STDMETHOD_(BOOL, IsStationSelected)(UINT iStationIndex)               const PURE;
    STDMETHOD_(void, SelectNextStation)()                                       PURE;
    STDMETHOD_(void, SelectPreviousStation)()                                   PURE;
    STDMETHOD_(void, ToggleStation)(UINT iStationIndex)                         PURE;
    STDMETHOD_(void, SelectStationOn) (UINT iStationIndex, BOOL bExclusiveOn)   PURE;   //bExclusive => Turns all others off
    STDMETHOD_(void, SelectStationOff)(UINT iStationIndex, BOOL bAllOff)        PURE;   //bAllOff => Turns all stations off

    STDMETHOD_(void, SelectNextCountermeasure)()                                PURE;
    STDMETHOD_(void, SelectPreviousCountermeasure)()                            PURE;
    STDMETHOD_(void, ResetCountermeasures)()                                    PURE;
};

DEFINE_GUID(IID_ICountermeasureSystem, 0xb1fe591d, 0xfa0c, 0x469a, 0xaa, 0x72, 0x2d, 0x1, 0xfb, 0xdb, 0xcb, 0x30);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
ICountermeasureService - Interface for getting countermeasure parameters for this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (ICountermeasureService, IUnknown)
{
    STDMETHOD  (SetIsAttachedToOwner)(BOOL bAttached, UINT uOwnerId)          PURE;     //Called from countermeasure system when attached or fired (0 = invalid id)
    STDMETHOD_ (BOOL, IsAttachedToOwner)()                              const PURE;     //Is weapon currently attached to parent object
    STDMETHOD_ (UINT, GetOwnerId)()                                     const PURE;     //ID of object in which countermeasure is attached (shoud remain valid even after detached)
    STDMETHOD  (GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)          const PURE;     //Gets the offset on the weapon in which it is attached to the parent
};

DEFINE_GUID(IID_ICountermeasureService, 0x872e6f7b, 0xe348, 0x4592, 0x9c, 0x55, 0x96, 0x7, 0x4, 0xc0, 0xbf, 0x3f);

DECLARE_INTERFACE_ (ICountermeasureServiceV01, ICountermeasureService)
{
};

DEFINE_GUID(IID_ICountermeasureServiceV01,  0xb6ed8246, 0xc666, 0x40b3, 0x9b, 0xbf, 0x42, 0x53, 0xa4, 0x8e, 0xc7, 0x41);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IFireControlSystem - Interface for getting fire control system parameters for this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IFireControlSystem, ISimulation)
{
    STDMETHOD_(UINT, GetSelectedTargetID)()                      const PURE;
    STDMETHOD_(void, SetSelectedTargetID)(UINT id)                     PURE;
    STDMETHOD (GetSelectedTargetMissionID)(__out GUID& guid)     const PURE;
    STDMETHOD (SetSelectedTargetMissionID)(__in const GUID& guid)      PURE;
    STDMETHOD_(BOOL, GetTargetLLA)(__out P3D::DXYZ& vLLA)        const PURE;
    STDMETHOD_(void, SetTargetLLA)(__in const P3D::DXYZ& vLLA)         PURE;
};

DEFINE_GUID(IID_IFireControlSystem, 0x96d3952b, 0x2bd3, 0x4fc5, 0xa6, 0xc, 0xb3, 0x1b, 0x86, 0x54, 0xa6, 0xfd);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IGuidanceSystem - Interface for getting guidance parameters for this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IGuidanceSystem, IUnknown)
{
    STDMETHOD_(void,   SetTargetObjectID)(UINT targetedObjectID)        PURE;
    STDMETHOD_(UINT,   GetTargetObjectID)()                       const PURE;
    STDMETHOD_(void,   SetTargetLLA)(__in const P3D::DXYZ& vLLA)        PURE;
    STDMETHOD_(BOOL,   GetTargetLLA)(__out P3D::DXYZ& vLLA)       const PURE;
};

DEFINE_GUID(IID_IGuidanceSystem, 0x9f8a59df, 0x12d1, 0x4051, 0x8c, 0x4a, 0x62, 0x7e, 0x8b, 0x54, 0x7d, 0x81);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IPylonService - Interface for getting parameters for a weapon pylon
**********************************************************************************************************/
DECLARE_INTERFACE_ (IPylonService, IUnknown)
{
    STDMETHOD  (SetOwnerId)(__in UINT uOwnerId)                        PURE;     //ID of object in which weapon is attached
    STDMETHOD_ (UINT, GetOwnerId)()                              const PURE;     //ID of object in which weapon is attached
    STDMETHOD  (GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)   const PURE;     //Gets the offset on the weapon in which it is attached to the parent
};

DEFINE_GUID(IID_IPylonService, 0x75b884f, 0x26a9, 0x4a97, 0xb5, 0x1e, 0x6c, 0x67, 0x98, 0x3d, 0x2a, 0x79);

}
