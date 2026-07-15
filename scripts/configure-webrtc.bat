@ECHO OFF

echo
echo ==========================================================
echo Configuring libwebrtc
echo ==========================================================

ECHO Add depot_tools to PATH
set PATH=%DEPOT_TOOLS%;%PATH% || goto :error

ECHO SET DEPOT_TOOLS_WIN_TOOLCHAIN=0
SET DEPOT_TOOLS_WIN_TOOLCHAIN=0 || goto :error

ECHO cd SOURCE_DIR
cd %SOURCE_DIR% || goto :error


:: We've already added the VS environment variables. When `gn gen` attempts to do it again, we may hit "The input line is too long"
:: because Windows is kind of shite. To prevent this, we're going to restore PATH and clear out the relevant environment variables
:: so gn can start fresh (but we do need depot tools still)
if "%__VSCMD_PREINIT_PATH%" NEQ "" set "PATH=%DEPOT_TOOLS%;%__VSCMD_PREINIT_PATH%"
set INCLUDE=
set LIB=
set LIBPATH=
set VSCMD_ARG_TGT_ARCH=
set VSCMD_ARG_HOST_ARCH=
set __VSCMD_PREINIT_VSINSTALLDIR=


ECHO gn gen %BINARY_DIR% "--args=%GN_GEN_ARGS%"
CALL gn gen %BINARY_DIR% "--args=%GN_GEN_ARGS%" || goto :error

: ------------------------------------------------------------------------------------
goto :EOF
:error
@ECHO OFF
echo ======================================================================
echo Failed with error #%errorlevel%
exit /b %errorlevel%

