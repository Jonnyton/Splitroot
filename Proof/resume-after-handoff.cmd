@echo off
rem One-shot resume runner (2026-06-11 Cowork session). Safe to re-run.
powershell -ExecutionPolicy Bypass -File "%~dp0resume-after-handoff.ps1"
pause
