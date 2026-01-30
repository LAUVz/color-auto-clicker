@echo off
echo Generating app.rc with version from package.json...
call generate-version.bat
echo.
echo Setting up Visual Studio 2022 environment...
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" > nul
if errorlevel 1 (
    echo ERROR: Failed to initialize Visual Studio environment
    exit /b 1
)
echo Compiling resources...
rc.exe /nologo app.rc
if errorlevel 1 (
    echo ERROR: Resource compilation failed
    exit /b 1
)
echo Building ColorClicker.exe...
cl.exe main.cpp app.res /EHsc /std:c++17 /D_UNICODE /DUNICODE /Fe:ColorClicker.exe /link user32.lib gdi32.lib gdiplus.lib comctl32.lib /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup /nologo
if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)
echo.
echo Build successful! ColorClicker.exe has been created.
echo.
