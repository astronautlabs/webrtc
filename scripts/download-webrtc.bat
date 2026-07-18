@ECHO OFF

echo ==========================================================
echo Downloading libwebrtc
echo ==========================================================

set PATH=%DEPOT_TOOLS%;%PATH%
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

@ECHO ON
CALL gclient config --unmanaged --spec solutions=[{\"name\":\"src\",\"url\":\"https://webrtc.googlesource.com/src.git\"}] || goto :error
CALL gclient sync --shallow --no-history --nohooks --with_branch_heads -r %WEBRTC_REVISION% -R || goto :error
CALL python src\build\util\lastchange.py -o src\build\util\LASTCHANGE || goto :error
CALL python src\build\vs_toolchain.py update --force || goto :error
CALL python src\tools\clang\scripts\update.py || goto :error

:: Fix Git timestamps across the Chromium tree to ensure zero-action Ninja builds
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0restore-mtimes.ps1"

rmdir webrtc
mklink /j webrtc src || goto :error

: ------------------------------------------------------------------------------------
goto :EOF
:error
@ECHO OFF
echo ======================================================================
echo Failed with error #%errorlevel%
exit /b %errorlevel%
