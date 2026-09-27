//=============================================================================
//  AltitudeDatum.h — the rule behind --alt-datum-offset
//
//  Why this exists
//    The HOST's Entity Control altitude is expressed in the HOST's terrain
//    database; the Prepar3D on an IG PC holds the same aircraft in P3D's. At one
//    airfield the two disagree by about a metre. A driver that writes the HOST's
//    number straight through therefore parks its aircraft *off P3D's own
//    surface*, and P3D's ground model then pushes back at it every frame. That
//    fight, not the write, is the shake a parked aircraft shows.
//
//    On this IG the driver is somebody else's program (see the "Sim driver" row
//    in agents.md), so CigiStarter cannot correct the write. What it can do is
//    correct what the extra IG PCs are told: --alt-datum-offset measures the gap
//    here, once, and adds it to every Entity 0 altitude in the fan-out copies
//    (Relay.h), so each client IG PC's own driver lands its aircraft on its own
//    P3D surface instead of on the HOST's terrain database.
//
//  The measurement, in one sentence
//    Prepar3D only rests the aircraft at its own altitude while it is *holding*
//    the aircraft on the ground, so the reading is taken then -- the sim's
//    weight-on-wheels flag (0x0366) is the gate -- and the gap is that resting
//    altitude minus the altitude the HOST commands. The rule itself is here,
//    dependency-free, so it is one place rather than a line inside a thread; the
//    reading, the gate and the window's reporting stay in main.cpp.
//
//  Refusals matter as much as the offset
//    A wrong offset *moves* every client's aircraft. A reading that cannot be a
//    datum gap -- not a real number, or past kMaxOffsetM -- must therefore leave
//    the copies alone rather than apply itself. Started with the aircraft
//    somewhere unexpected, the correct behaviour is to refuse and wait.
//
//  The same rule, and the same 20 m, as CigiP3dSimConnect's src\AltitudeDatum.h,
//  which applies it to the write itself. The two are deliberately identical, so
//  that a driver and a fan-out fed by the same stream agree; if one is ever
//  changed, change the other.
//=============================================================================
#pragma once

namespace datum {

// A terrain-datum gap at one airfield is metres. Anything past this is a
// reading taken while the aircraft was somewhere unexpected, not a datum
// difference -- 20 m is two orders of magnitude above the measured values and
// still far below any real terrain disagreement the HOST could mean.
constexpr double kMaxOffsetM = 20.0;

// What --alt-datum-offset has managed to do. The window shows this, and it is
// the difference between "not asked for", "asked for, still waiting for a
// reading" and "measured". Applied is final for the run: the measurement is
// taken once, at the field the aircraft is parked on when CigiStarter starts.
enum class State : int {
    Off = 0,    // the option was not given; the copies are byte for byte
    Waiting,    // given, but no usable reading yet (no sim, or no on-ground sample)
    Applied,    // measured; offsetM is what the copies carry
    Refused,    // a reading arrived but cannot be a datum gap; retrying
};

enum class Verdict {
    Applied,      // a usable gap: offsetM is what to add to every altitude
    NoReading,    // nothing to compare yet, so nothing was measured
    Implausible,  // a reading, but not one that can be a datum gap
};

struct Decision {
    Verdict verdict = Verdict::NoReading;
    double  offsetM = 0.0;
};

// Compare the altitude the sim is resting the aircraft at -- read while the
// sim's own flag says the aircraft is on the ground, the only time it is
// holding the aircraft rather than following it -- against the altitude the
// HOST commands, and say what to add to every altitude in the copies.
//
// Zero is a perfectly good answer: it means the two datums agree. The caller
// must go by the verdict rather than by `offsetM != 0`, or it would re-measure
// on every sample.
//
// A non-finite `restM` and a gap past +/-kMaxOffsetM are both Implausible, and
// both leave offsetM at 0 so the copies keep the HOST's altitude -- the safe
// reading of "I do not know where that aircraft is".
inline Decision decideOffset(double restM, double commandedM) {
    const double gap = restM - commandedM;

    Decision d;
    // `gap == gap` is false only for NaN; the comparisons also reject +/-inf,
    // so one test covers every reading that is not a real number.
    if (!(gap == gap) || gap > kMaxOffsetM || gap < -kMaxOffsetM) {
        d.verdict = Verdict::Implausible;
        return d;
    }
    d.verdict = Verdict::Applied;
    d.offsetM = gap;
    return d;
}

} // namespace datum
