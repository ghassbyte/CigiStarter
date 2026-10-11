//=============================================================================
//  Version.h
//
//  The build's own identity, so the running exe can answer "which build is
//  this, exactly?" -- on the bench, on the IG PC, and in the window title,
//  without opening the repo or a debugger.
//
//  The values are NOT written here. They are generated at build time into a
//  header (BuildInfo.h) by tools\gen_buildinfo.ps1, from `git describe`, and
//  compiled into src\Version.cpp. That is deliberate: a version typed by hand
//  drifts the moment someone forgets to bump it, which is the very problem
//  this module exists to solve. Git is the single source of truth; the build
//  simply records what it saw.
//
//  What the numbers mean
//  ---------------------
//    semver()     the x.y.z part of the nearest tag ("1.0.0"), or "0.0.0"
//                 when the commit carries no tag anywhere in its history.
//    describe()   the full `git describe` string -- "v1.0.0" exactly on a
//                 tagged commit, "v1.0.0-3-g31d8f4b" three commits past it,
//                 a bare short hash with no tags at all, and a trailing
//                 "-dirty" when the working tree had uncommitted edits.
//    gitHash()    the abbreviated commit hash the build came from.
//    gitDirty()   true when the build was made from a dirty tree.
//    buildDate()  when the build ran ("YYYY-MM-DD HH:MM", local time).
//    config()     "Debug" or "Release".
//
//  Only src\main.cpp and src\Gui.cpp read this: --version, and the operator
//  window's title. It is not a policy header the offline harnesses compile: it
//  reports, it decides nothing.
//=============================================================================
#pragma once

namespace cigi {
namespace version {

// "1.0.0" (the tag's x.y.z), or "0.0.0" if the commit is untagged.
const char* semver();

// The full `git describe` string; see the header comment for its shapes.
const char* describe();

// The abbreviated commit hash, e.g. "31d8f4b".
const char* gitHash();

// True when the build was made from a working tree with uncommitted edits.
bool gitDirty();

// When the build ran: "YYYY-MM-DD HH:MM" (local).
const char* buildDate();

// "Debug" or "Release".
const char* config();

// One human line naming the build, without the program name:
//   v1.0.0-3-g31d8f4b (Release, built 2026-10-11 01:05)
const char* line();

} // namespace version
} // namespace cigi
