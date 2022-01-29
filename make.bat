@echo off
:: I much prefer gnu make to this...
:: since I'm not sure how to replicate all my fancy make shenanigans,
:: this script will always rebuild the entire project (for now at least)

echo %PATH% | find /c /i "msvc" > nul
if "%errorlevel%" == "0" goto hasCL
    :: TODO: find whatever version of msvc is used and add vars for it
    :: for some reason microsoft enjoys making things difficult so idk
    :: call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_arm64
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
:hasCL
:: this may require some manual intervension from whoever builds this
:: alternatively, simply run this script from an msvc-enabled developer console

:: TODO: compile with /WX
set CFLAGS_COMMON=/nologo /W4 /wd4996 /std:c11 /TC /DWINDOWS
set CFLAGS_DEBUG=/Zi /Od /DDEBUG
set CFLAGS_RELEASE=/O2
set CFLAGS=%CFLAGS_COMMON% %CFLAGS_DEBUG%

if "%1%" == ""         goto build
if "%1%" == "debug"    goto build
if "%1%" == "release"  goto release
if "%1%" == "test"     goto test
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

:test
    pushd obj
        cl %CFLAGS% /Fe..\bin\test.exe ..\test\testall.c
        set doTest=%errorlevel%
    popd
    if "%doTest%" == "0" .\bin\test.exe
goto :EOF

:clean
    del bin\trash.exe bin\test.exe obj\*.obj obj\*.pdb bin\*.pdb bin\*.ilk
goto :EOF
