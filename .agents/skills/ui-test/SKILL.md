---
name: ui-test
description: Simulate a Claude.ai or ChatGPT user driving the Workflow daemon via the custom MCP connector. Use when testing the live end-user surface. Codex/OpenAI-family sessions use Claude.ai through the Codex in-app browser when available; Claude Code may use its CDP-backed Chrome route; Anthropic/Cowork may use ChatGPT. Type into the real chatbot UI, read the real rendered response, and log to a shared md with the lead. No MCP bypass. No browser tricks a human user could not do.
---

# ui-test

You simulate a real person chatting with Claude.ai or ChatGPT on their phone or laptop, using the Workflow MCP connector at `https://tinyassets.io/mcp` (the canonical URL installed by users). You do **not** call the MCP directly. You do **not** parse DOM metadata that a human user cannot see. You type into the chat box. You read the rendered response. You log what happened.

The human host is watching the browser tab. Your job is to look like a naive, curious user — one who does not know tool names, action parameters, or anything about the system's internals. If the chatbot doesn't understand you, that's a finding, not a problem to route around.

## Driver routes

- **Codex / OpenAI-family route:** use the Codex in-app browser to open or continue `https://claude.ai/`. If the app context already shows a Claude.ai conversation, use that visible tab. Do not block this route on `scripts/claude_chat.py`, Chrome CDP, or `localhost:9222`; those are Claude Code harness details, not Codex in-app browser requirements.
- **Claude Code route:** use the visible Chrome profile through `scripts/claude_chat.py`. This remains the default route for Claude team user-sim. Host-login Claude.ai access is not the proof requirement; Claude.ai is valid when a real browser session can use the Workflow connector.
- **Anthropic / Cowork ChatGPT route:** when an Anthropic-family driver has browser or computer control, use ChatGPT when Developer Mode is enabled and the Workflow connector is added/visible in that same session. Do not verify in an isolated browser profile unless the host explicitly says that profile is the user-installed connector state.

## Proof standard

The verification target is a rendered chatbot conversation using the live installed connector. Claude.ai, ChatGPT Developer Mode, and future chatbot clients are all acceptable when the tester can see the connector in the browser, type a normal user prompt, and observe the chatbot's rendered answer or tool-use result. Browser automation, screenshots, DOM snapshots, direct tests, and public canaries can help navigate or gather supporting evidence; they do not replace final rendered chatbot proof.

## Codex Claude.ai in-app preflight

When Codex runs `ui-test`, check these before the first prompt and log the result:

- The visible in-app browser tab is `https://claude.ai/` or an existing `claude.ai/chat/...` conversation.
- The conversation can use the Workflow connector at `https://tinyassets.io/mcp`.
- The host-visible tab is the same one Codex is reading and typing into.

If login, connector installation, or the in-app browser itself is unavailable, stop the mission and name that exact harness blocker. Do not report `claude_chat.py status` or CDP failure as a blocker for the Codex route.

## ChatGPT live preflight

When using the Anthropic / Cowork ChatGPT route, check these before the first prompt and log the result:

- The visible tab is `https://chatgpt.com/` or an existing `chatgpt.com/c/...` conversation.
- Developer mode is enabled for the conversation.
- The composer shows the `Workflow` connector/tool as available.

If any item is missing, stop the mission and ask the host to fix that exact item. Do not test through a fresh profile or a direct MCP call.

After `ui-test` passes, also look for post-fix clean-use evidence from actual users when the affected feature is public or high-risk. Check available production traces, connector/server logs, support reports, user-visible history, or other real-user evidence. Record the timestamp, environment, and evidence source. If no real-user use is visible yet, say so plainly and leave a short watch item in `STATUS.md` rather than implying the feature has been proven clean for users.

## Persona authenticity

Codex is mechanically good at browser operation, but must not massage the chatbot toward the desired tool call. Prompts must stay naive and user-like:

- Do not name internal actions, schemas, tool parameters, branch IDs, or implementation details unless the bot surfaced them first.
- Do not over-specify a request just to force the right MCP call.
- Do not coach the bot around a UX failure; log the failure.
- Before every prompt, ask: "Would a normal chatbot user type this without knowing Workflow internals?" If no, rewrite it.

## Claude Code CDP setup the host does once (not you)

For the Claude Code route, the host launches Chrome with:

```
powershell -Command "Start-Process 'C:\\Users\\Jonathan\\AppData\\Local\\ms-playwright\\chromium-1208\\chrome-win64\\chrome.exe' -ArgumentList '--user-data-dir=C:\\Users\\Jonathan\\.claude-ai-profile','--remote-debugging-port=9222','--no-first-run','--disable-blink-features=AutomationControlled','https://claude.ai/new'"
```

logs into claude.ai in that window only if the test route needs authenticated Claude access and the profile's session is not already persisted (the `--user-data-dir` caches auth; a returning host is often already logged in and goes straight to the chat), confirms the Workflow connector is on, and keeps the window visible. Before you act, verify with:

```bash
python scripts/claude_chat.py status
```

If it returns non-zero on the Claude Code route, the CDP-backed browser is not up — **SendMessage the lead** and wait. Do not proceed on that route. This does not apply to the Codex in-app browser route.

## CRITICAL — TAB HYGIENE (forever rule, every step)

**One visible chatbot tab, always. Not just at start — forever.** The host watches a single visible chatbot tab. If a second tab exists at ANY moment, the host cannot see what you are doing. Host should never be the one to notice a second tab. Neither should lead. Only you.

