#pragma once
//-----------------------------------------------------------------------------
// Environment.h
//
// The PDK-independent half of the environment path: what the HOST asked the
// world to be, what an operator override makes of it, and the METAR that
// carries the cloud and precipitation into Prepar3D's weather system.
//
// It is a header rather than part of a translation unit for the same reason
// SimLinkMap.h is: the contract test (tools\PdkSimLink\test) compiles exactly
// this code, and so does the add-on (tools\PdkSimLink\PdkSimLink.cpp) and
// CigiStarter (main.cpp).  Nothing here includes a PDK header or talks to a
// window, so all three can share it and the test needs no simulator.
//
// The pieces:
//
//   * HostEnvironment  -- what the HOST has told us so far, accumulated from
//     the CIGI messages as they arrive.  Each message is partial (a Celestial
//     Sphere Control carries the clock, an Atmosphere Control the air, a
//     Weather Control the cloud, a Component Control a cloud *pattern*), so the
//     fields are sticky: the newest message of each kind wins and a field that
//     has never arrived stays at its default.
//
//   * resolve()        -- the HOST's own values with the operator's override on
//     top and defaults underneath, as the row the add-on actually applies
//     (simlink::Environment).  This is the policy the IG owns; the add-on only
//     applies what it is handed.
//
//   * metarFor()/dewPointC() -- the cloud and precipitation, which the PDK
//     weather system only takes as a synthesised METAR observation.
//
//   * cloudPatternSpec()/applyCloudPattern() -- the HOST's cloud *pattern*, the
//     one way a real capture describes a sky, turned into the same resolved row.
//     A pattern is a footprint and an altitude; Prepar3D takes layers, so the
//     footprint becomes a coverage over the whole world.  The mapping is this
//     IG's, not the wire's (the vendor's own is not in the capture) and is
//     recorded as an assumption where it is applied.
//
// The four time-of-day captures (06:15 / 12:00 / 17:30 / 23:30) are read as
// *local* time, not Zulu: the site is UTC+7, so Zulu would put the "day" capture
// at 19:00 local and every preset on the wrong side of sunset.  The Zulu-vs-
// local reading is still owed a rig confirmation (TODO.md, 8).
//-----------------------------------------------------------------------------

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include "cigi.h"
#include "SimLinkIpc.h"

