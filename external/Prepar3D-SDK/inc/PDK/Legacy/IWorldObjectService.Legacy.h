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

namespace P3D
{
    /** \defgroup worldobjectservice   World Object Service
     *
     * This PDK service allows callers to create, move, and delete objects in the world.
     * \{
     **/
    DECLARE_INTERFACE_(IWorldObjectServiceV450, IUnknown)
    {
        /**
        * Creates a world object by model GUID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    guid   The model GUID of the object to be created.
        * @param    location   The location that the model should be placed.
        * @param    scale   The amount that the model should be scaled. 1.0 is the default scale.
        * @param    isOnGround   Enable to ignore the specified altitude and get pulled to the ground.
        * @param    idObject   Object ID of the newly created object.
        */
        STDMETHOD(CreateObject)(__in const GUID & guid, __in const ObjectWorldTransform & location, __in FLOAT scale, __in BOOL isOnGround, __out UINT & idObject) abstract;

        /**
        * Moves a world object by object ID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    idObject   The object ID of the object to be moved.
        * @param    location   The location that the model should be moved.
        * @param    isOnGround   Enable to ignore the specified altitude and get pulled to the ground.
        */
        STDMETHOD(MoveObject)(__in UINT idObject, __in const ObjectWorldTransform & location, __in BOOL isOnGround) abstract;

        /**
        * Removes a world object by object ID. Returns S_OK upon success, E_FAIL upon failure.
        * @param    idObject   The object ID of the object to be removed.
        */
        STDMETHOD(RemoveObject)(__in UINT idObject) abstract;
    };

    /// WorldObjectService interface ID
    DEFINE_GUID(IID_IWorldObjectServiceV450, 0xe27e820, 0x4a5d, 0x4f14, 0x85, 0x60, 0x1d, 0xf2, 0x82, 0xb2, 0x5e, 0xfb);
    /// WorldObjectService service ID
    //DEFINE_GUID(SID_WorldObjectService, 0x8c8c2fff, 0x338f, 0x4c33, 0x90, 0xfb, 0x76, 0x20, 0x73, 0xe0, 0xec, 0xfb);

    /** \} */
};