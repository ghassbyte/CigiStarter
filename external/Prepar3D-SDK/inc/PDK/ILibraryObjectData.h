// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// ILibraryObject.h
// Prepar3D SDK interface for querying data on library objects
//----------------------------------------------------------------------------------

#pragma once

namespace P3D
{
    /** \defgroup   libraryobjectdata  Library Object Data
    * The ILibraryObjectData interface provides capabilities associated with
    * library objects.  
    *
    * An ILibraryObjectData pointer can be obtained from the IPdk QueryService() method.  
    * \{ 
    */

/**
*   ILibraryObject
*   Provides data and access on a library object.
**/
DECLARE_INTERFACE_(ILibraryObjectV400, IUnknown)
{
    /**
    *   @return Returns the Prepar3D unique ID for this object. -1 is an invalid ID.
    **/
    STDMETHOD_(UINT, GetId)()                                                       const PURE;
    /**
    *   @return Returns the description of the object. When adding an object in SimDirector, this will be the "Object Name".
    **/
    STDMETHOD_(LPCTSTR, GetDescription)()                                           const PURE;
    /**
    *   Query for the world position of the object.
    *   @param  vLonAltLat   X = Longitude in radians, Y = altitude in feet, Z = Latitude in radians.
    *   @return   Returns S_OK if the query returns valid data.
    **/
    STDMETHOD (GetLonAltLat)(__out  P3DDXYZ& vLonAltLat)                            const PURE;
    /**
    *   Enables an IUnknown-based service to be attached to this object.  This is meant for ownership and storage only.
    *   It is up to the developer to implement and execute any runtime methods.
    *   @param  Guid for the specific service.
    *   @param  pUnkService void pointer to a service to be attached.
    *   @return Returns S_OK if the service was attached appropriately.
    **/
    STDMETHOD (AttachService)(__in REFGUID guidService, __in IUnknown *pUnkService)       PURE;
    /**
    *   Removes an IUnknown-based interface to be attached to this object.
    *   @param  Guid for the specific service.
    *   @return Returns S_OK if the service was found and detached successfully
    **/
    STDMETHOD (DetachService)(__in REFGUID guidService)                                   PURE;
};
/// Current version of ILibraryObject interface
DEFINE_GUID(IID_ILibraryObjectV400,        0xa95b7913, 0xf6fc, 0x463c, 0x94, 0x7, 0x9, 0x52, 0xda, 0x2a, 0x5, 0x79);

/** \} */
}
