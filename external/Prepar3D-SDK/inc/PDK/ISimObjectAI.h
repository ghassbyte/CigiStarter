// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ISimObjectAI.h
// Provides public interfaces for AI Implementations for ISimObject implementations
//-----------------------------------------------------------------------------------

#pragma once

#include "ISimObject.h"
#include "Legacy\ISimObjectAI.Legacy.h"  //Include previous version


namespace P3D
{

/** @addtogroup isim */ /** @{ */

/** ## *Professional Plus Only*
* Interface for this object's AIBehaviorManager */
DECLARE_INTERFACE_(IAIBehaviorManagerV01, IAIBehaviorManager)
{
    STDMETHOD (ActivateBehavior)(__in const GUID& BehaviorGuid, BOOL bActivate)              PURE; /**< Activates or deactivates a behavior */
    STDMETHOD_(BOOL, IsBehaviorActive)(__in const GUID& BehaviorGuid)              const     PURE; /**< Returns whether a behavior is active (true) or inactive (false) */
    STDMETHOD_(UINT, GetNumberOfBehaviors)()                                       const     PURE; /**< Returns the total number of behaviors */
    STDMETHOD (GetBehavior)(__in UINT uIndex,  __out IAIBehavior** ppBehavior)     const     PURE; /**< Gets a behavior from an index*/
};

DEFINE_GUID(IID_IAIBehaviorManagerV01,  0x63b89de3, 0xd773, 0x4d2d, 0xa7, 0x2e, 0x9a, 0x79, 0x4f, 0xe6, 0xf6, 0x84);
DEFINE_GUID(SID_AIBehaviorManager,      0x252b465e, 0x8d64, 0x4c37, 0x84, 0xe3, 0xa0, 0xc7, 0x83, 0xcc, 0x34, 0x6a);
DEFINE_GUID(SID_AIBehavior, 0x83cf1981, 0xb584, 0x4359, 0xa9, 0x89, 0x1d, 0xae, 0x2e, 0x18, 0xe3, 0xa7);


/** ## *Professional Plus Only*
* Interface for the Wingman Formation AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorWingmanFormationV01, IAIBehaviorWingmanFormation)
{
    STDMETHOD_(GUID, GetBehaviorGuid)()                                 const PURE; /**< Gets the Guid ID of a behavior */
    STDMETHOD_(void, SetLeaderObjectID)(int objectID)                         PURE; /**< Sets the ID of the object's leader object */
    STDMETHOD_(int,  GetLeaderObjectID)()                               const PURE; /**< Gets the ID of the object's leader object */
    STDMETHOD_(void, SetOffsetPosition)(const P3D::DXYZ &offsetFeet)          PURE; /**< Sets the offset position of a behavior (in feet) */
    STDMETHOD_(void, GetOffsetPosition)(P3D::DXYZ &offsetFeet)          const PURE; /**< Gets the offset position of a behavior (in feet) */

    STDMETHOD_(void, WarpToTarget)()                                          PURE; /**< Moves the object to its target's location */
};

DEFINE_GUID(IID_IAIBehaviorWingmanFormationV01,  0xdf6269f1, 0xf9e5, 0x407f, 0x8a, 0x85, 0x3a, 0xb8, 0xdc, 0x93, 0x9b, 0x77); 
DEFINE_GUID(SID_AIBehaviorWingmanFormation,      0xadf2cb20, 0x95c3, 0x4915, 0xae, 0xe9, 0x57, 0x5f, 0x61, 0x95, 0xec, 0xb5);

