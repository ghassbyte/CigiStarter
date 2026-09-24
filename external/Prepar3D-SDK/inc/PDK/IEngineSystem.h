// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-------------------------------------------------------------------------------------------------
// IEngineSystem.h
// Description: Interface for an externally developed engine/propulsion system for core P3D aircraft
//--------------------------------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>

#include "Types.h"
#include "ISimObject.h"

namespace P3D
{

interface ISavedEngineSystemStateV500;

/** \defgroup enginesystem   IEngineSystem
* This interface can be used to implement a custom engine/propulsion system plugin, and still allow other 
* core aircraft subsystems to interact appropriately with the custom engine system.
* A sample implementation using this interface can be found in the MyEngine sample in the PDK General Samples.
* \remarks 
* - Engine indexes are 0-based.\n\n
* - To enable a custom engine system plugin on a native aircraft, set in the aircraft.cfg or sim.cfg:\n\n
*   [GeneralEngineData]\n
*   EngineSystem = {bc95b363-1d22-42aa-82b1-f10905b22c40}\n
*   engine_type = 2\n\n
*   Where,\n
*   - engine type = 2 indicates to P3D to not create any core engine implementations\n
*   - The EngineSystem guid must match the service ID (SID_) of the subsystem factory registered at DLL load 
*     with the PDK. See the MyEngine sample.
* \{ 
*/

 
 /**
 * IEngineSystem is the primary interface for the core P3D aircraft simulation to get and set data inside
 * the engine system implementation.
 **/
DECLARE_INTERFACE_(IEngineSystemV500, ISimulationV310)
{
    /**
    * Returns a reference to a saved class holding the current state of the engine system. This is
    * used for scenario saving and state transfer when switching aircraft.
    * @param    ppSavedState    Address of a object pointer holding the current state.
    **/
    STDMETHOD(SaveState)(__out       ISavedEngineSystemStateV500** ppSavedState)  const  PURE;
    /**
    * Passes a reference to a saved class holding the the state to initialize the engine system. This is
    * used for scenario loading and state transfer when switching aircraft.
    * @param    pSavedState    Address of a object holding the current state.
    **/
    STDMETHOD(LoadState)(__in  const ISavedEngineSystemStateV500*   pSavedState)         PURE;
    /**
    * Called on aircraft initialization.
    **/
    STDMETHOD(Init)()                                                         PURE;
    /**
    * Called on aircraft de-initialization.
    **/
    STDMETHOD(DeInit)()                                                       PURE;
    /**
    * Returns total number of engines.
    **/
    STDMETHOD_(UINT,   GetNumberOfEngines)()                            const PURE;
    /**
    * Returns total number of propellers.
    **/
    STDMETHOD_(UINT,   GetNumberOfPropellers)()                         const PURE;
    /**
    * Returns the maximum rated RPM for the engines.
    **/
    STDMETHOD_(double, GetMaxRpm)()                                     const PURE;
    /**
    * Request to set engine power off.  Generally used for initializations such as
    * repositioning from the UI.  
    * @param    bSetAllEng  TRUE dictates all engines should be set.  If FALSE, set the engine indicated by iEng.
    * @param    iEng        Indicates which engine to set. Ignored if bSetAllEng = TRUE.
    **/
    STDMETHOD (SetPowerOff)(   BOOL bSetAllEng = TRUE, UINT iEng = 0)         PURE;
    /**
    * Request to set engine power to idle.  Generally used for initializations such as
    * repositioning from the UI.  
    * @param    bSetAllEng  TRUE dictates all engines should be set.  If FALSE, set the engine indicated by iEng.
    * @param    iEng        Indicates which engine to set. Ignored if bSetAllEng = TRUE.
    **/
    STDMETHOD (SetPowerIdle)(  BOOL bSetAllEng = TRUE, UINT iEng = 0)         PURE;
    /**
    * Request to set engine power to a cruise setting.  Generally used for initializations such as
    * repositioning from the UI.  
    * @param    bSetAllEng  TRUE dictates all engines should be set.  If FALSE, set the engine indicated by iEng.
    * @param    iEng        Indicates which engine to set. Ignored if bSetAllEng = TRUE.
    **/
    STDMETHOD (SetPowerCruise)(BOOL bSetAllEng = TRUE, UINT iEng = 0)         PURE;
    /**
    * Resets all damage states on the engines.
    **/
    STDMETHOD (ResetDamage)()                                                 PURE;
    /**
    * Set engine failed state.
    **/
    STDMETHOD (SetFailed)(BOOL bFailed, UINT iEng)                            PURE;
    //General engine accessors
    /**
    * Returns engine combustion state
    **/
    STDMETHOD_(BOOL,   IsCombusting)(UINT iEng)                         const PURE;
    /**
    * Returns engine throttle percentage, typically 0.0 - 1.0.
    **/
    STDMETHOD_(double, GetThrottle)(UINT iEng)                          const PURE;
    /**
    * Sets throttle percentage, typically 0.0 - 1.0. This would be used for both initialization
    * as well as autopilot throttle controllers.
    **/
    STDMETHOD (SetThrottle)(double dThrottle, UINT iEng)                      PURE;
    /**
    * Returns the engine RPM percentage of maximum rated RPM, typically 0.0 - 1.0.
    **/
    STDMETHOD_(double, GetPercentRpm)(UINT iEng)                        const PURE;
    /**
    * Sets the engine RPM percentage of maximum rated RPM, typically 0.0 - 1.0. Typically used
    * for initialization, as well as certain transmission feedback scenarios on some aircraft.
    **/
    STDMETHOD (SetPercentRpm)(double dPctRpm, UINT iEng)                      PURE;
    /**
    * Returns the offset (in feet) of the engine from the aircraft's datum.
    * @param    vOffset Out-parameter set to the total offset vector.
    **/
    STDMETHOD (GetOffset)(__out FXYZ& vOffset, UINT iEng)               const PURE;
    /**
    * Returns the angle (in radians) of the engine's thrust from the aircraft's X-Z axes.  
    * Banks angle (Z-component) is typically expected to be zero.
    * @param    vAngle Out-parameter set to the total angle vector.
    **/
    STDMETHOD (GetAngle)(__out FXYZ& vAngle, UINT iEng)                const PURE;
    /**
    * Returns the thrust scalar value (pounds-force).
    **/
    STDMETHOD_(double, GetThrust)(UINT iEng)                            const PURE;
    /**
    * Returns the engine shaft torque (foot-pounds).
    **/
    STDMETHOD_(double, GetShaftTorque)(UINT iEng)                       const PURE; 
    /**
    * Returns the engine fuel flow rate (pounds per hour).
    **/
    STDMETHOD_(double, GetFuelFlowPPH)(UINT iEng)                       const PURE; 
    /**
    * Returns the fuel selector ID value for the engine.
    * Refer to the Fuel Tank Selection list in Simulation Variables for a complete list of IDs.
    **/
    STDMETHOD_(int, GetFuelTankSelector)(UINT iEng)                     const PURE; 
    /**
    * Sets the fuel selector ID value for the engine.
    * Refer to the Fuel Tank Selection list in Simulation Variables for a complete list of IDs.
    **/
    STDMETHOD (SetFuelTankSelector)(int eSelector, UINT iEng)                 PURE; 
    /**
    * Called by the fuel system with a bit-field of tanks currently being used.  It is the engine system's responsibility to maintain this data.
    * @param    bFuelAvailable  Indicates if the fuel system is providing available fuel to the engine, based on the engine's fuel selector.
    * @param    bfTanksUsed     Bit-field indicating which specific tanks are feeding the engine.  The complete list of individual tank masks can be found in Simulation Variables.
    * @param    nTanksUsed      Total number of tanks currently feeding the engine.
    **/
    STDMETHOD (SetFuelTanksUsed)(__in BOOL bFuelAvailable, __in int bfTanksUsed, __in UINT nTanksUsed, __in UINT iEng)        PURE;
    /**
    * Returns a bit-field of tanks currently being used and the total count of tanks used.  It is the engine system's responsibility to maintain this data.
    * @param    bfTanksUsed     Bit-field indicating which specific tanks are feeding the engine.  The complete list of individual tank masks can be found in Simulation Variables.
    * @param    nTanksUsed      Total number of tanks currently feeding the engine.
    **/
    STDMETHOD (GetFuelTanksUsed)(__out int& bfTanksUsed, __out UINT& nTanksUsed, __in UINT iEng)                        const PURE;
    /**
    * Returns for a turbine engine the N1 engine speed.  Typically 0 - 100.
    **/
    STDMETHOD_(double, GetTurbineN1)(UINT iEng)                         const PURE;
    /**
    * Returns for a turbine engine the Bleed Air Preussure (pounds per square foot).
    **/
    STDMETHOD_(double, GetTurbineBleedAirPressure)(UINT iEng)           const PURE;     //PSF
    /**
    * Returns for a turbine engine the percetage of maximum rated torque (typically 0.0 - 1.0).
    **/
    STDMETHOD_(double, GetTurbinePercentTorque)(UINT iEng)              const PURE;
    /**
    * Returns for a reciprocating engine manifold pressure (pounds per square foot).
    **/
    STDMETHOD_(double, GetRecipManifoldPressure)(UINT iEng)             const PURE;     //PSF
    /**
    * Returns for a propeller the percentage of maximum rated RPM (typically 0.0 - 1.0).
    **/
    STDMETHOD_(double, GetPropellerPercentRpm)(UINT iProp)              const PURE;
    /**
    * Returns for a propeller the torque required, or absorbed, for its current speed and atmospheric conditions (foot-pounds).
    **/
    STDMETHOD_(double, GetPropellerAbsorbedTorque)(UINT iProp)          const PURE;
    /**
    * Returns for a propeller the thrust scalar being generated (pounds-force).
    **/
    STDMETHOD_(double, GetPropellerThrust)(UINT iProp)                  const PURE;
    /**
    * Returns for a propeller the induced velocity currently being generated (feet per second).  This is used
    * on center-line thrust propeller aircraft to generate additional forces on tail surfaces.
    **/
    STDMETHOD_(double, GetPropellerInducedVelocity)(UINT iProp)         const PURE;
    /**
    * Returns additional moments generated by the propeller (foot-pounds).
    * Examples of effects contributing to this are gyroscopic effects and P-factor.
    * This should not include moments due to the propellers offset from the center-of-gravity.
    * @param    vMoment Out-parameter set to the total moment vector.
    **/
    STDMETHOD (GetPropellerMoment)(__out DXYZ& vMoment, UINT iProp)     const PURE;
    /**
    * Returns the propeller offset from the aircraft's datum (feet).
    * @param    vOffset Out-parameter set to the total offset vector.
    **/
    STDMETHOD (GetPropellerOffset)(__out FXYZ& vOffset, UINT iProp)     const PURE;
};

/// Current versioned ID of the Engine System interface.
DEFINE_GUID(IID_IEngineSystemV500,   0xbf0d67b8, 0x5dbe, 0x4288, 0x97, 0xb4, 0x5a, 0x55, 0xc8, 0x50, 0x23, 0x12);
///Service ID for the engine system.
DEFINE_GUID(SID_EngineSystem ,       0x864e3526, 0x156a, 0x47fc, 0xad, 0x8b, 0xcb, 0xcb, 0x6b, 0xdb, 0xf6, 0x6);

/**
* ISavedEngineSystemState is used during loading and saving the state of the engine system
* when saving and loading scenarios, as well as when switching aircraft.
**/
DECLARE_INTERFACE_(ISavedEngineSystemStateV500, IUnknown)
{
    /** Returns the number of engines on this aircraft.*/
    STDMETHOD_(UINT,  GetNumberOfEngines)()                     const PURE;
    /** Returns TRUE if the specified engine is combusting.    */
    STDMETHOD_(BOOL,  GetCombustion)(UINT iEng)                 const PURE;
    /** Returns the percentage of maximum rated RPM for the specified engines.  Typically 0.0 - 1.0.*/
    STDMETHOD_(float, GetPercentEngineRpm)(UINT iEng)           const PURE;
    /** Returns the throttle percentage for the specified engine.  Typically 0.0 - 1.0. */
    STDMETHOD_(float, GetThrottleLeverPosition)(UINT iEng)      const PURE;
    /** Returns the propeller lever percentage for the specified engine.  Typically 0.0 - 1.0. */
    STDMETHOD_(float, GetPropellerLeverPosition)(UINT iEng)     const PURE;
    /** Returns the mixture (or condition) lever percentage for the specified engine.  Typically 0.0 - 1.0. */
    STDMETHOD_(float, GetMixtureLeverPosition)(UINT iEng)       const PURE;
};

/// Current versioned ID of the Saved Engine State interface
DEFINE_GUID(IID_ISavedEngineSystemStateV500, 0x2732b3a1, 0x9959, 0x4038, 0x8e, 0x97, 0xa5, 0xac, 0xa5, 0xa1, 0x5f, 0xa5);
 
/** \} */

};
