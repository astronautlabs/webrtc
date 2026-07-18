@ECHO OFF

echo ==========================================================
echo Building libwebrtc
echo ==========================================================
echo
echo Working directory: %CD%

set PATH=%DEPOT_TOOLS%;%PATH%
set TARGETS=libjingle_peerconnection builtin_video_encoder_factory builtin_video_decoder_factory media_engine rtc_software_fallback_wrappers
: TODO: ARM specific targets (see build-webrtc.sh)

@ECHO ON

:: CircleCI's tar truncates NTFS 100-ns timestamps during cache restore, and we frequently have issues 
:: with timestamps not lining up. Normally the libwebrtc sources are not changing as they are pinned to a specific 
:: branch head, so if there are built versions of the objects, we will assume they are correct and can be used as-is.
:: This does mean changing the branch head will require you to start from a fresh build, but *you should be doing that 
:: anyway*.
:: Force Ninja to update its internal database to match the truncated disk timestamps.
call ninja -t restat

:: Build libwebrtc
call ninja webrtc %TARGETS% -j 24 -d explain || goto :error

:: CircleCI's `tar` breaks NTFS Directory Junctions when saving/restoring cache.
:: We must safely delete the broken cache artifact and recreate the junction 
:: before moving on to the addon build to ensure headers resolve correctly.
node -e "const fs=require('node:fs'); fs.rmSync('../webrtc', {recursive:true, force:true});" >nul 2>&1
pushd ..
if not exist "webrtc\" (
    echo Repairing WebRTC directory junction for Windows CI Cache...
    mklink /J webrtc src
)
popd

: ------------------------------------------------------------------------------------
goto :EOF
:error
@ECHO OFF
echo ======================================================================
echo Failed with error #%errorlevel%
exit /b %errorlevel%
