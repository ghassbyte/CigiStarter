# external

Vendored third-party copies, so a checkout is self-contained and nothing has to
be hunted down elsewhere. Two kinds live here: the SDKs the builds link against,
and the CIGI Host Emulator the bench tests drive.

| Folder | Used by | Notes |
|---|---|---|
| `UIPC64_SDK_C_version2\` | `CigiStarter.vcxproj` | FSUIPC SDK — the prebuilt v140 `FSUIPCuser64.lib` and `FSUIPC_User64.h` for the aircraft-position and terrain fallback. |
| `npcap-sdk-1.16\` | `CigiStarter.vcxproj` (`Relay.cpp`) | Npcap headers and x64 import libs for the fan-out. `wpcap.dll` itself is *not* here: it is loaded at run time, so the IG PC needs the Npcap runtime installed. |
| `Prepar3D-SDK\inc\PDK\` | `tools\PdkTerrainProbe\PdkTerrainProbe.vcxproj` | The Prepar3D PDK headers only, from the v6 SDK — the project's default, so a bare build makes the v6 DLL. The add-on links system libraries alone, so a full (326 MB) SDK installation is not needed; a v5 build points `/p:P3DSdkDir` at the v5 SDK (see the probe's `README.md`). |
| `HostEmulator_Win32_v3.3.3\` | `tools\test-hathot-emulator.ps1` | The CIGI Host Emulator 3.3.3, the HOST side of a bench link. Nothing links it: it is run as it stands, to drive CigiStarter's HAT/HOT answering and to watch the exchange. GPLv2 — `license.txt` and `Change History.txt` are the distribution's own files, kept as shipped. |

The SDK copies also live beside `CigiP3dBridge`; these are copies. Each project
defaults to its copy here, and takes an override on the command line:
`/p:FsuipcSdkDir=`, `/p:NpcapSdkDir=` or `/p:P3DSdkDir=`.

The Host Emulator is a *bench* tool, not a build input: it is a Windows program
(Boeing TSS's `Hemu3.exe` plus its driver `HemuDrv.exe`) that plays a CIGI HOST
against CigiStarter on a single PC. It keeps its link setup in the registry
(`HKCU\Software\Boeing TSS\CIGI Host Emulator 3`), and its scenario scripts and
`.def` tables are read from the folder it runs in. `README.md`, at the top of
the tree, has the bench procedure it is used in.
