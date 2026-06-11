<#
.SYNOPSIS
Player-style playtest driver: operates the real windowed game through the
OS user path only (real window, injected mouse/keyboard, screen capture).

The harness rule (Jonathan 2026-06-10): no cheating — everything goes
through what a human player could do. Perception = screenshots of the
real window. Action = SendInput. Verification = capture + -abslog markers.

Loop discipline (from the 2026-06 AI-game-harness research):
act -> settle -> capture -> verify, one short action burst at a time.
Keys are injected as SCANCODES (UE reads raw input and can ignore
virtual-key synthesis). Mouse look is RELATIVE deltas in small steps.

.EXAMPLE
.\Proof\playtest-drive.ps1 -Action launch
.\Proof\playtest-drive.ps1 -Action capture -Name menu.png
.\Proof\playtest-drive.ps1 -Action click -X 960 -Y 480
.\Proof\playtest-drive.ps1 -Action key -Key escape
.\Proof\playtest-drive.ps1 -Action hold -Key w -Ms 1200
.\Proof\playtest-drive.ps1 -Action look -Dx 600 -Dy 0
.\Proof\playtest-drive.ps1 -Action markers -Pattern 'PauseMenu'
#>
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('launch','adopt','capture','quietshot','lower','key','hold','click','look','markers','stop','status','sessions','clear')]
    [string]$Action,
    # Rook-session etiquette (Jonathan 2026-06-10): harness launches start
    # MINIMIZED so they never steal the desktop; each action restores the
    # window for its burst and re-minimizes after, unless -KeepUp is set or
    # Jonathan himself restored the window (then it stays up — it's his).
    [switch]$KeepUp,
    # Bypass the human-idle gate (drive even though Jonathan recently used
    # the machine). Only for moments he has explicitly handed over.
    [switch]$Force,
    [string]$Name = 'capture.png',
    [string]$Key = '',
    [int]$Ms = 500,
    [int]$X = -1,
    [int]$Y = -1,
    [int]$Dx = 0,
    [int]$Dy = 0,
    [string]$Pattern = 'ArchonFactoryCanary:',
    [string]$MapUrl = '/Engine/Maps/Entry?ArchonMainMenu',
    # Extra command-line args for launch (e.g. '-ArchonBotMatch -ArchonBotCountPerTeam=8').
    [string]$ExtraArgs = '',
    [int]$BootWaitSeconds = 35,
    [int]$SettleMs = 800,
    [string]$ProjectRoot = '',
    [switch]$AllowLiveStop
)
$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..')).Path
}
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public class ArchonDrive {
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowTextLength(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, StringBuilder sb, int max);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct CURSORINFO { public int cbSize; public int flags; public IntPtr hCursor; public POINT ptScreenPos; }
    [DllImport("user32.dll")] public static extern bool GetCursorInfo(ref CURSORINFO pci);
    [DllImport("user32.dll")] public static extern bool DrawIcon(IntPtr hDC, int X, int Y, IntPtr hIcon);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
    [StructLayout(LayoutKind.Sequential)] public struct LASTINPUTINFO { public uint cbSize; public uint dwTime; }
    [DllImport("user32.dll")] public static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);
    [DllImport("kernel32.dll")] public static extern uint GetTickCount();
    public static double SecondsSinceLastUserInput() {
        var lii = new LASTINPUTINFO();
        lii.cbSize = (uint)Marshal.SizeOf(typeof(LASTINPUTINFO));
        if (!GetLastInputInfo(ref lii)) { return 0.0; }
        return (GetTickCount() - lii.dwTime) / 1000.0;
    }

    [StructLayout(LayoutKind.Sequential)] public struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public UIntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Sequential)] public struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public UIntPtr dwExtraInfo; }
    [StructLayout(LayoutKind.Explicit)] public struct INPUTUNION { [FieldOffset(0)] public MOUSEINPUT mi; [FieldOffset(0)] public KEYBDINPUT ki; }
    [StructLayout(LayoutKind.Sequential)] public struct INPUT { public uint type; public INPUTUNION U; }
    [DllImport("user32.dll", SetLastError=true)] public static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);

    public static void KeyScan(ushort scan, bool down) {
        var i = new INPUT[1];
        i[0].type = 1; i[0].U.ki.wScan = scan;
        i[0].U.ki.dwFlags = down ? 0x0008u : (0x0008u | 0x0002u); // SCANCODE [| KEYUP]
        SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    }

    public static void MouseButton(bool down) {
        var i = new INPUT[1]; i[0].type = 0;
        i[0].U.mi.dwFlags = down ? 0x0002u : 0x0004u; // LEFTDOWN / LEFTUP
        SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
    }

    // Relative look: UE consumes raw deltas; send small steps so the
    // game's per-frame input sampling catches every one.
    public static void MouseMoveRelative(int totalDx, int totalDy, int stepCounts, int stepDelayMs) {
        int sentX = 0, sentY = 0;
        int steps = Math.Max(Math.Abs(totalDx), Math.Abs(totalDy)) / Math.Max(1, stepCounts) + 1;
        for (int s = 1; s <= steps; s++) {
            int targetX = (int)((long)totalDx * s / steps);
            int targetY = (int)((long)totalDy * s / steps);
            var i = new INPUT[1]; i[0].type = 0;
            i[0].U.mi.dx = targetX - sentX; i[0].U.mi.dy = targetY - sentY;
            i[0].U.mi.dwFlags = 0x0001; // MOUSEEVENTF_MOVE (relative)
            SendInput(1, i, Marshal.SizeOf(typeof(INPUT)));
            sentX = targetX; sentY = targetY;
            System.Threading.Thread.Sleep(stepDelayMs);
        }
    }

    public static List<IntPtr> FindPidWindows(uint pid) {
        var found = new List<IntPtr>();
        EnumWindows((h, l) => {
            uint wpid; GetWindowThreadProcessId(h, out wpid);
            if (wpid == pid && IsWindowVisible(h) && GetWindowTextLength(h) > 0) { found.Add(h); }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
[ArchonDrive]::SetProcessDPIAware() | Out-Null

$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$shotDir = Join-Path $proofDir 'PlaytestDrive'
$statePath = Join-Path $proofDir 'playtest-drive-session.json'
New-Item -ItemType Directory -Force -Path $shotDir | Out-Null

$scanMap = @{
    'escape' = 0x01; 'tab' = 0x0F; 'space' = 0x39; 'enter' = 0x1C;
    'w' = 0x11; 'a' = 0x1E; 's' = 0x1F; 'd' = 0x20;
    'e' = 0x12; 'r' = 0x13; 'f' = 0x21; 'n' = 0x31; 'm' = 0x32;
    'c' = 0x2E; 'v' = 0x2F;
    'shift' = 0x2A; 'ctrl' = 0x1D
}

function Get-SessionState {
    if (-not (Test-Path -LiteralPath $statePath)) { throw 'No playtest session. Run -Action launch first.' }
    return Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
}

function Find-LiveArchonProcess {
    $candidates = Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" |
        Where-Object { $_.CommandLine -match 'ArchonFactoryCanary\.uproject' } |
        Sort-Object CreationDate -Descending
    if (-not $candidates) {
        throw 'No running ArchonFactoryCanary UnrealEditor.exe process found.'
    }
    return $candidates | Select-Object -First 1
}

function Get-GameWindow([int]$gamePid) {
    $windows = [ArchonDrive]::FindPidWindows([uint32]$gamePid)
    foreach ($h in $windows) {
        $sb = New-Object System.Text.StringBuilder 256
        [ArchonDrive]::GetWindowText($h, $sb, 256) | Out-Null
        if ($sb.ToString() -match 'Archon') { return @{ Hwnd = $h; Title = $sb.ToString() } }
    }
    throw "No Archon game window for pid $gamePid (title verification failed)."
}

function Set-GameForeground($win) {
    [ArchonDrive]::SetWindowPos($win.Hwnd, [IntPtr](-1), 0, 0, 0, 0, 0x0001 -bor 0x0002 -bor 0x0040) | Out-Null
    $fg = [ArchonDrive]::GetForegroundWindow()
    $fgThread = [ArchonDrive]::GetWindowThreadProcessId($fg, [ref]([uint32]0))
    $myThread = [ArchonDrive]::GetCurrentThreadId()
    [ArchonDrive]::AttachThreadInput($myThread, $fgThread, $true) | Out-Null
    [ArchonDrive]::SetForegroundWindow($win.Hwnd) | Out-Null
    [ArchonDrive]::AttachThreadInput($myThread, $fgThread, $false) | Out-Null
    Start-Sleep -Milliseconds 350

    # Keyboard input needs real focus; SetForegroundWindow can be silently
    # denied (foreground lock). First unlock: tap ALT (we become the
    # "last input" process and ALT maps to nothing in-game — a title-bar
    # CLICK leaks into UE as a raw LMB and fires the bow, found 2026-06-10).
    if ([ArchonDrive]::GetForegroundWindow() -ne $win.Hwnd) {
        [ArchonDrive]::KeyScan(0x38, $true)   # ALT down
        Start-Sleep -Milliseconds 40
        [ArchonDrive]::KeyScan(0x38, $false)  # ALT up
        [ArchonDrive]::SetForegroundWindow($win.Hwnd) | Out-Null
        Start-Sleep -Milliseconds 350
    }

    # Last resort: the title-bar click (focuses reliably but MAY register
    # as an in-game LMB through raw input — warn so captures are read
    # with that in mind).
    if ([ArchonDrive]::GetForegroundWindow() -ne $win.Hwnd) {
        Write-Output 'WARN UsingTitleBarClickFocus=true (may register as in-game LMB)'
        $rect = New-Object ArchonDrive+RECT
        [ArchonDrive]::GetWindowRect($win.Hwnd, [ref]$rect) | Out-Null
        $titleX = [int](($rect.Left + $rect.Right) / 2)
        $titleY = $rect.Top + 12
        [ArchonDrive]::SetCursorPos($titleX, $titleY) | Out-Null
        Start-Sleep -Milliseconds 150
        [ArchonDrive]::MouseButton($true)
        Start-Sleep -Milliseconds 60
        [ArchonDrive]::MouseButton($false)
        Start-Sleep -Milliseconds 350
        if ([ArchonDrive]::GetForegroundWindow() -ne $win.Hwnd) {
            Write-Output 'WARN FocusNotAcquired=true (keys may not reach the game)'
        }
    }
}

# Human-idle gate (Jonathan 2026-06-10: "it keeps popping up in front of
# what I'm doing"): never steal the surface while he is actively using
# the machine. Drive only when input has been idle for a while.
function Assert-HumanIdle {
    if ($Force) { return }
    $idle = [ArchonDrive]::SecondsSinceLastUserInput()
    if ($idle -lt 45) {
        throw "HumanActive: user input $([int]$idle)s ago (need 45s idle). Jonathan is using the machine - do not drive. Use -Force only if he explicitly handed over."
    }
}

# Surface etiquette: restore a minimized session only for the action
# burst, and put it back the way it was. If Jonathan restored the window
# himself, it is HIS surface — leave it up afterward.
function Enter-GameSurface($win) {
    $script:PrevForeground = [ArchonDrive]::GetForegroundWindow()
    $wasBackground = [ArchonDrive]::IsIconic($win.Hwnd) -or ($script:PrevForeground -ne $win.Hwnd)
    if ([ArchonDrive]::IsIconic($win.Hwnd)) {
        [ArchonDrive]::ShowWindow($win.Hwnd, 9) | Out-Null  # SW_RESTORE
        Start-Sleep -Milliseconds 700
    }
    Set-GameForeground $win
    return $wasBackground
}

function Exit-GameSurface($win, [bool]$wasBackground) {
    if ($wasBackground -and -not $KeepUp) {
        # Return the surface: drop to bottom z-order (still renderable for
        # quietshot, visible on the taskbar, never over Jonathan's work)
        # and hand focus back to whatever he had active.
        Set-GameLowered $win
        if ($script:PrevForeground -and $script:PrevForeground -ne $win.Hwnd) {
            [ArchonDrive]::SetForegroundWindow($script:PrevForeground) | Out-Null
        }
        Write-Output 'SurfaceReturned=lowered'
    }
}

# Quiet perception (the "good cheat", Jonathan 2026-06-10): capture the
# window's composited content via PrintWindow(PW_RENDERFULLCONTENT)
# without restoring, focusing, or touching the foreground. Valid ONLY
# because the equivalence test proved it shows the same pixels a player
# with the window open would see. Does not work while truly minimized —
# use the 'lower' posture (restored, bottom z-order, never activated).
function Save-QuietCapture($win, [string]$fileName) {
    $rect = New-Object ArchonDrive+RECT
    [ArchonDrive]::GetWindowRect($win.Hwnd, [ref]$rect) | Out-Null
    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top
    if ([ArchonDrive]::IsIconic($win.Hwnd) -or $w -lt 200 -or $h -lt 200) {
        Write-Output 'QuietCapture=unavailable reason=window_minimized (use -Action lower first)'
        return
    }
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $hdc = $gfx.GetHdc()
    $ok = [ArchonDrive]::PrintWindow($win.Hwnd, $hdc, 2) # PW_RENDERFULLCONTENT
    $gfx.ReleaseHdc($hdc)
    $gfx.Dispose()
    $out = Join-Path $shotDir $fileName
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "PrintWindowOk=$ok"
    Write-Output "Screenshot=$out"
}

function Set-GameLowered($win) {
    # Restored but never in the way: bottom of the z-order, no activation,
    # and explicitly NOT topmost (undo any earlier topmost pin).
    if ([ArchonDrive]::IsIconic($win.Hwnd)) {
        [ArchonDrive]::ShowWindow($win.Hwnd, 4) | Out-Null # SW_SHOWNOACTIVATE
        Start-Sleep -Milliseconds 500
    }
    [ArchonDrive]::SetWindowPos($win.Hwnd, [IntPtr](-2), 0, 0, 0, 0, 0x0001 -bor 0x0002 -bor 0x0010) | Out-Null # NOTOPMOST
    [ArchonDrive]::SetWindowPos($win.Hwnd, [IntPtr](1), 0, 0, 0, 0, 0x0001 -bor 0x0002 -bor 0x0010) | Out-Null  # HWND_BOTTOM, NOACTIVATE
}

function Save-WindowCapture($win, [string]$fileName, [int]$settleMs) {
    Start-Sleep -Milliseconds $settleMs
    $rect = New-Object ArchonDrive+RECT
    [ArchonDrive]::GetWindowRect($win.Hwnd, [ref]$rect) | Out-Null
    $w = $rect.Right - $rect.Left
    $h = $rect.Bottom - $rect.Top
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)
    $gfx.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size($w, $h)))
    $ci = New-Object ArchonDrive+CURSORINFO
    $ci.cbSize = [System.Runtime.InteropServices.Marshal]::SizeOf($ci)
    $cursor = 'hidden'
    if ([ArchonDrive]::GetCursorInfo([ref]$ci) -and ($ci.flags -band 1) -eq 1) {
        $hdc = $gfx.GetHdc()
        [ArchonDrive]::DrawIcon($hdc, $ci.ptScreenPos.X - $rect.Left, $ci.ptScreenPos.Y - $rect.Top, $ci.hCursor) | Out-Null
        $gfx.ReleaseHdc($hdc)
        $cursor = 'visible'
    }
    $gfx.Dispose()
    $out = Join-Path $shotDir $fileName
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Output "Cursor=$cursor"
    Write-Output "WindowRect=$($rect.Left),$($rect.Top),$($rect.Right),$($rect.Bottom)"
    Write-Output "Screenshot=$out"
}

