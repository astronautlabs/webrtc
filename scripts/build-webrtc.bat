@ECHO OFF
SET EL=0

ECHO Add depot_tools to PATH
set PATH=%DEPOT_TOOLS%;%PATH%

ECHO PATH is: %PATH%

IF %ERRORLEVEL% NEQ 0 GOTO ERROR

set TARGETS=libjingle_peerconnection builtin_video_encoder_factory builtin_video_decoder_factory media_engine
: TODO: ARM specific targets (see build-webrtc.sh)

ECHO ninja
call ninja webrtc %TARGETS% -v -j 24
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE

:ERROR
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE

EXIT /b %EL%
