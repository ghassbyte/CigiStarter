// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IVisualEffects.h
// Prepar3D SDK interface for visual effects
//----------------------------------------------------------------------------

#pragma once

#include <ObjBase.h>
#include "Types.h"
#include "Legacy\IVisualEffects.Legacy.h"

namespace P3D
{
    /**  \defgroup visualEffects    Visual Effect Service
    * This PDK service allows callers to manipulate visual effects during runtime.  Effects spawned using this interface are defined
    * with a world-based position and are not associated with any other object.
    * \{
    **/

    interface IVisualEffectCallbackV430;

    /** 
    * This PDK service allows callers to manipulate visual effects during runtime in the following ways:
    *  - Turn effects off and on
    *  - Modify the position and orientation or an effect.
    *  - Modify the duration of an active effect.
    * 
    * \remark  Effects spawned using this interface are defined
    * with a world-based position and are not associated with any other object.
    **/
    DECLARE_INTERFACE_(IVisualEffectManagerV530, IVisualEffectManagerV430)
    {
        /**
        *  Turn effect on
        * @param       pszEffectName         Name of effect.
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon. 
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank.
        * @param       bOnGround             Is effect on ground.
        * @param[out]  iEffectID             Effect id.
        **/
        STDMETHOD (EffectOn)(__in __notnull LPCWSTR pszEffectName, __in const P3DDXYZ& vWorldPosRadiansFeet, __in const P3DDXYZ& vOrienatationRadians, __in BOOL bOnGround, __out int& iEffectID) PURE;
        /**
        * Turn effect off
        * @param   iEffectID    Effect id
        **/
        STDMETHOD (EffectOff)(__inout int& iEffectID) PURE;
        /**
        * Set Effect Duration In Seconds 
        * @param   iEffectID      Effect id.
        * @param   fDurationSecs  Duration in seconds.
        **/
        STDMETHOD (SetEffectDuration)(__in int iEffectID, float fDurationSecs) PURE;                                        //added duration from time of call.
                                                                                                                            /**
                                                                                                                            * Set effect world position in radians/feet
                                                                                                                            * @param       iEffectID             Effect id.
                                                                                                                            * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon. 
                                                                                                                            **/
        STDMETHOD (SetEffectWorldPosRadiansFeet)(__in int iEffectID, __in const P3DDXYZ& vWorldPosRadiansFeet) PURE;        //Update lon/alt/lat (radians/feet/radians)
                                                                                                                            /**
                                                                                                                            * Set effect world orientation in radians
                                                                                                                            * @param       iEffectID             Effect id.
                                                                                                                            * @param       vOrientationRadians   Orientation in Radians. X=Pitch Y=Heading Z=Bank
                                                                                                                            **/
        STDMETHOD (SetEffectWorldOrientationRadians)(__in int iEffectID, __in const P3DDXYZ& vOrientationRadians) PURE;    //Update pitch/heading/bank (radians)        
                                                                                                                           /**
                                                                                                                           * Creates a dynamic "rope" visual effect.  This can be used to attach physics, such as a hoisted load.
                                                                                                                           * @param       vStart                  Initial offset of start of rope, in feet.
                                                                                                                           * @param       vEnd                    Initial offset of end of rope, in feet.
                                                                                                                           * @param       fRopeMass               Mass of rope, in slugs.
                                                                                                                           * @param       fRopeRadius             Radius of rope, in feet.
                                                                                                                           * @param       fRestingRopeLength      Unstretched length of rope, in feet.
                                                                                                                           * @param       fRelativeGroundPosition Relative height of the ground, in feet.  Typically negative.
                                                                                                                           * @param       riid                    Interface ID for the version of the IRopeSimulation being requested.
                                                                                                                           * @param       ppvIRopeSim             Returns reference-counted rope simulation which can be updated in real-time through the IRopeSimulation interface (below).
                                                                                                                           **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, float fRopeRadius, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __in REFIID riid, __out void** ppvIRopeSim) PURE;
        /**
        * Deprecated versions of CreateRopeSimulation.
        **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __in REFIID riid, __out void** ppvIRopeSim) PURE;
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __out IRopeSimulationV400** ppIRopeSim) PURE;
        /**
        * Registers a visual effect notification callback.
        * The callback is issued when one or more visual effects are created.
        * @param        pfCallback              A pointer to the callback interface to be registered.
        * @return       S_OK if the visual effect notification was successfully registered, E_FAIL otherwise.
        **/
        virtual HRESULT RegisterVisualEffectCallback(IVisualEffectCallbackV430* pfCallback) abstract;
        /**
        * Unregisters a visual effect notification callback.
        * @param        pfCallback              A pointer to the callback interface to be unregistered.
        * @return       S_OK if the visual effect notification was successfully unregistered, E_FAIL otherwise.
        **/
        virtual HRESULT UnRegisterVisualEffectCallback(IVisualEffectCallbackV430* pfCallback) abstract;
        /**
        * Get effect world position in radians/feet
        * @param       hEffect               Effect handle.
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon.
        **/
        STDMETHOD (GetEffectWorldPosRadiansFeet)(__in HANDLE hEffect, __out P3DDXYZ& vWorldPosRadiansFeet) PURE;      //Returns lon/alt/lat (radians/feet/radians)
                                                                                                                      /**
                                                                                                                      * Get effect world orientation in radians
                                                                                                                      * @param       hEffect               Effect handle.
                                                                                                                      * @param       vOrientationRadians   Orientation in Radians. X=Pitch Y=Heading Z=Bank
                                                                                                                      **/
        STDMETHOD (GetEffectWorldOrientationRadians)(__in HANDLE hEffect, __out P3DDXYZ& vOrientationRadians) PURE;   //Returns pitch/heading/bank (radians)  
    };

