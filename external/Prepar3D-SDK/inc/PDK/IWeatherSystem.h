// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// IWeatherSystem.h
// Description: Interface to Prepar3D Weather System
//----------------------------------------------------------------------------

#pragma once

#include <atlcomcli.h>


#include "Types.h"
#include "Legacy\IWeatherSystem.Legacy.h"   //Include previous version

namespace P3D
{
    /** \defgroup weatherservice   Weather System Service
     *
     * This PDK service allows callers to manipulate the weather system during runtime.
     * The interface gives the developer control over current weather settings as well as
     * current weather station data.
     *
     * \{
     */


    //
    //IWeatherStationV430
    //
    DECLARE_INTERFACE_(IWeatherStationV430, IUnknown)
    {
    public:

        //Returns true if this weather station is still present and valid in the weather system, false otherwise
        STDMETHOD_(bool, IsValid)() const abstract;

        //Returns the surface wind layer speed of the tower -- knots
        STDMETHOD_(float, GetSurfaceWind)() const abstract;
        //Sets the surface wind speed of the tower -- knots
        STDMETHOD(SetSurfaceWind)(float aVal) const abstract;
        
        //Returns the barometer pressure of the tower -- millibars - at sea level
        STDMETHOD_(float, GetBaroPressure)() const abstract;
        //Sets the barometer pressure of the tower -- millibars - at sea level
        STDMETHOD(SetBaroPressure)(float aVal) const abstract;
       
        //Returns the number of cloud layers at a tower
        STDMETHOD_(int, GetCloudLayerCount)() const abstract; 
        //Returns the cloud layer at a given index of the tower
        STDMETHOD(GetCloudAtIndex)(__in UINT uIndex, P3D::CloudLayer& aLayer) const abstract;
        //Adds a new cloud layer to the tower, given a CloudLayer
        STDMETHOD(AddNewCloudToTower)(__in CloudLayer& aLayer) const abstract;
        //Removes a Cloud layer from the tower given an index 
        STDMETHOD(RemoveCloudLayerFromTower)(int uIndex) const abstract;
        //Clears all of the clouds of the given tower
        STDMETHOD(ClearAllCloudsAtTower)() const abstract;
        
        //Returns the number of temp layers at a tower
        STDMETHOD_(UINT, GetTempLayerCount)() const abstract;
        //Returns the temp layer at a given index of the tower
        STDMETHOD(GetTempLayerAtIndex)(__in UINT uIndex, __out P3D::TempLayer& aLayer) const abstract;
        //Adds a new temperature layer to the tower, given a CloudLayer
        STDMETHOD(AddNewTempToTower)(__in TempLayer& aLayer) const abstract;
        //Removes a temperature layer from the tower given an index
        STDMETHOD(RemoveTempLayerFromTower)(int uIndex) const abstract;
        //Clears all of the temperature layers of the given tower
        STDMETHOD(ClearAllTempLayersFromTower)() const abstract;

        //Returns the number of aLoft Layers at a tower
        STDMETHOD_(UINT, GetAloftLayerCount)() const abstract;
        //Returns the aLoft wind layer at a given index of the tower
        STDMETHOD(GetAloftLayerAtIndex)(__in UINT uIndex, __out P3D::WindAloftLayer& aLayer) const abstract;
        //Adds a new aloft wind layer to the tower, given a WindAloftLayer
        STDMETHOD(AddNewAloftToTower)(__in WindAloftLayer& aLayer) const abstract;
        //Removes an aloft wind layer from the tower given an index 
        STDMETHOD(RemoveAloftLayerFromTower)(int uIndex) const abstract;
        //Clears all of the wind  of the given tower
        STDMETHOD(ClearAllWindLayersFromTower)() const abstract;

        //Returns the number of visibility Layers at a tower 
        STDMETHOD_(UINT, GetVisibilityLayerCount)() const abstract;
        //Returns the visibility layer at a given index of the tower
        STDMETHOD(GetVisibilityLayerAtIndex)(__in UINT uIndex, __out P3D::VisibilityLayer& aLayer) const abstract;
        //Adds a new visibility layer to the tower, given a VisibilityLayer
        STDMETHOD(AddNewVisToTower)(__in VisibilityLayer& aLayer) const abstract;
        //Removes a visibility layer from the tower given an index 
        STDMETHOD(RemoveVisLayerFromTower)(int uIndex) const abstract;
        //Clears all of the visibility layers of the given tower
        STDMETHOD(ClearAllVisLayersFromTower)() const abstract;

        
        
        
    };

    

