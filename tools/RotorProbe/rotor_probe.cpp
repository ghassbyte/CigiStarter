// ---------------------------------------------------------------------------
//  tools\RotorProbe\rotor_probe.cpp
//
//  A standalone bench probe for the questions the offline suite cannot answer
//  about the AS332 (Nemeth Designs Eurocopter AS332 MK2) running as Prepar3D's
//  user aircraft:
//
//    * does its stock rotor keep turning while the sim is paused, and while
//      slew is engaged?
//    * which of ROTOR RPM PCT:1 / ROTOR ROTATION ANGLE:1 / ENG ROTOR RPM:1
//      actually move?
//    * do the standard LIGHT NAV / BEACON / LANDING vars drive the model's
//      exterior lights, and can they be written from outside?
//
//  It links SimConnect only, takes no CIGI, needs no elevation, and belongs to
//  no solution. Run it with Prepar3D v6 already up.
//
//  Every var is added to one data definition as FLOAT64, so a name the sim
//  does not know is simply dropped and reported rather than shifting the
//  buffer and corrupting every field after it (the classic SimConnect trap).
//  TITLE is read on its own definition for the same reason.
//
//  Build with `tools\rotor-probe.ps1`. Run:
//
//    rotor_probe report                one full state read-out
//    rotor_probe watch [secs]          a per-sample trace
//    rotor_probe test                  the pause / slew experiment
//    rotor_probe event PAUSE_ON        transmit any named key event
//    rotor_probe start                 engine auto-start, then watch
//    rotor_probe lights on|off         set NAV/BEACON/LANDING and read back
//    rotor_probe set "LIGHT NAV" 1     set any single var
// ---------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SimConnect.h>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
//  Definitions and requests
// ---------------------------------------------------------------------------

enum : SIMCONNECT_DATA_DEFINITION_ID { DEF_STATE = 1, DEF_TITLE = 2 };
enum : SIMCONNECT_DATA_REQUEST_ID    { REQ_STATE = 100, REQ_TITLE = 101 };

struct VarSpec { const char* name; const char* unit; };

// Every numeric var we try to read. Each is registered as FLOAT64, so the
// received buffer is simply one double per active var in this order.
const VarSpec kVarSpecs[] = {
    { "ROTOR RPM PCT:1",              "percent"  },
    { "ROTOR ROTATION ANGLE:1",       "degrees"  },
    { "ENG ROTOR RPM:1",              "rpm"      },
    { "GENERAL ENG RPM:1",            "rpm"      },
    { "GENERAL ENG COMBUSTION:1",     "bool"     },
    { "GENERAL ENG COMBUSTION:2",     "bool"     },
    { "SIM ON GROUND",                "bool"     },
    { "ABSOLUTE TIME",                "seconds"  },
    { "PLANE LATITUDE",               "degrees"  },
    { "PLANE LONGITUDE",              "degrees"  },
    { "PLANE ALTITUDE",               "feet"     },
    { "PLANE HEADING DEGREES TRUE",   "degrees"  },
    { "LIGHT NAV",                    "bool"     },
    { "LIGHT BEACON",                 "bool"     },
    { "LIGHT LANDING",                "bool"     },
    { "LIGHT STROBE",                 "bool"     },
    { "LIGHT TAXI",                   "bool"     },
    { "LIGHT PANEL",                  "bool"     },
    { "LIGHT CABIN",                  "bool"     },
    { "LIGHT RECOGNITION",            "bool"     },
    { "DISK PITCH PCT:1",             "percent"  },
    { "DISK BANK PCT:1",              "percent"  },
    { "DISK CONING PCT:1",            "percent"  },
    { "KOHLSMAN SETTING MB",          "millibars"},
};

HANDLE g_h = nullptr;
bool   g_quit = false;
bool   g_verbose = false;          // print every sample as it arrives
bool   g_haveSample = false;
double g_startedAt = 0.0;          // wall clock at connect, for t=+N.NNs

