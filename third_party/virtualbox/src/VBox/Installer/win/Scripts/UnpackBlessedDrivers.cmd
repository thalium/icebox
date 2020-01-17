@echo off
rem $Id: UnpackBlessedDrivers.cmd $
rem rem @file
rem Windows NT batch script for unpacking drivers after being signed.
rem

rem
rem Copyright (C) 2018-2019 Oracle Corporation
rem
rem This file is part of VirtualBox Open Source Edition (OSE), as
rem available from http://www.virtualbox.org. This file is free software;
rem you can redistribute it and/or modify it under the terms of the GNU
rem General Public License (GPL) as published by the Free Software
rem Foundation, in version 2 as it comes in the "COPYING" file of the
rem VirtualBox OSE distribution. VirtualBox OSE is distributed in the
rem hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
rem


setlocal ENABLEEXTENSIONS
setlocal

rem
rem Globals and Check for environment variables we need.
rem
if ".%KBUILD_DEVTOOLS%" == "." (echo KBUILD_DEVTOOLS is not set & goto end_failed)
set _MY_DRIVER_BASE_NAMES=VBoxDrv VBoxNetAdp6 VBoxNetLwf VBoxUSB VBoxUSBMon
set _MY_DRIVER_BASE_NAMES=VBoxDrv VBoxNetAdp6 VBoxNetLwf VBoxUSB VBoxUSBMon
set _MY_UNZIP=%KBUILD_DEVTOOLS%\win.x86\bin\unzip.exe
if not exist "%_MY_UNZIP%" (echo "%_MY_UNZIP%" does not exist & goto end_failed)

rem
rem Parse arguments.
rem
set _MY_OPT_BINDIR=..\bin
set _MY_OPT_INPUT=
set _MY_OPT_SIGN_CAT=1
set _MY_OPT_SIGN_VERIFY=1

:argument_loop
if ".%1" == "."             goto no_more_arguments

if ".%1" == ".-h"           goto opt_h
if ".%1" == ".-?"           goto opt_h
if ".%1" == "./h"           goto opt_h
if ".%1" == "./H"           goto opt_h
if ".%1" == "./?"           goto opt_h
if ".%1" == ".-help"        goto opt_h
if ".%1" == ".--help"       goto opt_h

if ".%1" == ".-b"           goto opt_b
if ".%1" == ".--bindir"     goto opt_b
if ".%1" == ".-i"           goto opt_i
if ".%1" == ".--input"      goto opt_i
if ".%1" == ".-n"           goto opt_n
if ".%1" == ".--no-sign-cat" goto opt_n
if ".%1" == ".-v"           goto opt_v
if ".%1" == ".--no-sign-verify" goto opt_v

echo syntax error: Unknown option: %1
echo               Try --help to list valid options.
goto end_failed

:argument_loop_next_with_value
shift
shift
goto argument_loop

:opt_b
if ".%~2" == "."            goto syntax_error_missing_value
set _MY_OPT_BINDIR=%~2
goto argument_loop_next_with_value

:opt_h
echo This script unpacks the zip-file containing the blessed driver files from
echo Microsoft, replacing original files in the bin directory.  The catalog files
echo will be signed again and the Microsoft signature merged with ours.
echo .
echo Usage: UnpackBlessedDrivers.cmd [-b bindir] [-n/--no-sign-cat] [-v/--no-sign-verify] -i input.zip
echo .
echo Warning! This script should normally be invoked from the repack directory
goto end_failed

:opt_i
if ".%~2" == "."            goto syntax_error_missing_value
set _MY_OPT_INPUT=%~2
goto argument_loop_next_with_value

:opt_n
set _MY_OPT_SIGN_CAT=0
shift
goto argument_loop

:opt_v
set _MY_OPT_SIGN_VERIFY=0
shift
goto argument_loop

:syntax_error_missing_value
echo syntax error: missing or empty option value after %1
goto end_failed

:error_bindir_does_not_exist
echo syntax error: Specified BIN directory does not exist: "%_MY_OPT_BINDIR%"
goto end_failed

:error_input_not_found
echo error: Input file does not exist: "%_MY_OPT_INPUT%"
goto end_failed

:no_more_arguments
rem validate specified options
if not exist "%_MY_OPT_BINDIR%"     goto error_bindir_does_not_exist

rem figure defaults here: if ".%_MY_OPT_INPUT%" == "." if exist "%_MY_OPT_BINDIR%\x86" set _MY_OPT_INPUT=VBoxDrivers-amd64.cab
rem figure defaults here: if ".%_MY_OPT_INPUT%" == "."       set _MY_OPT_INPUT=VBoxDrivers-x86.cab
if not exist "%_MY_OPT_INPUT%"      goto error_input_not_found

rem
rem Unpack the stuff.
rem We ignore error level 1 here as that is what unzip returns on warning (slashes).
rem
"%_MY_UNZIP%" -o -j "%_MY_OPT_INPUT%" -d "%_MY_OPT_BINDIR%" && goto unzip_okay
if NOT ERRORLEVEL 1 goto end_failed
:unzip_okay

if ".%_MY_OPT_SIGN_VERIFY%" == ".0" goto no_sign_verify
rem
rem Verify it against the PreW10 catalog files we saved.
rem
set _MY_SIGNTOOL=%KBUILD_DEVTOOLS%\win.x86\sdk\v8.1\bin\x86\signtool.exe
if not exist "%_MY_SIGNTOOL%" set _MY_SIGNTOOL=%KBUILD_DEVTOOLS%\win.x86\selfsign\r3\signtool.exe

for %%d in (%_MY_DRIVER_BASE_NAMES%) do (
    @echo * Verifying %%d against %%d.cat...
    "%_MY_SIGNTOOL%" verify /kp /c "%_MY_OPT_BINDIR%\%%d.cat"        "%_MY_OPT_BINDIR%\%%d.inf" || goto end_failed
    "%_MY_SIGNTOOL%" verify /kp /c "%_MY_OPT_BINDIR%\%%d.cat"        "%_MY_OPT_BINDIR%\%%d.sys" || goto end_failed
    @echo * Verifying %%d against %%d-PreW10.cat...
    "%_MY_SIGNTOOL%" verify /kp /c "%_MY_OPT_BINDIR%\%%d-PreW10.cat" "%_MY_OPT_BINDIR%\%%d.inf" || goto end_failed
    "%_MY_SIGNTOOL%" verify /kp /c "%_MY_OPT_BINDIR%\%%d-PreW10.cat" "%_MY_OPT_BINDIR%\%%d.sys" || goto end_failed
)
:no_sign_verify

rem
rem Modify the catalog signatures.
rem
if "%_MY_OPT_SIGN_CAT%" == "0" goto no_sign_cat
set PATH=%PATH%;%_MY_OPT_BINDIR%
for %%d in (%_MY_DRIVER_BASE_NAMES%) do (
    copy /y "%_MY_OPT_BINDIR%\%%d.cat" "%_MY_OPT_BINDIR%\%%d.cat.ms" || goto end_failed
    call sign-dual.cmd "%_MY_OPT_BINDIR%\%%d.cat" || goto end_failed
    "%_MY_OPT_BINDIR%\tools\RTSignTool.exe"  add-nested-cat-signature -v "%_MY_OPT_BINDIR%\%%d.cat" "%_MY_OPT_BINDIR%\%%d.cat.ms" || goto end_failed
)
:no_sign_cat
goto end

:end_failed
@echo failed (%ERRORLEVEL%)
@endlocal
@endlocal
@exit /b 1

:end
@endlocal
@endlocal

