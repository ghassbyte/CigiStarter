// Copyright (c) 2010-2018 Lockheed Martin Corporation. All rights reserved.
// Use of this file is bound by the PREPAR3D® SOFTWARE DEVELOPER KIT END USER LICENSE AGREEMENT
//----------------------------------------------------------------------------
// Author: Prepar3D Dev.
// Description: Weather Types to Prepar3D Weather System
//----------------------------------------------------------------------------

#pragma once

namespace P3D
{
    /** @addtogroup types    
    *  @{
    */

    // Weather modes.
    enum WEATHER_MODE
    {
        WEATHER_MODE_THEME,
        WEATHER_MODE_CUSTOM,
        WEATHER_MODE_GLOBAL,
    };

    // Thermal visualization modes.
    enum THERMAL_VISUAL_TYPE
    {
        THERMAL_VISUAL_NONE,
        THERMAL_VISUAL_NATURAL,
        THERMAL_VISUAL_SCHEMATIC,
        THERMAL_VISUAL_MAX = THERMAL_VISUAL_SCHEMATIC,
    };

    // Cloud coverage densities.
    enum CLOUD_COVERAGE_DENSITY
    {
        CLOUD_COVERAGE_DENSITY_MIN = 5,
        CLOUD_COVERAGE_DENSITY_LOW = CLOUD_COVERAGE_DENSITY_MIN,
        CLOUD_COVERAGE_DENSITY_MEDIUM,
        CLOUD_COVERAGE_DENSITY_HIGH,
        CLOUD_COVERAGE_DENSITY_MAXIMUM,
        CLOUD_COVERAGE_DENSITY_MAX = CLOUD_COVERAGE_DENSITY_MAXIMUM,
    };

    // Cloud draw distances.
    enum CLOUD_DRAW_DISTANCE
    {
        CLOUD_DRAW_DISTANCE_MIN = 3,
        CLOUD_DRAW_DISTANCE_60_MILES = CLOUD_DRAW_DISTANCE_MIN,
        CLOUD_DRAW_DISTANCE_70_MILES,
        CLOUD_DRAW_DISTANCE_80_MILES,
        CLOUD_DRAW_DISTANCE_90_MILES,
        CLOUD_DRAW_DISTANCE_100_MILES,
        CLOUD_DRAW_DISTANCE_110_MILES,
        CLOUD_DRAW_DISTANCE_MAX = CLOUD_DRAW_DISTANCE_110_MILES,
    };
    // Cloud Types
    enum CLOUD_TYPE
    {
        CLOUD_NONE,
        CLOUD_CIRRUS,
        CLOUD_STRATUS,
        CLOUD_CUMULUS,
        CLOUD_CUMULONIMBUS,
        CLOUD_VISIBILITY_LAYER,
        CLOUD_THUNDERSTORM,
        CLOUD_MAX
    };
    // Cloud Coverage Amounts
    enum CLOUD_COVER
    {
        CLOUD_CLEAR,
        CLOUD_FEW_1_8,
        CLOUD_FEW_2_8,
        CLOUD_SCATTERED_3_8,
        CLOUD_SCATTERED_4_8,
        CLOUD_BROKEN_5_8,
        CLOUD_BROKEN_6_8,
        CLOUD_BROKEN_7_8,
        CLOUD_OVERCAST_8_8,
        CLOUD_C_MAX
    };
    // Cloud top types
    enum CLOUD_TOP
    {
        CLOUDTOP_FLAT,
        CLOUDTOP_ROUND,
        CLOUDTOP_ANVIL,
        CLOUDTOP_TOP_MAX
    };
    // Turbulance amounts
    enum TURBULANCE
    {
        TURB_NONE,
        TURB_LIGHT,
        TURB_MODERATE,
        TURB_HEAVY,
        TURB_SEVERE,
        TURB_MAX
    };
    // Types of Precipitation
    enum PRECIPTYPE
    {
        PRECIP_NONE,
        PRECIP_RAIN,
        PRECIP_SNOW,
        PRECIP_MAX
    };
    // Rate of Precipitation
    enum PRECIPRATE
    {
        PRECIPRATE_VLOW,
        PRECIPRATE_LOW,
        PRECIPRATE_MODERATE,
        PRECIPRATE_HIGH,
        PRECIPRATE_VHIGH,
        PRECIPRATE_MAX
    };
    // Rate of Icing
    enum ICINGRATE
    {
        ICINGRATE_NONE,
        ICINGRATE_TRACE,
        ICINGRATE_LIGHT,
        ICINGRATE_MODERATE,
        ICINGRATE_SEVERE,
        ICINGRATE_MAX
    };
    //Wind Shear amount
    enum WINDSHEAR
    {
        WINDSHEAR_GRADUAL,
        WINDSHEAR_MODERATE,
        WINDSHEAR_STEEP,
        WINDSHEAR_INSTANTANEOUS,
        WINDSHEAR_MAX
    };

    struct CloudLayer
    {
        CLOUD_TYPE    eCloudType;
        float         fCloudBase; //meters
        float         fCloudTops; //meters
        float         fCloudDeviation; //meters
        CLOUD_COVER   eCloudCover;
        CLOUD_TOP     eCloudTop;
        TURBULANCE    eTurbulance;
        PRECIPTYPE    ePrecipType;
        float         fPrecipBase; //meters
        PRECIPRATE    ePrecipRate;
        ICINGRATE     eIcingRate;
        float         fCloudDensity;
        float         fCloudScatter;
    };

    struct WindAloftLayer
    {
        float fAlt; //meters
        float fSpeed; //knots
        float fGusts; //knots
        float fDirection; //degrees
        float fVariance; //degrees
        TURBULANCE eTurb;
        WINDSHEAR eWindShear;
    };

    struct TempLayer
    {
        float   fAlt; //meters
        float   fTemp; //Deg C
        float   fRange; //Deg C
        float   fDewPoint; //Deg C
    };

    struct PressureInfo
    {
        float fPressureAtAlt; // millibars - at requested elevation
        float fPressureAtSL; // millibars - at sear level
        float fRange;
    };
    
    struct VisibilityLayer
    {
        float fVis; //meters
        float fBase; //meters
        float fTops; //meters
    };

    /** @} */ // end of group
};