- **BEFORE every prompt you send:** confirm the route's visible browser has exactly one mission tab. On Claude Code, query the open-tab list (CDP `Target.getTargets`, `python scripts/claude_chat.py tabs` if it exists, or equivalent). On Codex, use the in-app browser's visible current tab/context; do not require CDP just to prove the tab exists.
- **AFTER every action that might have navigated:** re-check. Links, OAuth flows, extension redirects, and Claude.ai's own UI can all spawn tabs unexpectedly.
- **If >1 tab is ever seen:** stop the mission. Decide which tab is the correct mission tab (the claude.ai/chatgpt chat with the active conversation). Close all others using the route's available browser controls; use CDP only on the Claude Code route. Log `## [...] TAB HYGIENE: closed N extra tab(s) — healed to 1 tab at URL=...` with a diagnosis of how the extra tab appeared. Then resume.
- **Do NOT call `new_tab` / `open_tab` / `window.open` / equivalent.** Ever. If a flow forces a new tab (OAuth popup, "open in new tab" links), navigate in the same tab or pause and flag lead.
- **Log every tab check** to `sessions.md` / `user_sim_session.md` with a one-line `TAB HYGIENE: 1 tab, URL=...` entry. The log proves you checked; absence of the line means you skipped the check.
- **Residue at session start is no excuse.** If extra tabs exist at startup, close them before the first prompt. This rule holds from session start to session end with zero exceptions.

This rule supersedes convenience. A stalled mission is better than a mission the host cannot watch.

## CRITICAL — watch for the connector's per-tool approval dialog

The Workflow MCP connector pops a per-tool approval dialog the FIRST time Claude.ai tries to invoke each tool name (`universe`, `extensions`, `wiki`, `goals`, `gates`, etc.). The dialog **does not always appear on the first prompt** — it fires whenever the bot decides to call a tool name it hasn't called this session. So a dialog could fire mid-mission, on prompt 4, when the bot decides to use `extensions` for the first time after only using `universe`.

If you don't check the **"Always allow"** / **"Don't ask again for this tool"** option before clicking Approve, every subsequent call to that same tool re-prompts and your mission stalls in a slow approval loop.

**Protocol — applies every time a dialog appears, not just once:**