    /// VisualEffectManager interface ID
    DEFINE_GUID(IID_IVisualEffectManagerV530, 0xff962e5d, 0x7b30, 0x48a8, 0x9d, 0xd7, 0x2b, 0x34, 0xd5, 0x4c, 0x18, 0xe0);

    /// VisualEffectManager service ID
    DEFINE_GUID(SID_VisualEffectManager, 0x5e7300c9, 0x8ec3, 0x4e43, 0xa5, 0x6e, 0x13, 0xb0, 0xb0, 0xf4, 0xc, 0xc0);

    DECLARE_INTERFACE_(IVisualEffectCallbackV430, IUnknown)
    {
        /**
        * Called when a visual effect is created.
        * @param       hEffect              Effect handle.
        * @param       pszEffectName        Name of effect.
        * @param       vLocation            Location of effect in radians/feet. If uObjectID != 0, it's the offset relative to the vehicle in feet.
        * @param       vOrientation         Orientation of effect in radians. If uObjectID != 0, it's the orientation relative to the vehicle in radians.
        * @param       uObjectID            Object's id, 0 if not attached.
        **/
        virtual void CreateEffect(HANDLE hEffect, LPCWSTR pszEffectName, const P3D::P3DDXYZ& vLocation, const P3D::P3DDXYZ& vOrientation, UINT uObjectID) abstract;
        /**
        * Called when a visual effect is destroyed.
        * @param       iEffectID            Effect id.
        **/
        virtual void DestroyEffect(HANDLE hEffect) abstract;
    };

    DEFINE_GUID(IID_IVisualEffectCallbackV430, 0x67428aed, 0x5e4d, 0x4f47, 0xb7, 0x3b, 0xe8, 0xb0, 0x21, 0xa2, 0xdb, 0xae);