/** ## *Professional Plus Only*
* Interface for the Attacker AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorAttackerV400, IAIBehavior)
{
    STDMETHOD_(GUID,  GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a behavior */
    STDMETHOD_(void,  SetFireRadiusMin)(float radiusFeet)                 PURE; /**< Sets the minimum fire radius of an attacker (in feet) */
    STDMETHOD_(float, GetFireRadiusMin)()                           const PURE; /**< Gets the minimum fire radius of an attacker (in feet) */
    STDMETHOD_(void,  SetFireRadiusMax)(float radiusFeet)                 PURE; /**< Sets the maximum fire radius of an attacker (in feet) */
    STDMETHOD_(float, GetFireRadiusMax)()                           const PURE; /**< Gets the maximum fire radius of an attacker (in feet) */
    STDMETHOD_(void,  SetFireFOVDegrees)(float degrees)                   PURE; /**< Sets the fire FOV of an attacker (in degrees) */
    STDMETHOD_(float, GetFireFOVDegrees)()                          const PURE; /**< Gets the fire FOV of an attacker (in degrees) */

    STDMETHOD_(void,  SetWeaponTitle)(WCHAR* weaponTitle)                 PURE; /**< Sets the title of an attacker's weapon */
    STDMETHOD_(const WCHAR*, GetWeaponTitle)()                      const PURE; /**< Gets the title of an attacker's weapon */
    STDMETHOD_(void,  SetWeaponType)(WCHAR* weaponType)                   PURE; /**< Sets the type of an attacker's weapon */
    STDMETHOD_(const WCHAR*, GetWeaponType)()                       const PURE; /**< Gets the type of an attacker's weapon */

    STDMETHOD_(void,  SetGunTitle)(WCHAR* gunTitle)                       PURE; /**< Sets the title of an attacker's gun */
    STDMETHOD_(const WCHAR*, GetGunTitle)()                         const PURE; /**< Gets the title of an attacker's gun */
    STDMETHOD_(void,  SetGunType)(WCHAR* gunType)                         PURE; /**< Sets the type of an attacker's gun */
    STDMETHOD_(const WCHAR*, GetGunType)()                          const PURE; /**< Gets the type of an attacker's gun */

    STDMETHOD_(void,  SetAttackDelay)(float delaySeconds)                 PURE; /**< Sets the delay between an attacker's attacks (in seconds) */
    STDMETHOD_(float, GetAttackDelay)()                             const PURE; /**< Gets the delay between an attacker's attacks (in seconds) */

    STDMETHOD_(void,  SetGunBurstDuration)(float durationSeconds)         PURE; /**< Sets the duration of an attacker's gun burst (in seconds) */
    STDMETHOD_(float, GetGunBurstDuration)()                        const PURE; /**< Gets the duration of an attacker's gun burst (in seconds) */

    STDMETHOD_(BOOL,  IsWithinFireZone)()                           const PURE; /**< Returns true if attacker is within fire zone, false otherwise */
};

DEFINE_GUID(IID_IAIBehaviorAttackerV400, 0xc1223c20, 0xffdc, 0x441e, 0x98, 0xd4, 0xb9, 0x42, 0xc5, 0xbe, 0xf8, 0xa9);
DEFINE_GUID(SID_AIBehaviorAttacker,      0x93cb5f2b, 0xcc0a, 0x4453, 0x90, 0xf1, 0x55, 0xf7, 0x74, 0x9c, 0x73, 0x16);

/** ## *Professional Plus Only*
* Interface for the Pursue AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorPursueV500, IAIBehaviorPursueV01)
{
    STDMETHOD_(GUID,  GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a pursue behavior */
    STDMETHOD_(void,  SetInvestigateRadius)(float radiusFeet)             PURE; /**< Sets the how far an object will investigate from its current location (in feet) */
    STDMETHOD_(float, GetInvestigateRadius)()                       const PURE; /**< Gets the how far an object will investigate from its current location (in feet) */

    STDMETHOD_(void, SetStoppingDistance)(float stoppingFeet)             PURE; /**< Sets the how far an object will stop from the AI object being pursued (in feet) */
    STDMETHOD_(float, GetStoppingDistance)()                        const PURE; /**< Gets the how far an object will stop from the AI object being pursued (in feet) */

    STDMETHOD_(void,  SetPursueObjectID)(int objectID)                    PURE; /**< Sets the ID of the object being pursued */
    STDMETHOD_(int,   GetPursueObjectID)()                          const PURE; /**< Gets the ID of the object being pursued */

    STDMETHOD_(void,  SetMaxPursuitDurationSeconds)(float maxSeconds)     PURE; /**< Sets the maximum time an object will pursue for (in seconds) */
    STDMETHOD_(float, GetSetMaxPursuitDurationSeconds)()            const PURE; /**< Gets the maximum time an object will pursue for (in seconds) */

    STDMETHOD_(void,  SetInterceptGroundObjects)(BOOL intercept)          PURE; /**< Sets if object can intercept ground objects */
    STDMETHOD_(BOOL,  GetInterceptGroundObjects)()                  const PURE; /**< Gets if object can intercept ground objects */
};

