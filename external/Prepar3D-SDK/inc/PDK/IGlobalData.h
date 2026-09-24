// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//-----------------------------------------------------------------------------------
// IGlobalData.h
// Prepar3D SDK interface for data querying data that is global to the application
//----------------------------------------------------------------------------------

#pragma once

#include "Legacy\IGlobalData.Legacy.h"

namespace P3D
{
    /** \defgroup   globaldata  Global Data Service 
     * This service is provides certain global data that is not associated with 
     * any specific object in the application such as time and pause state.
     * A sample implementation using this interface can be found in the SimpleAirplane 
     * sample in the ISimObject Samples.
     * \{ 
    */

    /**
    * Service for requesting general data not associated with any specific object in the application.
    * \remark Various "time" queries are made relative to either Zulu (UTC) time Local time using the TIMEREF enum.
    **/
    DECLARE_INTERFACE_(IGlobalDataV610, IGlobalDataV430)
    {
        /**
        * Paused - Is application paused
        * \return   True if application is paused, false if not. 
        **/
        STDMETHOD_(BOOL, IsPaused)()    const PURE;
        /**
        * Sim Rate.  Percentage scalar on real time.  Typically ranges 0.25 - 128.0. Normal = 1.0.
        * \return   The current simulation rate.
        **/
        STDMETHOD_(float, GetSimRate)()    const PURE;
        /**
        * License Type - Get application License Type
        * \return   License type.
        **/
        STDMETHOD_(LICENSE_TYPE, GetLicenseType)()    const PURE;
        /**
        * Provided here for convenience to convert unit string name to internal code
        * \param       pszPropertyUnits      string name for unit
        * \param[out]  iUnitCode      Will contain requested value
        **/
        STDMETHOD (GetUnitCode)(__in LPCWSTR pszPropertyUnits, __out int& iUnitCode) const PURE;
        /**
        * Time since 00:00:00 year 0000
        * \param[out]  dValue      Will contain requested value
        * \param       eUnits      Units of return value
        **/
        STDMETHOD(GetAbsoluteTime)(__out double& dValue, __in int eUnits) const PURE;
        /**
        * Time of day (since midnight) Zulu or Local time ref
        * \param[out]  dValue      Will contain requested value
        * \param       eUnits      Units of return value
        **/
        STDMETHOD(GetTime)(          __out double& dValue, __in int eUnits, __in TIMEREF eTimeRef) const PURE;
        /**
        * Get year
        * \param[out]  uValue      Will contain requested value
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(GetYear)(          __out UINT& uValue, __in TIMEREF eTimeRef) const PURE;
        /**
        * Get month of year.
        * \param[out]  uValue      Will contain requested value
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(GetMonthOfTheYear)(__out UINT& uValue, __in TIMEREF eTimeRef) const PURE;
        /**
        * Get day of the year
        * \param[out]  uValue      Will contain requested value
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(GetDayOfTheYear)(  __out UINT& uValue, __in TIMEREF eTimeRef) const PURE;
        /**
        * Get day of the month
        * \param[out]  uValue      Will contain requested value
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(GetDayOfTheMonth)( __out UINT& uValue, __in TIMEREF eTimeRef) const PURE;
        /**
        * Get day of week
        * \param[out]  uValue      Will contain requested value
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(GetDayOfTheWeek)(  __out UINT& uValue, __in TIMEREF eTimeRef) const PURE;
        /**
        * Time Offset of current time zone from UTC
        * \param[out]  dValue      Will contain requested value
        * \param       eUnits      Units of return value
        **/
        STDMETHOD(GetTimeZoneOffset)(__out double& dValue, __in int eUnits) const PURE;
        /**
        * General time of day.
        * \return  time of day.
        **/
        STDMETHOD_(TIMEOFDAY, GetGeneralTimeOfDay)() const PURE;
        /**
        * Time since 00:00:00 year 0000
        * \param       dValue      The requested time
        * \param       eUnits      Units of return value
        **/
        STDMETHOD(SetAbsoluteTime)(__in double dValue, __in int eUnits) PURE;
        /**
        * Time of day (since midnight) Zulu or Local time ref
        * \param       uHour       The requested hour
        * \param       uMinute     The requested minute
        * \param       uSeconds    The requested seconds
        * \param       eTimeRef    Indicates local or zulu time
        **/
        STDMETHOD(SetTime)(__in UINT uHour, __in UINT uMinute, __in UINT uSeconds, __in TIMEREF eTimeRef) PURE;
        /**
        * Date in Zulu or Local time ref
        * \param       uYear            The requested year
        * \param       uMonth           The requested month
        * \param       uDayOfTheMonth   The requested day of the month
        * \param       eTimeRef         Indicates local or zulu time
        **/
        STDMETHOD(SetDate)(__in UINT uYear, __in UINT uMonth, __in UINT uDayOfTheMonth, __in TIMEREF eTimeRef) PURE;

        /**
        * Day night interpolant value used for transitioning between day and night lighting.
        * \return   The current day night interpolant. 0 = Night, 1 = Day
        **/
        STDMETHOD_(float, GetDayNightInterpolant)() const PURE;

        /**
        * Sun moon interpolant value used for determining the primary light source.
        * \return   The current sun moon interpolant. 0 = Sun, 1 = Moon
        **/
        STDMETHOD_(float, GetSunMoonInterpolant)() const PURE;

        /**
        * Exposure interpolant value used for transitioning between day and night exposure.
        * \return   The current exposure interpolant. 0 = Night, 1 = Day
        **/
        STDMETHOD_(float, GetExposureInterpolant)() const PURE;

        /**
        * Emissive interpolant value used for transitioning between day and night emissive intensities.
        * \return   The current emissive interpolant. 0 = Night, 1 = Day
        **/
        STDMETHOD_(float, GetEmissiveInterpolant)() const PURE;
    };

    /// Current version of GlobalData interface
    DEFINE_GUID(IID_IGlobalDataV610,    0x57b2dad9, 0xba5, 0x495f, 0xb3, 0xb8, 0xca, 0x72, 0xc5, 0x70, 0x43, 0xb8);
    /// GlobalData service ID
    DEFINE_GUID(SID_GlobalData,         0x7eb08a5d, 0x4997, 0x48be, 0x92, 0x78, 0x7e, 0x22, 0xad, 0x81, 0x4, 0x94);

    /** \} */
}
