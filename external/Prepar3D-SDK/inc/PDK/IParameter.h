#pragma once

#include "IRange.h"

namespace P3D
{

DECLARE_INTERFACE(IParameterV453)
{
    virtual ~IParameterV453()
    {
    }

    STDMETHOD_(int, GetRangeCount)() PURE;
    STDMETHOD_(IRange*, GetRange)(int index) PURE;
    STDMETHOD_(LPCTSTR, GetName)() PURE;
    STDMETHOD_(LPCTSTR, GetUnits)() PURE;
    STDMETHOD_(void, SetUnits)(LPCTSTR pcszUnitsString) PURE;
};

///---------------------------------------------------------------------------
/// Predefined UUIDs

interface __declspec(uuid("{3C8984CA-398E-4C5D-BADA-78D4023FD014}")) IParameterV453;
extern __declspec(selectany) REFIID IID_IParameterV453 = __uuidof(IParameterV453);

//--------------------------------------------------------------------------------------------------
// These typedefs and definitions exist to ease development using these interfaces.  
// Update to keep in sync with the latest version.

extern __declspec(selectany) REFIID  IID_IParameter = IID_IParameterV453;

typedef IParameterV453           IParameter;
}