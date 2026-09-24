#pragma once

//
// IGoal interface
//
namespace P3D
{
enum GoalState
{
    GOAL_PENDING,
    GOAL_COMPLETED,
    GOAL_FAILED,
    
    GoalState_Count
};

enum GoalResolution
{
    RESOLVE_COMPLETED,
    RESOLVE_FAILED,
};

DECLARE_INTERFACE_(IGoalV453, IUnknown)
{
    STDMETHOD(Resolve)(GoalResolution resolution)PURE;
    STDMETHOD_(GoalState, GetState)() PURE;
    STDMETHOD_(ULONG, GetOrder)() PURE;
    STDMETHOD_(LPCTSTR, GetText)() PURE;
    STDMETHOD_(BOOL, IsOptional)() PURE;
    STDMETHOD_(ULONG, GetPointValue)() PURE;
    STDMETHOD_(ULONG, GetChildGoalCount)() PURE;
    STDMETHOD_(IGoalV453*, GetChildGoalByIndex)(int index) PURE;
    STDMETHOD_(LPCTSTR, GetFailedText)() PURE;
    STDMETHOD_(LPCTSTR, GetSucceededText)() PURE;
};

///---------------------------------------------------------------------------
/// Predefined UUIDs

interface __declspec(uuid("{66EF5971-42F8-4000-AB70-6C416F6C9D95}")) IGoalV453;
extern __declspec(selectany) REFIID IID_IGoalV453 = __uuidof(IGoalV453);

//--------------------------------------------------------------------------------------------------
// These typedefs and definitions exist to ease development using these interfaces.  
// Update to keep in sync with the latest version.

extern __declspec(selectany) REFIID  IID_IGoal = IID_IGoalV453;

typedef IGoalV453           IGoal;

}
