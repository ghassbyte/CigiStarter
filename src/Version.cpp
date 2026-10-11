//=============================================================================
//  Version.cpp
//
//  Implementation of Version.h. The only thing of note is BuildInfo.h: it does
//  not exist in the repo, it is generated into the build's intermediate
//  directory by tools\gen_buildinfo.ps1 before the compiler runs (see the
//  PreBuildEvent in CigiStarter.vcxproj), and that directory is on this
//  project's include path. If IntelliSense complains about a missing
//  BuildInfo.h on a fresh clone, build once.
//=============================================================================

#include "Version.h"

#include "BuildInfo.h"   // generated; see the header comment above

#include <string>

namespace cigi {
namespace version {

const char* semver()    { return CIGI_BUILD_SEMVER; }
const char* describe()  { return CIGI_BUILD_DESCRIBE; }
const char* gitHash()   { return CIGI_BUILD_GIT_HASH; }
const char* buildDate() { return CIGI_BUILD_DATE; }
const char* config()    { return CIGI_BUILD_CONFIG; }
bool        gitDirty()  { return CIGI_BUILD_GIT_DIRTY != 0; }

const char* line() {
    // Built once, on first use, and kept for the process's life: the string is
    // handed to the window title and to the --version message box, both of
    // which want a plain const char*.
    static const std::string s = [] {
        std::string out = CIGI_BUILD_DESCRIBE;
        out += " (";
        out += CIGI_BUILD_CONFIG;
        out += ", built ";
        out += CIGI_BUILD_DATE;
        out += ")";
        return out;
    }();
    return s.c_str();
}

} // namespace version
} // namespace cigi
