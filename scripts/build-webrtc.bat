@ECHO OFF

echo ==========================================================
echo Building libwebrtc
echo ==========================================================

set PATH=%DEPOT_TOOLS%;%PATH%
set TARGETS=libjingle_peerconnection builtin_video_encoder_factory builtin_video_decoder_factory media_engine rtc_software_fallback_wrappers
: TODO: ARM specific targets (see build-webrtc.sh)

@ECHO ON
call ninja webrtc %TARGETS% -j 24 -d explain || goto :error

: ------------------------------------------------------------------------------------
goto :EOF
:error
@ECHO OFF
echo ======================================================================
echo Failed with error #%errorlevel%
exit /b %errorlevel%
