#pragma once

#include "IParameter.h"

namespace P3D
{

DECLARE_INTERFACE_(IFlightSegmentV453, IUnknown)
{
    STDMETHOD_(GUID, GetGoalInstanceID)() PURE;
    STDMETHOD_(int, GetParameterCount)() PURE;
    STDMETHOD_(IParameter*, GetParameter)(int index) PURE;
    STDMETHOD_(bool, IsDemonstrating)() PURE;
};

///---------------------------------------------------------------------------
/// Predefined UUIDs

interface __declspec(uuid("{49656727-5F62-4CC5-AFE1-A5CF3A74F7D6}")) IFlightSegmentV453;
extern __declspec(selectany) REFIID IID_IFlightSegmentV453 = __uuidof(IFlightSegmentV453);

//--------------------------------------------------------------------------------------------------
// These typedefs and definitions exist to ease development using these interfaces.  
// Update to keep in sync with the latest version.

extern __declspec(selectany) REFIID  IID_IFlightSegment = IID_IFlightSegmentV453;

typedef IFlightSegmentV453           IFlightSegment;
}
