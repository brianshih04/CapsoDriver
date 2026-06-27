@echo off

REM  --> Check for permissions
IF "%PROCESSOR_ARCHITECTURE%" EQU "amd64" (
    >nul 2>&1 "%SYSTEMROOT%\SysWOW64\cacls.exe" "%SYSTEMROOT%\SysWOW64\config\system"
) else (
    >nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"
)

REM --> If error flag set, we do not have admin.
if '%errorlevel%' NEQ '0' (
    echo Requesting administrative privileges...
    goto UACPrompt
) else ( goto gotAdmin )

:UACPrompt
    echo Set UAC = CreateObject^("Shell.Application"^) > "%temp%\getadmin.vbs"
    set params = %*:"=""
    echo UAC.ShellExecute "cmd.exe", "/c ""%~s0"" %params%", "", "runas", 1 >> "%temp%\getadmin.vbs"

    "%temp%\getadmin.vbs"
    del "%temp%\getadmin.vbs"
    exit /B

:gotAdmin
    pushd "%CD%"
    CD /D "%~dp0"
::--------------------------------------

setlocal EnableDelayedExpansion
set "USBID="
for %%f in ("%~dp0*.inf") do (
	echo delete inf package %%f
	pnputil.exe -f -d "%%f"
	for %%o in ("%WINDIR%\inf\oem*.inf") do (
		fc "%%f" %%o > nul
		if !ERRORLEVEL!==0 (
			echo delete inf package %%o
			pnputil.exe -f -d %%~nxo
		)
	)
	for /f "tokens=2 delims=," %%a in ('findstr "USB\\" "%%f"') do (
		echo "!USBID!" | findstr /i %%a >nul
		if errorlevel 1 (
			echo remove %%a from Device Manager
			rmreg %%a
			set "USBID=!USBID! %%a"
		)
	)
)

pause