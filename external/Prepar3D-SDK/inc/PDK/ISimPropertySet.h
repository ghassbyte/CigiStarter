// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// ISimPropertySet.h

#pragma once

typedef GUID  _SIMID, * FAR PSIMID;
typedef const _SIMID* const REFSIMID; // standard extern 'C' expansion of REFGUID

#ifdef INITGUID
#define DEFINE_SIMID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        DEFINE_GUID(_##name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8); \
        EXTERN_C REFSIMID name = &_##name
#else
#define DEFINE_SIMID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        DEFINE_GUID(_##name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8); \
        EXTERN_C REFSIMID name
#endif

#define VALID_SIMID(rsid)                (((rsid) != NULL) && !IsEqualSIMID(rsid, &GUID_NULL))
#define InlineIsEqualSIMID(rsimid1, rsimid2)  \
        (((unsigned long *) rsimid1)[0] == ((unsigned long *) rsimid2)[0] &&   \
        ((unsigned long *) rsimid1)[1] == ((unsigned long *) rsimid2)[1] &&    \
        ((unsigned long *) rsimid1)[2] == ((unsigned long *) rsimid2)[2] &&    \
        ((unsigned long *) rsimid1)[3] == ((unsigned long *) rsimid2)[3])
#define IsEqualSIMID(rsid1, rsid2)       ((rsid1 && rsid2) ? (0 == memcmp(rsid1, rsid2, sizeof(_SIMID))) : FALSE)
#define ASSIGN_SIMID(pidDst, rsidSrc)    ((pidDst) = (PSIMID)(rsidSrc))


//
//  SIMPROPERTYID
//
#ifndef __SIMPROPERTYID_DEFINED__
#define __SIMPROPERTYID_DEFINED__

typedef _SIMID                      _SIMPROPERTYID, * FAR PSIMPROPERTYID;
typedef REFSIMID                    SIMPROPERTYID;
#define DEFINE_SIMPROPERTYID        DEFINE_SIMID
#define VALID_SIMPROPERTYID         VALID_SIMID
#define InlineIsEqualSIMPROPERTYID  InlineIsEqualSIMID
#define IsEqualSIMPROPERTYID        IsEqualSIMID
#define ASSIGN_SIMPROPERTYID        ASSIGN_SIMID

#endif __SIMPROPERTYID_DEFINED__

//
//  SIMPROPERTYTYPE
//
#ifndef __SIMPROPERTYTYPE_DEFINED__
#define __SIMPROPERTYTYPE_DEFINED__

typedef _SIMID                        _SIMPROPERTYTYPE, * FAR PSIMPROPERTYTYPE;
typedef REFSIMID                      SIMPROPERTYTYPE;
#define DEFINE_SIMPROPERTYTYPE        DEFINE_SIMID
#define VALID_SIMPROPERTYTYPE         VALID_SIMID
#define InlineIsEqualSIMPROPERTYTYPE  InlineIsEqualSIMID
#define IsEqualSIMPROPERTYTYPE        IsEqualSIMID
#define ASSIGN_SIMPROPERTYTYPE        ASSIGN_SIMID

#endif __SIMPROPERTYTYPE_DEFINED__

//
//  Additional type definitions
//
typedef int ENUM;

//
//  forwards
//
__interface ISimPropertySet;
__interface ISimPropertyType;
typedef struct _TREEITEM *HTREEITEM;
typedef struct _ACEEDITPROPERTIES ACEEDITPROPERTIES;

//
//  Base type definitions
//
//  STID_BYTE4 - Byte quadruplet
typedef union
{
    BYTE rgb[4];
    DWORD dw;
} BYTE4;

#define COLORREF_FROM_BYTE4(b4)		RGB((b4).rgb[0], (b4).rgb[1], (b4).rgb[2]);

//  STID_LONG2 - platform-native signed long doublet
typedef union LONG2
{
    LONG rgl[2];
    POINT pt;
    SIZE size;
    LONGLONG ll;
    FILETIME ft;

#ifdef __cplusplus
    bool operator== (const LONG2& src) const
    {
        if (rgl[0] != src.rgl[0] || rgl[1] != src.rgl[1])
        {
            return false;
        }
        return true;
    }
    bool operator!= (const LONG2& src) const
    {
        return !(*this == src);
    }
#endif

} LONG2;

