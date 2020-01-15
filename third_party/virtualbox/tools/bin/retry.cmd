@echo off
rem $Id: retry.cmd $
rem rem @file
rem Windows NT batch script that retries a command 5 times.
rem

rem
rem Copyright (C) 2009-2017 Oracle Corporation
rem
rem This file is part of VirtualBox Open Source Edition (OSE), as
rem available from http://www.virtualbox.org. This file is free software;
rem you can redistribute it and/or modify it under the terms of the GNU
rem General Public License (GPL) as published by the Free Software
rem Foundation, in version 2 as it comes in the "COPYING" file of the
rem VirtualBox OSE distribution. VirtualBox OSE is distributed in the
rem hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
rem

rem
rem Note! We're using %ERRORLEVEL% here instead of the classic
rem IF ERRORLEVEL 0 GOTO blah because the latter cannot handle
rem the complete range or status codes while the former can.
rem
rem Note! SET changes ERRORLEVEL on XP+, so we have to ECHO
rem before incrementing the counter.
rem
set /a retry_count = 1
:retry
%*
if %ERRORLEVEL% == 0 goto success
if %retry_count% GEQ 5 goto give_up
echo retry.cmd: Attempt %retry_count% FAILED(%ERRORLEVEL%),  retrying: %*
set /a retry_count += 1
goto retry

:give_up
echo retry.cmd: Attempt %retry_count% FAILED(%ERRORLEVEL%), giving up: %*
set retry_count=
exit /b 1

:success
if %retry_count% NEQ 1 echo retry.cmd: Success after %retry_count% tries: %*!
set retry_count=
exit /b 0