namespace cigienv {

//-----------------------------------------------------------------------------
// Operator presets
//-----------------------------------------------------------------------------

// Follow the HOST, or pin the clock to one of the four captures.  `Host` is
// first (the zero value) on purpose: doing nothing is the default, and an
// operator override is the deliberate act.
enum class TimePreset : int {
    Host = 0,   // follow the HOST's Celestial Sphere Control
    Dawn,       // 06:15 local -- the "dawn" capture
    Day,        // 12:00 local -- the "day" capture
    Dusk,       // 17:30 local -- the "dusk" capture
    Night,      // 23:30 local -- the "night" capture
    Count,
};

inline const char* timePresetName(TimePreset p) {
    switch (p) {
    case TimePreset::Host:  return "Host";
    case TimePreset::Dawn:  return "Dawn 06:15";
    case TimePreset::Day:   return "Day 12:00";
    case TimePreset::Dusk:  return "Dusk 17:30";
    case TimePreset::Night: return "Night 23:30";
    default:                return "?";
    }
}

struct TimeOfDay {
    int hour   = 0;
    int minute = 0;
};

// The clock a preset stands for.  False for `Host`, which has none of its own.
inline bool timePresetClock(TimePreset p, TimeOfDay& out) {
    switch (p) {
    case TimePreset::Dawn:  out = TimeOfDay{ 6, 15 };  return true;
    case TimePreset::Day:   out = TimeOfDay{ 12,  0 }; return true;
    case TimePreset::Dusk:  out = TimeOfDay{ 17, 30 }; return true;
    case TimePreset::Night: out = TimeOfDay{ 23, 30 }; return true;
    default:                return false;
    }
}

// Follow the HOST, or pin cloud/precipitation/visibility to a preset.  The
// presets deliberately do NOT touch temperature, pressure or wind: those belong
// to the flight model, which is the HOST's, and an operator wanting a rainy sky
// is not asking to change the air the aircraft flies in.
enum class WeatherPreset : int {
    Host = 0,   // follow the HOST's Weather Control (none arrives in this rig)
    Clear,      // no cloud, no precipitation, visibility clear
    Cloudy,     // a broken cumulus deck, no precipitation
    Rain,       // nimbostratus, rain
    Storm,      // cumulonimbus, rain, lightning
    Count,
};

inline const char* weatherPresetName(WeatherPreset p) {
    switch (p) {
    case WeatherPreset::Host:   return "Host";
    case WeatherPreset::Clear:  return "Clear";
    case WeatherPreset::Cloudy: return "Cloudy";
    case WeatherPreset::Rain:   return "Rain";
    case WeatherPreset::Storm:  return "Storm";
    default:                    return "?";
    }
}

// CIGI CloudTypeGrp values this header needs, named so it does not have to lean
// on the numeric values scattered through the CIGI code (cigi.h's WeatherControl
// keeps the whole enum in its comment).
constexpr uint32_t kCloudNone         = 0;
constexpr uint32_t kCloudCumulonimbus = 6;
constexpr uint32_t kCloudCumulus      = 7;
constexpr uint32_t kCloudNimbostratus = 8;

// One preset's worth of world.
struct WeatherSpec {
    uint32_t cloudType        = kCloudNone;
    double   coverage         = 0.0;                    // 0..100 percent
    uint32_t precipType       = simlink::kPrecipNone;   // simlink::PrecipType
    uint32_t severity         = 0;                      // 0..5, least..most
    bool     lightning        = false;
    double   visibilityMeters = 200000.0;
    double   baseElevMeters   = 0.0;                    // cloud base
    double   thicknessMeters  = 0.0;                    // cloud layer depth
};

// The preset's world.  False for `Host`, which has none of its own.
//
// `Clear` still asks for a sky with no cloud rather than "whatever is there":
// that is the point of the preset -- an operator switching from Cloudy back to
// Clear wants the cloud gone, so it carries an explicit cloud type and coverage
// of zero (and the add-on's METAR always names a sky, `SKC` included, so the sim
// is told to clear rather than left holding the last observation).
inline bool weatherPresetSpec(WeatherPreset p, WeatherSpec& out) {
    switch (p) {
    case WeatherPreset::Clear:
        out = WeatherSpec{ kCloudNone, 0.0, simlink::kPrecipNone, 0, false,
                           200000.0, 0.0, 0.0 };
        return true;
    case WeatherPreset::Cloudy:
        out = WeatherSpec{ kCloudCumulus, 60.0, simlink::kPrecipNone, 0, false,
                           8000.0, 900.0, 600.0 };
        return true;
    case WeatherPreset::Rain:
        out = WeatherSpec{ kCloudNimbostratus, 90.0, simlink::kPrecipRain, 3,
                           false, 4000.0, 600.0, 1500.0 };
        return true;
    case WeatherPreset::Storm:
        out = WeatherSpec{ kCloudCumulonimbus, 100.0, simlink::kPrecipRain, 5,
                           true, 1500.0, 400.0, 3000.0 };
        return true;
    default:
        return false;
    }
}

//-----------------------------------------------------------------------------
// The HOST's cloud pattern
//-----------------------------------------------------------------------------
// The reference HOST has no Weather Control: the three `cloud pat*.bin`
// captures describe their sky with a *component* instead -- a CIGI Component
// Control (id 4) on the entity the same capture creates for it (cigi.h has the
// wire evidence).  Three floats come with it, all three in metres, and they are
// the pattern's footprint and its height:
//
//   data 1  width,  metres   (14816 = 8 x 1852, i.e. 8 nautical miles)
//   data 2  depth,  metres   (12964 = 7 x 1852, i.e. 7 nautical miles)
//   data 3  altitude, metres (914.4 = 3000 ft)
//
// The whole-number multiples of 1852 are what identify them: the vendor's tool
// writes its distances in nautical miles and its altitude in feet, and converts
// both to the metres CIGI carries, so 3000 ft arrives as 914.4 and not as 3000.
//
// The component state is the pattern's index -- 1, 2 and 3 for the three
// captures, 0 for "take it away" -- and the cloud's *position* is the entity's
// own Entity Control, in the same datagram.
//
// None of that is a sky, which is the only thing Prepar3D's weather system can
// be given: it has cloud layers, not cloud volumes at a place.  So a pattern is
// resolved to a layer, and the choices below are this IG's, not the wire's.
// They are written down rather than buried because the vendor's own mapping is
// not in the capture and could only be recovered from the tool that wrote it:
//
//   * the base is data 3 -- the only altitude that is part of the pattern
//     itself, and the values it carries (3000, 3500 and 3000 ft) are cloud-base
//     heights;
//   * the coverage is the footprint's area at one per cent per square nautical
//     mile, floored at 30 % so even the smallest pattern puts cloud in the sky
//     and capped at 100 %.  A larger pattern covers more sky, which makes the
//     three captures three visibly different skies (56 %, 30 % and 100 % --
//     broken cumulus, scattered cumulus, overcast);
//   * the type is cumulus and the thickness a cumulus deck's ~900 m: the wire
//     carries neither, and the METAR below needs only the base and the amount.
//
// Precipitation is never a cloud pattern's business, so it is cleared.
constexpr uint16_t kCloudPatternComponentId = 0x1001;
constexpr double   kNauticalMileMeters      = 1852.0;

// Does this Component Control carry the cloud pattern?  Both the component id
// and the class are checked: the id is the vendor's own, and its class is
// Entity (0) even though what it describes is weather (cigi.h says why the
// class is not gated on Global Layered Weather).
inline bool isCloudPattern(const cigi::ComponentControl& c) {
    return c.componentId == kCloudPatternComponentId &&
           c.componentClass == cigi::kComponentEntity;
}

// The cloud layer one pattern asks for, as a `WeatherSpec`.  `pattern` is the
// component's state, so 0 is the HOST taking its cloud away -- carried as an
// explicit clear sky, because a sky that is simply not present leaves the sim
// holding the last one.
inline WeatherSpec cloudPatternSpec(uint32_t pattern,
                                    const cigi::ComponentControl& c) {
    WeatherSpec s{};
    s.cloudType = kCloudNone;
    if (pattern == 0) return s;

    const double widthNm  = static_cast<double>(c.data1) / kNauticalMileMeters;
    const double depthNm  = static_cast<double>(c.data2) / kNauticalMileMeters;
    double coverage = widthNm * depthNm;
    if (!(coverage > 30.0)) coverage = 30.0;
    if (coverage > 100.0)   coverage = 100.0;

    double baseM = static_cast<double>(c.data3);   // already metres
    if (!(baseM > 0.0)) baseM = 900.0;   // no height on the wire: a cumulus base

    s.cloudType        = kCloudCumulus;
    s.coverage         = coverage;
    s.precipType       = simlink::kPrecipNone;
    s.severity         = 0;
    s.lightning        = false;
    s.visibilityMeters = 200000.0;       // a pattern is cloud, not visibility
    s.baseElevMeters   = baseM;
    s.thicknessMeters  = 900.0;
    return s;
}

//-----------------------------------------------------------------------------
// What the HOST has said
//-----------------------------------------------------------------------------

// The newest of each kind of environment message.  Both an Atmosphere Control
// (id 10) and a Weather Control (id 12) carry the air -- temperature, visibility,
// wind, pressure, humidity -- so they share one set of fields and the newer
// message wins; only the Weather Control carries cloud and precipitation.
//
// `have*` is what distinguishes "the HOST said clear" from "the HOST has not
// spoken": a Weather Control with no cloud is a real instruction, and an absent
// one is not.
struct HostEnvironment {
    bool     haveTime  = false;
    uint32_t hour      = 0;
    uint32_t minute    = 0;
    uint32_t second    = 0;     // CIGI's Celestial Sphere Control has no seconds
    bool     dateValid = false;
    uint32_t year      = 0;
    uint32_t month     = 0;
    uint32_t day       = 0;