switch ($Action) {
    'adopt' {
        $procInfo = Find-LiveArchonProcess
        $logPath = Join-Path $proofDir 'playtest-drive.log'
        if ($procInfo.CommandLine -match '-abslog="([^"]+)"') {
            $logPath = $Matches[1]
        }
        $win = Get-GameWindow ([int]$procInfo.ProcessId)
        @{ Pid = [int]$procInfo.ProcessId; LogPath = $logPath; Adopted = $true } |
            ConvertTo-Json |
            Set-Content -LiteralPath $statePath -Encoding utf8
        Write-Output "AdoptedPid=$($procInfo.ProcessId)"
        Write-Output "WindowTitle=$($win.Title)"
        Write-Output "Log=$logPath"
    }
    'launch' {
        Assert-HumanIdle
        if (Test-Path -LiteralPath $statePath) {
            $old = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
            if (Get-Process -Id $old.Pid -ErrorAction SilentlyContinue) {
                throw "Session pid $($old.Pid) still running. Use -Action stop first (a running session also locks the module DLL against rebuilds)."
            }
        }
        $editor = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe'
        $uproject = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
        $logPath = Join-Path $proofDir 'playtest-drive.log'
        Remove-Item -LiteralPath $logPath -Force -ErrorAction SilentlyContinue
        # Rook sessions live LOWERED: bottom of the z-order, never activated,
        # never over Jonathan's work — but still rendered, so quietshot can
        # see it any time and he can click it on the taskbar to watch or
        # take over. NEVER launch minimized: UE 5.7's D3D12 swapchain
        # never initializes for a minimized window and the first restore
        # crashes the render thread (found 2026-06-10).
        $launchArgs = @(
            "`"$uproject`"", $MapUrl,
            '-game', '-windowed', '-ResX=1280', '-ResY=720',
            "-abslog=`"$logPath`""
        )
        if ($ExtraArgs -ne '') { $launchArgs += ($ExtraArgs -split ' ') }
        $proc = Start-Process -FilePath $editor -ArgumentList $launchArgs -PassThru
        @{ Pid = $proc.Id; LogPath = $logPath } | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
        # Lower as soon as the window exists so the boot flash is brief.
        $lowered = $false
        for ($i = 0; $i -lt 20 -and -not $lowered; $i++) {
            Start-Sleep -Milliseconds 500
            try {
                $earlyWin = Get-GameWindow $proc.Id
                Set-GameLowered $earlyWin
                $lowered = $true
            } catch {}
        }
        Start-Sleep -Seconds $BootWaitSeconds
        $proc.Refresh()
        if ($proc.HasExited) { throw 'Game process exited during boot.' }
        $win = Get-GameWindow $proc.Id
        Set-GameLowered $win
        Write-Output "Pid=$($proc.Id)"
        Write-Output "WindowTitle=$($win.Title)"
        Write-Output "Log=$logPath"
        Write-Output "Posture=lowered"
        Save-QuietCapture $win 'launch.png'
    }
    'capture' {
        Assert-HumanIdle
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        $wasMin = Enter-GameSurface $win
        Save-WindowCapture $win $Name $SettleMs
        Exit-GameSurface $win $wasMin
    }
    'quietshot' {
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        Save-QuietCapture $win $Name
    }
    'lower' {
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        Set-GameLowered $win
        Write-Output 'Lowered=true (restored, bottom z-order, not activated)'
    }
    'key' {
        Assert-HumanIdle
        if (-not $scanMap.ContainsKey($Key)) { throw "Unknown key '$Key'. Known: $($scanMap.Keys -join ', ')" }
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        $wasMin = Enter-GameSurface $win
        [ArchonDrive]::KeyScan([uint16]$scanMap[$Key], $true)
        Start-Sleep -Milliseconds 70
        [ArchonDrive]::KeyScan([uint16]$scanMap[$Key], $false)
        Write-Output "PressedKey=$Key"
        if ($Name -ne 'capture.png') { Save-WindowCapture $win $Name $SettleMs }
        Exit-GameSurface $win $wasMin
    }
    'hold' {
        Assert-HumanIdle
        if (-not $scanMap.ContainsKey($Key)) { throw "Unknown key '$Key'." }
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        $wasMin = Enter-GameSurface $win
        [ArchonDrive]::KeyScan([uint16]$scanMap[$Key], $true)
        try { Start-Sleep -Milliseconds $Ms } finally { [ArchonDrive]::KeyScan([uint16]$scanMap[$Key], $false) }
        Write-Output "HeldKey=$Key ms=$Ms"
        if ($Name -ne 'capture.png') { Save-WindowCapture $win $Name $SettleMs }
        Exit-GameSurface $win $wasMin
    }
    'click' {
        Assert-HumanIdle
        if ($X -lt 0 -or $Y -lt 0) { throw 'click needs -X and -Y (screen coordinates).' }
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        $wasMin = Enter-GameSurface $win
        [ArchonDrive]::SetCursorPos($X, $Y) | Out-Null
        Start-Sleep -Milliseconds 250
        [ArchonDrive]::MouseButton($true)
        Start-Sleep -Milliseconds 80
        [ArchonDrive]::MouseButton($false)
        Write-Output "Clicked=$X,$Y"
        if ($Name -ne 'capture.png') { Save-WindowCapture $win $Name $SettleMs }
        Exit-GameSurface $win $wasMin
    }
    'look' {
        Assert-HumanIdle
        $state = Get-SessionState
        $win = Get-GameWindow $state.Pid
        $wasMin = Enter-GameSurface $win
        [ArchonDrive]::MouseMoveRelative($Dx, $Dy, 30, 10)
        Write-Output "Looked=dx:$Dx,dy:$Dy"
        if ($Name -ne 'capture.png') { Save-WindowCapture $win $Name $SettleMs }
        Exit-GameSurface $win $wasMin
    }
    'markers' {
        $state = Get-SessionState
        if (-not (Test-Path -LiteralPath $state.LogPath)) { throw "No log at $($state.LogPath)" }
        $found = [regex]::Matches((Get-Content -LiteralPath $state.LogPath -Raw), "ArchonFactoryCanary: [^\r\n]*$Pattern[^\r\n]*|$Pattern[^\r\n]*")
        if ($found.Count -eq 0) { Write-Output "MARKERS-NONE pattern=$Pattern" }
        $found | Select-Object -Last 25 | ForEach-Object { Write-Output "MARKER $($_.Value)" }
    }
    'status' {
        $state = Get-SessionState
        $alive = [bool](Get-Process -Id $state.Pid -ErrorAction SilentlyContinue)
        if (-not $alive) {
            Write-Output "Pid=$($state.Pid) Alive=false Log=$($state.LogPath)"
        } else {
            $win = Get-GameWindow $state.Pid
            $iconic = [ArchonDrive]::IsIconic($win.Hwnd)
            $fgIsGame = [ArchonDrive]::GetForegroundWindow() -eq $win.Hwnd
            # Restored + foreground while the harness is idle = a human is
            # (or was just) in the session.
            Write-Output "Pid=$($state.Pid) Alive=true Minimized=$iconic ForegroundIsGame=$fgIsGame HumanLikelyActive=$((-not $iconic) -and $fgIsGame) Log=$($state.LogPath)"
        }
    }
    'sessions' {
        $capRoot = Join-Path $ProjectRoot 'Saved\PlaytestCaptures'
        if (Test-Path -LiteralPath $capRoot) {
            $dirs = Get-ChildItem -LiteralPath $capRoot -Directory
            if (-not $dirs) { Write-Output 'SESSIONS-NONE' }
            foreach ($d in $dirs) {
                $count = (Get-ChildItem -LiteralPath $d.FullName -Filter *.png -ErrorAction SilentlyContinue | Measure-Object).Count
                Write-Output "SESSION $($d.Name) shots=$count path=$($d.FullName)"
            }
        } else {
            Write-Output 'SESSIONS-NONE'
        }
    }
    'clear' {
        # Auto-clear consumed playtest artifacts (Jonathan 2026-06-10):
        # run AFTER ingesting findings into the skill learning log.
        $capRoot = Join-Path $ProjectRoot 'Saved\PlaytestCaptures'
        if (Test-Path -LiteralPath $capRoot) {
            Remove-Item -LiteralPath $capRoot -Recurse -Force -Confirm:$false
            Write-Output 'ClearedPlaytestCaptures=true'
        }
        if (Test-Path -LiteralPath $shotDir) {
            Get-ChildItem -LiteralPath $shotDir -Filter *.png -ErrorAction SilentlyContinue | Remove-Item -Force -Confirm:$false
            Write-Output 'ClearedDriveShots=true'
        }
    }
    'stop' {
        if (-not $AllowLiveStop) {
            Write-Output 'StopRefused=true'
            Write-Output 'Reason=The live SPLITROOT canary is shared telemetry/playtest infrastructure. Do not stop it without Jonathan explicitly authorizing this specific interruption.'
            exit 2
        }
        $state = Get-SessionState
        if (Get-Process -Id $state.Pid -ErrorAction SilentlyContinue) {
            Stop-Process -Id $state.Pid -Force -Confirm:$false
            Write-Output "Stopped=$($state.Pid)"
        } else {
            Write-Output "AlreadyExited=$($state.Pid)"
        }
        Remove-Item -LiteralPath $statePath -Force -ErrorAction SilentlyContinue
    }
}
