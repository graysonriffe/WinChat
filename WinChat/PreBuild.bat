@echo off

echo [Pre Build] Startup

::Set Version
echo [Pre Build] Setting version...

::First make sure git is in the path
where git >nul 2>&1
if %errorlevel% neq 0 goto noGit
goto yesGit

:noGit
echo [Pre Build] Error: git not found on PATH!
exit 1

::We have git on the command line, so set the version string
:yesGit
git describe HEAD > temp1
git branch --show-current > temp2
set /p commit= < temp1
set /p branch= < temp2
del temp1
del temp2

::If the current branch is stable, don't include it in the version string
if "%branch%" == "stable" goto noBranch

set version=%commit%+%branch%
goto testOutput

:noBranch
set version=%commit%

::Don't touch the version file if it's already up to date
:testOutput
set /p oldVersion= < src\version.h
if "%oldVersion%" == "#define APPVERSION "%version%"" goto sameVersion

echo #define APPVERSION "%version%"> src\version.h
goto continue

:sameVersion
echo [Pre Build] Same version

:continue
echo [Pre Build] Exit
exit