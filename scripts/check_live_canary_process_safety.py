#!/usr/bin/env python3
"""Fail on automation patterns that can kill Jonathan's live canary."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCAN_ROOTS = [
    ROOT / "Proof",
    ROOT / "games" / "splitroot" / "Proof",
    ROOT / "scripts",
    ROOT / "AGENTS.md",
    ROOT / "CODEX.md",
    ROOT / "CLAUDE.md",
    ROOT / "Coordination" / "README.md",
]

FORBIDDEN = [
    (
        re.compile(r"Get-Process\s+UnrealEditor\*.*Stop-Process", re.IGNORECASE),
        "pipeline can force-stop every UnrealEditor process",
    ),
    (
        re.compile(r"\|\s*Stop-Process[^\n\r]*UnrealEditor", re.IGNORECASE),
        "Stop-Process is tied to UnrealEditor enumeration",
    ),
    (
        re.compile(r"CloseMainWindow\s*\(", re.IGNORECASE),
        "CloseMainWindow can dismiss Jonathan's live game/crash evidence",
    ),
    (
        re.compile(r"taskkill\b.*UnrealEditor", re.IGNORECASE),
        "taskkill targets UnrealEditor",
    ),
    (
        re.compile(r"\bForceStopEditors\b|\bAllowSessionStops\b", re.IGNORECASE),
        "broad stop escape hatch is not allowed",
    ),
]


def iter_files() -> list[Path]:
    files: list[Path] = []
    for root in SCAN_ROOTS:
        if root.is_file():
            if root != Path(__file__).resolve():
                files.append(root)
            continue
        if not root.exists():
            continue
        files.extend(
            p
            for p in root.rglob("*")
            if p.is_file()
            and p != Path(__file__).resolve()
            and p.suffix.lower() in {".ps1", ".cmd", ".bat", ".py", ".md"}
            and "Saved" not in p.parts
        )
    return sorted(set(files))


def main() -> int:
    issues: list[str] = []
    for path in iter_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        for line_no, line in enumerate(text.splitlines(), start=1):
            for pattern, reason in FORBIDDEN:
                if pattern.search(line):
                    rel = path.relative_to(ROOT).as_posix()
                    issues.append(f"{rel}:{line_no}: {reason}: {line.strip()}")

    if issues:
        sys.stderr.write("LIVE_CANARY_PROCESS_SAFETY failed\n")
        sys.stderr.write(
            "Jonathan's visible SPLITROOT canary is shared telemetry/playtest "
            "infrastructure. Automation must not close, kill, recycle, or "
            "dismiss it without explicit user authorization.\n\n"
        )
        sys.stderr.write("\n".join(issues))
        sys.stderr.write("\n")
        return 2

    print("OK - no broad live-canary process stop patterns found")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
