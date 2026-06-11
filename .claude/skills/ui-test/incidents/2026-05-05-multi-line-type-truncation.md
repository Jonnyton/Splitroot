---
incident_date: 2026-05-05
short_name: multi-line-type-truncation
severity: p2
recurrences: 2
applied_by: cowork-vision
---

# Incident: chatbot multi-line input truncated by `type` action's `\n`

## Symptom

`computer.action=type` with text containing `\n` characters submits the message after line 1. Chatbot receives only the first line; rest of the message is lost. Probe wastes a turn re-asking; user-sim experience corrupted.

## Recurrences this session

1. Probe #008 first attempt — recipe paste with embedded `\n` lines; chatbot received only the lead-in question, not the recipe content. Manifested as chatbot asking for recipes in a template (it didn't see the ones I'd already pasted).
2. Probe #008b second attempt — markdown-formatted recipe paste with `\n` lines; chatbot received only `# Source: Aunt Linda` and replied "Got it — keep going. Paste Aunt Linda's full version."

## Root cause

ChatGPT (and most modern chatbot inputs) interpret bare Enter (`\n`) as Submit. The browser-automation `type` action sends each character as a keystroke, so embedded `\n` triggers Submit at the first newline.

## Immediate fix applied

Added Multi-line input section to `.agents/skills/ui-test/SKILL.md` and synced to `.claude/skills/ui-test/SKILL.md`. Section names three valid alternatives:

- A. Single-line collapse (replace `\n` with `. ` or just space).
- B. Line-by-line with `Shift+Return` between lines (no Submit).
- C. `form_input` with multi-line value (sets input directly, bypasses keyboard).

Plus a verify-after-submit rule: screenshot the chatbot's reply and confirm it addresses the FULL message, not just the first line.

## Auto-iterate ladder

- Rung 0 (before today): no documented rule. Recurrences possible.
- Rung 1 (today): SKILL.md section added; verify-after-submit rule mandatory.
- Rung 2 (next recurrence): build wrapper helper script that intercepts `type` calls containing `\n` and routes through Option B automatically.
- Rung 3 (recurrence after that): hook to reject `type` with `\n` in args.

## Cross-references

- `.agents/skills/ui-test/SKILL.md` (rule landed)
- `.claude/skills/ui-test/SKILL.md` (mirror)
- FUSE-truncation rule pattern in CLAUDE.md (template for stop-the-line / auto-iterate / hooks ladder)
- pages-notes-cowork-user-sim-probe-008-from-scratch-2026-05-05 (probe in flight when bug bit)
