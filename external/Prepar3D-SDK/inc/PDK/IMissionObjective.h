#pragma once

#include "IGoal.h"

namespace P3D
{

    enum MissionObjectiveStatus
    {
        OBJECTIVE_PENDING,
        OBJECTIVE_PASSED,
        OBJECTIVE_FAILED,

        MissionObjectiveStatus_Count
    };

    DECLARE_INTERFACE_(IMissionObjectiveV453, IUnknown)
    {
        STDMETHOD_(MissionObjectiveStatus, GetStatus)() = 0;
        STDMETHOD_(LPCTSTR, GetText)() = 0;
        STDMETHOD_(BOOL, GetObjectivePassed)() = 0;
        STDMETHOD_(ULONG, GetObjectivePassValue)() = 0;
        STDMETHOD_(void, SetObjectivePassValue)(ULONG) = 0;
        STDMETHOD_(void, EvaluateObjective)() = 0;
        STDMETHOD_(ULONG, GetCurrentScore)() = 0;
        STDMETHOD_(ULONG, GetTotalPointsPossible)() = 0;
        STDMETHOD_(ULONG, GetObjectivePointValue)() = 0;
        STDMETHOD_(ULONG, GetOrder)() = 0;
        STDMETHOD_(BOOL, IsOptional)() = 0;
        STDMETHOD_(ULONG, GetChildGoalCount)() = 0;
        STDMETHOD_(IGoal*, GetChildGoalByIndex)(int index) = 0;
    };

    ///---------------------------------------------------------------------------
    /// Predefined UUIDs
    
    interface __declspec(uuid("{533B1F0C-6BC9-4FAF-A0F4-529878BDEA15}")) IMissionObjectiveV453;
    extern __declspec(selectany) REFIID IID_IMissionObjectiveV453 = __uuidof(IMissionObjectiveV453);

    //--------------------------------------------------------------------------------------------------
    // These typedefs and definitions exist to ease development using these interfaces.  
    // Update to keep in sync with the latest version.

    extern __declspec(selectany) REFIID  IID_IMissionObjective = IID_IMissionObjectiveV453;

    typedef IMissionObjectiveV453           IMissionObjective;

}