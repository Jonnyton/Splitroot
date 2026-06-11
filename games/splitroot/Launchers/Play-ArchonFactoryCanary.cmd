@echo off
setlocal
for %%I in ("%~dp0..\..\..") do set "PROJECT_ROOT=%%~fI"
set "UPROJECT=%PROJECT_ROOT%\ArchonFactoryCanary.uproject"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
set "SUPERVISOR=%PROJECT_ROOT%\Proof\host-supervisor-loop.ps1"

if not exist "%UNREAL_EDITOR%" (
  echo UnrealEditor.exe was not found at "%UNREAL_EDITOR%".
  exit /b 1
)

if not exist "%UPROJECT%" (
  echo Project was not found at "%UPROJECT%".
  exit /b 1
)

if not exist "%SUPERVISOR%" (
  echo Host supervisor was not found at "%SUPERVISOR%".
  exit /b 1
)

rem Current playable surface (2026-06-11): a supervisor-owned SPLITROOT host.
rem It runs match after match, builds at match boundaries when source is newer,
rem and relaunches the next match on the latest effective version. If an older
rem direct-launched current build is already running, takeover is armed only at
rem the next-match boundary so the following match becomes supervisor-owned.
rem The loop is idempotent; double-clicking while it is already running exits
rem the duplicate.
powershell -NoProfile -ExecutionPolicy Bypass -Command "$argsList = @('-NoProfile','-ExecutionPolicy','Bypass','-File','%SUPERVISOR%','-ProjectRoot','%PROJECT_ROOT%','-AllowBoundaryRefresh','-BuildAtBoundary','-AdoptExternalAtBoundary','-RestartOnCrash','-ForceLaunch'); Start-Process -FilePath 'powershell.exe' -ArgumentList $argsList -WindowStyle Hidden"
exit /b 0
