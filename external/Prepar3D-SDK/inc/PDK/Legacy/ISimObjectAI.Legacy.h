// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
/************************************************************************************************************** 
    ISimObjectAI.Legacy.h    
    Provides legacy interfaces for AI Implementations for ISimObject implementations
***************************************************************************************************************/

#pragma once

#include "..\Types.h"

namespace P3D
{

interface IAIBehavior;

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehvaiorManager - Interface for this object's AIBehaviorManager
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehaviorManager, IServiceProvider)
{
    STDMETHOD (ActivateBehavior)(__in const GUID& BehaviorGuid, BOOL bActivate)              PURE;
    STDMETHOD_(BOOL, IsBehaviorActive)(__in const GUID& BehaviorGuid)              const     PURE;
    STDMETHOD_(UINT, GetNumberOfBehaviors)()                                       const     PURE;
    STDMETHOD (GetBehavior)(__in UINT uIndex,  __out IAIBehavior** ppBehavior)     const     PURE;
};

DEFINE_GUID(IID_IAIBehaviorManager, 0x497e7a52, 0x374c, 0x441f, 0x8b, 0xfc, 0xa1, 0x5a, 0xb0, 0x7a, 0x6d, 0x52);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehvaior - Base interface for AI Behaviors
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehavior, IUnknown)
{
    STDMETHOD_(GUID, GetBehaviorGuid)()                                 const PURE;
};

DEFINE_GUID(IID_IAIBehavior, 0xfc2a58c4, 0x9689, 0x4b47, 0xab, 0xd3, 0x81, 0x6b, 0xb1, 0xd7, 0x82, 0x4f);


/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehvaiorWingmanFormation - Interface for the Wingman Formation AI Behavior
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehaviorWingmanFormation, IAIBehavior)
{
    STDMETHOD_(void, SetLeaderObjectID)(int objectID)                         PURE;
    STDMETHOD_(int,  GetLeaderObjectID)()                               const PURE;

    STDMETHOD_(void, SetOffsetPosition)(const P3D::DXYZ &offsetFeet)          PURE;     // Feet
    STDMETHOD_(void, GetOffsetPosition)(P3D::DXYZ &offsetFeet)          const PURE;     // Feet

    STDMETHOD_(void, WarpToTarget)()                                          PURE;
};

DEFINE_GUID(IID_IAIBehaviorWingmanFormation, 0xdec98242, 0xe935, 0x474e, 0xa1, 0x68, 0x76, 0x62, 0xc4, 0x9f, 0x55, 0x6d);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehvaiorPursue - Interface for the Pursue AI Behavior
**********************************************************************************************************/

DECLARE_INTERFACE_(IAIBehaviorPursue, IAIBehavior)
{
    STDMETHOD_(void,  SetInvestigateRadius)(float radiusFeet)             PURE;     //Feet
    STDMETHOD_(float, GetInvestigateRadius)()                       const PURE;     //Feet

    STDMETHOD_(void,  SetPursueObjectID)(int objectID)                    PURE;
    STDMETHOD_(int,   GetPursueObjectID)()                          const PURE;

    STDMETHOD_(void,  SetMaxPursuitDurationSeconds)(float maxSeconds)     PURE;     //Seconds
    STDMETHOD_(float, GetSetMaxPursuitDurationSeconds)()            const PURE;     //Seconds

    STDMETHOD_(void,  SetInterceptGroundObjects)(BOOL intercept)          PURE;
    STDMETHOD_(BOOL,  GetInterceptGroundObjects)()                  const PURE;
};

DEFINE_GUID(IID_IAIBehaviorPursue, 0x5e3a0e93, 0xf1ed, 0x49bd, 0xbf, 0xa, 0x43, 0x91, 0x54, 0x83, 0x9e, 0x73);

