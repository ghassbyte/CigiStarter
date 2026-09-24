#pragma once

#include "RenderingTypes.h"

namespace P3D
{

DECLARE_INTERFACE(IRangeV453)
{
    STDMETHOD(IncreaseExceededCount)() PURE;
    STDMETHOD_(int, GetExceededCount)() PURE;
    STDMETHOD_(float, GetMaxOverMeasured)() PURE;
    STDMETHOD_(float, GetMinUnderMeasured)() PURE;
    STDMETHOD_(LPCTSTR, GetName)() PURE;
    STDMETHOD_(ARGBColor, GetColor)() PURE;
};
///---------------------------------------------------------------------------
/// Predefined UUIDs

interface __declspec(uuid("{946D681B-101B-4DA4-A921-086A3F3C70EA}")) IRangeV453;
extern __declspec(selectany) REFIID IID_IRangeV453 = __uuidof(IRangeV453);

//--------------------------------------------------------------------------------------------------
// These typedefs and definitions exist to ease development using these interfaces.  
// Update to keep in sync with the latest version.

extern __declspec(selectany) REFIID  IID_IRange = IID_IRangeV453;

typedef IRangeV453           IRange;
}