//  STID_LONG3 - platform-native signed long triplet
typedef union LONG3
{
    LONG rgl[3];

#ifdef __cplusplus
    bool operator== (const LONG3& src) const
    {
        if (rgl[0] != src.rgl[0] || rgl[1] != src.rgl[1] || rgl[2] != src.rgl[2])
        {
            return false;
        }
        return true;
    }
    bool operator!= (const LONG3& src) const
    {
        return !(*this == src);
    }
#endif

} LONG3;

//  STID_LONG4 - platform-native signed long quad
typedef union LONG4
{
    LONG rgl[4];
    RECT rc;
    GUID guid;
    LONGLONG rgll[2];
    PBH32 pbh;

#ifdef __cplusplus
    bool operator== (const LONG4& src) const
    {
        if (rgl[0] != src.rgl[0] || rgl[1] != src.rgl[1] || rgl[2] != src.rgl[2] || rgl[3] != src.rgl[3])
        {
            return false;
        }
        return true;
    }
    bool operator!= (const LONG4& src) const
    {
        return !(*this == src);
    }
#endif
} LONG4;

typedef struct tagFLOATPOINT
{
    float x;
    float y;
} FLOATPOINT, *PFLOATPOINT, NEAR *NPFLOATPOINT, FAR *LPFLOATPOINT;

typedef struct tagFLOATSIZE
{
    float  cx;
    float  cy;
} FLOATSIZE, *PFLOATSIZE, NEAR *NPFLOATSIZE, FAR *LPFLOATSIZE;

//  SPT_FLOAT2   (e.g. <position>"45.233,32.50"</position>)
typedef union
{
    float         rgf[2];
    FLOATPOINT    pt;
    FLOATSIZE     size;
} FLOAT2;

typedef union
{
    float         rgf[3];
} FLOAT3;

typedef union
{
    float         rgf[4];
} FLOAT4;

//-----------------------------------------------------------------------------
//
//  property value representation
//
#pragma pack( push, 8 )
typedef union _SIMPROPERTYVALUE
{
    BYTE        byteVal;
    WORD        wVal;
    DWORD       dwVal;
    BYTE4       b4Val;
    LONG2       l2Val;
    LONGLONG    llVal;
    LONG3       l3Val;
    LONG4       l4Val;
    GUID        guidVal;
    VOID*       pVal;
    float       fVal;
    FLOAT2      f2Val;
    FLOAT3      f3Val;
    FLOAT4      f4Val;
    double      dVal;
    LATLONALT   llaVal;

} SIMPROPERTYVALUE;

//-----------------------------------------------------------------------------
//  SIMPROPERTY (header + instance ID + value)
typedef struct _SIMPROPERTY
{
    HANDLE           hSymbol;       // symbol handle
    LPCTSTR          pszInstanceID; // instance ID
    SIMPROPERTYVALUE value;
} SIMPROPERTY;

#pragma pack( pop )




//-----------------------------------------------------------------------------
//  Property and Property Set API
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//  SimProperty flags
#define SPF_XMLELEMENT        0x00000000
#define SPF_XMLATTRIBUTE      0x00000001
#define SPF_SERIALIZEALWAYS   0x00000002

//  SimPropertySet policy flags
#define SPSP_UNRESTRICTED     0x00000000
#define SPSP_EXCLUSIVE        0x00000001
#define SPSP_INCLUSIVE        0x00000002

// SimPropertySet_Get flags
#define	SPGF_NOFLAGS			0x00000000
#define	SPGF_DEFAULTIFNOTEXIST	0x00000001

//SimPropertySet End Tag flags
#define SPSET_DEFAULTENDTAG     0x00000000
#define SPSET_EMPTYENDTAG       0x00000001

//SimPropertySet create flags
#define SPCIF_LOADFROMSTREAM       0x0000001       //  informs the set that the property set or property is being loaded
                                                   //       from a stream rather than created programatically
                                                   //       (opposite of SPCIF_CREATENEW)