DECLARE_INTERFACE_(IAIBehaviorPursueV01, IAIBehaviorPursue)
{
    STDMETHOD_(GUID, GetBehaviorGuid)()                            const PURE; /**< Gets the Guid ID of a pursue behavior */
    STDMETHOD_(void, SetInvestigateRadius)(float radiusFeet)             PURE; /**< Sets the how far an object will investigate from its current location (in feet) */
    STDMETHOD_(float, GetInvestigateRadius)()                       const PURE; /**< Gets the how far an object will investigate from its current location (in feet) */

    STDMETHOD_(void, SetPursueObjectID)(int objectID)                    PURE; /**< Sets the ID of the object being pursued */
    STDMETHOD_(int, GetPursueObjectID)()                          const PURE; /**< Gets the ID of the object being pursued */

    STDMETHOD_(void, SetMaxPursuitDurationSeconds)(float maxSeconds)     PURE; /**< Sets the maximum time an object will pursue for (in seconds) */
    STDMETHOD_(float, GetSetMaxPursuitDurationSeconds)()            const PURE; /**< Gets the maximum time an object will pursue for (in seconds) */

    STDMETHOD_(void, SetInterceptGroundObjects)(BOOL intercept)          PURE; /**< Sets if object can intercept ground objects */
    STDMETHOD_(BOOL, GetInterceptGroundObjects)()                  const PURE; /**< Gets if object can intercept ground objects */
};

DEFINE_GUID(IID_IAIBehaviorPursueV01, 0xaf34d0eb, 0xb222, 0x4452, 0xa7, 0x55, 0xdf, 0xdc, 0x67, 0xc3, 0xf, 0x3c);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehaviorCombatAirPatrol - Interface for the Combat-Air-Patrol AI Behavior
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehaviorCombatAirPatrol, IAIBehavior)
{
    STDMETHOD_(void,  SetPatrolObjectID)(int objectID)                    PURE;
    STDMETHOD_(int,   GetPatrolObjectID)()                          const PURE;

    STDMETHOD_(void,  SetPatrolOrigin)(const P3D::DXYZ &lonAltLat)        PURE;     // Radians and Feet
    STDMETHOD_(void,  GetPatrolOrigin)(P3D::DXYZ &lonAltLat)        const PURE;     // Radians and Feet

    STDMETHOD_(void,  SetPatrolRadius)(float radiusFeet)                  PURE;     // Feet
    STDMETHOD_(float, GetPatrolRadius)()                            const PURE;     // Feet
};

DEFINE_GUID(IID_IAIBehaviorCombatAirPatrol, 0x8ce02584, 0x1a9, 0x4c90, 0xa8, 0xd0, 0x6c, 0x33, 0xd1, 0x6d, 0x90, 0xf5);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehaviorCloseAirSupport - Interface for the Close-Air-Support AI Behavior
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehaviorCloseAirSupport, IAIBehavior)
{
    STDMETHOD_(void,  SetSupportObjectID)(int objectID)                   PURE;
    STDMETHOD_(int,   GetSupportObjectID)()                         const PURE;

    STDMETHOD_(void,  SetSupportPosition)(const P3D::DXYZ &lonAltLat)     PURE;     // Radians and Feet
    STDMETHOD_(void,  GetSupportPosition)(P3D::DXYZ &lonAltLat)     const PURE;     // Radians and Feet
};

DEFINE_GUID(IID_IAIBehaviorCloseAirSupport, 0x407d3566, 0xd53c, 0x4310, 0xb0, 0x3f, 0xb0, 0x8f, 0x10, 0xd7, 0xc9, 0x10);

/**********************************************************************************************************
* PROFESSIONAL PLUS CAPABILITY *
IAIBehvaiorSearchTrack - Interface for the Search and Track AI Behavior
**********************************************************************************************************/
DECLARE_INTERFACE_(IAIBehaviorSearchTrack, IAIBehavior)
{
    STDMETHOD_(void,  SetSearchRadius)(float radiusFeet)                  PURE;     //Feet
    STDMETHOD_(float, GetSearchRadius)()                            const PURE;     //Feet
    STDMETHOD_(void,  SetSearchFOVDegrees)(float degrees)                 PURE;     //Degrees
    STDMETHOD_(float, GetSearchFOVDegrees)()                        const PURE;     //Degrees

    STDMETHOD_(void,  SetTrackRadius)(float radiusFeet)                   PURE;     //Feet
    STDMETHOD_(float, GetTrackRadius)()                             const PURE;     //Feet
    STDMETHOD_(void,  SetTrackFOVDegrees)(float degrees)                  PURE;     //Degrees
    STDMETHOD_(float, GetTrackFOVDegrees)()                         const PURE;     //Degrees

    STDMETHOD_(BOOL,  IsWithinSearchZone)()                         const PURE;
    STDMETHOD_(BOOL,  IsWithinTrackZone)()                          const PURE;
};

