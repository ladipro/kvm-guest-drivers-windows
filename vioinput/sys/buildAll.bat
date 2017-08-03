@echo off

set SYS_FILE_NAME=viohidkmdf

if "%1_%2" neq "_" goto %1_%2
for %%A in (W2k) do for %%B in (32 64) do call :%%A_%%B
set SYS_FILE_NAME=
goto :eof 

:buildsys
call buildOne.bat %1 %2
goto :eof

:buildpack
call :buildsys %1 %2
rem call :packsys %1 %2
set BUILD_OS=
set BUILD_ARC=
goto :eof

:W2k_32
call :buildpack W2K x86 "Win2k Release|x86" 0x500 buildfre_w2k_x86.log
goto :eof

:W2k_64
goto :eof