#define SPCIF_CREATENEW            0x0000002       //  directs the set to create a new, empty property or property set
#define SPCIF_OVERWRITEEXISTING    0x0000004       //  directs the set to overwrite any existing property or property set
#define SPCIF_CREATEDEFAULT        (SPCIF_CREATENEW|SPCIF_OVERWRITEEXISTING)
#define SPCIF_CREATEOPMASK         0x000000F       //  instance creation operation mask


//
//  property set insertion ops:
#define SPCIF_INSERTFIRST          0x0000010       //  directs the set to insert the property or property set at the head of the collection
#define SPCIF_INSERTLAST           0x0000020       //  directs the set to append the property or property set to the tail of the collection
#define SPCIF_INSERTBEFORE         0x0000030       //  directs the set to insert the property or property set before the specified sibling
#define SPCIF_INSERTAFTER          0x0000040       //  directs the set to insert the property or property set after the specifiec sibling
#define SPCIF_INSERTOPMASK         0x00000F0       //  insertion operation mask.

//-----------------------------------------------------------------------------
//  add'l forwards
typedef struct _SIMPROPERTYSETCREATESTRUCT SIMPROPERTYSETCREATESTRUCT;

//-----------------------------------------------------------------------------
//  __interface ISimPropertySet - SimProp property collection object interface
// {DFD821E6-BB11-41ba-927C-E0E81C000922}
// DEFINE_GUID(IID_ISimPropertySet,
// 0xdfd821e6, 0xbb11, 0x41ba, 0x92, 0x7c, 0xe0, 0xe8, 0x1c, 0x0, 0x9, 0x22);

__interface ISimPropertySet
    :   public IUnknown