DEFINE_GUID(IID_IAIBehaviorSearchTrack, 0x4f3ad17d, 0x4263, 0x4100, 0x9f, 0x5c, 0x9c, 0x8f, 0x53, 0x90, 0x53, 0x6b);

/**********************************************************************************************************
ISimObjectAI - Interface for getting AI parameters from this object
**********************************************************************************************************/
DECLARE_INTERFACE_(ISimObjectAI, IServiceProvider)
{
    STDMETHOD (UpdateSimulationFrame)(__in double dDeltaT)          PURE;
    STDMETHOD_(UNITMODE, GetPilotMode)() const                      PURE;
    STDMETHOD_(void, SetPilotMode)(UNITMODE eMode, BOOL bOn = TRUE) PURE;
    STDMETHOD_(void, Deactivate)()                                  PURE;
    STDMETHOD_(void, Activate)()                                    PURE;
    STDMETHOD (SetWaypoint)(__in BasicWaypoint)                     PURE;

    STDMETHOD (SetDesiredHeading)(double dTrueHeading)              PURE;   //Radians
    STDMETHOD (SetDesiredPitch)(double dPitch)                      PURE;   //Radians
    STDMETHOD (SetDesiredSpeed)(double dSpeed)                      PURE;   //Feet per second (Indicated airspeed for aircraft)
    STDMETHOD (SetDesiredAltitude)(double dAltitudeMSL)             PURE;   //Feet

};

DEFINE_GUID(IID_ISimObjectAI, 0xff38f8b4, 0x79d4, 0x4a6b, 0x85, 0xb1, 0x5f, 0x15, 0xe1, 0x4, 0x76, 0x1f);

DECLARE_INTERFACE_(ISimObjectAIV01, ISimObjectAI)
{
    //Identical to ISimObjectAI
};

DEFINE_GUID(IID_ISimObjectAIV01, 0xdda0d059, 0xf991, 0x47d8, 0x9f, 0xa, 0xfa, 0xc4, 0x6, 0x59, 0xda, 0x6b);

/** The ISimObjectAI interface is an interface on the AI "pilot" implementation for a simobject. A custom AI can be implemented
* on simobjects created using the ISimObject SDK. The interface may be accessed by systems such as the Traffic Manager or ATC. */
DECLARE_INTERFACE_(ISimObjectAIV02, ISimObjectAIV01)
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
};

DEFINE_GUID(IID_ISimObjectAIV02, 0x27428180, 0xe4, 0x489a, 0xa1, 0x45, 0x30, 0x92, 0xb, 0xa2, 0xdd, 0x8e);

