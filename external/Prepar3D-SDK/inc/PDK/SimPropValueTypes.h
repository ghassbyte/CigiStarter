// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// SimPropValueTypes.h

#pragma once

#pragma pack(push, 4)

#define VOID void
typedef void            *PVOID;
typedef const void      *PCVOID;
typedef PVOID           *PPVOID;
typedef PCVOID           *PPCVOID;

typedef union   ANGL48
{
    struct
    {
        UINT16  pad;
        UINT16  lo;
        UINT32  hi;
    };

    UINT64      i64;

} ANGL48, *PANGL48, **PPANGL48;

typedef union   SIF48
{
    struct
    {
        UINT16  pad;
        UINT16  f;
        SINT32  i;
    };

    INT64      i64;

} SIF48, *PSIF48, **PPSIF48;

// LatLonAlt - used to store a position in 3D space
typedef struct  LATLONALT
{
    SIF48	lat;
    ANGL48	lon;
    SIF48	alt;
#ifdef __cplusplus
    bool operator== (const LATLONALT& src) const
    {
        if (lat.i64 != src.lat.i64 || lon.i64 != src.lon.i64 || alt.i64 != src.alt.i64)
        {
            return false;
        }
        return true;
    }
    bool operator!= (const LATLONALT& src) const
    {
        return !(*this == src);
    }
#endif
} LATLONALT, *PLATLONALT, **PPLATLONALT;

typedef struct  PBH32
{
    UINT32  pitch;
    UINT32  bank;
    UINT32  heading;

#ifdef __cplusplus
    bool operator== (const PBH32& src) const
    {
        if (pitch != src.pitch || bank != src.bank || heading != src.heading)
        {
            return false;
        }
        return true;
    }
    bool operator!= (const PBH32& src) const
    {
        return !(*this == src);
    }
#endif

} PBH32, *PPBH32, **PPPBH32;

#pragma pack(pop)