std::vector<VarSpec>    g_active;  // vars the sim accepted, in registration order
std::vector<double>     g_values;
std::string             g_title;

double nowSeconds() {
    static const auto t0 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

// Value of an active var, or a NAN-less sentinel if it was not registered.
bool getVar(const char* name, double& out) {
    for (size_t i = 0; i < g_active.size(); ++i) {
        if (std::strcmp(g_active[i].name, name) == 0) {
            if (i >= g_values.size()) return false;
            out = g_values[i];
            return true;
        }
    }
    return false;
}

double getVarOr(const char* name, double fallback) {
    double v = fallback;
    getVar(name, v);
    return v;
}

void printfException(DWORD code) {
    const char* text = nullptr;
    switch (code) {
        case SIMCONNECT_EXCEPTION_NONE:                       text = "NONE"; break;
        case SIMCONNECT_EXCEPTION_ERROR:                      text = "ERROR"; break;
        case SIMCONNECT_EXCEPTION_SIZE_MISMATCH:              text = "SIZE_MISMATCH"; break;
        case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:            text = "UNRECOGNIZED_ID"; break;
        case SIMCONNECT_EXCEPTION_UNOPENED:                   text = "UNOPENED"; break;
        case SIMCONNECT_EXCEPTION_VERSION_MISMATCH:           text = "VERSION_MISMATCH"; break;
        case SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS:            text = "TOO_MANY_GROUPS"; break;
        case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:          text = "NAME_UNRECOGNIZED"; break;
        case SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES:       text = "TOO_MANY_EVENT_NAMES"; break;
        case SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE:         text = "EVENT_ID_DUPLICATE"; break;
        case SIMCONNECT_EXCEPTION_TOO_MANY_MAPS:              text = "TOO_MANY_MAPS"; break;
        case SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS:           text = "TOO_MANY_OBJECTS"; break;
        case SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS:          text = "TOO_MANY_REQUESTS"; break;
        case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:       text = "WEATHER_INVALID_PORT"; break;
        case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:      text = "WEATHER_INVALID_METAR"; break;
        case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:
                                                             text = "WEATHER_UNABLE_TO_GET_OBSERVATION"; break;
        case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:
                                                             text = "WEATHER_UNABLE_TO_CREATE_STATION"; break;
        case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:
                                                             text = "WEATHER_UNABLE_TO_REMOVE_STATION"; break;
        case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:          text = "INVALID_DATA_TYPE"; break;
        case SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE:          text = "INVALID_DATA_SIZE"; break;
        case SIMCONNECT_EXCEPTION_DATA_ERROR:                 text = "DATA_ERROR"; break;
        case SIMCONNECT_EXCEPTION_INVALID_ARRAY:              text = "INVALID_ARRAY"; break;
        case SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED:       text = "CREATE_OBJECT_FAILED"; break;
        case SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED:     text = "LOAD_FLIGHTPLAN_FAILED"; break;
        case SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE:
                                                             text = "OPERATION_INVALID_FOR_OBJECT_TYPE"; break;
        default: break;
    }
    std::printf("  [exception] %lu%s%s\n", (unsigned long)code,
                text ? " " : "", text ? text : "");
}

void CALLBACK dispatch(SIMCONNECT_RECV* recv, DWORD /*cbData*/, void* /*ctx*/) {
    switch (recv->dwID) {
        case SIMCONNECT_RECV_ID_OPEN: {
            auto* open = static_cast<SIMCONNECT_RECV_OPEN*>(recv);
            std::printf("  connected to %s %d.%d.%d.%d\n",
                        open->szApplicationName,
                        open->dwApplicationVersionMajor,
                        open->dwApplicationVersionMinor,
                        open->dwApplicationBuildMajor,
                        open->dwApplicationBuildMinor);
            break;
        }

        case SIMCONNECT_RECV_ID_QUIT:
            std::printf("  the sim has shut down.\n");
            g_quit = true;
            break;

        case SIMCONNECT_RECV_ID_EXCEPTION: {
            auto* ex = static_cast<SIMCONNECT_RECV_EXCEPTION*>(recv);
            printfException(ex->dwException);
            break;
        }

        case SIMCONNECT_RECV_ID_SIMOBJECT_DATA: {
            auto* data = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(recv);
            if (data->dwRequestID == REQ_TITLE) {
                g_title.assign(reinterpret_cast<const char*>(&data->dwData));
                break;
            }
            if (data->dwRequestID != REQ_STATE) break;

            const double* p = reinterpret_cast<const double*>(&data->dwData);
            g_values.assign(p, p + g_active.size());
            g_haveSample = true;

            if (g_verbose) {
                std::printf("  t=+%5.2fs  rotor=%6.1f%%  angle=%9.1f  engRotorRpm=%7.1f"
                            "  absTime=%11.2f  lat=%11.6f  lon=%12.6f  alt=%8.1f  hdg=%5.1f\n",
                            nowSeconds(),
                            getVarOr("ROTOR RPM PCT:1", 0.0),
                            getVarOr("ROTOR ROTATION ANGLE:1", 0.0),
                            getVarOr("ENG ROTOR RPM:1", 0.0),
                            getVarOr("ABSOLUTE TIME", 0.0),
                            getVarOr("PLANE LATITUDE", 0.0),
                            getVarOr("PLANE LONGITUDE", 0.0),
                            getVarOr("PLANE ALTITUDE", 0.0),
                            getVarOr("PLANE HEADING DEGREES TRUE", 0.0));
                std::fflush(stdout);
            }
            break;
        }

        default:
            break;
    }
}

bool pump(double seconds) {
    const auto end = std::chrono::steady_clock::now() +
                     std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                         std::chrono::duration<double>(seconds));
    while (!g_quit && std::chrono::steady_clock::now() < end) {
        SimConnect_CallDispatch(g_h, dispatch, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
    return !g_quit;
}

// ---------------------------------------------------------------------------
//  Connection
// ---------------------------------------------------------------------------

bool connect() {
    HRESULT hr = SimConnect_Open(&g_h, "RotorProbe", nullptr, 0, nullptr, 0);
    if (FAILED(hr) || g_h == nullptr) {
        std::printf("Could not open SimConnect (0x%08lX). Is Prepar3D v6 running?\n",
                    (unsigned long)hr);
        return false;
    }
    g_startedAt = nowSeconds();

    // TITLE on its own definition: a string cannot share the double buffer.
    if (FAILED(SimConnect_AddToDataDefinition(g_h, DEF_TITLE, "TITLE", nullptr,
                                              SIMCONNECT_DATATYPE_STRING256))) {
        std::printf("  note: TITLE could not be defined.\n");
    }
    SimConnect_RequestDataOnSimObject(g_h, REQ_TITLE, DEF_TITLE,
                                      SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_ONCE);
    pump(0.3);

    // Numeric vars, one at a time, keeping only the ones the sim accepted.
    int rejected = 0;
    for (const VarSpec& spec : kVarSpecs) {
        HRESULT vhr = SimConnect_AddToDataDefinition(g_h, DEF_STATE, spec.name, spec.unit,
                                                     SIMCONNECT_DATATYPE_FLOAT64);
        if (FAILED(vhr)) {
            std::printf("  note: \"%s\" (%s) is not known to this sim -- skipped.\n",
                        spec.name, spec.unit);
            ++rejected;
        } else {
            g_active.push_back(spec);
        }
    }
    if (g_active.empty()) {
        std::printf("No usable vars -- nothing to probe.\n");
        return false;
    }
    std::printf("  %d vars accepted, %d rejected.\n", (int)g_active.size(), rejected);
    return true;
}

void disconnect() {
    if (g_h) {
        SimConnect_Close(g_h);
        g_h = nullptr;
    }
}

bool requestStream(SIMCONNECT_PERIOD period) {
    HRESULT hr = SimConnect_RequestDataOnSimObject(g_h, REQ_STATE, DEF_STATE,
                                                   SIMCONNECT_OBJECT_ID_USER, period);
    if (FAILED(hr)) {
        std::printf("  RequestDataOnSimObject failed (0x%08lX).\n", (unsigned long)hr);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
//  Events
// ---------------------------------------------------------------------------

bool transmitEvent(const char* name, DWORD data = 0) {
    static std::map<std::string, int> cache;
    int id = 0;
    auto it = cache.find(name);
    if (it == cache.end()) {
        id = 1000 + (int)cache.size();
        HRESULT hr = SimConnect_MapClientEventToSimEvent(g_h, id, name);
        if (FAILED(hr)) {
            std::printf("  \"%s\": MapClientEventToSimEvent failed (0x%08lX).\n",
                        name, (unsigned long)hr);
            return false;
        }
        cache[name] = id;
    } else {
        id = it->second;
    }

    HRESULT hr = SimConnect_TransmitClientEvent(g_h, SIMCONNECT_OBJECT_ID_USER, id, data,
                                                SIMCONNECT_GROUP_PRIORITY_HIGHEST,
                                                SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
    if (FAILED(hr)) {
        std::printf("  \"%s\": TransmitClientEvent failed (0x%08lX).\n",
                    name, (unsigned long)hr);
        return false;
    }
    std::printf("  %-24s -> sent\n", name);
    std::fflush(stdout);
    return true;
}

// ---------------------------------------------------------------------------
//  Writing
// ---------------------------------------------------------------------------

// Each call gets its own data definition rather than reusing one and clearing
// it in between: clearing made Prepar3D answer a perfectly writable var with
// UNRECOGNIZED_ID, which is indistinguishable from a read-only one.  The two
// must not be confused, so nothing here clears anything.
bool setVar(const char* name, const char* unit, double value) {
    static int nextDefine = 200;
    const SIMCONNECT_DATA_DEFINITION_ID def =
        static_cast<SIMCONNECT_DATA_DEFINITION_ID>(nextDefine++);

    HRESULT hr = SimConnect_AddToDataDefinition(g_h, def, name, unit,
                                                SIMCONNECT_DATATYPE_FLOAT64);
    if (FAILED(hr)) {
        std::printf("  \"%s\": AddToDataDefinition failed (0x%08lX).\n",
                    name, (unsigned long)hr);
        return false;
    }
    hr = SimConnect_SetDataOnSimObject(g_h, def, SIMCONNECT_OBJECT_ID_USER, 0, 0,
                                       sizeof(double), &value);
    if (FAILED(hr)) {
        std::printf("  SetDataOnSimObject(\"%s\") failed (0x%08lX).\n",
                    name, (unsigned long)hr);
        return false;
    }
    std::printf("  %-24s <- %.2f\n", name, value);
    return true;
}

// ---------------------------------------------------------------------------
//  Reporting
// ---------------------------------------------------------------------------

struct Snapshot {
    double time = 0.0;
    double rotorPct = 0.0;
    double angle = 0.0;
    double engRotorRpm = 0.0;
    double absTime = 0.0;
    double lat = 0.0, lon = 0.0, alt = 0.0, hdg = 0.0;
};

Snapshot snapshot() {
    Snapshot s;
    s.time        = nowSeconds();
    s.rotorPct    = getVarOr("ROTOR RPM PCT:1", 0.0);
    s.angle       = getVarOr("ROTOR ROTATION ANGLE:1", 0.0);
    s.engRotorRpm = getVarOr("ENG ROTOR RPM:1", 0.0);
    s.absTime     = getVarOr("ABSOLUTE TIME", 0.0);
    s.lat         = getVarOr("PLANE LATITUDE", 0.0);
    s.lon         = getVarOr("PLANE LONGITUDE", 0.0);
    s.alt         = getVarOr("PLANE ALTITUDE", 0.0);
    s.hdg         = getVarOr("PLANE HEADING DEGREES TRUE", 0.0);
    return s;
}

void printState() {
    std::printf("  aircraft        : %s\n", g_title.empty() ? "(not reported)" : g_title.c_str());
    std::printf("  rotor %%=%6.1f  angle=%9.1f deg  engRotorRpm=%7.1f  engRpm=%7.1f\n",
                getVarOr("ROTOR RPM PCT:1", 0.0),
                getVarOr("ROTOR ROTATION ANGLE:1", 0.0),
                getVarOr("ENG ROTOR RPM:1", 0.0),
                getVarOr("GENERAL ENG RPM:1", 0.0));
    std::printf("  eng1=%d eng2=%d  onGround=%d  absTime=%.2f\n",
                (int)(getVarOr("GENERAL ENG COMBUSTION:1", 0.0) != 0.0),
                (int)(getVarOr("GENERAL ENG COMBUSTION:2", 0.0) != 0.0),
                (int)(getVarOr("SIM ON GROUND", 0.0) != 0.0),
                getVarOr("ABSOLUTE TIME", 0.0));
    std::printf("  pos %.6f, %.6f  %.1f ft  hdg %.1f\n",
                getVarOr("PLANE LATITUDE", 0.0), getVarOr("PLANE LONGITUDE", 0.0),
                getVarOr("PLANE ALTITUDE", 0.0), getVarOr("PLANE HEADING DEGREES TRUE", 0.0));
    std::printf("  lights nav=%d beacon=%d landing=%d strobe=%d taxi=%d panel=%d cabin=%d recog=%d\n",
                (int)(getVarOr("LIGHT NAV", 0.0) != 0.0),
                (int)(getVarOr("LIGHT BEACON", 0.0) != 0.0),
                (int)(getVarOr("LIGHT LANDING", 0.0) != 0.0),
                (int)(getVarOr("LIGHT STROBE", 0.0) != 0.0),
                (int)(getVarOr("LIGHT TAXI", 0.0) != 0.0),
                (int)(getVarOr("LIGHT PANEL", 0.0) != 0.0),
                (int)(getVarOr("LIGHT CABIN", 0.0) != 0.0),
                (int)(getVarOr("LIGHT RECOGNITION", 0.0) != 0.0));
}

// One phase of the experiment: sample for `seconds` and report what moved.
struct PhaseResult {
    int    samples = 0;
    double dAngle = 0.0, dRotorPct = 0.0, dAbsTime = 0.0, dPos = 0.0;
    double maxStep = 0.0;          // largest change between two adjacent samples
    bool   rotorTurning = false;
    bool   clockRunning = false;
    Snapshot first, last;
};

PhaseResult runPhase(const char* label, double seconds) {
    PhaseResult r;
    g_haveSample = false;
    g_verbose = false;

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                              std::chrono::duration<double>(seconds));
    bool haveFirst = false, havePrev = false;
    double prevAngle = 0.0;
    while (!g_quit && std::chrono::steady_clock::now() < deadline) {
        SimConnect_CallDispatch(g_h, dispatch, nullptr);
        if (g_haveSample) {
            g_haveSample = false;
            Snapshot s = snapshot();
            if (!haveFirst) { r.first = s; haveFirst = true; }
            if (havePrev) {
                const double d = std::fabs(s.angle - prevAngle);
                if (d > r.maxStep) { r.maxStep = d; }
            }
            prevAngle = s.angle;
            havePrev = true;
            r.last = s;
            ++r.samples;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }

    std::printf("  %-26s samples=%-4d", label, r.samples);
    if (r.samples >= 2) {
        r.dAngle    = r.last.angle - r.first.angle;
        r.dRotorPct = r.last.rotorPct - r.first.rotorPct;
        r.dAbsTime  = r.last.absTime - r.first.absTime;
        // Rough ground distance across the phase, in feet.
        const double dLatFt = (r.last.lat - r.first.lat) * 364000.0;
        const double dLonFt = (r.last.lon - r.first.lon) * 364000.0 * 0.62;
        r.dPos = std::sqrt(dLatFt * dLatFt + dLonFt * dLonFt);
        // maxStep, not the net difference: a rotor aliasing against a 16 Hz
        // sample can land back where it started, and a frozen one never moves
        // at all.  The largest single-sample jump separates the two cleanly.
        r.rotorTurning = r.maxStep > 5.0;
        r.clockRunning = std::fabs(r.dAbsTime) > 0.05;
        std::printf("maxStep=%9.1f  dAbsTime=%6.2f  moved=%6.1fft  "
                    "rotor=%s  clock=%s\n",
                    r.maxStep, r.dAbsTime, r.dPos,
                    r.rotorTurning ? "TURNING" : "still  ",
                    r.clockRunning ? "running" : "FROZEN");
    } else {
        std::printf("(too few samples to judge)\n");
    }
    std::fflush(stdout);
    return r;
}

// ---------------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------------

int cmdReport() {
    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    pump(1.5);
    std::printf("\nState:\n");
    printState();
    return 0;
}

int cmdWatch(double seconds) {
    std::printf("Watching for %.1f s (Ctrl+C to stop).\n", seconds);
    std::printf("  t=+  0.00s  rotor=   ...%%  angle=      ...  engRotorRpm=    ..."
                "  absTime=      ...  lat=     ...  lon=      ...  alt=   ...  hdg=  ...\n");
    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    g_verbose = true;
    pump(seconds);
    g_verbose = false;
    return 0;
}

int cmdStart(double watchSeconds) {
    std::printf("\nSending ENGINE_AUTO_START (the Ctrl+E event)...\n");
    transmitEvent("ENGINE_AUTO_START");
    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    pump(1.0);
    std::printf("\nAfter 1 s:\n");
    printState();
    std::printf("\nWatching the spin-up for %.1f s:\n", watchSeconds);
    g_verbose = true;
    pump(watchSeconds);
    g_verbose = false;
    std::printf("\nFinal:\n");
    printState();
    return 0;
}

int cmdTest() {
    std::printf("\n=== Rotor vs pause and slew ===\n");
    std::printf("The rig pauses Prepar3D so the HOST can own the pose. This asks what that\n");
    std::printf("does to the stock rotor, and what slew does instead.\n\n");

    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    pump(0.5);

    const double phase = 3.0;

    std::printf("Phase A -- baseline (no holds)\n");
    PhaseResult a = runPhase("baseline", phase);
    if (!a.rotorTurning) {
        std::printf("\nThe rotor is not turning, so this experiment cannot say anything yet.\n");
        std::printf("Run `rotor_probe start` to bring the aircraft up, then re-run `test`.\n");
        printState();
        return 2;
    }

    std::printf("\nPhase B -- PAUSE_ON\n");
    transmitEvent("PAUSE_ON");
    PhaseResult b = runPhase("paused", phase);

    std::printf("\nPhase C -- PAUSE_OFF\n");
    transmitEvent("PAUSE_OFF");
    runPhase("unpaused", phase);

    std::printf("\nPhase D -- SLEW_ON\n");
    transmitEvent("SLEW_ON");
    PhaseResult d = runPhase("slew on", phase);

    std::printf("\nPhase E -- SLEW_OFF\n");
    transmitEvent("SLEW_OFF");
    runPhase("slew off", phase);

    std::printf("\nPhase F -- PAUSE_ON + SLEW_ON (what the rig would use)\n");
    transmitEvent("PAUSE_ON");
    transmitEvent("SLEW_ON");
    PhaseResult f = runPhase("paused + slew", phase);

    std::printf("\nPhase G -- restore (PAUSE_OFF + SLEW_OFF)\n");
    transmitEvent("PAUSE_OFF");
    transmitEvent("SLEW_OFF");
    runPhase("restored", phase);

    std::printf("\n=== What this says ===\n");
    std::printf("  pause holds rotor   : %s\n", (!b.rotorTurning && b.samples >= 2) ? "yes" : "no ");
    std::printf("  pause holds clock   : %s\n", (!b.clockRunning && b.samples >= 2) ? "yes" : "no ");
    std::printf("  slew  holds rotor   : %s\n", (!d.rotorTurning && d.samples >= 2) ? "yes" : "no ");
    std::printf("  slew  holds clock   : %s\n", (!d.clockRunning && d.samples >= 2) ? "yes" : "no ");
    std::printf("  pause+slew holds it : %s\n", (!f.rotorTurning && f.samples >= 2) ? "yes" : "no ");
    std::printf("  samples while paused: %d (0 would mean SimConnect stops answering)\n", b.samples);
    std::fflush(stdout);
    return 0;
}

// The model's own panel drives these lights with key events rather than by
// writing the vars: aircraft.cfg carries no [LIGHTS] points for them, so the
// visible glow is MDL geometry P3D lights from A:LIGHT NAV/BEACON/LANDING --
// which the sim makes read-only (see `set`).  Events are the way in, and these
// are the events ND_AS332!systems itself sends.
int cmdLights(const std::string& how) {
    const bool on = (how != "off");
    std::printf("\nSending the %s light events...\n", on ? "ON" : "OFF");
    transmitEvent(on ? "NAV_LIGHTS_ON"     : "NAV_LIGHTS_OFF");
    transmitEvent(on ? "BEACON_LIGHTS_ON"  : "BEACON_LIGHTS_OFF");
    transmitEvent(on ? "LANDING_LIGHTS_ON" : "LANDING_LIGHTS_OFF");

    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    pump(1.0);
    std::printf("\nRead back:\n");
    printState();
    return 0;
}

// Can we drive the rotor ourselves?  While paused the sim is not updating the
// rotor, which is the only interval where a write can be told apart from the
// FDM immediately overwriting it.
int cmdWriteTest() {
    std::printf("\n=== Which rotor vars will the sim let us write? ===\n");
    requestStream(SIMCONNECT_PERIOD_VISUAL_FRAME);
    pump(0.4);

    // Control first: a var that is definitely writable.  Without this, a
    // refused write says nothing about the var -- only about our own plumbing.
    const double kohlsmanBefore = getVarOr("KOHLSMAN SETTING MB", 0.0);
    setVar("KOHLSMAN SETTING MB", "millibars", 1013.25);
    pump(0.5);
    const double kohlsmanAfter = getVarOr("KOHLSMAN SETTING MB", 0.0);
    const bool pathWorks = std::fabs(kohlsmanAfter - 1013.25) < 0.5;
    std::printf("  control: KOHLSMAN SETTING MB %.2f -> %.2f : %s\n",
                kohlsmanBefore, kohlsmanAfter,
                pathWorks ? "the write path works" : "THE WRITE PATH IS BROKEN");
    if (pathWorks && kohlsmanBefore > 0.0) {
        setVar("KOHLSMAN SETTING MB", "millibars", kohlsmanBefore);
        pump(0.3);
    }

    struct Cand { const char* name; const char* unit; double delta; };
    const Cand cands[] = {
        { "ROTOR ROTATION ANGLE:1", "degrees", 90.0 },   // the spin itself
        { "ROTOR RPM PCT:1",        "percent", 25.0 },   // picks still/slow/blurred
        { "DISK PITCH PCT:1",       "percent", 20.0 },   // rotor_pitch
        { "DISK BANK PCT:1",        "percent", 20.0 },   // rotor_bank
    };

    std::printf("\n  live : angle=%9.1f  rotorPct=%6.1f\n",
                getVarOr("ROTOR ROTATION ANGLE:1", 0.0),
                getVarOr("ROTOR RPM PCT:1", 0.0));

    transmitEvent("PAUSE_ON");
    pump(0.6);
    std::printf("  paused (the sim is no longer writing any of these)\n");

    int writable = 0;
    for (const Cand& c : cands) {
        const double v0 = getVarOr(c.name, 0.0);
        const double target = v0 + c.delta;
        setVar(c.name, c.unit, target);
        pump(0.5);
        const double v1 = getVarOr(c.name, 0.0);
        const bool took = std::fabs(v1 - v0) > 0.5;
        if (took) { ++writable; }
        std::printf("    %-24s %9.2f -> wrote %9.2f -> read %9.2f : %s\n",
                    c.name, v0, target, v1, took ? "WRITABLE" : "read-only");
        if (took) {
            setVar(c.name, c.unit, v0);
            pump(0.3);
        }
    }

    transmitEvent("PAUSE_OFF");
    pump(0.4);
    std::printf("  resumed: angle=%9.1f  rotorPct=%6.1f\n",
                getVarOr("ROTOR ROTATION ANGLE:1", 0.0),
                getVarOr("ROTOR RPM PCT:1", 0.0));

    if (!pathWorks) {
        std::printf("\n  The control failed, so this said nothing about the rotor.\n");
    } else if (writable == 0) {
        std::printf("\n  Every rotor var is read-only: the animation is entirely the flight\n"
                    "  model's, so a live rotor needs a live model -- unpaused and out of slew.\n");
    } else {
        std::printf("\n  %d of %d rotor vars are writable, so the IG can shape the rotor\n"
                    "  without a live flight model.\n", writable, (int)(sizeof(cands)/sizeof(cands[0])));
    }
    std::fflush(stdout);
    return (pathWorks && writable > 0) ? 0 : 1;
}

void usage() {
    std::printf(
        "rotor_probe -- a SimConnect bench probe for the AS332's rotor and lights\n"
        "\n"
        "  report                    one full state read-out\n"
        "  watch [secs]              a per-sample trace (default 10)\n"
        "  test                      the pause / slew experiment\n"
        "  start [secs]              engine auto-start, then watch (default 25)\n"
        "  event NAME [data]         transmit any named key event\n"
        "  writetest                 which rotor vars will the sim let us write?\n"
        "  lights on|off             send the NAV/BEACON/LANDING light events\n"
        "  set \"VAR\" value [unit]    set any single var\n"
        "\n"
        "Prepar3D v6 must already be running.\n");
}

} // namespace

int main(int argc, char** argv) {
    const std::string cmd = (argc > 1) ? argv[1] : "report";

    if (cmd == "-h" || cmd == "--help" || cmd == "help") { usage(); return 0; }

    if (!connect()) return 1;

    int rc = 0;
    if (cmd == "report") {
        rc = cmdReport();
    } else if (cmd == "watch") {
        rc = cmdWatch(argc > 2 ? std::atof(argv[2]) : 10.0);
    } else if (cmd == "test") {
        rc = cmdTest();
    } else if (cmd == "start") {
        rc = cmdStart(argc > 2 ? std::atof(argv[2]) : 25.0);
    } else if (cmd == "event") {
        if (argc < 3) { usage(); rc = 1; }
        else rc = transmitEvent(argv[2], argc > 3 ? (DWORD)std::atol(argv[3]) : 0) ? 0 : 1;
    } else if (cmd == "lights") {
        rc = cmdLights(argc > 2 ? argv[2] : "on");
    } else if (cmd == "writetest") {
        rc = cmdWriteTest();
    } else if (cmd == "set") {
        if (argc < 4) { usage(); rc = 1; }
        else rc = setVar(argv[2], argc > 4 ? argv[4] : "number", std::atof(argv[3])) ? 0 : 1;
    } else {
        std::printf("Unknown command \"%s\".\n\n", cmd.c_str());
        usage();
        rc = 1;
    }

    disconnect();
    return rc;
}
