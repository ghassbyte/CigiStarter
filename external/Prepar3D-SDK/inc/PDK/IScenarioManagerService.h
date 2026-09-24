// Copyright (c) 2010-2020 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT

// IScenarioManagerService.h

#ifndef SCENARIOMANAGERSERVICE_H
#define SCENARIOMANAGERSERVICE_H

#include "Types.h"
#include "Legacy\ISimObject.Legacy.h"  // Include legacy ISimObject header for access to base class
#include "Helpers\ListBuilder.h"
#include "IMissionObjective.h"
#include "IGoal.h"
#include "IFlightSegment.h"

/** @defgroup scenariomanagerservice Scenario Manager Service
 *
 *  This PDK service provides the current scenario (also known as "flight") and mission files.
 *  @{
 */

namespace P3D
{

using IMissionObjectiveList = IListBuilder<IMissionObjective>;
using MissionObjectiveList = CComPtrVecBuilder<IMissionObjective>;

using IGoalList = IListBuilder<IGoal>;
using GoalList = CComPtrVecBuilder<IGoal>;

using IFlightSegmentList = IListBuilder<IFlightSegment>;
using FlightSegmentList = CComPtrVecBuilder<IFlightSegment>;

/** Provides the current scenario (also known as "flight") and mission files. */
DECLARE_INTERFACE_(IScenarioManagerV453, IScenarioManagerV430)
{
    /**
     * Provides the fully qualified path to the scenario file.
     * @param pszFilePath
     * @param uLength
     */
    STDMETHOD(GetScenarioFilePath)(__out LPWSTR pszFilePath, __in UINT uLength) const    PURE;

    /**
     * Provides the fully qualified path to the associated object file. If one does not exist a zero-length string is returned.
     * @param pszFilePath
     * @param uLength
     */
    STDMETHOD(GetObjectFilePath) (__out LPWSTR pszFilePath, __in UINT uLength)  const    PURE;

    /**
     * Provides the file name of the associated object file. If one does not exist a zero-length string is returned.
     * @param pszFileName
     * @param uLength
     */
    STDMETHOD(GetObjectFileName) (__out LPWSTR pszFileName, __in UINT uLength)  const    PURE;

    /**
     * Provides a list of scenario paths.
     * @param names
     */
    STDMETHOD(GetScenarioFilePathList)(__out INameList & names)                 const    PURE;

    /**
     * Loads the specified scenario.
     * @param pszFilePath
     */
    STDMETHOD(LoadScenario)(__in LPWSTR pszFilePath)                            const    PURE;

    /**
     * Provides the list of mission objectives for the scenario
     * @param missionObjectives
     */
    STDMETHOD(GetMissionObjectiveList) (__out IMissionObjectiveList & missionObjectives) const  PURE;

    /**
     * Provides the list of goals for the scenario
     * @param goals
     */
    STDMETHOD(GetGoalList) (__out IGoalList & goals)                            const  PURE;

    /**
     * Provides the list of flight segments.
     * @param flightSegments
     */
    STDMETHOD(GetFlightSegmentList) (__out IFlightSegmentList & flightSegments) const  PURE;

    //  Function callback registration for notification that a new mission or scenario (also known as a "flight") is loaded or saved.
    /**
     * This allows registering a callback function for notification when the current scenario has been loaded or changed. See Types.h for the function profile of PNewScenarioNotify.
     * @param pcbNotifyFunction
     */
    STDMETHOD(RegisterForNewScenarioNotification)(__in PNewScenarioNotify pcbNotifyFunction)      PURE;
};

} // end of namespace P3D

/** @} */ // end of scenariomanagerservice group

#endif