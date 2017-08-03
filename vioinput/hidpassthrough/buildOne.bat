@echo off

rem if "%DDKVER%"=="" set DDKVER=7600.16385.1
set DDKVER=6001.18002
set BUILDROOT=C:\WINDDK\%DDKVER%

if "%_BUILD_MAJOR_VERSION_%"=="" set _BUILD_MAJOR_VERSION_=101
if "%_BUILD_MINOR_VERSION_%"=="" set _BUILD_MINOR_VERSION_=58000
if "%_RHEL_RELEASE_VERSION_%"=="" set _RHEL_RELEASE_VERSION_=61

set DDKBUILDENV=
pushd %BUILDROOT%
echo call %BUILDROOT%\bin\setenv.bat %BUILDROOT% fre %1
call %BUILDROOT%\bin\setenv.bat %BUILDROOT% fre %1
popd

echo _NT_TARGET_VERSION %_NT_TARGET_VERSION%

set /a _NT_TARGET_MAJ_ARCH="%_NT_TARGET_VERSION% >> 8
set /a _NT_TARGET_MIN_ARCH="%_NT_TARGET_VERSION% & 255

set /a _NT_TARGET_MAJ="(%_NT_TARGET_VERSION% >> 8) * 10 + (%_NT_TARGET_VERSION% & 255)"
set STAMPINF_VERSION=%_NT_TARGET_MAJ%.%_RHEL_RELEASE_VERSION_%.%_BUILD_MAJOR_VERSION_%.%_BUILD_MINOR_VERSION_% 

build -cZg

set DDKVER=
set BUILDROOT=
