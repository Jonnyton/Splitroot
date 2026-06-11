# 004 — Quietshot capture of the live loop session

Goal: a frame Rook can READ to verify visuals: mossy valley floor
(M_StandInGround), real wooden map table (not cube), trees/rocks.

Depends: 001 (new build must be live in the loop first).

Steps:
1. With the loop session running on the post-001 build:
   `powershell -ExecutionPolicy Bypass -File Proof\playtest-drive.ps1 -Action quietshot`
2. Note the output PNG path in the Result section.

Done-when: PNG exists on disk from the post-001 build.

Report-to: move to Coordination\done\ with ## Result naming the PNG
path. Rook reads the image itself — do not describe it in detail.

## Result

Quietshot captured successfully.

- Command: `powershell -ExecutionPolicy Bypass -File .\Proof\playtest-drive.ps1 -Action quietshot`
- PNG: `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory\Saved\Proof\PlaytestDrive\capture.png`
- File exists: `true`
- Size: `915979` bytes
- Last write: `2026-06-10 19:50:54` local time
- Session: loop monitor PID `23128`; game session PID `19212`