/**********************************************************************************************************
IAIService - Interface for getting AI parameters from this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IAIService, IUnknown)
{
};

DEFINE_GUID(IID_IAIService, 0xd95eef22, 0x61ce, 0x463a, 0xaa, 0x71, 0x5, 0x4b, 0x1a, 0x5b, 0xe9, 0x2b);

///IAircraftAIService - Interface for getting AI parameters from this aircraft object
///Note: There is no versioned interface for the IAircraftAIService, as it serves as a base
///class interface for derived AircraftAIService, which are versioned

/*********************************************************************************************************
IAircraftAIService - Interface for getting AI parameters from this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IAircraftAIServiceV01, IAIService)
{
    STDMETHOD_(float,  GetIndicatedAirspeed)()              const PURE;  //FPS
    STDMETHOD_(float,  GetTrueAirspeed)()                   const PURE;  //FPS    
    STDMETHOD_(float,  GetCruiseSpeedTrue)()                const PURE;  //FPS
    STDMETHOD_(float,  GetMaxIndicatedAirspeed)()           const PURE;  //FPS
    STDMETHOD_(float,  GetMach)()                           const PURE;  
    STDMETHOD_(float,  GetMaxMach)()                        const PURE;  
    STDMETHOD_(float,  GetIndicatedAltitude)()              const PURE;  //Feet

    STDMETHOD_(float,  GetTotalWeight)()                    const PURE;  //Pounds
    STDMETHOD_(float,  GetEmptyRollingMomentOfInertia)()    const PURE;  //Slugs-Ft^2
    STDMETHOD_(float,  GetDynamicPressure)()                const PURE;  //PSF
    STDMETHOD_(float,  GetIncidenceAlpha)()                 const PURE;  //Radians
    STDMETHOD_(float,  GetIncidenceBeta)()                  const PURE;  //Radians
    STDMETHOD_(float,  GetBetaDot)()                        const PURE;  //Radians/sec
    STDMETHOD_(float,  GetRollRate)()                       const PURE;  //Radians
    STDMETHOD_(float,  GetSigmaSqrt)()                      const PURE;
    STDMETHOD_(float,  GetLongitudinalAccelerationBody)()   const PURE;  //Feet/sec^2
    STDMETHOD_(float,  GetLateralAccelerationBody)()        const PURE;  //Feet/sec^2
    STDMETHOD_(float,  GetVerticalAccelerationBody)()       const PURE;  //Feet/sec^2

    STDMETHOD_(BOOL,   IsEngineTypePiston)()                const PURE;
    STDMETHOD_(UINT,   GetEngineCount)()                    const PURE;
    STDMETHOD_(float,  GetGeneralEngineRPM)(int iEngine)    const PURE;
    STDMETHOD_(BOOL,   IsEngineRunning)(int iEngine)        const PURE;
    STDMETHOD_(float,  GetTotalFuelCapacity)()              const PURE;  //Gallons
    STDMETHOD_(float,  GetFuelWeightPerGallon)()            const PURE;  //Pounds/Gallon
    STDMETHOD_(float,  GetEstimatedFuelFlowLbsPerHour)()    const PURE;  //Pounds / Hour
    STDMETHOD_(void,   StartEngines)()                            PURE;
    STDMETHOD_(void,   ShutdownEngines)()                         PURE;

    STDMETHOD_(BOOL, GetParkingBrake)()                     const PURE;
    STDMETHOD_(void, TapBrakes)()                                 PURE; 
    STDMETHOD_(void, SetBrakePosition)(int iBrake,float fPos)     PURE;  // none 0->1 full  
    STDMETHOD_(void, SetParkingBrake)(BOOL bOn)                   PURE;
    STDMETHOD_(void, ExtendLandingGear)()                         PURE;
    STDMETHOD_(void, RetractLandingGear)()                        PURE;

    STDMETHOD_(BOOL,   IsTailDragger)()                     const PURE;  
    STDMETHOD_(BOOL,   IsSeaPlane)()                        const PURE;
    STDMETHOD_(float,  GetMinTakeoffLength)()               const PURE;  //Feet
    STDMETHOD_(float,  GetMinLandingLength)()               const PURE;  //Feet

    STDMETHOD_(void, SetNavLights)                  (BOOL bOn)    PURE;
    STDMETHOD_(void, SetBeaconLights)               (BOOL bOn)    PURE;
    STDMETHOD_(void, SetLogoLights)                 (BOOL bOn)    PURE;
    STDMETHOD_(void, SetRecognitionLights)          (BOOL bOn)    PURE;
    STDMETHOD_(void, SetTaxiLights)                 (BOOL bOn)    PURE;
    STDMETHOD_(void, SetLandingLights)              (BOOL bOn)    PURE;
    STDMETHOD_(void, SetStrobeLights)               (BOOL bOn)    PURE;
    STDMETHOD_(void, SetPanelLights)                (BOOL bOn)    PURE;
    STDMETHOD_(void, SetCabinLights)                (BOOL bOn)    PURE;
};

DEFINE_GUID(IID_IAircraftAIServiceV01, 0x164cbb43, 0xd52b, 0x4018, 0xb2, 0x3, 0x4, 0x6, 0xe, 0x1b, 0x79, 0x4a);

/**********************************************************************************************************
IAirplaneAIService - Interface for getting AI parameters from this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IAirplaneAIServiceV01, IAircraftAIServiceV01)
{
    //Airplane Specific
    STDMETHOD_(float,  GetStallSpeedDirty)()                const PURE;  //FPS
    STDMETHOD_(float,  GetStallSpeedClean)()                const PURE;  //FPS
    STDMETHOD_(float,  GetMinDragSpeed)()                   const PURE;  //FPS
    STDMETHOD_(float,  GetZeroLiftAngleOfAttack)()          const PURE;  //Radians
    STDMETHOD_(float,  GetCriticalAngleOfAttack)()          const PURE;  //Radians
    STDMETHOD_(float,  GetLinearCLAlpha)()                  const PURE;  //Per Radian
    STDMETHOD_(float,  GetWingArea)()                       const PURE;  //Square Feet
    STDMETHOD_(float,  GetWingSpan)()                       const PURE;  //Feet
    STDMETHOD_(float,  GetTotalLongitudinalThrust)()        const PURE;  //Pounds
    STDMETHOD_(float,  GetLiftForce)()                      const PURE;  //Pounds

    STDMETHOD_(double, GetThrottlePercent)()               const PURE; 
    STDMETHOD_(double, GetElevatorPercent)()               const PURE;
    STDMETHOD_(double, GetAileronPercent)()                const PURE;
    STDMETHOD_(double, GetRudderPercent)()                 const PURE;
    STDMETHOD_(double, GetSpoilersPercent)()               const PURE;  
    STDMETHOD_(double, GetFlapsPercent)()                  const PURE;  

    STDMETHOD_(void,   SetThrottlePercent)(double dPct)          PURE;  
    STDMETHOD_(void,   SetElevatorPercent)(double dPct)          PURE;  // pitch down <-0-> pitch up
    STDMETHOD_(void,   SetAileronPercent)(double dPct)           PURE;  // roll left  <-0-> roll right
    STDMETHOD_(void,   SetRudderPercent)(double dPct)            PURE;  // yaw left   <-0-> yaw right
    STDMETHOD_(void,   SetFlapsPercent)(double dPct)             PURE;  // none 0-> full
    STDMETHOD_(void,   SetSpoilersPercent)(double dPct)          PURE;  // retracted 0-> extended

    //Controller functions (e.g. PID) based on aircraft dynamics
    STDMETHOD_(double, CalculateDesiredBank)(double dHeadingError /*radians*/, double dDeltaT) PURE; //Radians
    STDMETHOD_(double, CalculateDeltaThrottle)(double dSpeedError /*feet/sec*/,double dDeltaT) PURE; //Radians
    
    //Taxi and Pushback logic is handled by the simulation
    STDMETHOD_(void,    SetTaxiHeading)(float fHeading)           PURE; //Radians
    STDMETHOD_(void,    SetTaxiSpeed)(float fSpeed)               PURE; //FPS
    STDMETHOD_(void,    StopTaxi)()                               PURE;
    STDMETHOD_(void,    SetPushBack)(BOOL bOn)                    PURE;
};

DEFINE_GUID(IID_IAirplaneAIServiceV01,  0x4c30551e, 0xde5a, 0x4989, 0xb6, 0x2d, 0xf4, 0x6d, 0x38, 0xe6, 0x44, 0xde); 

/** Because this interface derives from the IAircraftAIService, this interface may also be used for features related to AI monitoring, such as the User Tips feature. */
DECLARE_INTERFACE_ (IHelicopterAIServiceV01, IAircraftAIServiceV01)
{
};

DEFINE_GUID(IID_IHelicopterAIServiceV01,  0xcb17b92d, 0xa4e, 0x4b62, 0x8c, 0xbb, 0xa7, 0x3b, 0x86, 0x49, 0x7f, 0x80);

/**********************************************************************************************************
IGroundVehicleAIService - Interface for getting AI parameters from this object
**********************************************************************************************************/
DECLARE_INTERFACE_ (IGroundVehicleAIService, IAIService)
{
};

DEFINE_GUID(IID_IGroundVehicleAIService, 0xc9b04d0f, 0x29d5, 0x4585, 0xb0, 0xef, 0x67, 0x25, 0x75, 0x5c, 0x78, 0x4f);


} //End namespace P3D