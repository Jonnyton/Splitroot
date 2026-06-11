# 039 - Remove old archon folder after Splitroot cutover

Goal:
Finish the local folder rename cleanup after the playable workspace was cut
over to `C:\Users\Jonathan\Projects\Users\Splitroot`.

Context:
- The GitHub repository has been renamed to `Jonnyton/Splitroot`.
- The active local playable path is now `C:\Users\Jonathan\Projects\Users\Splitroot`.
- The desktop shortcut `C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk`
  now points at `C:\Users\Jonathan\Projects\Users\Splitroot\Launchers\Play-CurrentBuild.cmd`.
- The supervisor, launch child, and Unreal editor were restarted from the
  `Splitroot` path with `-ForceLaunch` during the cutover.
- The old folder `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory`
  could not be renamed in place because Explorer and multiple Codex/Codex node
  processes still held directory handles under that path.

Steps:
1. Wait until active Codex sessions and Explorer windows no longer hold handles
   under `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory`.
2. Verify no live canary process uses the old path:
   `Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*archon-rts-fps-factory*' }`
3. Verify the live canary still uses the new path:
   `Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*C:\Users\Jonathan\Projects\Users\Splitroot*' }`
4. Remove or archive the old `archon-rts-fps-factory` folder only after the new
   `Splitroot` workspace is confirmed current and no process holds the old path.
5. Re-check the desktop shortcut target and `git remote -v` from the new folder.

Done-when:
- `C:\Users\Jonathan\Projects\Users\archon-rts-fps-factory` no longer exists,
  or it is explicitly archived outside the active workspace lane.
- `C:\Users\Jonathan\Projects\Users\Splitroot` remains the active working tree.
- The current-build shortcut and live supervisor both point at `Splitroot`.

Report-to:
Gauge and Hex via `Coordination/done/`.
