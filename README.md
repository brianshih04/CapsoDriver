# CapsoDriver

CapsoDriver contains the CapsoVision CDAS WinUSB driver package, the low-level
DLL source code, an MFC sample application, prebuilt binaries, the public API
header, and the CDAS3 command specification workbook.

## Repository Contents

- `API_Capso.h`  
  Public header for the exported `CapsoLLD.dll` API.

- `CDAS3 New command_V16_AVISION.xlsx`  
  CDAS3 command specification, including command layouts and device error-code
  definitions.

- `CapsoDriver.rar`  
  Original packaged archive.

- `CapsoDriver/CapsoDriver/CapsoDriver_winusb`  
  WinUSB driver installation package:
  - `AVCapso.inf`
  - `avcapso.cat`
  - `InstallDev.bat`
  - `RemoveDev.bat`

- `CapsoDriver/CapsoDriver/CapsoTest_winusb`  
  Visual Studio solution containing:
  - `CapsoLLD`: low-level WinUSB DLL source.
  - `CapsoTest`: MFC dialog sample application.

- `CapsoDriver/CapsoDriver/x32` and `CapsoDriver/CapsoDriver/x64`  
  Prebuilt sample binaries.

- `CapsoDriver/CapsoDriver/VC_redist.x86.exe` and
  `CapsoDriver/CapsoDriver/VC_redist.x64.exe`  
  Visual C++ runtime installers.

## Architecture

The sample application loads the DLL dynamically:

```text
CapsoTest.exe
  -> LoadLibrary("CapsoLLD.dll")
    -> AVCapso_* exported APIs
      -> SetupAPI device enumeration
      -> WinUSB bulk read/write
        -> CDAS USB device
```

The WinUSB INF binds the device:

```text
USB\VID_0638&PID_0931
```

The registered device interface GUID is:

```text
{CC0EDA17-920D-4F8B-A011-7240658BF2F9}
```

## Exported DLL API

The public API is declared in `API_Capso.h`.

```cpp
BOOL AVCapso_Initialize();
BOOL AVCapso_Terminate();
BOOL AVCapso_DeviceList(DEVICELIST* pDeviceList);
BOOL AVCapso_IOLock(char* szDockingSystemSN);
BOOL AVCapso_IOUnlock(char* szDockingSystemSN);
BOOL AVCapso_Read(char* szDockingSystemSN, LPVOID lpBuffer, DWORD size,
                  LPDWORD transferred, LPOVERLAPPED overlapped);
BOOL AVCapso_Write(char* szDockingSystemSN, LPCVOID lpBuffer, DWORD size,
                   LPDWORD transferred, LPOVERLAPPED overlapped);
```

The API return value is `BOOL`, so it only reports API-level success or failure.
Device command error codes are returned in the command status packet read by
`AVCapso_Read()`.

## Status Packet and Error Codes

According to `CDAS3 New command_V16_AVISION.xlsx`, the 32-byte status packet has
this layout:

```text
Byte 0..3 : "USBS"
Byte 4..7 : Tag
Byte 8    : Error code
Byte 9..31: Command-specific status data
```

The sample application now validates:

1. Status signature: `USBS`
2. Returned tag
3. Error code at byte 8

Known error codes from the workbook:

| Code | Name |
| --- | --- |
| `0x10` | `ERROR_Command` |
| `0x11` | `ERROR_Unknown_Command` |
| `0x20` | `ERROR_ReadPage` |
| `0x21` | `ERROR_Data` |
| `0x22` | `ERROR_Decode` |
| `0x23` | `ERROR_Failed_CRC` |
| `0x24` | `ERROR_FIFO_Overflow` |
| `0x30` | `ERROR_WritePage` |
| `0x31` | `ERROR_EraseBlock` |
| `0x40` | `ERROR_StrongSignal` |
| `0x41` | `ERROR_WeakSignal` |
| `0x42` | `ERROR_NoCapsule` |
| `0x50` | `ERROR_OverCurrent` |
| `0x51` | `ERROR_CoverOpen` |
| `0x70` | `ERROR_UpdateFx2` |
| `0x71` | `ERROR_UpdateFPGA` |

## Build Requirements

- Windows
- Visual Studio 2022 Build Tools
- MSVC v143 toolset
- Windows SDK
- MFC for v143 build tools

The sample application uses MFC, so MFC is required to build `CapsoTest.exe`.
`CapsoLLD.dll` itself does not use MFC.

## Build Command

From the repository root:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' `
  'CapsoDriver\CapsoDriver\CapsoTest_winusb\CapsoTest.sln' `
  /p:Configuration=Release `
  /p:Platform=x64 `
  /m
```

Expected output:

```text
CapsoDriver\CapsoDriver\CapsoTest_winusb\x64\Release\CapsoTest.exe
CapsoDriver\CapsoDriver\CapsoTest_winusb\x64\Release\CapsoLLD_x64.dll
```

Note: the current DLL project has a Visual Studio warning because the project
target name and linker output name do not match (`CapsoLLD.dll` vs
`CapsoLLD_x64.dll`). The build still succeeds.

## Driver Installation

Run the installer script as Administrator:

```powershell
CapsoDriver\CapsoDriver\CapsoDriver_winusb\InstallDev.bat
```

To remove the driver package:

```powershell
CapsoDriver\CapsoDriver\CapsoDriver_winusb\RemoveDev.bat
```

## Runtime Notes

- `CapsoTest.exe` loads `CapsoLLD.dll` with `LoadLibrary("CapsoLLD.dll")`.
- Place the DLL next to the executable or ensure it is available in the DLL
  search path.
- Use the matching architecture: x86 executable with x86 DLL, or x64 executable
  with x64 DLL.

## Recent Sample-Code Update

The MFC sample was updated to parse device error codes from the status packet
returned by `AVCapso_Read()`. It now reports messages such as:

```text
Device returned error code 0x42 (ERROR_NoCapsule).
```

The update also fixes the previous tag-comparison bug where assignment was used
instead of comparison.
