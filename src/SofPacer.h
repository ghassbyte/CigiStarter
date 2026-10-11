//=============================================================================
//  SofPacer.h — when may the next Start-of-Frame go out?
//
//  Why this exists
//    The HOST is a request/response slave: it answers each SOF with one
//    datagram, and it is the HOST that owns the frame rate. An IG that keeps
//    sending SOFs regardless therefore asks the HOST for more frames than it can
//    produce, and above roughly the HOST's own frame rate that is what used to
//    take the HOST down.
//
//    The reference Master IG never had to consider this: it transmits on a
//    hardware tick and waits for nobody. CigiStarter runs on a PC, so it can be
//    told to hold a SOF back while the previous one is unanswered — the stream
//    then fits whatever the HOST can absorb, whatever rate is typed in the
//    window. That is the flow-control mode, and it is the default.
//
//  The rule
//    - paused — nothing is sent;
//    - free-running (flow control off) — every due frame is sent;
//    - HOST has never answered — every due frame is sent, because the HOST stays
//      silent until it sees a SOF at all, so poking it is the only way to start;
//    - the HOST answered the previous SOF — send;
//    - otherwise the previous SOF is still unanswered — hold this one back,
//      until that SOF has been unanswered for kPokeMs. After that one SOF goes
//      out anyway, so a HOST that has dropped out still receives something to
//      wake up on.
//
//    kPokeMs is a wall-clock limit rather than a count of frames on purpose: it
//    keeps a mistyped rate from turning into a load. With it, a HOST that answers
//    more slowly than one frame per kPokeMs sees at most ~10 requests a second,
//    whether the rate box says 150 or 6000. A count of frames would make the
//    retries scale with the rate, which is exactly the runaway this whole change
//    is about.
//
//    The rate schedule itself (the deadline pacing) stays in the transmit loop:
//    this header only decides whether a due SOF is sent or held back, so the
//    configured rate remains an upper bound on the stream.
//
//  Pure logic, no sockets, no clock: main.cpp's transmit loop owns the timing and
//  the counters, which also makes the rule directly testable.
//=============================================================================
#pragma once

#include <cstdint>

namespace pacer {

// How long an unanswered SOF may sit before the loop sends the next one anyway.
// 100 ms is about ten times the reference HOST's frame interval, so a HOST that
// is merely busy is never poked, while one that has dropped out gets a SOF often
// enough to be woken as soon as it comes back.
constexpr int64_t kPokeMs = 100;

// What the transmit loop knows at the top of a frame.
struct Inputs {
    bool     sending;          // START pressed: the SOF stream is running
    bool     flowControl;      // hold a SOF back while the HOST owes us a reply
    uint64_t replies;          // datagrams received from the HOST so far (cumulative)
    int64_t  msSinceLastSend;  // how long ago the last SOF went out
};

// What the loop has to remember between frames.
struct State {
    uint64_t repliesAtLastSend = 0;  // `replies` as it was when the last SOF went
};

// True when the transmit loop should send a SOF this frame.
inline bool shouldSend(const State& st, const Inputs& in) {
    if (!in.sending) {
        return false;  // paused
    }
    if (!in.flowControl) {
        return true;   // free-running: the rate schedule alone decides
    }
    if (in.replies == 0) {
        return true;   // nothing has ever answered: poke until the HOST wakes up
    }
    if (in.replies != st.repliesAtLastSend) {
        return true;   // the HOST answered the previous SOF
    }
    return in.msSinceLastSend >= kPokeMs;  // unanswered too long: send one anyway
}

// Call after a SOF has been transmitted.
inline void noteSent(State& st, const Inputs& in) {
    st.repliesAtLastSend = in.replies;
}

} // namespace pacer
