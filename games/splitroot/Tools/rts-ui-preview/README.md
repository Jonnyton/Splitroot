# SPLITROOT RTS UI Preview

Standalone HTML plus PNG preview for fast RTS command-surface iteration.

It is intentionally outside `Source/` and does not launch, reload, compile,
or control Unreal. Edit `state.json` or `index.html`, then render a screenshot:

```powershell
powershell -ExecutionPolicy Bypass -File .\games\splitroot\Tools\rts-ui-preview\Render-RtsUiPreview.ps1
```

Output:

```text
Saved\RtsUiPreview\splitroot-rts-ui-preview.png
```

The HTML file can also be opened directly in a browser for quick visual checks.

Current design target: Warcraft III-style RTS command cockpit with a persistent
top resource strip, interactive minimap semantics, selected unit portrait/stats,
production queue, and fixed 12-slot command card while leaving the map-table
view translucent enough to notice nearby FPS-world danger.
