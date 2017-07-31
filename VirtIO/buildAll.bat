@echo off
:
: Set global parameters: 
:

: Use Windows 7 DDK
if "%DDKVER%"=="" set DDKVER=3790.1830

: By default DDK is installed under C:\WINDDK, but it can be installed in different location
if "%DDKINSTALLROOT%"=="" set DDKINSTALLROOT=C:\WINDDK\

call :BuildProject "Win2k Release|x86" buildfre_wxp_x86.log
