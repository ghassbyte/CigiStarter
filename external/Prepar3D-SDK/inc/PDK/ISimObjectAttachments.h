// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ISimObjectAttachments.h
// Provides public interfaces for ISimObject Attachment Systems implementations
//-----------------------------------------------------------------------------------

#pragma once

#include "Legacy\ISimObjectAttachments.Legacy.h"  //Include previous version

namespace P3D
{

/** @addtogroup isim */ /** @{ */

interface ICountermeasureService;
interface IGunV400;

/** ## *Professional Plus Only*
* Interface to the Prepar3D native weapon system. 
* Can also be used to implement a custom weapon system */
DECLARE_INTERFACE_(IWeaponsSystemV440, IWeaponsSystemV430)
{
    STDMETHOD_(BOOL, GetIgnoreAttachmentForces)()                const PURE; /**< Gets the Global Attachment Setting for ignoring forces */
    STDMETHOD_(void, SetIgnoreAttachmentForces)(BOOL enabled)          PURE; /**< Sets the Global Attachment Setting for ignoring forces */

    STDMETHOD_(BOOL, GetIgnoreAttachmentWeight)()                const PURE; /**< Gets the Global Attachment Setting for ignoring weight */
    STDMETHOD_(void, SetIgnoreAttachmentWeight)(BOOL enabled)          PURE; /**< Sets the Global Attachment Setting for ignoring forces */

    STDMETHOD_(UINT, GetNumberOfStations)()                      const PURE; /**< Gets the number of stations available to the weapon system */
    STDMETHOD_(UINT, GetStationQuantity)(UINT iStationIndex)     const PURE; /**< Gets the the total number of weapons loaded at the given */
    STDMETHOD_(BOOL, HasPylon)(UINT iStationIndex)               const PURE; /**< Returns true if a station contains a pylon, false otherwise */
    STDMETHOD_(UINT, GetNumberOfPylonPoints)(UINT iStationIndex) const PURE; /**< Gets the total number of pylon points that a station contains */

    STDMETHOD (GetWeapon)(__in UINT iStationIndex, __in UINT iPylonIndex, __out UINT& uObjectId, __out IWeaponServiceV400** ppWeapon)  const PURE; /**< Gets a weapon */
    STDMETHOD (GetPylon)(__in UINT iStationIndex, __out UINT& uObjectId, __out IPylonService** ppPylon)  const PURE; /**< Gets a pylon */

    STDMETHOD_(void, SetStationLoadOut)(__in UINT32 stationIndex, __in LPCTSTR pszWeaponTitle, __in UINT32 roundsRemaining, __in UINT32 roundsDefault, __in LPCTSTR pszPylonTitle) PURE; /**< Sets the station weapon and pylon */

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

DEFINE_GUID(IID_IWeaponsSystemV440,	0xe621d892, 0x892e, 0x40c4, 0x9a, 0xa6, 0xf6, 0xc6, 0xdb, 0xe6, 0x11, 0x4b);
DEFINE_GUID(SID_WeaponsSystem,      0xf48af1c9, 0x6db2, 0x4647, 0xbb, 0xb3, 0x2e, 0x3f, 0x65, 0xe3, 0x96, 0x57);

/** ## *Professional Plus Only*
* Interface for getting weapon parameters for this object */
DECLARE_INTERFACE_ (IWeaponServiceV420, IWeaponServiceV400)
{
    STDMETHOD  (SetIsAttachedToOwner)(BOOL bAttached, UINT uOwnerId, BOOL bJettisoned)        PURE;     /**< Called from weapon system when attached, jettisoned, or fired (0 = invalid id) */
    STDMETHOD_ (BOOL, IsAttachedToOwner)()                                              const PURE;     /**< Is weapon currently attached to parent object */
    STDMETHOD_ (UINT, GetOwnerId)()                                                     const PURE;     /**< ID of object in which weapon is attached (should remain valid even after detached) */
    STDMETHOD  (GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)                          const PURE;     /**< Gets the offset on the weapon in which it is attached to the parent */
    STDMETHOD_ (BOOL, CanWeaponBeReleased)()                                            const PURE;     /**< Called upon firing of weapon.  The weapon implementation can block being released */

    STDMETHOD_ (float, GetAerodynamicsDragCoefficient)(float fMach)  const PURE; /**< Gets the aerodynamic drag for the weapon loadout UI in SimDirector */

    /** Gets the string type of weapon (e.g. "AAM", "SAM"). These are defined for native weapons in sim.cfg. It is dependent on the weapon implementation, but can be used for arbitrary categorization */
    STDMETHOD (GetType)(__out LPWSTR pszType, __in unsigned int uLength)   const PURE;

    STDMETHOD_(BOOL, GetCausesWeaponCollision)() const PURE;     /**< Gets whether or not the weapon should collide with other weapons */
};

DEFINE_GUID(IID_IWeaponServiceV420, 0x6914eea9, 0xf77d, 0x47f5, 0x95, 0x51, 0xbe, 0x6f, 0x4b, 0x2b, 0x33, 0x8c);
DEFINE_GUID(SID_WeaponService,      0x498955b6, 0x58b2, 0x48e4, 0x97, 0x20, 0x71, 0x20, 0x2c, 0xc2, 0x37, 0xe2);

/** ## *Professional Plus Only*
* Used to implement or query  for a countermeasure system */
DECLARE_INTERFACE_(ICountermeasureSystemV01, ICountermeasureSystem)
{
    STDMETHOD_(UINT, GetNumberOfStations)()                               const PURE; /**< Gets number of countermeasure stations */
    STDMETHOD_(UINT, GetStationQuantity)(UINT iStationIndex)              const PURE; 

    STDMETHOD (GetCountermeasure)(__in UINT iStationIndex, __in UINT iPylonIndex, __out UINT& uObjectId, __out ICountermeasureService** ppCM)  const PURE; /**< Gets a countermeasure system */

    STDMETHOD_(BOOL, IsSystemOn)()                                        const PURE; /**< Returns whether or not the system is on. */
    STDMETHOD_(BOOL, IsSystemArmed)()                                     const PURE; /**< Returns whether or not the system is armed. */

    STDMETHOD_(void, ToggleSystem)()                                            PURE; /**< Toggles a countermeasure system on and off */
    STDMETHOD_(void, ToggleArmed)()                                             PURE; /**< Arms and disarms a countermeasure system */

    STDMETHOD_(void, EngageTrigger)(BOOL bSingleShot)                           PURE; /**< Triggers a countermeasure @param bSingleShot Sets max of one shot per trigger engage*/
    STDMETHOD_(void, DisengageTrigger)()                                        PURE; /**< Detriggers a countermeasure */

    STDMETHOD_(BOOL, IsStationSelected)(UINT iStationIndex)               const PURE; /**< Returns true if a station if selected */
    STDMETHOD_(void, SelectNextStation)()                                       PURE; /**< Selects the next station */
    STDMETHOD_(void, SelectPreviousStation)()                                   PURE; /**< Selects the previous station */
    STDMETHOD_(void, ToggleStation)(UINT iStationIndex)                         PURE; /**< Toggles a specific station on and off */
    STDMETHOD_(void, SelectStationOn) (UINT iStationIndex, BOOL bExclusiveOn)   PURE; /**< Turns selected station on @param bExclusiveOn Turns all other stations off */
    STDMETHOD_(void, SelectStationOff)(UINT iStationIndex, BOOL bAllOff)        PURE; /**< Turns selected station off @param bAllOff Turns all stations off */

    STDMETHOD_(void, SelectNextCountermeasure)()                                PURE; /**< Select the next countermeasure */
    STDMETHOD_(void, SelectPreviousCountermeasure)()                            PURE; /**< Select the previous countermeasure */
    STDMETHOD_(void, ResetCountermeasures)()                                    PURE; /**< Reset all countermeasures */
};

DEFINE_GUID(IID_ICountermeasureSystemV01,   0x3504d031, 0xc034, 0x4e15, 0xa0, 0xe1, 0x0, 0xda, 0x4b, 0xcd, 0x4, 0x60);
DEFINE_GUID(SID_CountermeasureSystem,       0x426c5966, 0xec8a, 0x4fb9, 0xae, 0xe5, 0xd3, 0x64, 0xfb, 0xca, 0x2a, 0x36);


/** ## *Professional Plus Only*
* Interface for getting countermeasure parameters for this object */
DECLARE_INTERFACE_ (ICountermeasureServiceV02, ICountermeasureServiceV01)
{
    STDMETHOD  (SetIsAttachedToOwner)(BOOL bAttached, UINT uOwnerId)          PURE;     /**< Called from countermeasure system when attached, jettisoned, or fired (0 = invalid id) */
    STDMETHOD_ (BOOL, IsAttachedToOwner)()                              const PURE;     /**< Is weapon currently attached to parent object */
    STDMETHOD_ (UINT, GetOwnerId)()                                     const PURE;     /**< ID of object in which countermeasure is attached (shoud remain valid even after detached) */
    STDMETHOD  (GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)          const PURE;     /**< Gets the offset on the weapon in which it is attached to the parent */
    STDMETHOD_ (BOOL, GetCausesWeaponCollision)()                       const PURE;     /**< Gets whether or not the countermeasure should collide with weapons */
};

DEFINE_GUID(IID_ICountermeasureServiceV02,  0xd528012a, 0x88ff, 0x4492, 0xad, 0xc7, 0x44, 0xe5, 0x25, 0x11, 0xa5, 0x82);
DEFINE_GUID(SID_CountermeasureService,      0x223c6ce1, 0x38b0, 0x490c, 0xac, 0x6, 0xc2, 0x3b, 0x99, 0x84, 0x3a, 0xfd);

/** ## *Professional Plus Only*
* Interface for getting gun system parameters for this object */
DECLARE_INTERFACE_(IGunSystemV440, IGunSystemV400)
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

    /*
    * Expected in Radians/Meters
    */
	STDMETHOD_(void, SetCrosshairTarget)(__in const double& lat, __in const double& lon, __in const double& alt) PURE;

	STDMETHOD_(void, ClearCrosshairTarget)()            PURE;
};

DEFINE_GUID(IID_IGunSystemV440, 0x5a31bef7, 0x85b0, 0x4418, 0xb8, 0xa9, 0xb, 0x15, 0xde, 0x31, 0x8b, 0xdf);
DEFINE_GUID(SID_GunSystem,      0x7814d5b7, 0x409e, 0x42d9, 0xa1, 0xe6, 0x85, 0x41, 0xd5, 0xb6, 0xa8, 0x6c);

/** ## *Professional Plus Only*
* Interface for getting gun parameters for this object */
DECLARE_INTERFACE_ (IGunV400, IUnknown)
{   
    STDMETHOD_(void,            Simulate(__in double deltaT))               PURE; /**< Called once per step on all guns. Delta time is in seconds */
    STDMETHOD_(HRESULT,         Fire(__in double deltaT))                   PURE; /**< Called once per step on selected guns. Fire() will be repeatedly called while the trigger is engaged. Users can use an HRESULT return type */
    STDMETHOD_(void,            Purge())                                    PURE; /**< Called on all guns when the trigger is released */
    STDMETHOD_(void,            Stop())                                     PURE; /**< Called on all guns while there are no user inputs for gun rotations and automatic targeting is disabled */

    STDMETHOD_(void,            SetRoundsRemaining(__in UINT ammoCount))    PURE; /**< Sets the total number of rounds in a gun */
    STDMETHOD_(UINT,            GetRoundsRemaining())               const   PURE; /**< Gets the total number of rounds in a gun */
    STDMETHOD_(void,            ResetRounds())                              PURE; /**< Resets the total number of rounds in a gun. Called on each gun when ResetGuns() is called by the GunSystem. */

    STDMETHOD_(const WCHAR*,    GetName())                          const   PURE; /**< Gets the name of a gun */
    STDMETHOD_(const WCHAR*,    GetGunType())                       const   PURE; /**< Gets the type of a gun */

    STDMETHOD_(void,            Rotate(__in double xAxisOffset, __in double yAxisOffset, __in double deltaT))      PURE; /**< Called when the user provides input to rotate guns. Values passed should be expected to be mapped from -1.0 to 1.0. */
    /** called if automatic guns are enabled, providing the developer with information on the target. Target position is longitude, altitude and latitude in that order, where 
    * the units are radians and feet. Target velocity and acceleration are in feet per second while orientation is in world coordinate radians. */
    STDMETHOD_(void,            ProcessTargeting(__in const P3D::DXYZ &targetLla, __in const P3D::DXYZ &targetBodyVelocity, __in const P3D::DXYZ &targetBodyAcceleration, __in const P3D::DXYZ &targetOrientation, __in double deltaT))     PURE;
};

DEFINE_GUID(IID_IGunV400, 0x385cf313, 0x2a03, 0x4541, 0xbf, 0xd5, 0xd9, 0xb6, 0xa2, 0xaa, 0xb7, 0xb7);
DEFINE_GUID(SID_Gun,      0x97115396, 0xafa, 0x4dfe, 0xa6, 0x53, 0xcd, 0x2f, 0xb6, 0x86, 0xa, 0xf3);

/** ## *Professional Plus Only*
* Interface for getting fire control system parameters for this object */
DECLARE_INTERFACE_ (IFireControlSystemV01, IFireControlSystem)
{
    STDMETHOD_(UINT, GetSelectedTargetID)()                      const PURE; /**< Get the ID of the target selected by the fire control system */
    STDMETHOD_(void, SetSelectedTargetID)(UINT id)                     PURE; /**< Sets the fire control system target by object */
    STDMETHOD (GetSelectedTargetMissionID)(__out GUID& guid)     const PURE; /**< Gets the instance ID of the target selected by the fire control system (Structured scenarios with objects only) */
    STDMETHOD (SetSelectedTargetMissionID)(__in const GUID& guid)      PURE; /**< Sets the fire control system target by instance ID (Missions only) */
    /** If the fire control system's target is a latitude/longitude/altitude, this will return that position. Otherwise the return will be FALSE. (radians/radians/feet) */
    STDMETHOD_(BOOL, GetTargetLLA)(__out P3D::DXYZ& vLLA)        const PURE; 
    /** Sets the fire control system's target to be a latitude/longitude/altitude. (radians/radians/feet) */
    STDMETHOD_(void, SetTargetLLA)(__in const P3D::DXYZ& vLLA)         PURE; 
};

DEFINE_GUID(IID_IFireControlSystemV01,   0xd480c0c2, 0x77, 0x471f, 0x8f, 0x86, 0x3c, 0x49, 0x11, 0x32, 0x8e, 0x41);
DEFINE_GUID(SID_FireControlSystem,       0x111e9fd6, 0x4ef7, 0x4734, 0xbd, 0x15, 0x98, 0x9a, 0xc1, 0x42, 0xd3, 0x57);

/** ## *Professional Plus Only*
* Interface for getting guidance parameters for this object */
DECLARE_INTERFACE_ (IGuidanceSystemV01, IGuidanceSystem)
{
    STDMETHOD_(void,   SetTargetObjectID)(UINT targetedObjectID)        PURE; /**< Sets the ID of a target */
    STDMETHOD_(UINT,   GetTargetObjectID)()                       const PURE; /**< Gets the ID of a target */
    STDMETHOD_(void,   SetTargetLLA)(__in const P3D::DXYZ& vLLA)        PURE; /**< sets the guidance system's target to be a latitude/longitude/altitude. (radians/radians/feet) */
    /** If the guidance system's target is a latitude/longitude/altitude, this will return that position. Otherwise the return will be FALSE. (radians/radians/feet) */
    STDMETHOD_(BOOL,   GetTargetLLA)(__out P3D::DXYZ& vLLA)       const PURE; 
};

DEFINE_GUID(IID_IGuidanceSystemV01,     0xb7f8f33c, 0xa485, 0x4e94, 0xa8, 0xeb, 0x83, 0x68, 0x31, 0x67, 0x13, 0xeb);
DEFINE_GUID(SID_GuidanceSystem,         0x5aea610a, 0x69f5, 0x41c8, 0xbb, 0xcd, 0x62, 0x88, 0x3d, 0x68, 0xfe, 0xf9);

/** ## *Professional Plus Only*
* Interface for getting parameters for a weapon pylon */
DECLARE_INTERFACE_ (IPylonServiceV01, IPylonService)
{
    STDMETHOD  (SetOwnerId)(__in UINT uOwnerId)                        PURE;     /**< Sets the ID of the object that the pylon is attached */
    STDMETHOD_ (UINT, GetOwnerId)()                              const PURE;     /**< Gets the ID of the object that the pylon is attached */
    STDMETHOD  (GetAttachOffsetFeet)(__out P3D::DXYZ& vOffset)   const PURE;     /**< Gets the offset on the pylon in which it is attached to the parent */
};

DEFINE_GUID(IID_IPylonServiceV01,   0x63ae109, 0x7aca, 0x45f8, 0xa9, 0xe, 0xbf, 0xe3, 0xbd, 0xbd, 0x4a, 0xa1);
DEFINE_GUID(SID_PylonService,       0x244d37c8, 0xd40b, 0x46d0, 0x92, 0xa4, 0x91, 0xae, 0xc1, 0xda, 0xbc, 0x7);

/** @} */

}
