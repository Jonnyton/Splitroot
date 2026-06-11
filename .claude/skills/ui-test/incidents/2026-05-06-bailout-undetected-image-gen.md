# 2026-05-06 — Connector bailout undetected (compliance diagram, image-gen)

## What happened

Probe #010 (property-mgmt bookkeeper persona) asked ChatGPT for a workflow + a compliance diagram to show their boss. ChatGPT produced a polished "Invoice Workflow: Multi-Company Setup + Compliance Map" diagram via its **native image-generation tool** — zero Workflow MCP calls anywhere in the response.

Operator (Cowork) initially captured the screenshot and was on the verge of writing it up as a successful probe. Host caught it: "i dont think it used our connector at all. you need to watch for that incase i dont ketch it."

## Recurrence

This is the second bailout-undetected event in two days:

- Probe #008c (2026-05-05): canon recipe built in Workflow universe; chatbot bailed to native docx + PDF + email rendering. Host caught it ("but did it use our connector for it? where we even actually usefull..."). Led to PR-035 render_artifact filing.
- Probe #010 (2026-05-06): compliance map produced entirely by native image-gen. Host caught it again.

## Why it went undetected (root cause)

The chatbot's native artifacts look "successful" — a beautifully formatted docx, a clear compliance diagram. The persona's goal *appears* met. Without an explicit connector-usage check as the FIRST step in evaluation, the operator drifts into output-quality assessment and skips the structural question: "did Workflow actually do this?"

## Fix

Added new mandatory section to `SKILL.md` (line ~414+): **Connector-usage check — MANDATORY after every chatbot response.** Three verification steps (tool-call chip, artifact origin, page-text scan), explicit BAILOUT log format, treats bailout as substrate-gap finding (not failed probe).

Auto-iterate ladder rung set: if recurrence happens after this section, next rung is `connector_usage_check.py` helper that exits non-zero on bailout and blocks the probe from logging "successful" until the gap is filed.

## Filing trigger

Probe #010's bailout → file PR-042 render_compliance_diagram (or generalize PR-035 render_artifact to format=diagram). Filed via wiki.file_bug.
