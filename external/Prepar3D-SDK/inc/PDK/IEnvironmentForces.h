// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// IEnvironmentForces.h
// Prepar3D SDK interface for environment forces
//----------------------------------------------------------------------------

#pragma once

#include <ObjBase.h>

#include "Types.h"
#include "Helpers\ListBuilder.h"

namespace P3D
{
    /**  \defgroup environmentforces    Environment Force Service
    * This PDK service allows callers to manipulate environment forces during runtime.
    * Environment forces registered and unregistered using this interface.
    * Environment forces utilize world-based positioning and may or may not be associated with an object.
    * \{
    **/

    // Forward declarations
    interface IEnvironmentForceV400;

    using IEnvironmentForceList = IListBuilder<IEnvironmentForceV400>;
    using EnvironmentForceList = CComPtrVecBuilder<IEnvironmentForceV400>;

    /**
    * The interface used when registering for environment force notifications.
    **/
    DECLARE_INTERFACE_(IEnvironmentForceCallbackV410, IUnknown)
    {
        /**
        * This function is called when one or more environment forces are within the given radius of the given object.
        * @param   uObjectId            The object id that was registered to receive the callback.
        * @param   ppEnvironmentForces  The array of environment forces within the registered radius.
        * @param   uCount               The number of environment forces in the array.
        * @remarks                      The returned environment force vector includes forces associated with the registered object.
        *                               These forces can be filtered using the IEnvironmentForceV400::GetOwnerId() function.
        **/
        virtual void Invoke(__in UINT uObjectId, __in const IEnvironmentForceV400* const* ppEnvironmentForces, __in UINT uCount) abstract;
    };

    /**
    * This service is responsible for the management of environment forces.
    **/
    DECLARE_INTERFACE_(IEnvironmentForceManagerV410, IUnknown)
    {
        /**
        * Registers a developer defined environment force.
        * @param        pEnvironmentForce       The IEnvironmentForceV400 to be registered.
        * @return       S_OK if the environment force was successfully registered, E_FAIL otherwise.
        **/
        virtual HRESULT RegisterEnvironmentForce(__in __notnull IEnvironmentForceV400* pEnvironmentForce) PURE;

        /**
        * Unregisters a developer defined environment force.
        * @param        pEnvironmentForce       The environment force to be destroyed.
        * @return       S_OK if the environment force was successfully unregistered, E_FAIL otherwise.
        **/
        virtual HRESULT UnregisterEnvironmentForce(__in __notnull IEnvironmentForceV400* pEnvironmentForce) PURE;

        /**
        * Returns a list of forces for a given radius.
        * @param        vWorldPosRadiansFeet    The world location in radians/feet.
        * @param        fRadiusFeet             The radius in feet.
        * @param        vecForces               The list of forces.
        * @return       This function currently only returns S_OK.
        **/
        virtual HRESULT GetForcesInRadius(__in const DXYZ& vWorldPosRadiansFeet, __in float fRadiusFeet, __in IEnvironmentForceList& vecForces) const PURE;

        /**
        * Registers an environment force notification callback.
        * The callback is issued when one or more environment forces are within the given radius of the given object.
        * @param        uObjectId               The object's id.
        * @param        fRadiusFeet             The radius in feet.
        * @param        pfCallback              A pointer to the callback interface to be registered.
        * @return       S_OK if the environment force notification was successfully registered, E_FAIL otherwise.
        **/
        virtual HRESULT RegisterNotification(__in UINT uObjectId, __in float fRadiusFeet, __in __notnull IEnvironmentForceCallbackV410* pfCallback) PURE;

        /**
        * Unregisters an environment force notification callback.
        * @param        uObjectId               The object's id.
        * @param        pfCallback              A pointer to the callback interface to be unregistered.
        * @return       S_OK if the environment force notification was successfully unregistered, E_FAIL otherwise.
        **/
        virtual HRESULT UnregisterNotification(__in UINT uObjectId, __in __notnull IEnvironmentForceCallbackV410* pfCallback) PURE;
    };

    DEFINE_GUID(IID_IEnvironmentForceManagerV410, 0xbc06ef47, 0xc29b, 0x4b2e, 0xa0, 0x92, 0xa, 0xee, 0x2b, 0x32, 0xb4, 0xf1);
    DEFINE_GUID(SID_EnvironmentForceManager, 0xa416dd00, 0x1836, 0x450a, 0x8c, 0xad, 0xa6, 0xb3, 0x96, 0xb6, 0xdb, 0xbc);

    /**
    * This interface may be implemented by developers to register environment forces.
    **/
    DECLARE_INTERFACE_(IEnvironmentForceV400, IUnknown)
    {
        /**
        * The effective radius of the environment force.
        * @return       The radius in feet.
        **/
        virtual float   GetRadius() const PURE;

        /**
        * A half-angle value used to further define the volume of the environment force.
        * @return       The half-angle of the volume in radians.
        * @remarks      Examples:
        *                   PI / 4 would define a cone.
        *                   PI / 2 would define a half-sphere.
        *                   PI would define a complete sphere.
        **/
        virtual float   GetHalfAngle() const PURE;

        /**
        * The world location of the environment force.
        * @param[out]   vWorldPosRadiansFeet    The world location in radians/feet.
        **/
        virtual void    GetPosition(__out DXYZ& vWorldPosRadiansFeet) const PURE;

        /**
        * The world force at the environment force location.
        * @param[out]   vWorldForcePounds       The world force in pounds.
        **/
        virtual void    GetForce(__out DXYZ& vWorldForcePounds) const PURE;

        /**
        * The world force at the given world position.
        * @param        vWorldPosRadiansFeet    The world location in radians/feet.
        * @param[out]   vWorldForcePounds       The world force in pounds.
        **/
        virtual void    GetForceAtLocation(__in const DXYZ& vWorldPosRadiansFeet, __out DXYZ& vWorldForcePounds) const PURE;

        /**
        * The object id of the environment force owner.
        * @return       The owner's object id (zero for no owner).
        **/
        virtual UINT    GetOwnerId() const PURE;
    };

    DEFINE_GUID(IID_IEnvironmentForceV400, 0x30c4618d, 0x4a15, 0x4e0a, 0xaa, 0x40, 0xc9, 0xad, 0x8e, 0xc1, 0x2, 0x16);
    DEFINE_GUID(SID_EnvironmentForce, 0x7d0d4928, 0xc5c1, 0x4e08, 0xb5, 0x7b, 0xa0, 0x4b, 0x5d, 0x72, 0xc0, 0xa2);

    /** \}*/ // end of doc group
};
