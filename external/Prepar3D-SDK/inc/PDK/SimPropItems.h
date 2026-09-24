// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// SimPropItems.h

#if !defined(_simpropitems_H_)
#define _simpropitems_H_

#include "ISimPropertySet.h"

// Macro to compute the offset of an element inside of a larger structure
#define OFFSETOF(s,m)   (BYTE*)&(((s *)0)->m)

enum SIMPROP_ITEM_VAR
{
    SIMPROP_ITEM_VAR_NONE,      // Dummy variable
    SIMPROP_ITEM_VAR_POINTER,   // variable is an absolute pointer, no further processing required
    SIMPROP_ITEM_VAR_OFFSET,    // variable is an offset from base passed into read/write functions    
};

enum SIMPROP_ITEM_TYPE
{
    SIMPROP_ITEM_TYPE_NONE,         // dummy entry, can be used to stop loading/writing some item
    SIMPROP_ITEM_TYPE_STRING,       // item is a pointer to a string buffer, varLength is the length to copy
    SIMPROP_ITEM_TYPE_STRING_ALLOC, // item is a pointer to a string.  System will allocate proper amount of memory for the string on load
    SIMPROP_ITEM_TYPE_LONG,         // item is a pointer to a LONG
    SIMPROP_ITEM_TYPE_LONG2,        // item is a pointer to unsigned long
    SIMPROP_ITEM_TYPE_BOOL,         // item is a pointer to a C++ bool
    SIMPROP_ITEM_TYPE_ENUM,         // item is a pointer to a C++ enum
    SIMPROP_ITEM_TYPE_BOOL32,       // item is of type 32bit BOOL
    SIMPROP_ITEM_TYPE_LOOKUP32,     // item is of type 32bit ENUM, uses optional pLookupTable to map.
    SIMPROP_ITEM_TYPE_A2B_FORMAT,   // item uses ascii_to_binary() to convert value (variable is assumed to point to correct output type)
    SIMPROP_ITEM_TYPE_GUID,         // item is a pointer to a guid
    SIMPROP_ITEM_TYPE_FLOAT32,      // item is a pointer to a float32
	SIMPROP_ITEM_TYPE_FLOAT64,      // item is a pointer to a float64, property type is double
    SIMPROP_ITEM_TYPE_FLOAT_TO_64,  // item is a pointer to a float64, property type is float. May lose some accuracy
    SIMPROP_ITEM_TYPE_ULONG,        // item is a pointer to unsigned long
    SIMPROP_ITEM_TYPE_ANGL32,       // item is pointer to an ANGLE32
    SIMPROP_ITEM_TYPE_LATLONALT,    // item is pointer to a LATLONALT type
    SIMPROP_ITEM_TYPE_PBH32,        // item is a pointer to a PBH type
    SIMPROP_ITEM_TYPE_XYF32,        // item is a pointer to an XYF32 data type
    SIMPROP_ITEM_TYPE_XYZF32,       // item is a pointer to an XYZF32 data type
    SIMPROP_ITEM_TYPE_XYZWF32,      // item is a pointer to an XYZWF32 data type
    SIMPROP_ITEM_TYPE_SET,          // item is a pointer to an ISimPropSetWrapper 
    SIMPROP_ITEM_TYPE_SET_VECTOR,       // item is a pointer to an ISimPropSetWrapper
    SIMPROP_ITEM_TYPE_PROPERTY_VECTOR,  // item is a pointer to an ISimPropSetWrapper
    SIMPROP_ITEM_TYPE_SHORT,        // item is a pointer to a 16-bit short
    SIMPROP_ITEM_TYPE_USHORT,       // item is a pointer to a 16-bit unsigned short
    SIMPROP_ITEM_TYPE_LONGLONG,     // item is a pointer to a 64-bit integer
    SIMPROP_ITEM_TYPE_ULONGLONG,    // item is a pointer to a 64-bit unsigned integer

};

