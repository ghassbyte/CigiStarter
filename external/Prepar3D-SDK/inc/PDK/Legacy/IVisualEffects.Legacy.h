// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IVisualEffects.Legacy.h
// Prepar3D SDK interface for visual effects
//----------------------------------------------------------------------------

#pragma once

#include <ObjBase.h>
#include "Types.h"

namespace P3D
{
    interface IRopeSimulationV400;
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
    DECLARE_INTERFACE_(IVisualEffectManagerV400, IUnknown)
    {
        /**
        *  Turn effect on
        * @param       pszEffectName         Name of effect.
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        * @param       bOnGround             Is effect on ground.
        * @param[out]  iEffectID             Effect id.
        **/
        STDMETHOD (EffectOn)(__in __notnull LPCWSTR pszEffectName, __in const P3DDXYZ& vWorldPosRadiansFeet, __in const P3DDXYZ& vOrienatationRadians, __in BOOL bOnGround, __out int& iEffectID) PURE;
        /**
        * Turn effect off
        * @param   iEffectID    Effect id.
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
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        **/
        STDMETHOD (SetEffectWorldPosRadiansFeet)(    __in int iEffectID, __in const P3DDXYZ& vWorldPosRadiansFeet) PURE;    //Update lon/alt/lat (radians/feet/radians)
        /**
        * Set effect world orientation in radians
        * @param       iEffectID             Effect id.
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        **/
        STDMETHOD (SetEffectWorldOrientationRadians)(__in int iEffectID, __in const P3DDXYZ& vOrienatationRadians) PURE;    //Update pitch/heading/bank (radians)        
        /**
        * Creates a dynamic "rope" visual effect.  This can be used to attach physics, such as a hoisted load.
        * @param       vStart                  Initial offset of start of rope, in feet.
        * @param       vEnd                    Initial offset of end of rope, in feet.
        * @param       fRopeMass               Mass of rope, in slugs.
        * @param       fRestingRopeLength      Unstretched length of rope, in feet.
        * @param       fRelativeGroundPosition Relative height of the ground, in feet.  Typically negative.
        * @param       ppIRopeSim              Returns reference-counted rope simulation which can be updated in real-time through the IRopeSimulation interface (below).
        **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __out IRopeSimulationV400** ppIRopeSim) PURE;
    };

    /// VisualEffectManager interface ID
    DEFINE_GUID(IID_IVisualEffectManagerV400,   0x76880b65, 0x30dc, 0x4cb8, 0xa6, 0xa3, 0xf6, 0x81, 0x78, 0xb7, 0x5b, 0x6a);

    /** 
    * This PDK service allows callers to manipulate visual effects during runtime in the following ways:
    *  - Turn effects off and on.
    *  - Modify the position and orientation or an effect.
    *  - Modify the duration of an active effect.
    * 
    * \remark  Effects spawned using this interface are defined.
    * with a world-based position and are not associated with any other object.
    **/
    DECLARE_INTERFACE_(IVisualEffectManagerV410, IVisualEffectManagerV400)
    {
        /**
        *  Turn effect on
        * @param       pszEffectName         Name of effect.
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        * @param       bOnGround             Is effect on ground.
        * @param[out]  iEffectID             Effect id.
        **/
        STDMETHOD (EffectOn)(__in __notnull LPCWSTR pszEffectName, __in const P3DDXYZ& vWorldPosRadiansFeet, __in const P3DDXYZ& vOrienatationRadians, __in BOOL bOnGround, __out int& iEffectID) PURE;
        /**
        * Turn effect off
        * @param   iEffectID    Effect id.
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
        * @param       vWorldPosRadiansFeet  World position in Radians/Feet  X=Lat Y=Alt Z=Lon
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        **/
        STDMETHOD (SetEffectWorldPosRadiansFeet)(    __in int iEffectID, __in const P3DDXYZ& vWorldPosRadiansFeet) PURE;    //Update lon/alt/lat (radians/feet/radians)
        /**
        * Set effect world orientation in radians
        * @param       iEffectID             Effect id.
        * @param       vOrienatationRadians  Orientation in Radians. X=Pitch Y=Heading Z=Bank
        **/
        STDMETHOD (SetEffectWorldOrientationRadians)(__in int iEffectID, __in const P3DDXYZ& vOrienatationRadians) PURE;    //Update pitch/heading/bank (radians)        
        /**
        * Creates a dynamic "rope" visual effect.  This can be used to attach physics, such as a hoisted load.
        * @param       vStart                  Initial offset of start of rope, in feet.
        * @param       vEnd                    Initial offset of end of rope, in feet.
        * @param       fRopeMass               Mass of rope, in slugs.
        * @param       fRestingRopeLength      Unstretched length of rope, in feet.
        * @param       fRelativeGroundPosition Relative height of the ground, in feet.  Typically negative.
        * @param       riid                    Interface ID for the version of the IRopeSimulation being requested.
        * @param       ppvIRopeSim             Returns reference-counted rope simulation which can be updated in real-time through the IRopeSimulation interface (below).
        **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __in REFIID riid, __out void** ppvIRopeSim) PURE;

        /**
        * Deprecated version of CreateRopeSimulation.
        **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __out IRopeSimulationV400** ppIRopeSim) PURE;
    };

    /// VisualEffectManager interface ID
    DEFINE_GUID(IID_IVisualEffectManagerV410,   0x98614528, 0x413d, 0x4984, 0x99, 0xd8, 0x2b, 0x45, 0xbb, 0xe1, 0x35, 0x18);

    /** 
    * This PDK service allows callers to manipulate visual effects during runtime in the following ways:
    *  - Turn effects off and on
    *  - Modify the position and orientation or an effect.
    *  - Modify the duration of an active effect.
    * 
    * \remark  Effects spawned using this interface are defined
    * with a world-based position and are not associated with any other object.
    **/
    DECLARE_INTERFACE_(IVisualEffectManagerV430, IVisualEffectManagerV410)
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
                                                                                                                           * @param       fRestingRopeLength      Unstretched length of rope, in feet.
                                                                                                                           * @param       fRelativeGroundPosition Relative height of the ground, in feet.  Typically negative.
                                                                                                                           * @param       riid                    Interface ID for the version of the IRopeSimulation being requested.
                                                                                                                           * @param       ppvIRopeSim             Returns reference-counted rope simulation which can be updated in real-time through the IRopeSimulation interface (below).
                                                                                                                           **/
        STDMETHOD (CreateRopeSimulation)(__in const P3DFXYZ& vStart, __in const P3DFXYZ& vEnd, __in float fRopeMass, __in float fRestingRopeLength, __in float fRelativeGroundPosition, __in REFIID riid, __out void** ppvIRopeSim) PURE;
        /**
        * Deprecated version of CreateRopeSimulation.
        **/
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
    DEFINE_GUID(IID_IVisualEffectManagerV430, 0x132dfe3d, 0xea31, 0x4976, 0x88, 0xe2, 0xe4, 0x56, 0x44, 0x71, 0x76, 0x21);


    /**
    * This interface allows real-time updates of a rope created through the IVisualEffectManager. Examples of uses
    * would be helicopter hoists or tow plane cable.
    **/
    DECLARE_INTERFACE_(IRopeSimulationV400, IUnknown)
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
        * Returns rope length, in feet.
        **/
        STDMETHOD_(float, GetRopeLength)()                                   const PURE;
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
    };

    /// Interface ID
    DEFINE_GUID(IID_IRopeSimulationV400, 0x1b364767, 0x92d3, 0x46c4, 0xaa, 0xf5, 0xe9, 0xe9, 0x9c, 0x16, 0x4e, 0x3f);

    DECLARE_INTERFACE_(IRopeSimulationV410, IRopeSimulationV400)
    {
        STDMETHOD (SetRopeLength)(float fLength)                                   PURE;
        /**
        * Sets the world position of the rope anchor point.
        * @param    vPos, where X=longitude (radians), Y=altitude (feet), Z=latitude (radians)
        **/
    };

    DEFINE_GUID(IID_IRopeSimulationV410, 0xfb1b1294, 0xcae6, 0x447b, 0x84, 0x37, 0xf2, 0xe7, 0xf4, 0x98, 0xdf, 0x83);

    /**
    * This interface allows real-time updates of a rope created through the IVisualEffectManager. Examples of uses
    * would be helicopter hoists or tow plane cable.
    **/
    DECLARE_INTERFACE_(IRopeSimulationV420, IRopeSimulationV410)
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
    };

    DEFINE_GUID(IID_IRopeSimulationV420, 0x9bcdfbe8, 0x5bd0, 0x4cf2, 0xad, 0x33, 0x1c, 0x77, 0xc4, 0x5e, 0xbc, 0x74);

    /** \}*/ // end of doc group
}