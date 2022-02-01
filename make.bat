@echo off

echo %PATH% | find /c /i "msvc" > nul
if "%errorlevel%" == "0" goto hasCL
    echo Script must be run from an msvc-enabled terminal.
    :: this may require some manual intervension from the user
    :: either run this script from an msvc-enabled developer console,
    :: or 'enabling' your terminal by running one of the following commands
    :: (for some reason microsoft enjoys making things difficult so the exact path depends on the version)
    :: `call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64`
    :: `call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_arm64`
    goto :EOF
:hasCL

:: TODO: compile with /WX
set CFLAGS_COMMON=/nologo /W4 /wd4996 /std:c11 /TC /DWINDOWS
set CFLAGS_DEBUG=/Zi /Od /DDEBUG
set CFLAGS_RELEASE=/O2
set CFLAGS=%CFLAGS_COMMON% %CFLAGS_DEBUG%

if "%1%" == ""         goto build
if "%1%" == "debug"    goto build
if "%1%" == "release"  goto release
if "%1%" == "clean"    goto clean
echo Invalid argument '%1%'
goto :EOF

:release
    set CFLAGS=%CFLAGS_COMMON% %CFLAGS_RELEASE%
    goto build

:build
    pushd obj
        cl %CFLAGS% /Fe..\bin\trash.exe ..\src\*.c
    popd
goto :EOF

:clean
    del bin\trash.exe obj\*.obj obj\*.pdb bin\*.pdb bin\*.ilk
goto :EOF