struct SIMPROPSET_ITEM;

struct SIMPROP_ITEM_LOOKUP
{
    ENUM eProperty;     // Enum in the property
    ENUM eMap;          // Enum we're mapping to
};

struct SIMPROP_ITEM_DATA
{
    SIMPROP_ITEM_TYPE eType;               // type of formatting used to parse data
                                           // For enum, this is the length of the lookup table
    union
    {
        union
        {
            size_t cchString;              // For strings, this item represents the length of the string
            UINT32 uLength;                // For arrays, this represents the length of the array
        };
        UINT32 uFormatCode;                // used by SIMPROP_ITEM_FORMAT_A2B for the format code
    };
    union
    {
        const SIMPROP_ITEM_LOOKUP* pLookupTable;     // For enums, an optional lookup table can be passed for further processing
        const SIMPROPSET_ITEM *pPropSetItems;        // For nested sets
    };

    SIMPROP_ITEM_TYPE ePropVecType;         // Property subtype of a SIMPROP_ITEM_TYPE_PROPERTY_VECTOR
};

struct SIMPROPSET_ITEM
{
    static SIMPROPSET_ITEM CreatePropVector(SIMPROPERTYID setID, SIMPROPERTYID propID, PVOID pVar, SIMPROP_ITEM_VAR eVarType, SIMPROP_ITEM_TYPE eType, UINT32 cchSize)
    {
        SIMPROPSET_ITEM item = { setID, NULL, propID, NULL, pVar, eVarType };
        item.data.eType = SIMPROP_ITEM_TYPE_PROPERTY_VECTOR;
        item.data.cchString = cchSize;
        item.data.pPropSetItems = nullptr;
        item.data.ePropVecType = eType;
        return item;
    }

    static SIMPROPSET_ITEM CreateSet(SIMPROPERTYID setID, SIMPROPERTYID propID, PVOID pVar, SIMPROP_ITEM_VAR eVarType, const SIMPROPSET_ITEM* setItems, UINT32 itemCount)
    {
        SIMPROPSET_ITEM item = { setID, NULL, propID, NULL, pVar, eVarType };
        item.data.eType = SIMPROP_ITEM_TYPE_SET;
        item.data.uLength = itemCount;
        item.data.pPropSetItems = setItems;
        item.data.ePropVecType = SIMPROP_ITEM_TYPE_NONE;
        return item;
    }

    static SIMPROPSET_ITEM CreateSetVector(SIMPROPERTYID setID, SIMPROPERTYID propID, PVOID pVar, SIMPROP_ITEM_VAR eVarType, const SIMPROPSET_ITEM* setItems, UINT32 itemCount)
    {
        SIMPROPSET_ITEM item = { setID, NULL, propID, NULL, pVar, eVarType };
        item.data.eType = SIMPROP_ITEM_TYPE_SET_VECTOR;
        item.data.uLength = itemCount;
        item.data.pPropSetItems = setItems;
        item.data.ePropVecType = SIMPROP_ITEM_TYPE_NONE;
        return item;
    }

    SIMPROPERTYID simSetId;                 // Id of the (optional) set to recurse into
    LPCTSTR pszSetInstanceId;               // Set instance we're recusing into
    SIMPROPERTYID simPropertyId;            // Property we're reading from the set
    LPCTSTR pszInstanceId;                  // Instance we're looking for
    PVOID pVariable;                        // Variable to write
    SIMPROP_ITEM_VAR eVar;                  // type of addressing
    SIMPROP_ITEM_DATA data;                 // type specific data for parsing data
};

class ISimPropSetWrapper
{
public:
    virtual void SetChildCount(LONG count) abstract;
    virtual LONG GetChildCount() abstract;
    virtual PVOID GetChildAddress(LONG index) abstract;
   // virtual const SIMPROPSET_ITEM* GetSimPropItems() abstract;
   // virtual UINT GetSimPropItemCount() abstract;
};
#endif	//_simpropitems_H_