//-----------------------------------------------------------------------------
{
    //  IUnknown methods
    virtual STDMETHODIMP            QueryInterface(
                                        IN REFIID riid,
                                        OUT LPVOID* pvObj ) PURE; // not implemented
    virtual STDMETHODIMP_(ULONG)    AddRef() PURE;
    virtual STDMETHODIMP_(ULONG)    Release() PURE;

    //  property set and instance identifiers
    virtual STDMETHODIMP_(REFSIMID)
                                    GetID() PURE;

    virtual STDMETHODIMP_(REFSIMID)
                                    GetNamespaceID() PURE;

    virtual STDMETHODIMP_(LPCTSTR)
                                    GetInstanceID() PURE;

    virtual STDMETHODIMP_(REFSIMID)
                                    GetClassID() PURE;

    virtual STDMETHODIMP
                                    SetInstanceID(LPCTSTR pszInstanceID) PURE;

    //  attribute lookup
    virtual STDMETHODIMP_(LPCTSTR)  GetAttribute(IN LPCTSTR pszAttribute) PURE;

    //  lifetime
    virtual STDMETHODIMP            Destroy() PURE; // invoked by parent object;
                                                    // the object should release outstanding references and
	                                                // destroy internal data, but not destruct.

    //  parenting
    virtual STDMETHODIMP            SetParent( IN ISimPropertySet* pParent ) PURE;

    virtual STDMETHODIMP            GetParent( OUT ISimPropertySet** ppParent ) PURE;

    virtual STDMETHODIMP            Initialize(
                                        SIMPROPERTYID pidObject,
                                        DWORD dwCreateFlags) /*SPCIF_xxx*/ PURE;

    //  child set I/O
    virtual STDMETHODIMP            CreateChild(
                                        IN DWORD dwFlags, /*SPCIF_xxx*/
                                        IN SIMPROPERTYID spidChild,
                                        IN OPTIONAL LPCTSTR pszInstanceID,
                                        IN OPTIONAL SIMPROPERTYSETCREATESTRUCT* pocs,
                                        IN OPTIONAL ISimPropertySet* pInsertAt,
                                        OUT ISimPropertySet** ppChild ) PURE;

    virtual STDMETHODIMP            InsertChild(
                                        IN DWORD dwFlags, /*SPCIF_xxx*/
                                        IN ISimPropertySet* pChild,
                                        IN OPTIONAL ISimPropertySet* pInsertAt ) PURE;

	virtual STDMETHODIMP        	MoveUpProperty(IN SIMPROPERTY* pProperty) PURE;

    virtual STDMETHODIMP        	MoveDownProperty(IN SIMPROPERTY* pProperty) PURE;

    virtual STDMETHODIMP            MoveChildren(IN ISimPropertySet* pSimPropertySet) PURE;

	virtual STDMETHODIMP			MoveUpChild(IN ISimPropertySet* pSimPropertySet) PURE;
	
	virtual STDMETHODIMP			MoveDownChild(IN ISimPropertySet* pSimPropertySet) PURE;		

	
    virtual STDMETHODIMP_(LONG)     GetChildCount(
                                        IN OPTIONAL SIMPROPERTYID pidChild /*SSID_UNDEFINED counts all*/) PURE;

    virtual STDMETHODIMP            GetChild(
                                        IN OPTIONAL SIMPROPERTYID pidChild /*SSID_UNDEFINED retrieves i'th child set of any type*/,
                                        IN OPTIONAL LONG iChild,
                                        OUT ISimPropertySet** ppChild ) PURE;

    virtual STDMETHODIMP            GetChildInstance(
                                        IN SIMPROPERTYID pidChild,
                                        IN OPTIONAL LPCTSTR pszInstanceID,
                                        OUT ISimPropertySet** ppChild ) PURE;

    virtual STDMETHODIMP            DestroyChild(
                                        IN ISimPropertySet* pObject ) PURE;

    virtual STDMETHODIMP            DestroyChildren(
                                        IN SIMPROPERTYID pidChildren /*SSID_UNDEFINED destroys all*/) PURE;

    //  property I/O
    virtual STDMETHODIMP            CreateProperty(
                                        IN DWORD dwFlags, /*SPCIF_xxx*/
                                        IN SIMPROPERTYID spid,
                                        IN OPTIONAL LPCTSTR pszInstanceID,
                                        IN OPTIONAL const SIMPROPERTY* pInsertAt,
                                        OUT OPTIONAL SIMPROPERTY** ppProp ) PURE;

    virtual STDMETHODIMP            InsertProperty(
                                        IN DWORD dwFlags, /*SPCIF_xxx*/
                                        IN const SIMPROPERTY* pProperty,
                                        IN OPTIONAL const SIMPROPERTY* pInsertAt ) PURE;

    virtual STDMETHODIMP_(LONG)     GetPropertyCount(
                                        IN OPTIONAL SIMPROPERTYID pidProperties /*SPID_UNDEFINED counts all*/) PURE;

    virtual STDMETHODIMP            GetProperty(
                                        IN OPTIONAL SIMPROPERTYID pidProperty /*SPID_UNDEFINED retrieves i'th property of any type*/,
                                        IN OPTIONAL LONG iProperty, /* -1 == retrieve the first matching property */
                                        OUT OPTIONAL SIMPROPERTY** ppProp ) PURE;

    virtual STDMETHODIMP            GetPropertyInstance(
                                        IN SIMPROPERTYID pidProperty,
                                        IN OPTIONAL LPCTSTR pszInstanceID,
                                        OUT SIMPROPERTY** ppProp ) PURE;

    virtual STDMETHODIMP            SetPropertyXmlValue(
                                        IN SIMPROPERTYID spid,
                                        IN OPTIONAL LPCTSTR pszInstanceID,
                                        IN LPCTSTR pszXmlValue ) PURE;

    virtual STDMETHODIMP            DestroyProperties(
                                        IN SIMPROPERTY** prgProperties,
                                        IN LONG cProperties ) PURE;

    virtual STDMETHODIMP            DestroyPropertiesById(
                                        IN SIMPROPERTYID pid ) PURE;

    //  serialization
    virtual STDMETHODIMP            Write(
                                        ISequentialStream* pStream,
                                        IN ULONG cIndent,
                                        IN ULONG dwWriteFlags,
                                        OUT ULONG* pcbWritten ) PURE;

	virtual STDMETHODIMP_(BOOL)     IsDirty() PURE;
	virtual STDMETHODIMP_(VOID)     SetDirty(BOOL d = TRUE) PURE;

};

interface __declspec(uuid("{DFD821E6-BB11-41ba-927C-E0E81C000922}")) ISimPropertySet;
extern __declspec(selectany) REFIID  IID_ISimPropertySet = __uuidof(ISimPropertySet);