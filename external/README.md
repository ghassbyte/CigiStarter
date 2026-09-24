# external

Vendored copies of the build-time SDKs, so a checkout is self-contained and a
dependency never has to be hunted down elsewhere. The same SDKs also live beside
`CigiP3dBridge`; these are copies.

| Folder | Used by | Notes |
|---|---|---|
| `UIPC64_SDK_C_version2\` | `CigiStarter.vcxproj` | FSUIPC SDK — the prebuilt v140 `FSUIPCuser64.lib` and `FSUIPC_User64.h` for the aircraft-position and terrain fallback. |
| `npcap-sdk-1.16\` | `CigiStarter.vcxproj` (`Relay.cpp`) | Npcap headers and x64 import libs for the fan-out. `wpcap.dll` itself is *not* here: it is loaded at run time, so the IG PC needs the Npcap runtime installed. |
| `Prepar3D-SDK\inc\PDK\` | `tools\PdkTerrainProbe\PdkTerrainProbe.vcxproj` | The Prepar3D PDK headers only. The add-on links system libraries alone, so a full (326 MB) SDK installation is not needed to build it. |

Each project defaults to its copy here, and takes an override on the command
line: `/p:FsuipcSdkDir=`, `/p:NpcapSdkDir=` or `/p:P3DSdkDir=`.
