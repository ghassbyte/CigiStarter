// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// IWorldObjectService.h
// Prepar3D SDK interface to create, move, and delete objects in the world.
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>
#include "Types.h"
#include "RenderingTypes.h"
#include "Legacy\IWorldObjectService.Legacy.h"  //Include previous version

namespace P3D
{
    /** \defgroup worldobjectservice   World Object Service
     *
     * This PDK service allows callers to create, move, and delete objects in the world.
     * \{
     **/
    DECLARE_INTERFACE_(IWorldObjectServiceV510, IWorldObjectServiceV450)
    {
        /**
        * Creates a world object by model GUID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    guid   The model GUID of the object to be created.
        * @param    location   The location that the model should be placed.
        * @param    scale   The amount that the model should be scaled. 1.0 is the default scale.
        * @param    isOnGround   Enable to ignore the specified altitude and get pulled to the ground.
        * @param    idObject   Object ID of the newly created object.
        */
        STDMETHOD(CreateObject)(__in const GUID & guid, __in const ObjectWorldTransform & location, __in FLOAT scale, __in WorldObjectFlags flags, __out UINT & idObject) abstract;

        /**
        * Moves a world object by object ID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    idObject   The object ID of the object to be moved.
        * @param    location   The location that the model should be moved.
        * @param    isOnGround   Enable to ignore the specified altitude and get pulled to the ground.
        */
        STDMETHOD(MoveObject)(__in UINT idObject, __in const ObjectWorldTransform& location, __in BOOL isOnGround) abstract;

        /**
        * Removes a world object by object ID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    idObject   The object ID of the object to be removed.
        */
        STDMETHOD(RemoveObject)(__in UINT idObject) abstract;
    };

    /// WorldObjectService interface ID
    DEFINE_GUID(IID_IWorldObjectServiceV510, 0x8606f3a1, 0x7c79, 0x4165, 0x9c, 0xa8, 0xdd, 0x9a, 0xb4, 0xdd, 0xf2, 0x4f);
    /// WorldObjectService service ID
    DEFINE_GUID(SID_WorldObjectService, 0x8c8c2fff, 0x338f, 0x4c33, 0x90, 0xfb, 0x76, 0x20, 0x73, 0xe0, 0xec, 0xfb);

	/** \} */
};