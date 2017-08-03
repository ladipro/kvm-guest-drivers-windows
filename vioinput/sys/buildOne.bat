@echo off
setlocal

if "%DDKINSTALLROOT%"=="" set DDKINSTALLROOT=C:\WINDDK\
rem if "%DDKVER%"=="" set DDKVER=7600.16385.1
set DDKVER=6001.18002
set BUILDROOT=C:\WINDDK\%DDKVER%

if "%_BUILD_MAJOR_VERSION_%"=="" set _BUILD_MAJOR_VERSION_=101
if "%_BUILD_MINOR_VERSION_%"=="" set _BUILD_MINOR_VERSION_=58000
if "%_RHEL_RELEASE_VERSION_%"=="" set _RHEL_RELEASE_VERSION_=61

if /i "%1"=="prepare" goto %1

call :%1_%2
goto :eof

:Win10_x86
call :BuildProject "Win10 Release|x86" buildfre_win10_x86.log
goto :eof

:Win10_x64
call :BuildProject "Win10 Release|x64" buildfre_win10_amd64.log
goto :eof

:Win8_x86
call :BuildProject "Win8 Release|x86" buildfre_win8_x86.log
goto :eof

:Win8_x64
call :BuildProject "Win8 Release|x64" buildfre_win8_amd64.log
goto :eof

:Win7_x86
call :BuildProject "Win7 Release|x86" buildfre_win7_x86.log
goto :eof

:Win7_x64
call :BuildProject "Win7 Release|x64" buildfre_win7_amd64.log
goto :eof

:W2k_x86
call :BuildProject "Win2k Release|x86" buildfre_w2k_x86.log
goto :eof

:BuildProject
setlocal
if "%_NT_TARGET_VERSION%"=="" set _NT_TARGET_VERSION=0x602
if "%_BUILD_MAJOR_VERSION_%"=="" set _BUILD_MAJOR_VERSION_=101
if "%_BUILD_MINOR_VERSION_%"=="" set _BUILD_MINOR_VERSION_=58000
if "%_RHEL_RELEASE_VERSION_%"=="" set _RHEL_RELEASE_VERSION_=61

set _MAJORVERSION_=%_BUILD_MAJOR_VERSION_%
set _MINORVERSION_=%_BUILD_MINOR_VERSION_%
set /a _NT_TARGET_MAJ="(%_NT_TARGET_VERSION% >> 8) * 10 + (%_NT_TARGET_VERSION% & 255)"
set _NT_TARGET_MIN=%_RHEL_RELEASE_VERSION_%
set STAMPINF_VERSION=%_NT_TARGET_MAJ%.%_RHEL_RELEASE_VERSION_%.%_BUILD_MAJOR_VERSION_%.%_BUILD_MINOR_VERSION_%

call :prepare

pushd %BUILDROOT%
echo call %BUILDROOT%\bin\setenv.bat %BUILDROOT% fre W2k no_prefast
call %BUILDROOT%\bin\setenv.bat %BUILDROOT% fre W2k
popd

pushd ..\..\VirtIO
build -cZg
popd

pushd ..\..\VirtIO\WDF
build -cZg
popd

build -cZg

endlocal
goto :eof

:prepare_version
set /a _NT_TARGET_MAJ="(%_NT_TARGET_VERSION% >> 8) * 10 + (%_NT_TARGET_VERSION% & 255)"
goto :eof

:create2012H
echo #ifndef __DATE__
echo #define __DATE__ "%DATE%"
echo #endif
echo #ifndef __TIME__
echo #define __TIME__ "%TIME%"
echo #endif
echo #define _NT_TARGET_MAJ %_NT_TARGET_MAJ%
echo #define _NT_TARGET_MIN %_RHEL_RELEASE_VERSION_%
echo #define _MAJORVERSION_ %_BUILD_MAJOR_VERSION_%
echo #define _MINORVERSION_ %_BUILD_MINOR_VERSION_%
goto :eof

:prepare
set _NT_TARGET_VERSION=0x0500
call :prepare_version
call :create2012H  > 2012-defines.h
goto :eof
