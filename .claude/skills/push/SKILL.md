---
name: push
description: Push the current branch to its remote. Sets upstream on first push, refuses to force-push to protected branches, and runs sanity checks (clean tree, branch isn't main without authorization). Use when the user asks to push, "ship this", or send a branch up.
---

# push

Push the current branch to its tracking remote. Safe-by-default — refuses to force-push to protected branches and won't push if the working tree is dirty unless the user explicitly says so.

## Procedure

1. **Inspect state.** Run these in parallel:
   - `git status --short` — confirm clean tree (or know what's left).
   - `git branch --show-current` — capture the current branch name.
   - `git rev-parse --abbrev-ref --symbolic-full-name @{u} 2>/dev/null` — does this branch already track an upstream? Exit non-zero means no.
   - `git log @{u}.. --oneline 2>/dev/null` (only if upstream exists) — commits about to be pushed.

2. **Pre-flight checks.**

   - If working tree is **dirty** (uncommitted changes), tell the user and pause. Don't push half-state by accident. Offer: commit first via the `commit` skill, stash, or proceed anyway only on explicit confirmation.
   - If the current branch is **`main`** (or `master`) AND there is no explicit user instruction to push main, pause and confirm. Direct pushes to `main` should be deliberate — feature branches + PRs are the default flow for this repo.
   - If there are **zero commits ahead** of upstream, tell the user there's nothing to push and stop.

3. **Push.**

   - **First push (no upstream):**
     ```bash
     git push -u origin <branch-name>
     ```
     This sets the upstream so future `git push` / `git pull` need no args.

   - **Subsequent push (upstream exists):**
     ```bash
     git push
     ```

4. **Report.** Print:
   - The remote URL (`git remote get-url origin`)
   - Branch name and how many commits were pushed
   - A one-line list of the pushed commit subjects

## Force push

Don't ever use `--force` or `--force-with-lease` without an explicit user request. If the user says "force push," prefer `--force-with-lease` over `--force` (it refuses to clobber commits you haven't seen).

NEVER force-push to `main` or `master` without first warning the user about the blast radius — this rewrites history visible to everyone and can erase others' work. Even with permission, suggest a safer path first (revert commit, hotfix branch).

## Skip hooks?

NEVER use `--no-verify` unless the user explicitly asks. Hooks (lint, format, tests) exist for a reason; a failing hook means investigate, not bypass.

## After pushing a feature branch

If the branch was pushed for the first time AND it's not `main`/`master`, suggest the `pr` skill as the next step (opens a pull request via `gh`).
