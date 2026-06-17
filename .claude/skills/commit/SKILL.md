---
name: commit
description: Create a git commit for the current changes WITHOUT any AI co-author attribution (no Co-Authored-By Claude, Copilot, GPT, or any other tool). Use whenever the user asks to commit, stage and commit, or "commit this". Investigates and fixes pre-commit hook failures rather than bypassing them.
---

# commit

Create a clean git commit for the current change set. The single most important rule of this skill:

**NEVER add any `Co-Authored-By:` trailer.** Not Claude, not Copilot, not GPT, not any AI tool, not any other author. Commits authored through this skill are attributed solely to the human committer's configured git identity.

## Procedure

Run these three commands in parallel first to understand the state of the tree:

1. `git status` (do NOT use `-uall` — it scans too much on this repo as it grows)
2. `git diff` (covers staged + unstaged; pipe through `head` if huge)
3. `git log -n 10 --oneline` (so the new message matches existing style)

Then:

1. **Read the diff** and write a short message focused on *why*, not *what*. The diff already shows what changed.
2. **Pick the files to stage by name.** Do NOT use `git add -A` or `git add .` — those can sweep in `.env`, build artifacts, or unrelated work. If many files belong to the change, list them explicitly.
3. **Compose the message** as a single concise subject line, plus an optional body if context warrants it. Project convention (per `docs/DesignDoc.md` §23.3): when a system changes, mention the docs that were updated.
4. **Commit with a HEREDOC** — no `Co-Authored-By` line, no trailing AI signature:

   ```bash
   git commit -m "$(cat <<'EOF'
   <subject line>

   <optional body — focus on why>
   EOF
   )"
   ```

5. **Run `git status` after** to verify the commit landed and the tree is clean.

## Hook failures

If a pre-commit hook fails:

- **Do NOT pass `--no-verify`.** Hooks exist for a reason.
- **Do NOT `--amend`** the previous commit — the failed commit did not happen, so amend would silently rewrite an unrelated earlier commit.
- Read the hook output, fix the underlying issue, re-stage, and create a NEW commit.

## Push

Do NOT push unless the user explicitly says so. Committing is local; pushing is a separate decision.

## What this skill MUST NOT produce

```
Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
Co-Authored-By: GitHub Copilot <...>
🤖 Generated with Claude Code
Signed-off-by: <any AI tool>
```

None of these. The commit message ends at the last line of human-written content.
