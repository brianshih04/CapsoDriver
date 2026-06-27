# Changelog

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
  - `CapsoDriver/CapsoDriver/x32/CapsoLLD.dll`
  - `CapsoDriver/CapsoDriver/x32/CapsoTest.exe`
  - `CapsoDriver/CapsoDriver/x64/CapsoLLD.dll`
  - `CapsoDriver/CapsoDriver/x64/CapsoTest.exe`

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