     /**
      * This is the interface to the core Prepar3D weather system. An example on how to query
      * for the service can be found in the DLLStart() function of the Camera PDK Sample.
      * \remark   This interface uses several enumerated weather types which are defined in WeatherSystemTypes.h.
      * Several functions will also utilize the METAR Data Format.
      */
    DECLARE_INTERFACE_(IWeatherSystemV500, IWeatherSystemV430)
    {
    public:
        /// Get the current enhanced atmospherics setting. DEPRECATED
        STDMETHOD_(bool, GetEnhancedAtmospherics)() const abstract;

        /// Set the current enhanced atmospherics setting. DEPRECATED
        STDMETHOD(SetEnhancedAtmospherics)(__in bool bEnable) abstract;

        /// Get the current cloud coverage density.
        STDMETHOD_(CLOUD_COVERAGE_DENSITY, GetCloudCoverageDensity)() const abstract;

        /// Set the current cloud coverage density.
        /// The effects of setting this value will not be immediately seen.
        STDMETHOD(SetCloudCoverageDensity)(__in CLOUD_COVERAGE_DENSITY eDensity) abstract;

        /// Get the current cloud draw distance.
        STDMETHOD_(CLOUD_DRAW_DISTANCE, GetCloudDrawDistance)() const abstract;
        /// Set the current cloud draw distance.
        STDMETHOD(SetCloudDrawDistance)(__in CLOUD_DRAW_DISTANCE eDistance) abstract;

        /// Get the detailed cloud setting.
        STDMETHOD_(bool, GetDetailedClouds)() const abstract;
        /// Set the detailed cloud setting.
        STDMETHOD(SetDetailedClouds)(__in bool bIsDetailed) abstract;

        /// Get the current thermal visual type.
        STDMETHOD_(THERMAL_VISUAL_TYPE, GetThermalVisualType)() const abstract;
        /// Set the current thermal visual type.
        STDMETHOD(SetThermalVisualType)(__in THERMAL_VISUAL_TYPE eThermalType) abstract;

        /// Get the current number of pressure points.
        STDMETHOD_(int, GetNumberOfPressurePoints)() const abstract;
        
        /// Get the METAR string from the weather station with the given ICAO. 
        /// The maximum supported length for a METAR string is 2000 characters. See also the METAR Data Format.
        STDMETHOD(GetStationMetarData)(__in __notnull LPCSTR pszIcao, __out __notnull LPWSTR pszMetar, __in size_t cchMetar) const abstract;
        /// Get the METAR string from the nearest weather station at the given latitude, longitude, and altitude. 
        /// The maximum supported length for a METAR string is 2000 characters. See also the METAR Data Format.
        STDMETHOD(GetNearestStationMetarData)(__in double dLatRadians, __in double dLonRadians, __in double dAltFeet, __out __notnull LPWSTR pszMetar, __in size_t cchMetar) const abstract;
        /// Get the weather station interpolated METAR string at the given latitude, longitude, and altitude.
        /// The maximum supported length for a METAR string is 2000 characters. See also the METAR Data Format.
        STDMETHOD(GetInterpolatedMetarData)(__in double dLatRadians, __in double dLonRadians, __in double dAltFeet, __out __notnull LPWSTR pszMetar, __in size_t cchMetar) const abstract;
        /// Set the weather system METAR string.
        /// The maximum supported length for a METAR string is 2000 characters. See also the METAR Data Format.
        STDMETHOD(SetMetarData)(__in __notnull LPCWSTR pszMetar, __in int nSeconds) abstract;

        /**
        * Sets the current weather mode.
        * @param eWeatherMode IN:   The WEATHER_MODE to be set.
        * @param pszThemePath IN:   Null-terminated string containing the theme path and filename. The theme file path is only used by the WEATHER_MODE_THEME type.
        *                           The path can be either:
        *                           1. An absolute path (e.g. C:\\Program Files\\My Company\\My Product\\myweathertheme)
        *                           2. A path relative to the Lockheed Martin\\Prepar3D v6\\ installation folder (e.g. weather\\themes\\grayrain)
        *                           3. A path relative to any of the weather theme folders found in any of the weather.cfg configuration files (e.g. grayrain)
        *                           4. Null or an empty string to set clear weather
        */
        STDMETHOD(SetWeatherMode)(__in WEATHER_MODE eWeatherMode, __in_opt LPCWSTR pszThemePath = nullptr) abstract;

        /// Gets the current weather mode.
        STDMETHOD_(WEATHER_MODE, GetWeatherMode)() const abstract;

        /**
        * Gets the current weather theme file path.
        * @param pszThemePath OUT: The output string to store the theme file path.
        * @param cchThemePath IN:  The length of the pszThemePath parameter in characters.
        * @remark Succeeding and returning an empty string represents clear weather.
        */
        STDMETHOD(GetThemePath)(__out __notnull LPWSTR pszThemePath, __in size_t cchThemePath) const abstract;

        /**
        * Creates a weather station at the given ICAO location
        * @param IN: pszICAO The ICAO of the station to be created.
        * @param IN: pszName A descriptive name for the station. This name will appear on weather maps and within dialogs.
        * @param IN: dLatRadians The latitude of the station in radians.
        * @param IN: dLonRadians The longitude of the station in radians.
        * @param IN: dAltFeet The altitude of the station in feet.
        * @param OUT: ppStation The reference pointer to be used by RemoveStation().
        */
        STDMETHOD(CreateStation)(__in __notnull LPCWSTR pszICAO, __in __notnull LPCWSTR pszName, __in double dLatRadians, __in double dLonRadians, __in double dAltFeet, __out __notnull void** ppStation) abstract;

        /**
        * Removes the weather station with the given reference pointer.
        * @param IN: pStation The reference pointer returned by CreateStation().
        */
        STDMETHOD(RemoveStation)(__in __notnull void* pStation) abstract;

        /**
        * Creates a thermal at the given location
        * @param IN: dLatRadians The latitude of the thermal in radians.
        * @param IN: dLonRadians The longitude of the thermal in radians.
        * @param IN: dAltFeet The altitude of the thermal in feet.
        * @param IN: fRadiusMeters Specifies the radius of the thermal, in meters. The maximum radius of a thermal is 100 km.
        * @param IN: fHeightMeters Specifies the height of the thermal, in meters.
        * @param IN: fCoreRateMps Specifies the lift value, in meters per second, within the Core layer. A positive value will provide an updraft, a negative value a downdraft. The maximum rate is 1000 meters/second.
        * @param IN: fCoreTurbulenceMps Specifies a variation in meters per second that is applied to the coreRate. For example, if a value of 1.5 is entered, and the core rate is 5 m/s, the actual core rate applied will be randomly varying between 3.5 m/s and 6.5 m/s.
        * @param IN: fSinkRateMps Specifies the lift value, in meters per second, within the Sink layer. A positive value will provide an updraft, a negative value a downdraft. The maximum rate is 1000 meters/second.
        * @param IN: fSinkTurbulenceMps Specifies a variation in meters per second that is applied to the sinkRate. For example, if a value of 1.5 is entered, and the sink rate is 5 m/s, the actual sink rate applied will be randomly varying between 3.5 m/s and 6.5 m/s.
        * @param IN: fCoreSizeMeters Specifies the radius in meters of the Core of the thermal.
        * @param IN: fCoreTransitionSizeMeters Specifies the width in meters of the transition layer between the Core and the Sink of the thermal. Half of the width of this transition will be outside the Core, and half within.
        * @param IN: fSinkLayerSizeMeters Specifies the radius in meters of the Sink of the thermal.
        * @param IN: fSinkTransitionSizeMeters Specifies the width in meters of the transition layer between the Sink and the atmosphere outside of the thermal. Half of the width of this transition will be outside the radius of the Sink layer, and half within.
        * @param OUT: ppThermal The reference pointer to be used by RemoveThermal().
        */
        STDMETHOD(CreateThermal)(__in double dLatRadians, __in double dLonRadians, __in double dAltFeet, __in float fRadiusMeters, __in float fHeightMeters,
                                 __in float fCoreRateMps, __in float fCoreTurbulenceMps, __in float fSinkRateMps, __in float fSinkTurbulenceMps,
                                 __in float fCoreSizeMeters, __in float fCoreTransitionSizeMeters, __in float fSinkLayerSizeMeters, __in float fSinkTransitionSizeMeters,
                                 __out __notnull void** ppThermal) abstract;

        /**
        * Removes a thermal with the given reference pointer.
        * @param pThermal The reference pointer returned by CreateThermal().
        */
        STDMETHOD(RemoveThermal)(__in __notnull void* pThermal) abstract;

        /**
        * Returns the cloud state for the given area. 
        * @param IN: dLatRadiansMin Specifies the minimum latitude of the required area. This should simply be the lower of the two latitude numbers.
        * @param IN: dLonRadiansMin Specifies the minimum longitude of the required area. This should simply be the lower of the two longitude numbers.
        * @param IN: dAltFeetMin Specifies the minimum altitude of the required area, in feet.
        * @param IN: dLatRadiansMax Specifies the maximum latitude of the required area.
        * @param IN: dLonRadiansMax Specifies the maximum longitude of the required area.
        * @param IN: dAltFeetMax Specifies the maximum altitude of the required area, in feet.
        * @param IN: cbCloudState The size of the cloud state byte array. This value must be 64 * 64 (4096).
        * @param OUT: pCloudState The byte array to receive the cloud state data. This buffer must be 64 * 64 (4096) bytes.
        * Cell data within this array is structured as a 64 x 64 grid. A cell value of zero would mean no cloud layers, to a maximum of 255 layers.
        * @remark Note that the entire world's weather is not simulated all the time.
        * Only a region around the user aircraft with a radius of approximately 128 kilometers is modeled at any one time.
        * A request for cloud data outside this region will simply return zeros.
        * The defined area can cross the Equator or the Greenwich Meridian, but it cannot cross the Poles or the International Date Line.
        */
        STDMETHOD(RequestCloudState)(__in double dLatRadiansMin, __in double dLonRadiansMin, __in double dAltFeetMin,
                                     __in double dLatRadiansMax, __in double dLonRadiansMax, __in double dAltFeetMax,
                                     __in UINT cbCloudState, __out __notnull BYTE* pCloudState) const abstract;

        /**
        * Sets the rate at which cloud formations change.
        * @param Double word containing the rate. A value of zero indicates that cloud formations do not change at all. Values between 1 and 5 indicate that cloud formations should change from 1 (the slowest) to 5 (the fastest).
        */
        STDMETHOD(SetDynamicUpdateRate)(__in DWORD dwRate) abstract;

        /// Get the global temp in degrees C
        STDMETHOD_(float, GetGlobalTemp)() const abstract;
        /// Set the global temp using degrees C
        STDMETHOD(SetGlobalTemp)(__in float aVal) abstract;

        /// Get the global visibility range in Meters
        STDMETHOD_(float, GetGlobalVisRange)() const abstract;
        /// Set the global visibility range in Meters
        STDMETHOD(SetGlobalVisRange)(__in float aVal) abstract;

        /// Get the global horizontal wind speed in meters per second
        STDMETHOD_(float, GetGlobalHorizWindSpeed)() const abstract;
        /// Set the global horizontal wind speed in meters per second
        STDMETHOD(SetGlobalHorizWindSpeed)(__in float aVal) abstract;

        /// Get the global wind direction in degrees
        STDMETHOD_(float, GetGlobalWindDirection)() const abstract;
        /// Set the global wind direction in degrees
        STDMETHOD(SetGlobalWindDirection)(__in float aVal) abstract;

        /// Get the global barometer pressure in millibars
        STDMETHOD_(float, GetGlobalBaroPressure)() const abstract;
        /// Set the global barometer pressure in millibars
        STDMETHOD(SetGlobalBaroPressure)(__in float aVal) abstract;

        /// Get the global dew point
        STDMETHOD_(float, GetGlobalDewPoint)() const abstract;
        /// Set the global dew point
        STDMETHOD(SetGlobalDewPoint)(__in float aVal) abstract;

        STDMETHOD(GetWeatherStation)(__in LPCSTR pszIcao, __out P3D::IWeatherStationV430** ppStation) abstract;

        STDMETHOD(ReloadWeather)() abstract;
    };

    /// WeatherSystemPdk interface ID
    DEFINE_GUID(IID_IWeatherSystemV500, 0xad730d3a, 0x3c0f, 0x4899, 0xad, 0x28, 0x28, 0xf8, 0x37, 0xe, 0xcf, 0xfb);
    /// WeatherSystemPdk service ID
    DEFINE_GUID(SID_WeatherSystem, 0x1e341cce, 0xe302, 0x40c7, 0xa4, 0x58, 0x75, 0xb8, 0x8a, 0xb5, 0x8f, 0x1a);
    /// WeatherSystemPdk 
    DEFINE_GUID(IID_IWeatherStationV430, 0xe4452b96, 0x16ba, 0x4e82, 0x93, 0x42, 0xaa, 0x37, 0xaf, 0x1f, 0x5c, 0x26);
    /** \} */

};
