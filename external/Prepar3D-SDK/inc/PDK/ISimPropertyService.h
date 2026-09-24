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
typedef GUID  _SIMID, * FAR PSIMID;
typedef const _SIMID* const REFSIMID; // standard extern 'C' expansion of REFGUID
typedef REFSIMID SIMPROPERTYID;

#include "Legacy\ISimPropertyService.Legacy.h"

namespace P3D
{
    /**  \defgroup simpropservice    Sim Property Service
    * This service provides the functions for manipulating data stored in SimProp sets 
    * \{
    **/
    
    // Flags for SimPropertySet_ReadFile and SimPropertySet_ReadStream
    #define SIMPROP_FLAG_READ_DEFAULT            0x00000000  // read either xml or binary
    #define SIMPROP_FLAG_READ_BINARY             0x00000001  // force read binary
    #define SIMPROP_FLAG_READ_XML                0x00000002  // force read xml
    #define SIMPROP_FLAG_ENFORCE_SCHEMA          0x00000004  //force schema to match

    // Flags for SimPropertySet_WriteFile and SimPropertySet_WriteStream
    #define SIMPROP_FLAG_WRITE_XMLDEFAULT        SIMPROP_FLAG_WRITE_UTF8
    #define SIMPROP_FLAG_WRITE_UNICODE           0x00000010  // write unicode (UTF-16)
    #define SIMPROP_FLAG_WRITE_NOUNICODELEADWORD 0x00000020  // do not write unicode lead word
    #define SIMPROP_FLAG_WRITE_NOBINARYLEADWORD  0x00000020  // do not write the lead word indicating that this file is binary.
    #define SIMPROP_FLAG_WRITE_NONEWLINE         0x00000040  // no newline
    #define SIMPROP_FLAG_WRITE_ALWAYS            0x00000080  // override symbolic serialization restrictions
    #define SIMPROP_FLAG_WRITE_UTF8              0x00000100  // write unicode (UTF-8). WRITE_UNICODE and WRITE_UTF8 are mutually exclusive.
    #define SIMPROP_FLAG_WRITE_BINARY            0x00000200  // use the binary writing format
    #define SIMPROP_FLAG_WRITE_ASCII             0x00000400  // write ASCII


    /**
    * Factory interface implemented by the developer for creating objects from SimProp data
    **/
    DECLARE_INTERFACE_(ISimPropertySetFactoryV440, IUnknown)
    {
        /**
         * Callback to create an object from an ISimPropertySet
         * @param   pSet     Set from which to create an object
         * @param   pObject   Pointer that will hold the new object
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT Create(ISimPropertySet* pSet, IUnknown** pObject) abstract;
        /**
         * Callback to initialize an object from an ISimPropertySet
         * @param   pSet     Set from which to initialize an object
         * @param   pObject  Object to initialize
         * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
         **/
        virtual HRESULT Init(ISimPropertySet* pSet, IUnknown* pObject) abstract;
        /**
        * Callback to serialize an object into an ISimPropertySet
        * @param   pObject  Object to be serialized
        * @param   pSet     Set of properties to serialize into
        * @return HRESULT, S_OK if function succeeds or E_FAIL if it fails
        **/
        virtual HRESULT Serialize(IUnknown* pObject, ISimPropertySet** pSet ) abstract;
    };

    /**
    * This service provides access to data stored in SimProp XML files
    **/
    DECLARE_INTERFACE_(ISimPropertyServiceV510, ISimPropertyServiceV440)
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
        /**
        * Create an ISimPropertySet instance 
        *  @param        spid   GUID id of the set to create
        *  @param        dwCreateFlags  flags that set creation options
        *  @param[out]   ppSet  pointer to the ISimPropertySet* that will store the new instance         
        **/
        virtual	HRESULT SimPropertySet_CreateInstance(
            IN SIMPROPERTYID spid,
            IN DWORD dwCreateFlags, /*SPCIF_xxx*/
            OUT ISimPropertySet * *ppSet) abstract;
        /**
        * Read from a stream and fill in a property set
        *  @param        pStream      IStream to read from
        *  @param        pidRoot      id of the property of the root
        *  @param        dwReadFlags  flags that set read options
        *  @param        rgPath       An arrray of set ids ending with a zero GUID that can be used for loading a specific set from bindary simprop files
        *  @param[out]   ppSet        pointer to the ISimPropertySet* that will store the new instance         \
        **/
        virtual	HRESULT SimPropertySet_ReadStream(
            IN IStream * pStream,
            IN OPTIONAL SIMPROPERTYID pidRoot,
            IN OPTIONAL ULONG dwReadFlags,
            IN OPTIONAL SIMPROPERTYID * rgPath,
            OUT ISimPropertySet** ppSet) abstract;

        /**
        * Read from a file and fill in a property set
        *  @param        pszFile      file path
        *  @param        pidRoot      id of the property of the root
        *  @param        dwReadFlags  flags that set read options
        *  @param        rgPath       An arrray of set ids ending with a zero GUID that can be used for loading a specific set from bindary simprop files
        *  @param[out]   ppSet        pointer to the ISimPropertySet* that will store the new instance         
        **/
        virtual	HRESULT SimPropertySet_ReadFile(
            IN LPCWSTR pszFile,
            IN OPTIONAL SIMPROPERTYID pidRoot,
            IN OPTIONAL ULONG dwReadFlags,
            IN OPTIONAL SIMPROPERTYID * rgPath,
            OUT ISimPropertySet** ppSet) abstract;

        /**
        * Write a property set out to a stream
        *  @param        pSet          pointer to set that will be written
        *  @param        pStream       IStream to write to
        *  @param        dwWriteFlags  flags that set write options
        *  @param[out]   pcbWritten    stores the number of bytes written to stream        
        **/
        virtual	HRESULT SimPropertySet_WriteStream(
            IN ISimPropertySet * pSet,
            IN ISequentialStream * pStream,
            IN ULONG dwWriteFlags,
            IN OUT OPTIONAL ULONG * pcbWritten) abstract;

        /**
        * Write a property set out to a stream
        *  @param        pSet          pointer to set that will be written
        *  @param        pszFileName   file name of file to write
        *  @param        dwWriteFlags  flags that set write options
        *  @param[out]   pcbWritten    stores the number of bytes written to the file        
        **/
        virtual	HRESULT SimPropertySet_WriteFile(
            IN ISimPropertySet * pSet,
            IN LPCWSTR pszFileName,
            IN ULONG dwWriteFlags,
            IN OUT OPTIONAL ULONG * pcbWritten) abstract;
    };


    ///---------------------------------------------------------------------------
    /// Predefined UUIDs

    interface __declspec(uuid("{971350B0-7E5B-4632-82C5-8DB1DED17A08}")) ISimPropertyServiceV510;
    extern __declspec(selectany) REFIID  IID_ISimPropertyServiceV510 = __uuidof(ISimPropertyServiceV510);
    
    interface __declspec(uuid("{8587CE3A-8B66-494B-9BBC-2DAF8738B45C}")) ISimPropertySetFactoryV440;
    extern __declspec(selectany) REFIID  IID_ISimPropertySetFactoryV440 = __uuidof(ISimPropertySetFactoryV440);

    extern __declspec(selectany) REFIID  SID_SimPropertyService = IID_ISimPropertyServiceV440;

    /** \}*/ // end of doc group
};