DEFINE_GUID(IID_IAIBehaviorPursueV500, 0xD944C85F, 0x0C16, 0x429B, 0xae, 0x33, 0x87, 0xd8, 0xed, 0xb3, 021, 0x28);
DEFINE_GUID(SID_AIBehaviorPursue,      0xcc45604d, 0xbbb, 0x4abb, 0xbe, 0x39, 0x3c, 0x22, 0x8c, 0x56, 0x5a, 0xb1);

/** ## *Professional Plus Only*
* Interface for the Combat-Air-Patrol AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorCombatAirPatrolV01, IAIBehaviorCombatAirPatrol)
{
    STDMETHOD_(GUID,  GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a combat-air-patrol behavior */
    STDMETHOD_(void,  SetPatrolObjectID)(int objectID)                    PURE; /**< Sets the ID of the patrol object */
    STDMETHOD_(int,   GetPatrolObjectID)()                          const PURE; /**< Gets the ID of the patrol object */

    STDMETHOD_(void,  SetPatrolOrigin)(const P3D::DXYZ &lonAltLat)        PURE; /**< Sets the origin point for the patrol (in radians and feet) */
    STDMETHOD_(void,  GetPatrolOrigin)(P3D::DXYZ &lonAltLat)        const PURE; /**< Gets the origin point for the patrol (in radians and feet) */

    STDMETHOD_(void,  SetPatrolRadius)(float radiusFeet)                  PURE; /**< Sets the radius for the patrol (in feet) */
    STDMETHOD_(float, GetPatrolRadius)()                            const PURE; /**< Gets the radius for the patrol (in feet) */
};

DEFINE_GUID(IID_IAIBehaviorCombatAirPatrolV01,  0x3e2ea875, 0xaef2, 0x4559, 0x87, 0xe4, 0x7d, 0x94, 0x5, 0xbe, 0xc5, 0x5b);
DEFINE_GUID(SID_AIBehaviorCombatAirPatrol,      0xf223e319, 0x258b, 0x46cd, 0xb6, 0x88, 0xeb, 0x7c, 0x91, 0xa7, 0xcf, 0xfd);

/** ## *Professional Plus Only*
* Interface for the Close-Air-Support AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorCloseAirSupportV01, IAIBehaviorCloseAirSupport)
{

    STDMETHOD_(GUID,  GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a close-air-support behavior */
    STDMETHOD_(void,  SetSupportObjectID)(int objectID)                   PURE; /**< Sets the ID of the support object */
    STDMETHOD_(int,   GetSupportObjectID)()                         const PURE; /**< Gets the ID of the support object */

    STDMETHOD_(void,  SetSupportPosition)(const P3D::DXYZ &lonAltLat)     PURE; /**< Sets the position for the support (in radians and feet) */
    STDMETHOD_(void,  GetSupportPosition)(P3D::DXYZ &lonAltLat)     const PURE; /**< Gets the position for the support (in radians and feet) */
};

DEFINE_GUID(IID_IAIBehaviorCloseAirSupportV01,  0xcb966d65, 0xee83, 0x4261, 0x93, 0x42, 0x59, 0xb5, 0x8b, 0x6e, 0x41, 0xc6); 
DEFINE_GUID(SID_AIBehaviorCloseAirSupport,      0x354fdafe, 0x73ae, 0x491a, 0x8f, 0xb4, 0xa3, 0x8e, 0xef, 0x13, 0xfc, 0x1a);

