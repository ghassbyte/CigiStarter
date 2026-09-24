// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-------------------------------------------------------------------
// PDK.h
// Prepar3D SDK interface
//-------------------------------------------------------------------

#pragma once

#include <ObjBase.h>

/// Prepar3D SDK namespace used primarily for the PDK and its services
namespace P3D
{

    /** @defgroup pdk PDK
    *  
    *  @{
    */

///Simple vector structures for passing between P3D host and implementation
typedef struct {double dX; double dY; double dZ;} P3DDXYZ;     //64-bit
typedef struct {float  fX; float  fY; float  fZ;} P3DFXYZ;     //32-bit


///IPdk - The primary interface to the Prepar3D SDK (PDK)
///- This is a refcounted object which should be released when no longer needed
///- The primary function of this is QueryService which will provide the necessary services.
DECLARE_INTERFACE_(IPdk, IServiceProvider)
{
    ///Registers an IUnknown service that can be queried by consumers of the SDK
    STDMETHOD (RegisterService)(REFGUID guidService, IUnknown* punkService) PURE;

    ///UnRegisters an IUnknown service when it is no longer available.  e.g. shutdown
    STDMETHOD (UnRegisterService)(REFGUID guidService) PURE;
};

interface __declspec(uuid("{4B31D75B-44CA-4AAA-97F4-30ACD23F14E7}")) IPdk;
extern __declspec(selectany) REFIID IID_IPdk    =  __uuidof(IPdk);

///Current version of the Prepar3d Sdk
DECLARE_INTERFACE_(IPdkV01, IPdk) 
{
};

interface __declspec(uuid("{8874D8EB-0E1A-4989-9D46-65A60D0BFE7B}")) IPdkV01;
extern __declspec(selectany) REFIID IID_IPdkV01    =  __uuidof(IPdkV01);
/** @} */ // end group
}