    /**
    * This interface allows real-time updates of a rope created through the IVisualEffectManager. Examples of uses
    * would be helicopter hoists or tow plane cable.
    **/
    DECLARE_INTERFACE_(IRopeSimulationV530, IRopeSimulationV420)
    {
        /**
        * Sets the rope starting offset.  This is ignored if the starting point is fixed (see SetFixed().)
        * @param    vStart  Offset, in feet, from the Lat/Lon/Alt specified in SetRenderWorldPosition().
        **/
        STDMETHOD_(void, SetStart)(const P3DFXYZ& vStart)                          PURE;
        /**
        * Gets the rope starting offset, in feet, from the Lat/Lon/Alt specified in SetRenderWorldPosition().
        **/
        STDMETHOD_(P3DFXYZ, GetStart)()                                      const PURE;
        /**
        * Sets the rope ending offset.  This is ignored if the ending point is fixed (see SetFixed().)
        * @param    vEnd  Offset, in feet, from the Lat/Lon/Alt specified in SetRenderWorldPosition().
        **/
        STDMETHOD_(void, SetEnd)(const P3DFXYZ& vEnd)                              PURE;
        /**
        * Gets the rope ending offset, in feet, from the Lat/Lon/Alt specified in SetRenderWorldPosition().
        **/
        STDMETHOD_(P3DFXYZ, GetEnd)()                                        const PURE;
        /**
        * Sets whether the rope is fixed or not.  "Fixed" means that the point will be set (SetStart() or SetEnd())
        *  rather than allowing the rope simulation determine its position.  For example, a helicopter hoist cable
        *  starting point would be "fixed" to the helicopter.
        * @param    bStart    Set to true if changing the starting point, false if the end point.
        * @param    vVal      Set to true if "fixed", false otherwise.
        **/
        STDMETHOD_(void, SetFixed)(bool bStart, bool bVal)                         PURE;
        /**
        * Returns whether the rope start or end is "fixed". 
        * @param    bStart    Set to true if getting the starting point, false if the end point.
        **/
        STDMETHOD_(bool, GetFixed)(bool bStart)                              const PURE;
        /**
        * Sets the world-reference relative wind vector.  
        * @param    vWind  where X=longitude, Y=altitude, Z=latitude, in feet per second.
        **/
        STDMETHOD_(void, SetRelativeWind)(const P3DFXYZ& vWind)                    PURE;
        /**
        * Returns rope length, in feet
        **/
        STDMETHOD_(float, GetRopeLength)()                                   const PURE;
        /**
        * Sets new rope length
        * @param fLength, in feet.  The new rope length
        **/
        STDMETHOD (SetRopeLength)(float fLength)                                   PURE;
        /**
        * Sets the world position of the rope anchor point.
        * @param    vPos, where X=longitude (radians), Y=altitude (feet), Z=latitude (radians)
        **/
        STDMETHOD_(void, SetRenderWorldPosition)(const P3DDXYZ& vPos)               PURE;
        /**
        * Sets the relative ground position.  
        * @param    bCheckGround  Set to true if the simulation should include ground interaction.
        * @param    fAlt          The relative altitude, in feet, of the ground.  This should be negative if the ground is below.
        **/
        STDMETHOD_(void, SetRelativeGroundPosition)(bool bCheckGround, float fAlt) PURE;
        /**
        * Sets the 8-bit color RGBA of the rope
        * @param    uRed    Red channel code (0 - 255)
        * @param    uGreen  Green channel code (0 - 255)
        * @param    uBlue   Blue channel code (0 - 255)
        * @param    uAlpha  Alpha channel code (0 - 255)
        **/
        STDMETHOD (SetRopeColor)(__in unsigned char uRed, __in unsigned char uGreen, __in unsigned char uBlue, __in unsigned char uAlpha) PURE;
        /**
        * Sets the rope radius in feet
        * @param   fRadius, feet
        **/
        STDMETHOD (SetRopeRadius)(float fRadius)    PURE;
    };

    DEFINE_GUID(IID_IRopeSimulationV530, 0x48e18a16, 0x57db, 0x42cc, 0xbe, 0x92, 0xd8, 0x11, 0x44, 0xf7, 0x4d, 0xd2);

    /// Service ID
    DEFINE_GUID(SID_RopeSimulation, 0xe861ea03, 0xfe0b, 0x48d8, 0xa9, 0x37, 0xc, 0x35, 0xe, 0xeb, 0xf3, 0xe);

    /** \}*/ // end of doc group
}