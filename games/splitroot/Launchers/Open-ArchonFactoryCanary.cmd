@echo off
setlocal
set "PROJECT_ROOT=%~dp0..\..\.."
set "UPROJECT=%PROJECT_ROOT%\ArchonFactoryCanary.uproject"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%UNREAL_EDITOR%" (
  echo UnrealEditor.exe was not found at "%UNREAL_EDITOR%".
  exit /b 1
)

if not exist "%UPROJECT%" (
  echo Project was not found at "%UPROJECT%".
  exit /b 1
)

start "" "%UNREAL_EDITOR%" "%UPROJECT%"
exit /b 0