    bool     haveAtmos        = false;
    double   airTempC         = 0.0;
    double   visibilityMeters = 0.0;
    double   horizWindMps     = 0.0;
    double   vertWindMps      = 0.0;
    double   windDirDeg       = 0.0;
    double   baroPressureMb   = 0.0;
    double   humidityPct       = 0.0;

    bool     haveWeather      = false;
    uint32_t cloudType        = 0;                      // CIGI CloudTypeGrp
    uint32_t severity         = 0;                      // 0..5
    uint32_t precipType       = simlink::kPrecipNone;   // simlink::PrecipType
    double   coverage         = 0.0;                    // 0..100
    double   baseElevMeters   = 0.0;
    double   thicknessMeters  = 0.0;
    bool     lightning        = false;

    // The cloud pattern, when that is how the HOST said it (a Component Control
    // rather than a Weather Control).  It fills the sky fields above exactly as
    // a Weather Control does -- the two are the same group, and the newest of
    // them wins -- and `patternIndex` is kept so the window can say which of the
    // HOST's patterns is up (0 = it took the cloud away).
    bool     havePattern      = false;
    uint32_t patternIndex     = 0;
};

// Fold one parsed message in.  Each leaves the other kinds as they were, so the
// accumulation is order-independent: a repeated Atmosphere Control just refreshes
// the air.
inline void applyCelestialSphere(HostEnvironment& h, const cigi::CelestialSphere& c) {
    h.haveTime  = true;
    h.hour      = c.hour;
    h.minute    = c.minute;
    h.second    = 0;
    h.dateValid = c.dateValid();
    if (c.dateValid()) {
        // Month * 1000000 + Day * 10000 + Year, the packed form the ICD gives it.
        h.year  = c.date % 10000u;
        h.month = (c.date / 1000000u) % 100u;
        h.day   = (c.date / 10000u) % 100u;
    }
}

inline void applyAtmosphere(HostEnvironment& h, const cigi::Atmosphere& a) {
    h.haveAtmos        = true;
    h.humidityPct      = a.humidity;
    h.airTempC         = a.airTempC;
    h.visibilityMeters = a.visibilityM;
    h.horizWindMps     = a.horizWindMps;
    h.vertWindMps      = a.vertWindMps;
    h.windDirDeg       = a.windDirDeg;
    h.baroPressureMb   = a.baroPressureMb;
}

// CIGI names the precipitation with the layer ID, not the severity or the cloud
// type (CigiBaseWeatherCtrl.h `LayerIDGrp`): 0 ground fog, 1-3 cloud layers,
// 4 rain, 5 snow, 6 sleet, 7 hail, 8 sand, 9 dust.  Anything else is no
// precipitation.
inline uint32_t precipTypeFromLayer(uint8_t layerId) {
    switch (layerId) {
    case 4: return simlink::kPrecipRain;
    case 5: return simlink::kPrecipSnow;
    case 6: return simlink::kPrecipSleet;
    case 7: return simlink::kPrecipHail;
    case 8: return simlink::kPrecipSand;
    case 9: return simlink::kPrecipDust;
    default: return simlink::kPrecipNone;
    }
}

// The Weather Control repeats the air fields, so it refreshes those too.  Its
// scope (global/regional/entity) is not honoured: this link carries one world
// and the sim's weather system applies it globally, which is the only reading
// that matches a single IG with a single sky.
inline void applyWeather(HostEnvironment& h, const cigi::WeatherControl& w) {
    h.haveWeather      = true;
    h.haveAtmos        = true;
    // The newest description of the sky wins, and this one is a Weather Control
    // rather than the HOST's cloud pattern, so the pattern is no longer what the
    // world's sky came from.
    h.havePattern      = false;
    h.patternIndex     = 0;
    h.humidityPct      = w.humidity;
    h.airTempC         = w.airTempC;
    h.visibilityMeters = w.visibilityM;
    h.horizWindMps     = w.horizWindMps;
    h.vertWindMps      = w.vertWindMps;
    h.windDirDeg       = w.windDirDeg;
    h.baroPressureMb   = w.baroPressureMb;
    h.cloudType        = w.cloudType();
    h.severity         = w.severity();
    h.coverage         = w.coverage;
    h.baseElevMeters   = w.baseElevM;
    h.thicknessMeters  = w.thicknessM;
    h.lightning        = w.lightningEnabled();
    h.precipType       = precipTypeFromLayer(w.layerId);
}

// The HOST's other way of describing its sky: a cloud pattern on the cloud
// entity (a Component Control).  It fills the same fields a Weather Control
// does, so the two are one group and whichever arrives last is the sky the world
// carries; `havePattern` says which of the two it was.  A pattern is never rain,
// so the precipitation fields are the pattern spec's (none), not the last
// Weather Control's.
inline void applyCloudPattern(HostEnvironment& h, const cigi::ComponentControl& c) {
    const WeatherSpec spec = cloudPatternSpec(c.componentState, c);
    h.haveWeather      = true;
    h.havePattern      = true;
    h.patternIndex     = c.componentState;
    h.cloudType        = spec.cloudType;
    h.coverage         = spec.coverage;
    h.precipType       = spec.precipType;
    h.severity         = spec.severity;
    h.baseElevMeters   = spec.baseElevMeters;
    h.thicknessMeters  = spec.thicknessMeters;
    h.lightning        = spec.lightning;
}

//-----------------------------------------------------------------------------
// The resolved world
//-----------------------------------------------------------------------------

// The HOST's own world, the operator's override on top, defaults underneath.
//
// The returned row carries the *state* bits only (`kEnvTimeManual`,
// `kEnvWeatherManual`, `kEnvDateValid`, `kEnvLightning`) and the `*Valid` bits
// that mark which groups are actually present; which groups changed, and
// therefore which the add-on should re-apply, is the publisher's call
// (SimLink.cpp's publishEnvironment), because only it knows what was last
// written.
inline simlink::Environment resolve(const HostEnvironment& host,
                                    TimePreset time, WeatherPreset weather) {
    simlink::Environment e{};
    e.flags = 0u;

    // Clock.  An operator preset pins it and says so; otherwise the HOST's own
    // clock is carried through unchanged.  Either way `kEnvTimeValid` records
    // that there *is* a clock to apply -- without it a HOST that never sent a
    // Celestial Sphere would resolve to the 00:00 default and the add-on would
    // set the sim to midnight.
    if (time != TimePreset::Host) {
        TimeOfDay tod;
        if (timePresetClock(time, tod)) {
            e.hour   = static_cast<uint32_t>(tod.hour);
            e.minute = static_cast<uint32_t>(tod.minute);
            e.second = 0;
            e.flags |= simlink::kEnvTimeManual | simlink::kEnvTimeValid;
        }
    } else if (host.haveTime) {
        e.hour   = host.hour;
        e.minute = host.minute;
        e.second = host.second;
        e.flags |= simlink::kEnvTimeValid;
    }

    // The date rides with the clock and is the HOST's either way -- an operator
    // pinning 12:00 is not choosing a day, and the captures all fly 2000-01-01.
    if (host.dateValid) {
        e.year  = host.year;
        e.month = host.month;
        e.day   = host.day;
        e.flags |= simlink::kEnvDateValid;
    }

    // Air.  All of it the HOST's where it has spoken; the two fields without a
    // meaningful zero (pressure, humidity) get sane defaults so the METAR below
    // is well formed even before any message arrives.  `kEnvAtmosValid` keeps
    // those defaults from being *applied* when the HOST never sent an
    // Atmosphere Control: the defaults are for the METAR's benefit, not a
    // weather the sim should be moved to.
    if (host.haveAtmos) {
        e.airTempC         = host.airTempC;
        e.visibilityMeters = host.visibilityMeters;
        e.horizWindMps     = host.horizWindMps;
        e.vertWindMps      = host.vertWindMps;
        e.windDirDeg       = host.windDirDeg;
        e.baroPressureMb   = host.baroPressureMb;
        e.humidityPct      = host.humidityPct;
        e.flags |= simlink::kEnvAtmosValid;
    } else {
        e.baroPressureMb = 1013.25;
        e.humidityPct    = 50.0;
        // The visibility group belongs to the *sky* -- it is emitted whenever a
        // sky is -- so a world with a sky and no air still has one to emit, and
        // its default has to be "no restriction" rather than the zero a fresh
        // row starts at: a HOST whose only description of the weather is a cloud
        // pattern would otherwise put a zero-visibility observation into the
        // sim and fog the whole picture in with its cloud.
        e.visibilityMeters = 200000.0;
    }

    // Cloud and precipitation.  An operator preset replaces the HOST's sky --
    // and its visibility with it, which is why the preset is applied after the
    // air above.  Temperature, pressure and wind are left alone deliberately.
    // `kEnvWeatherValid` is what stops a HOST that never sent a Weather Control
    // (the real one never does) from being read as "clear the sky": with no
    // preset and no HOST weather there is simply no sky to apply.
    WeatherSpec spec{};
    if (weather != WeatherPreset::Host && weatherPresetSpec(weather, spec)) {
        e.cloudType        = spec.cloudType;
        e.coverage         = spec.coverage;
        e.precipType       = spec.precipType;
        e.severity         = spec.severity;
        e.baseElevMeters   = spec.baseElevMeters;
        e.thicknessMeters  = spec.thicknessMeters;
        e.visibilityMeters = spec.visibilityMeters;
        if (spec.lightning) e.flags |= simlink::kEnvLightning;
        e.flags |= simlink::kEnvWeatherManual | simlink::kEnvWeatherValid;
    } else if (host.haveWeather) {
        e.cloudType       = host.cloudType;
        e.coverage        = host.coverage;
        e.precipType      = host.precipType;
        e.severity        = host.severity;
        e.baseElevMeters  = host.baseElevMeters;
        e.thicknessMeters = host.thicknessMeters;
        if (host.lightning) e.flags |= simlink::kEnvLightning;
        e.flags |= simlink::kEnvWeatherValid;
        // Which of the HOST's two ways of describing a sky this was: the cloud
        // pattern (a Component Control) rather than a Weather Control.  It is
        // state, like the manual bits, so a reader can say what the sky is.
        if (host.havePattern) e.flags |= simlink::kEnvWeatherPattern;
    }

    return e;
}

//-----------------------------------------------------------------------------
// The sim's own numbering, for the add-on's read-back
//-----------------------------------------------------------------------------
// `EnvironmentStatus` carries the sky the add-on read out of the sim's weather
// system, and that is in Prepar3D's own numbering, not CIGI's: `GetCloudAtIndex`
// fills a `P3D::CloudLayer`, whose `CLOUD_TYPE` and `CLOUD_COVER` are below.
// The names live here so the operator window and the bench tests describe the
// reading the same way, and so the numbers are not re-spelled at each use.
//
// (The CIGI cloud types are different numbers again -- 6 Cb, 7 Cu, 8 Ns -- and
// Environment.h's `kCloud*` above are those.  A read-back is not a command.)
inline const char* p3dCloudTypeName(uint32_t t) {   // P3D::CLOUD_TYPE
    switch (t) {
    case 1:  return "cirrus";
    case 2:  return "stratus";
    case 3:  return "cumulus";
    case 4:  return "cumulonimbus";
    case 5:  return "visibility";
    case 6:  return "thunderstorm";
    default: return "none";
    }
}

inline const char* p3dCloudCoverName(uint32_t c) {  // P3D::CLOUD_COVER, eighths
    switch (c) {
    case 1:
    case 2:  return "FEW";
    case 3:
    case 4:  return "SCT";
    case 5:
    case 6:
    case 7:  return "BKN";
    case 8:  return "OVC";
    default: return "clear";
    }
}

//-----------------------------------------------------------------------------
// METAR
//-----------------------------------------------------------------------------

// Dew point from the Magnus approximation, the form the predecessor program
// used.  Clamped to a sane humidity so an absent field (0) cannot send the
// logarithm to minus infinity.
inline double dewPointC(double tempC, double humidityPct) {
    double rh = humidityPct;
    if (!(rh > 0.0))   rh = 1.0;
    if (rh > 100.0)    rh = 100.0;
    const double a = 17.27, b = 237.7;
    const double alpha = (a * tempC) / (b + tempC) + std::log(rh / 100.0);
    return (b * alpha) / (a - alpha);
}

// The station the world is applied to.  `GLOB` is the global observation -- the
// only station that matches "one IG, one sky" -- and it is also the station the
// add-on reads back out of the sim (SimLinkIpc.h, `simMetar`), so the two names
// cannot drift apart.
constexpr const char* kMetarStation = "GLOB";

// Render a synthesised METAR observation for the resolved world, in the shape
// IWeatherSystemV500::SetMetarData renders: station `GLOB` (the global
// observation, the one the sim's weather actually draws), the clock, wind,
// visibility, present weather, cloud and the temperature/dew point/pressure
// group.
//
//   GLOB 1230Z 27008KT 9999 -RA BKN030 30/24 Q1013
//
// The clock is carried for form's sake -- the sim ignores DATETIME on a set --
// and the ESP "&" extensions are deliberately absent, because they are not
// parsed back on a set.  Wind is in knots (the weather parser's own unit, so it
// has one less conversion to get wrong); the caller applies the exact
// metric/MPS values afterwards through the global setters, which wins wherever
// the two disagree.
//
// Wind, temperature/dew point and pressure are all air, and are emitted only
// when the HOST has actually sent the air (`kEnvAtmosValid`).  A sky can reach
// the add-on with no atmosphere at all -- an operator's weather preset chosen
// before the warm-up Atmosphere Control -- and a METAR that named the resolved
// row's place-holder 0 C / 1013 mb would parse those straight into the sim.  The
// visibility, present weather and cloud groups belong to the sky and are always
// named when this is called:
//
//   GLOB 0000Z 1500 +TSRA OVC013CB
//
// Returns the number of characters written (without the terminator), or 0 if
// there was nothing to write.
inline int metarFor(const simlink::Environment& e, char* out, size_t cap) {
    if (!out || cap == 0) return 0;

    const unsigned hour   = static_cast<unsigned>(e.hour   % 24u);
    const unsigned minute = static_cast<unsigned>(e.minute % 60u);
    const bool     haveAir = (e.flags & simlink::kEnvAtmosValid) != 0u;

    // Wind: direction to the nearest 10 degrees (METAR's own resolution, 360 for
    // north), speed to the nearest knot.  Calm reads 00000KT.
    char wind[12] = "";
    if (haveAir) {
        const double speedKt = e.horizWindMps * 1.9438444924406;
        const int    speed   = static_cast<int>(std::lround(speedKt));
        int dir = static_cast<int>(std::lround(e.windDirDeg / 10.0)) * 10;
        dir %= 360;
        if (dir < 0) dir += 360;
        if (dir == 0 && speed > 0) dir = 360;  // north is 360, not 000, when it blows
        std::snprintf(wind, sizeof(wind), " %03d%02dKT", dir, speed);
    }

    // Visibility: METAR's 9999 means "10 km or more".
    long vis = static_cast<long>(std::lround(e.visibilityMeters));
    if (vis < 0) vis = 0;
    if (vis > 9999) vis = 9999;
    else vis = (vis / 100) * 100;

    // Present weather: the intensity from the severity, the code from the
    // precipitation, and TS when lightning is present.  A thunderstorm with no
    // precipitation is just TS, which is a legal observation.
    char wx[20] = "";
    if (e.precipType != simlink::kPrecipNone ||
        (e.flags & simlink::kEnvLightning) != 0u) {
        const char* code = "";
        switch (e.precipType) {
        case simlink::kPrecipRain:  code = "RA";   break;
        case simlink::kPrecipSnow:  code = "SN";   break;
        case simlink::kPrecipSleet: code = "RASN"; break;
        case simlink::kPrecipHail:  code = "GR";   break;
        case simlink::kPrecipSand:  code = "SS";   break;
        case simlink::kPrecipDust:  code = "DU";   break;
        default: break;
        }
        char body[16] = "";
        int  n = 0;
        if ((e.flags & simlink::kEnvLightning) != 0u)
            n += std::snprintf(body + n, sizeof(body) - n, "TS");
        std::snprintf(body + n, sizeof(body) - n, "%s", code);

        const char intensity = e.severity <= 1u ? '-' : e.severity >= 4u ? '+' : 0;
        if (intensity) std::snprintf(wx, sizeof(wx), "%c%s", intensity, body);
        else           std::snprintf(wx, sizeof(wx), "%s", body);
    }
    char wxField[24] = "";
    if (wx[0]) std::snprintf(wxField, sizeof(wxField), " %s", wx);

    // Cloud: coverage bucket, base in hundreds of feet, and CB when the layer
    // is cumulonimbus.  No cloud is SKC.  A base the HOST did not give is
    // reported at 3000 ft, the flat default the ICD's own examples use.
    char clouds[16];
    if (e.cloudType == 0u || e.coverage <= 0.0) {
        std::snprintf(clouds, sizeof(clouds), "SKC");
    } else {
        const char* amount = e.coverage <= 25.0 ? "FEW"
                           : e.coverage <= 50.0 ? "SCT"
                           : e.coverage <= 87.0 ? "BKN" : "OVC";
        double baseFt = e.baseElevMeters * 3.280839895013123;
        if (baseFt <= 0.0) baseFt = 3000.0;
        long hundreds = static_cast<long>(std::lround(baseFt / 100.0));
        if (hundreds <   1) hundreds = 1;
        if (hundreds > 999) hundreds = 999;
        const char* cb = e.cloudType == kCloudCumulonimbus ? "CB" : "";
        std::snprintf(clouds, sizeof(clouds), "%s%03ld%s", amount, hundreds, cb);
    }

    // Temperature, dew point and pressure: air, and only present when it is.
    // Both temperatures are METAR's two-digit with M for minus.
    char air[24] = "";
    if (haveAir) {
        char temp[8], dew[8];
        const auto fmtTemp = [](double c, char* buf, size_t n) {
            long v = static_cast<long>(std::lround(c));
            if (v < 0) std::snprintf(buf, n, "M%02ld", -v);
            else       std::snprintf(buf, n, "%02ld", v);
        };
        fmtTemp(e.airTempC, temp, sizeof(temp));
        fmtTemp(dewPointC(e.airTempC, e.humidityPct), dew, sizeof(dew));

        int q = static_cast<int>(std::lround(e.baroPressureMb));
        if (q <= 0) q = 1013;
        std::snprintf(air, sizeof(air), " %s/%s Q%04d", temp, dew, q);
    }

    const int n = std::snprintf(out, cap, "%s %02u%02uZ%s %04ld%s %s%s",
                                kMetarStation, hour, minute, wind, vis, wxField,
                                clouds, air);
    if (n < 0) return 0;
    return n < static_cast<int>(cap) ? n : static_cast<int>(cap) - 1;
}

} // namespace cigienv
