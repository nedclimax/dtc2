@echo off
setlocal
pushd %~dp0

set bld_gui=1
set cflags=/nologo /FC /TC /MT /std:c17 /utf-8 /W4 /WX /MP
set cflags=%cflags% /Z7 /Oi /Gy /GS- /fp:except- /fp:fast /permissive- /Zc:inline /Zc:preprocessor
set lflags=/incremental:no /opt:ref,icf /merge:.CRT=.rdata /noimplib /noexp libvcruntime.lib

if "%bld_gui%"=="1" set lflags=%lflags% /subsystem:windows
if "%bld_gui%"=="0" set lflags=%lflags% /subsystem:console
if "%bld_gui%"=="0" set cflags=%cflags% /DBUILD_CONSOLE

if not exist build mkdir build
pushd build
del *.pdb >NUL 2>&1
cl %cflags% ..\src\gfx_test.c ..\src\xxh_x86dispatch.c /link %lflags%
popd
popd
