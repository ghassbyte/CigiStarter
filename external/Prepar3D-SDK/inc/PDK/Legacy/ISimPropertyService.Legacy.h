// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------
// ISimPropertyService.h
// Prepar3D SDK interface for configuration sets.
//----------------------------------------------------------------------------

#pragma once

#include "Types.h"
#include "Helpers\ListBuilder.h"
#include <ObjBase.h>
#include <atlcomcli.h>
#include "IEventService.h"

__interface ISimPropertySet;
struct SIMPROPSET_ITEM;
typedef void *PVOID;
typedef const void *PCVOID;

namespace P3D
{
    __interface ISimPropertySetFactoryV440;

    /**
    * This service provides access to data stored in SimProp XML files
    **/
    DECLARE_INTERFACE_(ISimPropertyServiceV440, IUnknown)
    {
        /**
        * Register a factory that can create objects from an ISimPropertySet
        * @param   setGUID    The GUID of the property set 
        * @param   pFactory   The factory to register
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT RegisterFactory(const GUID& setGUID, ISimPropertySetFactoryV440* pFactory) abstract;
        /**
        * Unegister a factory that can create objects from an ISimPropertySet
        * @param   setGUID    The GUID of the property set
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT UnRegisterFactory(const GUID& setGUID) abstract;
        /**
        * Read an item out of a ISimPropertySet into a specific memory location.  
        * This allows property values to be set directly to members of a 
        * class or structure.
        * @param   pSimPropSet  The property set to read from
        * @param   pBaseAddress  Base memory address to write data 
        * @param   pPropSetItem  The property set item description 
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual	HRESULT SimPropSetItemRead(IN ISimPropertySet *pSimPropSet, IN OUT PVOID pBaseAddress, IN const SIMPROPSET_ITEM *pPropSetItem) abstract;
        /**
        * Read multiple items out of a ISimPropertySet  into a specific memory location.  
        * This allows property values to be set directly to members of a
        * class or structure.
        * @param   pSimPropSet  The property set to read from
        * @param   pBaseAddress  Base memory address to write data
        * @param   aPropSetItems  Array of property set item descriptions
        * @param   uCountItems  Count of property set item descriptions
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual	HRESULT SimPropSetItemsRead(IN ISimPropertySet *pSimPropSet, IN OUT PVOID pBaseAddress, IN const SIMPROPSET_ITEM aPropSetItems[], IN UINT uCountItems) abstract;
        /**
        * Write an item from a specific memory location into an ISimPropertySet.
        * This allows property values to be set directly from members of a
        * class or structure.
        * @param   pSimPropSet  The property set to write to
        * @param   pBaseAddress  Base memory address to read from
        * @param   pPropSetItem  The property set item description
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual	HRESULT SimPropSetItemWrite(IN OUT ISimPropertySet *pSimPropSet, IN PCVOID pBaseAddress, IN const SIMPROPSET_ITEM *pPropSetItem) abstract;
        /**
        * Write multiple items from a specific memory location into an ISimPropertySet.
        * This allows property values to be set directly from members of a
        * class or structure.
        * @param   pSimPropSet  The property set to write to
        * @param   pBaseAddress  Base memory address to read from
        * @param   aPropSetItems  Array of property set item descriptions
        * @param   uCountItems  Count of property set item descriptions
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual	HRESULT SimPropSetItemsWrite(IN OUT ISimPropertySet *pSimPropSet, IN PCVOID pBaseAddress, IN const SIMPROPSET_ITEM pPropSetItems[], IN UINT uCountItems) abstract;
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs

    interface __declspec(uuid("{A3E7F1F1-2127-4544-A1FE-6054E41B9228}")) ISimPropertyServiceV440;
    extern __declspec(selectany) REFIID  IID_ISimPropertyServiceV440 = __uuidof(ISimPropertyServiceV440);
};
