# Changelog

## Unreleased

### Changed

- Reorganized the repository layout to remove the extra nested
  `CapsoDriver/CapsoDriver` folder.
- Moved the DLL source project to `CapsoLLD/`.
- Moved the MFC sample application project to `CapsoTest/`.
- Moved the Visual Studio solution to the repository root as `CapsoTest.sln`.
- Moved the WinUSB driver package to `Driver/`.
- Moved rebuilt sample binaries to `bin/x32/` and `bin/x64/`.
- Moved Visual C++ redistributable installers to `redist/`.
- Moved the command workbook to `docs/`.
- Moved the original packaged archive to `archive/`.
- Removed the duplicate root-level `API_Capso.h`; the public header now lives in
  `include/API_Capso.h`, while the DLL project keeps its build-local copy in
  `CapsoLLD/API_Capso.h`.

## 2026-06-27

### Added

- Added this changelog.
- Added a full English `README.md` describing the repository contents, driver
  package, DLL API, status packet format, error codes, build requirements, and
  runtime notes.

### Changed

- Updated the MFC sample application to parse the 32-byte status packet returned
  by `AVCapso_Read()`.
- Added device error-code handling for status byte 8.
- Added user-facing error messages in the sample dialog, for example:

  ```text
  Device returned error code 0x42 (ERROR_NoCapsule).
  ```

- Updated the prebuilt `x32` and `x64` sample binaries from the rebuilt output:
  - `bin/x32/CapsoLLD.dll`
  - `bin/x32/CapsoTest.exe`
  - `bin/x64/CapsoLLD.dll`
  - `bin/x64/CapsoTest.exe`

### Fixed

- Fixed the sample application's status tag check. The old code assigned the
  expected tag instead of comparing it.
- Fixed serial-number length validation in the sample application. The check now
  requires a length from 1 to 16 characters.
- The sample now validates the status signature as `USBS` before checking the
  returned tag and error code.

### Build

- Verified `Release|x64` build with Visual Studio 2022 Build Tools.
- Verified `Release|x86` build with Visual Studio 2022 Build Tools.
- Installed MFC support for the v143 toolset so the MFC sample application can
  build successfully.

### Repository

- Initialized the project as a Git repository.
- Published the repository to GitHub:

  <https://github.com/brianshih04/CapsoDriver>

- Changed the GitHub repository visibility from private to public.
