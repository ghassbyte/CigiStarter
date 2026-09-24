// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// GaugeTypes.h

typedef int     ID, *PID, **PPID;
typedef int     ENUM, *PENUM, **PPENUM;
typedef int     FLAGS, *PFLAGS, **PPFLAGS;
typedef unsigned    int     VAR32, *PVAR32, **PPVAR32;
typedef signed      int     SINT32, *PSINT32, **PPSINT32;
typedef double  FLOAT64, *PFLOAT64, **PPFLOAT64;

typedef UINT32 GENERATE_PHASE;
typedef VAR32   ERR, *PERR, **PPERR;
typedef VAR32   ID32, *PID32, **PPID32;
typedef VAR32   FLAGS32, *PFLAGS32, **PPFLAGS32;
typedef SINT32  PIXEL, *PPIXEL, **PPPIXEL;      // screen pixel

enum    VAR_TYPE;

typedef struct  GAUGEHDR *PGAUGEHDR, **PPGAUGEHDR;
typedef struct  ELEMENT_HEADER *PELEMENT_HEADER, **PPELEMENT_HEADER;
typedef struct  PIXPOINT        PIXPOINT, *PPIXPOINT, **PPPIXPOINT;
typedef struct  MOUSERECT       MOUSERECT, *PMOUSERECT, **PPMOUSERECT;
typedef struct  MODULE_VAR      MODULE_VAR, *PMODULE_VAR, **PPMODULE_VAR;

typedef union   UNIVERSAL_VAR   UNIVERSAL_VAR, *PUNIVERSAL_VAR, **PPUNIVERSAL_VAR;

typedef void(*GAUGE_KEY_EVENT_HANDLER) (ID32 event, UINT32 evdata, PVOID userdata);

class IPanelCCallback;
class IAircraftCCallback;