/** ## *Professional Plus Only*
* Interface for the Search and Track AI Behavior */
DECLARE_INTERFACE_(IAIBehaviorSearchTrackV01, IAIBehaviorSearchTrack)
{
    STDMETHOD_(GUID,  GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a search and track behavior */
    STDMETHOD_(void,  SetSearchRadius)(float radiusFeet)                  PURE; /**< Sets the radius to search (in feet) */
    STDMETHOD_(float, GetSearchRadius)()                            const PURE; /**< Gets the radius to search (in feet) */
    STDMETHOD_(void,  SetSearchFOVDegrees)(float degrees)                 PURE; /**< Sets the FOV of a search (in degrees) */
    STDMETHOD_(float, GetSearchFOVDegrees)()                        const PURE; /**< Gets the FOV of a search (in degrees) */

    STDMETHOD_(void,  SetTrackRadius)(float radiusFeet)                   PURE; /**< Sets the radius to track (in feet) */
    STDMETHOD_(float, GetTrackRadius)()                             const PURE; /**< Gets the radius to track (in feet) */
    STDMETHOD_(void,  SetTrackFOVDegrees)(float degrees)                  PURE; /**< Sets the FOV of a track (in degrees) */
    STDMETHOD_(float, GetTrackFOVDegrees)()                         const PURE; /**< Gets the FOV of a track (in degrees) */

    STDMETHOD_(BOOL,  IsWithinSearchZone)()                         const PURE; /**< Returns true if behavior is within search zone, false otherwise */
    STDMETHOD_(BOOL,  IsWithinTrackZone)()                          const PURE; /**< Returns true if behavior is within track zone, false otherwise */
};

DEFINE_GUID(IID_IAIBehaviorSearchTrackV01,  0x4014fd59, 0x4e6f, 0x4bba, 0xbb, 0xa8, 0x67, 0xce, 0x71, 0x2b, 0xa9, 0x2b);
DEFINE_GUID(SID_AIBehaviorSearchTrack,      0xaa0e9d2c, 0x24e5, 0x4552, 0xaa, 0x97, 0x42, 0xa5, 0x14, 0x5b, 0x40, 0x1a);

/** The ISimObjectAI interface is an interface on the AI "pilot" implementation for a simobject. A custom AI can be implemented
* on simobjects created using the ISimObject SDK. The interface may be accessed by systems such as the Traffic Manager or ATC. */
DECLARE_INTERFACE_(ISimObjectAIV510, ISimObjectAIV02)
{
    STDMETHOD(UpdateSimulationFrame)(__in double dDeltaT)          PURE;  /**< To be called from the simulation loop to keep the low level AI controllers in sync with the simulation */
    STDMETHOD_(UNITMODE, GetPilotMode)() const                      PURE; /**< Returns the current mode of the AI */
    STDMETHOD_(void, SetPilotMode)(UNITMODE eMode, BOOL bOn = TRUE) PURE; /**< Sets the current mode of the AI */
    STDMETHOD_(void, Deactivate)()                                  PURE; /**< Disables the AI control */
    STDMETHOD_(void, Activate)()                                    PURE; /**< Enables the AI control */
    STDMETHOD(SetWaypoint)(__in BasicWaypoint)                     PURE;  /**< Sets the next waypoint for the AI to track to */

    STDMETHOD(SetDesiredHeading)(double dTrueHeading)              PURE; /**< Sets the heading to be maintained (in radians) */
    STDMETHOD(SetDesiredPitch)(double dPitch)                      PURE; /**< Sets the pitch angle to be maintained (in radians) */
    STDMETHOD(SetDesiredSpeed)(double dSpeed)                      PURE; /**< Sets the desired speed to be maintained (Indicated airspeed for aircraft) (in feet per second) */
    STDMETHOD(SetDesiredAltitude)(double dAltitudeMSL)             PURE; /**< Sets the altitude to be maintained (in feet) */

    STDMETHOD_(double, GetDesiredHeading)()     const              PURE; /**< Gets the current heading to be maintained (in radians) */
    STDMETHOD_(double, GetDesiredPitch)()       const              PURE; /**< Gets the current pitch to be maintained (in radians) */
    STDMETHOD_(double, GetDesiredSpeed)()       const              PURE; /**< Gets the current speed to be maintained (in feet per second) */
    STDMETHOD_(double, GetDesiredAltitude)()    const              PURE; /**< Gets the current altitude to be maintained (in feet) */

    STDMETHOD_(bool, HasDesiredHeading)()       const              PURE; /**< Gets if a current heading value is set */
    STDMETHOD_(bool, HasDesiredPitch)()         const              PURE; /**< Gets if a current pitch value is set */
    STDMETHOD_(bool, HasDesiredSpeed)()         const              PURE; /**< Gets if a current speed value is set */
    STDMETHOD_(bool, HasDesiredAltitude)()      const              PURE; /**< Gets if a current altitude value is set */
};

// {8E5730CF-8A88-4810-9ADE-8E13E78C02C2}
DEFINE_GUID(IID_ISimObjectAIV510, 0x8e5730cf, 0x8a88, 0x4810, 0x9a, 0xde, 0x8e, 0x13, 0xe7, 0x8c, 0x2, 0xc2);

DEFINE_GUID(SID_SimObjectAI,      0xb891c04c, 0xbb7, 0x496b, 0x8c, 0x9f, 0xee, 0x8d, 0xba, 0xf9, 0x6c, 0x20);
DEFINE_GUID(SID_AIService, 0x4d8177b7, 0xabdb, 0x49cf, 0xba, 0x54, 0xb7, 0x1d, 0x7e, 0x3c, 0x8b, 0xc5);
DEFINE_GUID(SID_AircraftAIService,     0x4902641d, 0xbc33, 0x4817, 0x92, 0x46, 0x17, 0x6e, 0x34, 0x0, 0x26, 0x71);

/** The IAirplaneAIService should be implemented on any airplane intended to be controlled by Prepar3D's internal AI Pilot. */
DECLARE_INTERFACE_ (IAirplaneAIServiceV02, IAirplaneAIServiceV01)
{
    //Airplane Specific
    STDMETHOD_(float,  GetStallSpeedDirty)()                const PURE;    /**< Returns stall speed with gear and flaps extended (feet per second) */
    STDMETHOD_(float,  GetStallSpeedClean)()                const PURE;    /**< Returns stall speed with gear and flaps retracted (feet per second) */
    STDMETHOD_(float,  GetMinDragSpeed)()                   const PURE;    /**< Returns speed at which total drag is at its minimum (feet per second) */
    STDMETHOD_(float,  GetZeroLiftAngleOfAttack)()          const PURE;    /**< Return angle-of-attack at which zero lift is generated (radians) */
    STDMETHOD_(float,  GetCriticalAngleOfAttack)()          const PURE;    /**< Returns angle-of-attack at which the aircraft will stall (radians) */
    STDMETHOD_(float,  GetLinearCLAlpha)()                  const PURE;    /**< Returns the slope of the CL vs. angle-of-attack curve in the linear region, typically between zero lift and the critical angle-of-attack (radians) */
    STDMETHOD_(float,  GetWingArea)()                       const PURE;    /**< Returns the total area of the main wing (feet squared) */
    STDMETHOD_(float,  GetWingSpan)()                       const PURE;    /**< Return the wingspan of the main wing (feet) */
    STDMETHOD_(float,  GetTotalLongitudinalThrust)()        const PURE;    /**< Returns the thrust force in the longitudinal axis (pounds) */
    STDMETHOD_(float,  GetLiftForce)()                      const PURE;    /**< Returns the lift force generated by the airplane, including wing, tail, and fuselage) (pounds) */

    STDMETHOD_(double, GetThrottlePercent)()                const PURE;    /**< Returns the throttle position (0 - 1) */
    STDMETHOD_(double, GetElevatorPercent)()                const PURE;    /**< Returns the elevator position (-1 down - +1 up) */
    STDMETHOD_(double, GetAileronPercent)()                 const PURE;    /**< Returns the aileron position (-1 left - +1 right) */
    STDMETHOD_(double, GetRudderPercent)()                  const PURE;    /**< Returns rudder percent (-1 left - +1 right) */
    STDMETHOD_(double, GetSpoilersPercent)()                const PURE;    /**< Return the spoiler position (0 - 1) */
    STDMETHOD_(double, GetFlapsPercent)()                   const PURE;    /**< Returns the flap percent deflection (0 - 1) */

    STDMETHOD_(void,   SetThrottlePercent)(double dPct)           PURE;    /**< Sets the throttle to a specific position (0 - 1) */
    STDMETHOD_(void,   SetElevatorPercent)(double dPct)           PURE;    /**< Sets the elevator to a specific position (-1 down - +1 up) */
    STDMETHOD_(void,   SetAileronPercent)(double dPct)            PURE;    /**< Sets the aileron to a specific position (-1 left - +1 right) */
    STDMETHOD_(void,   SetRudderPercent)(double dPct)             PURE;    /**< Sets the rudder to a specific position (-1 left - +1 right) */
    STDMETHOD_(void,   SetFlapsPercent)(double dPct)              PURE;    /**< Sets the flaps to a specific position (0 - +1 extended) */
    STDMETHOD_(void,   SetSpoilersPercent)(double dPct)           PURE;    /**< Sets the spoilers to a specific position (0 - +1 extended) */


    //Controller functions (e.g. PID) based on aircraft dynamics
    /** Returns a desired bank for the AI pilot based on a heading error. This is typically calculated with a PID controller based on the airplane dynamics (radians) */
    STDMETHOD_(double, CalculateDesiredBank)(double dHeadingError, double dDeltaT) PURE;
    /** Returns the amount the throttle should be moved this frame based on the current speed error This is typically calculated with a PID controller based on the 
    * airplane and engine dynamics. (-1 - +1) */
    STDMETHOD_(double, CalculateDeltaThrottle)(double dSpeedError, double dDeltaT) PURE;
    
    //Taxi and Pushback logic is handled by the simulation
    /** Sets the taxi heading (radians) NOTE: Ground handling is assumed to be tightly coupled and calculated within the airplane simulation. */
    STDMETHOD_(void,    SetTaxiHeading)(float fHeading)           PURE;
    /** Sets the taxi speed (feet per second) NOTE: Ground handling is assumed to be tightly coupled and calculated within the airplane simulation. */
    STDMETHOD_(void,    SetTaxiSpeed)(float fSpeed)               PURE;
    /** Triggers the taxi operation to stop. NOTE: Ground handling is assumed to be tightly coupled and calculated within the airplane simulation. */
    STDMETHOD_(void,    StopTaxi)()                               PURE;
    /** Triggers the ground handling simulation to move backwards. For example, pushing back from a gate. NOTE: Ground handling is assumed to be tightly coupled and 
    * calculated within the airplane simulation. */
    STDMETHOD_(void,    SetPushBack)(BOOL bOn)                    PURE;

    //Begin Carrier related functionality
    STDMETHOD_(void,    ExtendTailhook)()                         PURE;    /**< Extends the aircraft's tailhook inorder to catch arrestor cables. */
    STDMETHOD_(void,    RetractTailhook)()                        PURE;    /**< Retracts the aircraft's tailhook. */
    STDMETHOD_(float,   GetTailhookPosition)()              const PURE;    /**< Returns the position of the aircraft's tailhook. (retracted=0.0; extended=1.0) */
    STDMETHOD_(BOOL,    HasTailhook)()                      const PURE;    /**< Returns TRUE if the aircraft has a valid tailhook, FALSE otherwise. */

    STDMETHOD_(void,    ExtendLaunchBar)()                        PURE;    /**< Extends the aircraft's launch bar inorder to attach to catapults. */
    STDMETHOD_(void,    RetractLaunchBar)()                       PURE;    /**< Retracts the aircraft's launch bar. */
    STDMETHOD_(float,   GetLaunchBarPosition)()             const PURE;    /**< Returns the position of the aircraft's launch bar. (retracted=0.0; extended=1.0) */
    STDMETHOD_(BOOL,    HasLaunchBar)()                     const PURE;    /**< Returns TRUE if the aircraft has a valid launch bar, FALSE otherwise. */

    STDMETHOD_(void,    FoldWings)()                              PURE;    /**< Folds the aircraft's wings, if available. */
    STDMETHOD_(void,    UnfoldWings)()                            PURE;    /**< Unfolds the aircraft's wings, if available. */
    STDMETHOD_(float,   GetLeftWingPosition)()              const PURE;    /**< Returns the current folded position of the aircraft's left wing. (unfolded=0.0; folded=1.0) */
    STDMETHOD_(float,   GetRightWingPosition)()             const PURE;    /**< Returns the current folded position of the aircraft's right wing. (unfolded=0.0; folded=1.0) */

    STDMETHOD_(float,   GetFullMilitaryThrottlePosition)()  const PURE;    /**< Returns the throttle position that is considered to be full military power (no afterburner). (position from 0.0 to 1.0) */

    STDMETHOD_(void,    ArmNearestCatapult)(BOOL bArm)            PURE;    /**< Attempts to attach and arm the nearest catapult. */

    STDMETHOD_(void,    FireArmedCatapult)()                      PURE;    /**< Fires the currently armed catapult. */
    //End Carrier related functionality
};

DEFINE_GUID(IID_IAirplaneAIServiceV02,  0x8ff7ec23, 0xb599, 0x499d, 0xa2, 0xda, 0xce, 0xc6, 0x2b, 0xe6, 0x12, 0x78);

DEFINE_GUID(SID_AirplaneAIService,      0xd1b5a0f5, 0x21a8, 0x4ca7, 0xa9, 0xdb, 0x20, 0x1a, 0x35, 0x85, 0x80, 0x19);

DECLARE_INTERFACE_ (IHelicopterAIServiceV420, IHelicopterAIServiceV01)
{
    STDMETHOD_(double, GetThrottlePercent)()          const PURE;    /**< Returns the throttle position (0 - 1) */
    STDMETHOD_(double, GetCollectivePercent)()        const PURE;    /**< Returns the collective position (0 - 1) */
    STDMETHOD_(double, GetTorquePedalPercent)()       const PURE;    /**< Returns the torque pedals position (-1 - 1) */
    STDMETHOD_(double, GetCyclicLateralPercent)()     const PURE;    /**< Returns the cyclic L/R position (0 - 1) */
    STDMETHOD_(double, GetCyclicLongitudinalPercent)()const PURE;    /**< Returns the cyclic F/A position (0 - 1) */

    STDMETHOD_(void,   SetThrottlePercent)(double dPct)             PURE;    /**< Sets the throttle to a specific position (0 - 1) */
    STDMETHOD_(void,   SetCollectivePercent)(double dPct)           PURE;    /**< Sets the collective to a specific position (0 - 1) */
    STDMETHOD_(void,   SetTorquePedalPercent)(double dPct)          PURE;    /**< Sets the torque pedals to a specific position (-1 - 1) */
    STDMETHOD_(void,   SetCyclicLateralPercent)(double dPct)        PURE;    /**< Sets the cyclic L/R position (0 - 1) */
    STDMETHOD_(void,   SetCyclicLongitudinalPercent)(double dPct)   PURE;    /**< Sets the cyclic F/A position (0 - 1) */
};

DEFINE_GUID(IID_IHelicopterAIServiceV420, 0x885c5782, 0xc844, 0x4fa9, 0xbc, 0x45, 0xd7, 0x7a, 0x10, 0x53, 0xc1, 0x95);
DEFINE_GUID(SID_HelicopterAIService,      0x9539e6bc, 0x9d19, 0x431d, 0x86, 0x70, 0x27, 0x41, 0xa8, 0x4d, 0xec, 0x84);

/** This interface enables ground vehicle implementations to be utilized by Prepar3D's internal AI controllers. */
DECLARE_INTERFACE_ (IGroundVehicleAIServiceV01, IGroundVehicleAIService)
{
};

DEFINE_GUID(IID_IGroundVehicleAIServiceV01,  0xe33d4ea9, 0x3bf3, 0x4cc3, 0x83, 0x9, 0x91, 0xa6, 0xc7, 0x5, 0x38, 0x32);
DEFINE_GUID(SID_GroundVehicleAIService,      0x2098723b, 0x92fb, 0x4600, 0xbc, 0x7f, 0x2f, 0x8a, 0x62, 0x38, 0x6d, 0x17);

/** This interface enables custom vehicle implementations to be utilized by Prepar3D's internal AI controllers. */
DECLARE_INTERFACE_ (ICustomAIServiceV520, IAIService)
{
};

DEFINE_GUID(IID_ICustomAIServiceV520, 0x12e5219, 0xd0ed, 0x4d88, 0x8a, 0xf0, 0x3c, 0xe2, 0xa8, 0xea, 0xe8, 0x88);
DEFINE_GUID(SID_CustomAIService, 0xa4f4fcf3, 0xbf01, 0x4325, 0x83, 0x24, 0x9e, 0x88, 0xd7, 0xd5, 0x75, 0x1c);

/** @} */
} //End namespace P3D