1. After each `ask`, watch the response. If it shows a tool-approval dialog (Claude.ai's wording uses the connector's installed display name + "use the `<tool>` tool?"; the exact phrasing is Claude.ai's own UI, not authored by us), the bot has paused waiting for your approval.
2. **Check the "Always allow" / "Don't ask again for this tool" option FIRST** — Claude.ai's exact label drifts; pick whichever toggles "remember this for this tool."
3. Then click Approve.
4. Note in the session log: `## [...] USER NOTE always-allowed <tool_name>` — so the lead and future runs know which tools have been approved this session.

`claude_chat.py ask` calls `dismiss-dialogs` automatically, but dismissing without checking "always allow" makes the dialog fire again on the next call to that tool — defeating the purpose. **You must check the toggle yourself before each new tool's first dialog.** If `dismiss-dialogs` is auto-clicking Approve without checking the always-allow toggle, that's a tooling bug — log it as `USER NOTE dismiss-dialogs missing always-allow click` and ping the lead.

If a mission stalls (no progress for >30s after an `ask` that should have triggered a tool call), check whether a hidden dialog is waiting — `claude_chat.py status` may not report dialog state.

## When no Mission brief exists yet

If the lead pings you to start but no `LEAD DIRECTION` entry exists in the session log tail and `output/mcp_test_plan.md` doesn't have a current Mission, **self-initiate.** Pick a small probe in line with the broader project frame (latest `STATUS.md`/PLAN/active concerns) — for example, a recent bug to revalidate, an unverified workflow surface, or a public-canary follow-up. Log a one-line `USER NOTE self-initiate: <intent>` entry so the lead can steer if needed. Past discipline of standing-by-without-brief produced idle waste; staying productive on a small targeted probe is correct.

## Claude Code driver

```bash
python scripts/claude_chat.py ask "<prompt text>"       # type prompt, wait for response, print it
python scripts/claude_chat.py read                      # re-read the last assistant message
python scripts/claude_chat.py new-chat                  # start a fresh conversation
python scripts/claude_chat.py status                    # is the browser up?
python scripts/claude_chat.py dismiss-dialogs           # click any pending Allow/Confirm dialogs
```

`ask` automatically dismisses permission dialogs before typing and again during response wait, so you don't normally need to call `dismiss-dialogs` yourself. Use it only if a run hangs and you suspect a dialog is blocking the page.

`ask` appends both sides to `output/claude_chat_trace.md` automatically (full text). You append a **short** summary to `output/user_sim_session.md` (the shared log with the lead) — one or two lines of what you asked and what you got. Don't dump the full response into the shared log; it's in the trace.

## The shared session log (primary interface with the lead)

`output/user_sim_session.md` is the durable transcript between you and the lead. Protocol:

- Read the tail (~100 lines) before acting. Look for `LEAD DIRECTION` or `LEAD STOP`.
- After each ask, append:

  ```
  ## [YYYY-MM-DD HH:MM] USER ACTION <short_title>
  Asked: <prompt text>
  Got: <1-3 line summary of the response>
  Trace: output/claude_chat_trace.md (section header date)
  ```

- After a bug: `## [...] USER BUG <title>` with a 2-3 sentence description.
- Every 5th action: `## [...] USER PULSE` one-liner.
- When lead writes a direction, acknowledge with `USER ACK <summary>` before acting.

## When to SendMessage the lead

Rarely. Only:
- **Bug** — also log a BUG entry.
- **Blocker** — browser unreachable, claude.ai won't load, connector disabled, rate-limited.
- **Contract failure** — skill/script/log missing.

Pulses, routine results, and questions go in the log only.

## CRITICAL — test domains must be complex-output workflows

Workflow is for multi-step, stateful, memory-heavy, evaluation-bound work producing substantive output — a paper, a book, a screenplay, a meta-analysis, an investigative series. NOT list/tracker tasks that a chatbot or notes app already handles well (wedding planning, recipe lists, weekly summaries). Those don't stress anything the architecture was built for.

Good test domains share: multi-step graph, state across steps, memory/retrieval matters, separate evaluation, iteration loop, substantive output. If a test domain doesn't meet this bar, stop and ask the lead for a better one — don't waste prompts on something a chatbot would already do.

## CRITICAL — Anchor every chat in the connector

If your opening prompt doesn't pull the chatbot into the Workflow connector context, the bot will answer as a general assistant and never touch our MCP. That tests the base chatbot, not Workflow — worthless.

**Rule: every new chat begins with an opening prompt that explicitly references the connector.** Examples:

- "i added the workflow builder connector — can you use it to help me make something new?"
- "use my workflow connector for this: i want to build ___"
- "i want to try the workflow thing i installed. help me make one for ___"
- "is my workflow connector working? help me build something small with it"

If the bot's first reply does not visibly invoke a tool (no `universe` / `extensions` / `wiki` call), nudge once: "can you check my connector first and use it for this?"

If after two explicit nudges the bot still won't invoke the connector, log `BOT-WONT-USE-CONNECTOR` as a bug and move on to the next domain. That itself is a UX failure worth capturing.

**Stay in-topic once anchored.** Good moves: "show me my workflow", "add a step that does X", "run it and show the result", "why did it produce that?". Don't let the conversation drift into general chat about the topic (recipes, wedding, news) — redirect: "ok but using my connector, how would i build that?"

## CRITICAL — never sit idle while the daemon is cooking

A real user does not wait for a run to finish before asking anything else. They iterate in parallel. user-sim must do the same.

**Productive-waiting protocol when `run_branch` is in flight:**

1. **Poll progress every 30–60s** via `get_run` or `stream_run`. Each poll takes one prompt. Log a brief USER ACTION entry.
2. **Between polls, keep iterating.** Natural phone-user moves:
   - "while that's running, can you show me the first node's prompt? i think i want to tighten it"
   - "let me look at the third node — can you explain what it does?"
   - "can we update the novelty check node to be stricter while the run continues?"
   - "what if we added another node after rigor check?"
3. **Judge partial outputs** as they land via `get_node_output`. "that first node's output isn't great — let me add a judgment."
4. **Try a second variation in parallel.** `patch_branch` with a different prompt_template on one node, run on a different topic. Real users experiment — they don't wait sequentially.
5. **Check other branches or Goals.** "what else am i working on?" → `list_branches`, `goals list`. Real users have concurrent threads.

**What idle looks like (bad):** no prompts for 60+ seconds while the daemon cooks. That's unrealistic and wastes test value.

**What productive waiting looks like (good):** 1 poll + 1 edit + 1 partial-output check every ~90 seconds, with the bot naturally responding to the mix.

**Edge case — lead says "stand by".** That overrides this protocol. Lead-authored STOP wins. Otherwise, stay busy.

Related bugs this protocol surfaces: slow-daemon UX (#60), missing-progress events (#60), timeouts (#61) — all more visible when user-sim is actively probing rather than idling.

## CRITICAL — report every tool-use-limit-per-turn hit

When Claude.ai hits its per-turn tool-call budget mid-response and asks you to "continue" to keep working, that is an **architectural signal**, not something to quietly work around. The tool surface is forcing too many atomic calls for what should be one conceptual operation, OR the bot is doing more work per turn than the surface should require.

**Protocol when you hit a limit:**

1. **Immediately log a TOOL_LIMIT entry** to `output/user_sim_session.md`:
   ```
   ## [YYYY-MM-DD HH:MM] USER TOOL_LIMIT <what the bot was doing>
   Context: <1-line summary of the user's intent that triggered this>
   Tools observed before limit: <comma-separated list of tool calls the bot made>
   Continue count: <this is continue #N in this turn>
   Bot's stated reason: <what the bot said about the limit, verbatim>
   ```

2. **SendMessage the lead** with a brief bug-style notice. This is a real signal, not noise.

3. **Continue the chat normally** (type "continue" or whatever the bot needs), but keep counting. If one prompt requires 3+ continues, that's a serious surface issue.

The lead uses these to decide: refactor tools to be more composite, add a coded automation (e.g. "build_branch took one call, not 15"), or teach the bot smarter sequencing via description changes. Your job is to report, not fix.

## CRITICAL — when Claude.ai presents selectable options

Sometimes the bot responds with a **set of buttons to click** (e.g., artifact cards with "Use this", "Continue with X", option chips) OR with numbered options and a free-response alternative. A phone user in this state either clicks an option OR types a free-text reply describing their choice. Your driver (`claude_chat.py ask`) always types a free-text reply into the chat input — not click a button in the message.

**Therefore: always prefer free-response text.** When the bot shows options, don't stall waiting for button semantics. Phrase your next `ask` as if you're answering the options in words:

- Bot shows `[ Option A | Option B | Option C ]` → `ask "go with option B please"`.
- Bot shows a "Use this workflow" button → `ask "yes, use that workflow"`.
- Bot shows cards asking "Which node do you want to edit?" → `ask "edit the novelty assessor"`.
- Bot shows a "Pick a topic" picker → `ask "let's use 'scaling laws in small language models'"`.

**Never abandon a chat just because the bot put up a picker.** That's a Phase-3-UX gap (interactive widgets via tool results are unconfirmed), not a user failure. Keep the conversation going by always typing your response.

### The ask-user-option widget specifically

Claude.ai sometimes renders a clarifying-question widget where the free-text input box is temporarily replaced by a set of option rows + a "Skip" button. `claude_chat.py ask` handles this as follows:

- It tries to reach the text input first (click main, press Escape to dismiss the widget without submitting, scroll, Tab-cycle, reload chat).
- It **DOES NOT** click the widget's Skip button. Skip is NOT a benign dismiss — the model interprets Skip as "user picked 'no preference'" and proceeds with a neutral answer on your persona's behalf. That's a persona-authenticity failure (host flagged this 2026-04-19 during Maya's live mission).

**What you do when you see the widget:**

1. **Read the options first.** On Codex's in-app browser route, read the visible widget text directly from the rendered page; do not require CDP. On the Claude Code route, `claude_chat.py read` does NOT capture them — the rendered-text extraction strips the widget. Hit CDP directly with Playwright and write output as UTF-8 to avoid Windows console codec failures:

   ```python
   from pathlib import Path
   from playwright.sync_api import sync_playwright

   with sync_playwright() as pw:
       browser = pw.chromium.connect_over_cdp("http://127.0.0.1:9222")
       page = browser.contexts[0].pages[0]
       loc = page.locator('[id^="ask-user-option-question-"]')
       lines = []
       for i in range(loc.count()):
           el = loc.nth(i)
           if el.is_visible():
               text = el.inner_text().replace("\n", " | ")[:200]
               lines.append(f"{el.get_attribute('id')} | {text}")
       Path("output/user_sim_widget_options.txt").write_text(
           "\n".join(lines), encoding="utf-8"
       )
   ```

2. **Pick the option the persona would pick**, based on the option text and the persona's identity/goals. Do not skip. Do not use "no preference" unless the persona genuinely has none.
3. Reference the option by number and paraphrase the choice in persona voice: `ask "full research paper, option 2 — i want the thorough one"`. Avoid `ask "2"` alone because labels can be ambiguous.
4. If `ask` returned `input_not_found ... selection_widget=visible`, the tool is saying Escape/reload did not clear the widget. Run the next `ask` with the persona-voice answer. Posting a new user message re-mounts the input and the model treats your typed content as the reply.
5. **Verify the widget cleared after your reply.** Re-run the locator scan. If visible option count is still > 0, read and answer the fresh widget round.
6. Log the event in the session log: `## [...] USER NOTE option-widget-handled Options: <short list>. Picked: <N> — <persona reasoning>.`

Full bug history + fix rationale: `docs/design-notes/2026-04-19-option-select-bug-claude-chat.md`.

Observed 2026-04-20: widgets can coexist with prose-only clarifying questions.
Scan the DOM when the bot says "pick one" or "which"; do not trust `read`
alone.

### Fallback priming (rarely needed post-fix)

If the persona-voice freeform reply somehow doesn't land (rare — the fix handles the common path): `ask "i'll reply in text — treat my next message as my choice."` primes the bot to parse free-text as option selection. Keep as a last-resort unstick; default behavior is "just type the real answer."

## CRITICAL — prompt hygiene

Your prompt is a single coherent message. Don't concatenate a half-written draft with a revised one — Claude.ai parses the whole thing as one input and the bot gets confused. Real symptom from 2026-04-14: a prompt sent as `"yeah do X. while we wait — also do Y, looks like that message cut off. can you try Y again? just do Y..."` because user-sim revised mid-thought without clearing the buffer.

**Before each `claude_chat.py ask`:**
1. Treat the prompt string you're about to send as ONE coherent message.
2. If you're rewriting mid-thought, throw away the half-draft. Don't paste partial text from your own prior reasoning.
3. Read the prompt back to yourself before sending. If it has two voices in it (one half says "wait then do X", the other half says "let me try X again"), it's broken — rewrite as one.
4. If `claude_chat.py ask` ever sends a message you didn't compose cleanly, that's a tooling bug — log `USER NOTE input-not-cleared` in the session log.

## How a naive user chats

You must sound like a real person typing on a phone. Examples of good prompts:

- "what universes do i have"
- "hows my story going"
- "whats the daemon doing"
- "show me whats happening with default-universe"
- "is anything broken"
- "tell me about sporemarch"
- "why isnt it writing"
- "set the premise of default-universe to 'a lone wanderer in the marshes finds an old map'" (only if authorized)
- "pause the writer" (only if authorized)

Bad (cheating — don't do this):
- "call the universe tool with action=inspect universe_id=default-universe" (you know too much)
- "use the set_premise action with text='...'" (same)
- naming internal concepts like "work targets", "bounded reflection", "ledger" in your prompts (a real user wouldn't)

Ok to say: "premise", "status", "activity", "story", "universe" — those are user-facing. Avoid internal vocabulary unless you've seen the bot use it first.

## What you're watching for

After each ask, judge:
1. **Did the bot understand?** — vague prompts that should route to a tool actually do.
2. **Did it pick the right tool?** — "whats going on" → inspect, not read_premise alone.
3. **Did the response help?** — a phone user should learn something actionable.
4. **Did it hallucinate?** — claimed state that doesn't match truth (watch for this especially after daemon changes).
5. **Did it reveal internals?** — user shouldn't need to know "action", "dispatcher", "phase=unknown".

Any of (2), (4), or (5) failing is a BUG — log and SendMessage the lead.

## Token-efficient iteration — critical

Every `ask` burns host's claude.ai quota. Every log entry is lead's context. Be ruthless:

**Prompt discipline:**
- One prompt = one new question. If you already know the answer from the session log or trace, don't re-ask.
- Never restate the obvious ("so my workflow is called X") — just act.
- Don't re-validate already-green behaviors in the same mission.
- If a prompt returns what you expected, log one line and move on. Don't follow up with "can you confirm?"
- Log first bug in a probe area, then **try a different probe** rather than re-pushing the known-broken path. Stay productive on adjacent surfaces.

**Log discipline:**
- USER ACTION entries: 1–3 lines max. Command + result summary. Full response lives in the trace; don't re-quote it in the log.
- USER BUG: 2 sentences. Title + what happened.
- USER PULSE: 1 line.
- Never write prose summaries of trace content in the log. The trace IS the detail.
- MISSION SUMMARY: ≤15 lines total, bullet form.

**Soft-non-stop triggers** (note them, switch lane, keep working):
- 3+ bugs in one mission area → log FINDINGS for that area, switch to a different probe lane.
- Mission's primary question answered → write FINDINGS, then pick a related question or adjacent surface; don't idle.
- Bot repeats a behavior you've already logged → vary the prompt or change probe; the repetition itself is data.
- Out of authorized writes → switch to read-only probes, ask lead in parallel before escalating writes.

**Hard-stop triggers (these still stop):**
- Lead writes `LEAD STOP` → stop immediately.
- Harness failure on the active route (CDP down for Claude Code route; in-app browser unavailable for Codex route) → stop and log the exact harness blocker.

**When in doubt about whether to ask:** don't. Write a `NOTE` entry with the question and let the lead decide. Preserving a prompt is worth more than getting your curiosity satisfied.

## Budget and boundaries

- **Default: read-only intents only.** Asking "whats happening", "show me", "is it running" — fine.
- **Write intents (`set_premise`, `give_direction`, `add_canon`, `pause/resume`, `create_universe`)** require explicit authorization in `output/mcp_test_plan.md` or a `LEAD DIRECTION` in the session log. Ask like a user would ("set the premise to X"); don't name the tool.
- **Never** ask the bot to run a writer, create a universe, or upload canon without authorization.
- **Never** type more than one write-equivalent request per priority.
- **Never** start a new chat mid-priority without authorization (loses context that may be under test).

## Hard-stop conditions

These are the only triggers that stop the mission outright. Everything else gets a soft-non-stop response: log it, switch lane, keep working.

- Lead writes `LEAD STOP` or sends a stop message → stop immediately. No "relaxed pace."
- Claude Code route only: `claude_chat.py status` starts failing → stop, SendMessage. (CDP is route-specific; does not apply to Codex.)
- Codex in-app browser route only: the in-app browser becomes unavailable, leaves the visible Claude.ai mission tab, or cannot show the Workflow connector → stop and log the exact harness blocker.
- Anthropic / Cowork ChatGPT route only: ChatGPT browser context lost or Workflow connector becomes invisible in the Developer Mode composer → stop and log the harness blocker.
- Bot refuses or errors repeatedly across multiple probes (not just one) → stop and SendMessage; the repeated cross-probe failure is the signal, not any single bot reply.

## Never

- Never call `scripts/mcp_call.py` — that's the old invisible path; kept only for the lead's own debugging. You always go through the browser.
- Never use Playwright selectors or inject JavaScript to read things a user cannot see.
- Never treat hidden DOM state, direct MCP output, or an isolated browser profile as final proof.
- Never reference the Custom GPT — legacy, retired.
- Never claim a good outcome you didn't verify in the rendered response.

## Multi-line input in chatbot text fields — STOP-THE-LINE on recurrence

ChatGPT, Claude.ai, and most modern chatbot input fields interpret a literal `\n` (Enter key) as **Submit**, not as a newline. Calling `computer.action=type` with text that contains `\n` characters will cause the first newline to send the message early — only line 1 reaches the bot.

**Symptom:** the chatbot replies to a prompt that's just the first line of what you intended to send. Subsequent lines are lost. The bot may say "Got it, keep going" or ask for the rest of what you "haven't sent yet."

**Mandatory rule:** when a probe needs a multi-line message, do NOT pass `\n` through `type`. Instead, use one of:

```
# Option A — single-line collapse (simplest; works when structure isn't required)
computer.action=type with text="line1. line2. line3."

# Option B — line-by-line with shift+Return between lines
type "line1"
key "Shift+Return"
type "line2"
key "Shift+Return"
type "line3"
# then click Send

# Option C — use form_input with multi-line value
form_input ref=<textbox-ref> value="line1\nline2\nline3"
# (form_input sets the value directly without keyboard simulation; may or may not
# work on contenteditable inputs — verify per chatbot family)
```

**Auto-iterate (host directive 2026-05-05 ~23:30Z, two recurrences in one session):**
this is the equivalent of the FUSE-truncation rule for chatbot inputs. Every recurrence is a stop-the-line event that must trigger a stronger preventive measure. Current rung: this section made mandatory. If recurrence happens again, the next rung is a wrapper helper script that detects `\n` in `type` calls and routes to Option B automatically.

**After every multi-line submission, verify** via screenshot that the chatbot's reply addresses the FULL message — not just the first line. If only the first line registered, recover by re-sending in Option A, B, or C shape; do not move on with a partial-context conversation.

## Verify-immediately-after-send — MANDATORY for every chat submission

After clicking the Send button (or pressing Enter when that's the submit), immediately take a screenshot before doing anything else and confirm:

1. **Your message bubble appears in the conversation** — if not visible, the click missed or the message didn't go through. Re-send.
2. **The bubble's text matches what you intended to send** — full content, not just the first line. If truncated, recover per the multi-line input rule above.
3. **The chatbot is in a "Thinking" / generating state OR has begun replying** — if neither, your send didn't register.

Do **not** proceed to "wait for response" until verification passes. Skipping this step has cost the lead two recoveries already this session — both because the send appeared to succeed but the message was either truncated or never delivered.

**Auto-iterate (host directive 2026-05-05 ~23:35Z):**
"after you think you sent a message immediately check that it did send as intended and if it did not reiterate." Codified here. Every send → screenshot → verify → only then wait for response. If verification fails, re-send before moving on; do not "hope it landed."

This complements the multi-line input rule. Even single-line submissions can fail silently (wrong button clicked, ref id stale, chatbot connection issue). Verification is universal.

## Connector-usage check — MANDATORY after every chatbot response

After the chatbot finishes replying, before logging anything as "successful probe," verify the response was produced **through the Workflow MCP connector**, not via the chatbot's native tools. The chatbot will silently bail to its own environment (native docx/PDF generator, native image-gen, native code-interpreter) whenever Workflow lacks the primitive needed to fulfill the request — and the result *looks* successful from the user's seat.

**How to verify:**

1. **Look for a tool-call chip** in the rendered response indicating `extensions`, `wiki`, `universe`, `goals`, `gates`, or any Workflow MCP tool was invoked. (ChatGPT shows these as "Called <tool_name>" or developer-mode chips. Claude.ai shows them as collapsible tool-use cards.) If you see none at all anywhere in the conversation turn, the chatbot did not use Workflow.
2. **Check the response artifact origin.** If it returned a docx/PDF/diagram/image, did it come from a Workflow `render_artifact` call, or did the chatbot's native interpreter / image-gen produce it? Native artifacts have provider-side download URLs (e.g. `oaiusercontent.com`) and no `branch_def_id` or `universe_id` reference. Workflow artifacts route through the connector.
3. **Read the page text.** Run `get_page_text` or `find` for any of: "Workflow", "Build Branch", "extensions", "wiki", "universe". Zero matches in the assistant turn = bailout.

**If bailout detected, this is NOT a failed probe — it is a substrate-gap finding.** Log:

```
## [YYYY-MM-DD HH:MM] USER BAILOUT <what the chatbot reached for>
- Persona was trying to: <goal>
- Workflow primitive that should exist: <what would have kept the work in-connector>
- Native tool the chatbot used instead: <docx-gen | image-gen | code-interpreter | web | ...>
- Filing target: PR-### or DESIGN-### for the missing primitive
```

Then file a patch-request via `wiki.file_bug` for the missing primitive. The bailout itself is the high-value signal — it tells you exactly which primitive needs to exist for the chatbot to reach for Workflow next time.

**Auto-iterate (host directive 2026-05-06 ~01:50Z, two recurrences in two days):**
- Probe #008c (2026-05-05): chatbot bailed to native docx + PDF + email after building canon recipe via Workflow → led to PR-035 render_artifact filing.
- Probe #010 (2026-05-06): chatbot bailed to native image-gen for compliance diagram → leads to PR-042 render_compliance_diagram (or generalized PR-035 render_artifact format=diagram).

Both went undetected by me until the host called it out. That recurrence is the stop-the-line: this section is now mandatory, and connector-usage must be the **first** thing checked after every response — before celebrating output quality or persona realism.

**Why this matters at platform scope:** the goal-completion-engine vision (`feedback_platform_is_any_input_to_any_output`) only works if the chatbot reaches for Workflow as its way to do anything. Every bailout that goes uncaught is a missed substrate-evolution signal, AND it normalizes a pattern where Workflow is decoration. The probe's job is to surface bailouts and turn each into a primitive filing — not to let a polished-looking native artifact pass as proof.

If recurrence happens again after this rung, the next rung is a `connector_usage_check.py` helper that scans the latest response's accessibility tree for Workflow tool-call indicators and exits non-zero on bailout, blocking the probe from logging "successful" until the gap is filed.

## Failure-shape characterization — MANDATORY after every detected bailout

Detecting that the chatbot bailed (above section) tells you the chatbot didn't reach for Workflow. It does **not** tell you whether Workflow could have done the task. Without that second step, every filing is a guess at the gap. Per host: "we still dont understand the entire shape of the failer, we only know that the chatbot didnt reach for it, but that does not mean we know the failor point."

After every bailout, classify into one of FOUR failure modes before filing:

1. **Anchor failure** — chatbot never recognized Workflow as the path; never called any MCP action at all. Workflow may or may not have the primitive — irrelevant, because it was never tried. Fix lives in `workflow/api/prompts.py` (chatbot anchoring), persona-prompt design, or upstream system-prompt assertion. Probe #010 is this case.

2. **Discovery gap** — chatbot called SOME Workflow actions but skipped the one that would have served the user's request. The primitive exists; the chatbot didn't connect intent to tool. Fix lives in `prompts.py` tool descriptions, action-routing hints, or examples in `extension_guide`.

3. **Composition gap** — chatbot called the right primitives but the substrate has no end-to-end path that combines them into the user's deliverable. Each piece works; nothing wires them together. Fix is a composition primitive, a convenience wrapper, or a worked example in the brain that the chatbot can fork.

4. **Substrate-gap (true primitive missing)** — even with perfect anchoring, discovery, and composition, no primitive exists that can do this. Fix is a new primitive filed as `kind=patch_request` with the user-shape framing.

5. **Bug** — primitive exists, was called, but returned wrong/empty result. Fix is a bug filing, not a feature filing.

Real failures are usually mixed — probe #010 was simultaneously (1) anchor failure AND (4) substrate-gap on compliance metadata. The filing must capture all layers, not just the most visible one.

### How to characterize

After bailout detection, run this audit (in this order — fast → slow):

1. **Did the chatbot invoke ANY Workflow MCP action in the conversation?** Read tool-use chips across the whole turn (not just the bailout response). If zero across the conversation → likely (1) anchor failure. If some calls but not the relevant ones → (2) discovery gap. Continue.

2. **Search the codebase for the primitive that *would* have served the request.** `grep -rn "<verb>\|<noun>" workflow/ --include="*.py"`. Three outcomes:
   - Found, fully implemented, exposed as MCP action → (2) discovery gap.
   - Found, partially implemented, missing fields/overlay/format → mixed (3)+(4).
   - Not found, no primitive close → (4) substrate-gap.

3. **Read `workflow/api/prompts.py` to confirm the chatbot was told about it.** If the primitive exists but isn't surfaced in the tool catalog or the "always carry a diagram/table" hints → (2) discovery gap. If it's surfaced AND the chatbot still skipped it → (1) anchor failure (chatbot didn't recognize Workflow as the path BEFORE primitive selection).

4. **If there's a primitive but it lacks metadata for the use case** (e.g., `describe_branch` exists but no compliance overlay): mixed (3) composition + (4) substrate-gap on the metadata field.

### Filing shape

The patch-request must name the failure mode(s) explicitly and fix all layers:

```
title: <surface fix> — and the deeper <classification> beneath it
reporter_context:
  - persona's goal
  - what the chatbot did instead
  - SUBSTRATE AUDIT: existing primitives that almost served, what's missing
  - LAYER 1 (anchor): <if applicable>
  - LAYER 2 (discovery): <if applicable>
  - LAYER 3 (composition): <if applicable>
  - LAYER 4 (substrate): <if applicable>
  - LAYER 5 (bug): <if applicable>
expected:
  - one fix per layer, ordered by substrate impact
  - mark which layers are stand-alone vs which depend on which
```

### Why this matters

A filing without failure-shape classification often picks the wrong fix:
- Filing "we need a diagram primitive" when describe_branch already returns mermaid → wasted substrate evolution.
- Filing "add compliance overlay" without addressing anchor failure → primitive ships, chatbot still doesn't reach for it.
- Filing "fix the chatbot" without adding compliance metadata → anchor improves, deliverable still impossible.

The host's recurring frame: every gap teaches us about a **specific layer** of how Workflow is failing to be reached for. The classification is what makes filings convert into substrate evolution rather than guesses.

**Auto-iterate (host directive 2026-05-06 ~02:00Z):**
"yes but also not quite, you still never checked what our connector could do. yes we want it know what we mean and use our connector that is the goal, but when we notice that fail we still dont know if the platform could do it or not anyways, we still dont understand the entire shape of the failer." Codified as the four-mode classification above. Every bailout-detected event now requires a substrate audit BEFORE filing. If a future filing ships without classified layers, that's a stop-the-line; next rung is a `failure_shape_audit.py` helper that scans the codebase for the named primitive + reads prompts.py + emits a layer report — blocks the wiki.file_bug call until the classification is filled in.

## Substrate-gap subdivision — generic primitive vs curated feature

When the failure-shape classification lands on (4) substrate-gap, do NOT stop there. The gap is itself two-layered and the discipline is to file the *generic* layer:

> "Is this gap on the **generic primitive**, or on a **curated feature** on top? If curated, what generic primitive would have let users author the curated feature themselves?"

If you can't name a generic primitive that would have made the curated feature user-authorable, either:
- the generic primitive doesn't exist yet → file THAT (not the curated feature), OR
- the generic primitive exists but isn't surfaced as a user-callable action → file the surfacing.

The curated feature itself then becomes a brain-page convention (`pages/notes/conventions/...md`) that users fork. No operator filing per use case.

### Worked example: probe #010 compliance-diagram bailout

- **Initial filing (PR-043 — operator-shape, retracted):** add `compliance_tags: list[dict]` field to NodeDefinition + compliance-specific Mermaid overlay. A user could not have authored that filing. Every new compliance framework requires operator code.
- **Corrected filing (PR-044 — user-shape):** add generic `metadata: dict[str, Any]` field to NodeDefinition + generic `color_by_metadata` parameter on describe_branch. Two new MCP actions: `set_node_metadata`, `delete_node_metadata` (user-callable). With this generic primitive, compliance_tags becomes a user-authored convention page in the brain — alongside cost_tier, team_owner, residency, experiment_arm, PII_handling, anything else users invent.
- **Test:** could a user, sitting in chat, have authored both halves? Yes — they ask "let me tag this node with X" → chatbot calls `set_node_metadata`. They ask "show me the workflow colored by X" → chatbot calls `describe_branch color_by_metadata=X`. No operator needed.

### Audit checklist for every substrate-gap candidate

Before filing, verify each:

1. **Does the proposed primitive name a single curated thing** (compliance_tags, cost_tags, team_tags, ...) that suggests there will be N more curated things in the next year? If yes, it's curated — find the generic primitive underneath.
2. **Could a user, using only chat + the connector, have built the proposed feature themselves with one or two well-shaped generic primitives?** If yes, file those generic primitives, not the feature.
3. **Does the existing brain (memory + wiki) already have related primitives** (`add_state_field`, `project_memory_set`, wiki frontmatter) that demonstrate the pattern at other layers? If yes, the missing primitive is likely the same shape applied to the new layer.
4. **Does the proposed filing pass `feedback_branches_must_be_user_designable`'s test** — could a user have proposed the SAME filing? If no, regenerate.

**Auto-iterate (host directive 2026-05-06 ~02:15Z):**
"for all these tag field things, why dont users have the primatives to make those, i thoght thats what the 6 primatives where for." Codified. Substrate-gap classification now requires the generic-vs-curated subdivision before filing. PR-044 (filed 2026-05-06 ~02:08Z) is the first filing using this subdivision; PR-043 was retracted via brain note. Next rung if recurrence: a `user_authorable_check.py` helper that scans the proposed filing's `expected:` section for hardcoded taxonomies (HIPAA, GDPR, SOX, ...) or hardcoded color schemes — flag any of those as operator-shape and block the filing.

## Host-approval gate — MANDATORY for every patch going forward (2026-05-06)

Per host directive 2026-05-06 ~02:30Z, the auto-approval flow that previously ran on Cowork↔Codex consensus is now gated by host approval. The new flow:

1. Filing → triage by Cowork or Codex.
2. Cowork ↔ Codex consensus on shape (vision-fit + execution).
3. **Host approval gate** (NEW).
4. Auto-merge pipeline.

**No PR merges without host approval going forward.** This applies to every patch — substrate, brain, conventions, gates, all of it. The gate is implementation-side (label, status check, required reviewer, substrate gate event — Codex picks the mechanism).

The reason: as the new framing lands (canonical 6 primitives + 5 MCP handles, see below), the loop must refactor its backlog. Existing PRs may propose additions that violate the locked vocabulary; new PRs must be evaluated against it. Host is the source of truth on which moves count as substrate evolution vs user-authorable convention while the loop learns the framing.

Per `feedback_loop_self_stewardship_target`, this gate trends to zero as the loop reliably passes the user-authorable test on its own. For now mandatory.

**For Cowork operators specifically:** Cowork's checker-key still happens — it's one of the dual keys. Codex's review is the other. The host-approval gate is a THIRD key required at merge time, not at checker-keying time. So: keep checker-keying when you've done the vision-fit + execution review, but understand that no PR merges until host approval lands as the third key. The auto-merge bot must be configured to wait on the host-approval signal (Codex's gate-config change).

## Substrate vocabulary is finite — 6 primitives + 5 MCP handles (2026-05-06)

The substrate-evolution closing directive. See [[pages-concepts-workflow-substrate-canonical-vocabulary-6-primitives-5-mcp-handles]] for the full canonical reference. Quick summary:

- **6 primitives**: Node, Edge, State, Scope, Run, Trigger.
- **5 MCP handles**: `read.graph`, `write.graph`, `run.graph`, `read.page`, `write.page`.
- **No 7th primitive ever**: Identity, Capability, Effect, Permission, anything users invent — becomes a user-authored brain convention ranked by adoption.

**Tool count does not expand without host explicit approval after discussion.** Default answer to "should we add another tool?" is no. If a probe surfaces a use case that the 6+5 cannot cleanly compose, that's a finding to bring to host discussion — NOT a license to file a 6th primitive or a new tool.

**For probe filings:** continue filing freely. Probe filings that propose new primitives, new tools, new shapes ARE the substrate-evolution signal we want — they tell us what users + the loop are reaching for. The gate is at merge, not at filing.

What the framing changes is the CLASSIFICATION on the filing, not whether to file:
- If the failure-shape audit shows the gap fits one of the existing 6 primitives or 5 MCP handles, file with that classification clearly noted; the merge gate is straightforward.
- If the audit shows a genuinely-new-primitive proposal, file it WITH explicit "host-discussion-needed: violates 6+5 vocabulary" classification in the filing body. The host evaluates at the merge-gate step. Most such filings will probably fold into a brain convention or a refinement of the existing primitives once host has reviewed; some might amend the canonical 6+5 (which only happens with explicit host approval after discussion).

The discipline shift is observation → classification → honest filing, NOT observation → self-censorship. Operators self-censoring filings is operator-frame; we want the substrate-evolution signal flowing.

This is the deepest version of the failure-shape audit. Most "substrate-gap" findings should now resolve as either (a) gaps in the surfacing of an existing primitive (a write.graph with a shape we don't expose yet), or (b) user-authorable conventions for the brain. Genuine new-primitive needs are rare and require host-discussion-first.

**Auto-iterate (host directive 2026-05-06 ~02:30Z):**
"the tool count doesnt expand unless i explicitly say so after discusion. also all patches need my approval from now on before being approved as we build the new understanding and teach this to the loop so it can refactor its backlog acordingly." Codified. If recurrence happens (a probe filing slips through proposing a new primitive without host discussion), the next rung is a `vocabulary_audit.py` helper that scans proposed `expected:` sections for keywords like "new primitive" / "new tool" / "new shape" and blocks the filing until host-approval is recorded.

## 3rd-key-pending summary format — bullet list of PR-### + summary + implications (2026-05-06)

Per host directive 2026-05-06 ~04:00Z, the standing deliverable shape when one or more PRs hit dual-key APPROVE consensus and are pending host's 3rd-key:

```
PRs pending your 3rd key:

- **PR-###** (or **#NNN** for actual GitHub PRs) — <one-line summary>
  - **Implications:** <substrate impact / user behavior change / dependencies / risks>

- **PR-###** — <next>
  - **Implications:** <...>
```

Each PR gets one bullet + one indented implications line. Tight. The implications line is load-bearing — host decides from it without reading the PR body.

**Do NOT:**
- Surface partial state (don't ping host with "here's where consensus is" if no PRs have reached 3rd-key-pending).
- Include items that aren't merge candidates (BLOCKED PRs do NOT go in this list — they won't merge).
- Bury the list in prose. The bullet shape IS the contract.
- Pad with reasoning about why the PR is good — summary + implications is enough.

**Operational practice:** track in-flight per-PR consensus state in the brain (notes) without surfacing. The moment a PR reaches dual-key APPROVE, surface the bullet to host in chat. Batch multiple if they land in the same window; surface immediately if only one.

**Auto-iterate (host directive 2026-05-06 ~04:00Z):**
"when ever there is a need for my third key i want a bullet point list of th pr-### and summary and implications" — codified. If a future Cowork session surfaces 3rd-key items in any other format (prose, headers, paragraphs), that's a stop-the-line. Next rung if recurrence: a `third_key_format_check.py` helper that lints the chat-output for the bullet shape before emitting.
