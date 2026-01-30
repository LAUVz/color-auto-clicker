@echo off
echo Generating app.rc from template with version from package.json...

REM Read version from package.json using Node.js
for /f "delims=" %%i in ('node -p "require('./package.json').version"') do set VERSION=%%i

REM Convert dots to commas for VERSION_COMMA (e.g., 1.0.0.1 -> 1,0,0,1)
set VERSION_COMMA=%VERSION:.=,%

echo Version: %VERSION%
echo Version (comma): %VERSION_COMMA%

REM Use PowerShell to do the replacement since batch is limited
powershell -Command "(Get-Content app.rc.template) -replace '{{VERSION}}', '%VERSION%' -replace '{{VERSION_COMMA}}', '%VERSION_COMMA%' | Set-Content app.rc"

echo app.rc generated successfully!
