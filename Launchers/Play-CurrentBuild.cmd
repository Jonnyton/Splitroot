@echo off
setlocal
for %%I in ("%~dp0..") do set "PROJECT_ROOT=%%~fI"
set "CANARY_PLAY_LAUNCHER=%PROJECT_ROOT%\games\splitroot\Launchers\Play-ArchonFactoryCanary.cmd"

if not exist "%CANARY_PLAY_LAUNCHER%" (
  echo Current build launcher was not found at "%CANARY_PLAY_LAUNCHER%".
  exit /b 1
)

rem Current-build behavior: launch the merged playable SPLITROOT surface.
rem Match-to-match continuity is handled by the supervisor-owned host loop:
rem source changes build at the match boundary, the host relaunches, and the
rem next match starts on the latest effective version. The supervisor only
rem refreshes a host process it launched itself.
call "%CANARY_PLAY_LAUNCHER%" %*
exit /b %ERRORLEVEL%
