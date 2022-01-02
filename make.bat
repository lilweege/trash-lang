@echo off
:: I much prefer gnu make to this...
:: since I'm not sure how to replicate all my fancy make shenanigans,
:: this script will always rebuild the entire project (for now at least)

set CFLAGS_COMMON=/W1 /WX /std:c11 /TC
set CFLAGS_DEBUG=/Zi
set CFLAGS_RELEASE=/Ox

if "%1%" == ""         goto debug
if "%1%" == "debug"    goto debug
if "%1%" == "release"  goto release
if "%1%" == "clean"    goto clean
echo Invalid argument '%1%'
goto :EOF

:debug
    set CFLAGS=%CFLAGS_COMMON% %CFLAGS_DEBUG%
    goto build
:release
    set CFLAGS=%CFLAGS_COMMON% %CFLAGS_RELEASE%
    goto build

:build
    echo %PATH% | find /c /i "msvc" > nul
    if "%errorlevel%" == "0" goto hasCL
        :: TODO: find whatever version of msvc is used and add vars for it
        :: for some reason microsoft enjoys making things difficult so idk
        :: call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_arm64
        call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
    :hasCL
    :: this may require some manual intervension from whoever builds this
    :: alternatively, simply run this script from an msvc-enabled developer console

    pushd obj
        cl %CFLAGS%  /Fe..\bin\trash.exe ..\src\*.c
    popd
goto :EOF

:clean
    del bin\trash.exe obj\*.obj obj\*.pdb bin\*.pdb bin\*.ilk
goto :EOF
