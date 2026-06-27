# CapsoDriver

CapsoVision CDAS WinUSB driver package, low-level DLL source, MFC sample tool, API header, and command specification workbook.

## Contents

- `CapsoDriver/CapsoDriver/CapsoDriver_winusb`: WinUSB INF/CAT installer files.
- `CapsoDriver/CapsoDriver/CapsoTest_winusb`: Visual Studio solution for `CapsoLLD.dll` and `CapsoTest.exe`.
- `CapsoDriver/CapsoDriver/x32` and `CapsoDriver/CapsoDriver/x64`: prebuilt sample binaries.
- `API_Capso.h`: exported DLL API header.
- `CDAS3 New command_V16_AVISION.xlsx`: command and error-code specification.

## Build

Build with Visual Studio 2022 Build Tools, v143 toolset, Windows SDK, and MFC for v143.

```powershell
MSBuild.exe CapsoDriver\CapsoDriver\CapsoTest_winusb\CapsoTest.sln /p:Configuration=Release /p:Platform=x64 /m
```
