---
name: commit
description: Create a git commit for the current changes using Conventional Commits format WITHOUT any AI co-author attribution (no Co-Authored-By Claude, Copilot, GPT, or any other tool). Use whenever the user asks to commit, stage and commit, or "commit this". Investigates and fixes pre-commit hook failures rather than bypassing them.
---

# commit

Create a clean git commit for the current change set. Two non-negotiable rules:

1. **NEVER add any `Co-Authored-By:` trailer.** Not Claude, not Copilot, not GPT, not any AI tool. Commits are attributed solely to the human committer's configured git identity.
2. **Subject line MUST follow [Conventional Commits](https://www.conventionalcommits.org/).** The project's git log is a contract; downstream tools (changelog generators, release automation, `git log --grep`) depend on the format.

## Conventional Commits format

```
<type>(<optional scope>): <imperative-mood subject, ≤ 72 chars>

<optional body — wrap at ~72 chars, explain WHY not WHAT>

<optional footers — e.g. BREAKING CHANGE: <description>, Refs: #123>
```

**Allowed types** (pick the most specific):

| Type       | When |
|------------|------|
| `feat`     | A new user-facing feature or capability |
| `fix`      | A bug fix |
| `perf`     | A change whose primary purpose is performance |
| `refactor` | Code change that neither fixes a bug nor adds a feature |
| `docs`     | Documentation only (README, design docs, comments at scale) |
| `style`    | Formatting, whitespace, missing semicolons — no logic change |
| `test`     | Adding or fixing tests only |
| `build`    | CMake, dependencies, compiler flags, packaging |
| `ci`       | CI/CD configuration only |
| `chore`    | Routine maintenance with no production code impact (deps bumps, repo housekeeping) |
| `revert`   | Reverts a previous commit |

**Scope** (optional, parens after the type): the module or area touched. For Starfall, prefer engine module names: `feat(runtime):`, `feat(scripting):`, `build(cmake):`, `docs(design):`, `chore(openspec):`, etc. Omit the scope only when the change spans more areas than the subject can name.

**Subject rules:**

- Imperative mood: `add foo`, not `added foo` or `adds foo`.
- No trailing period.
- Lowercase unless naming a proper noun.
- Aim for ≤ 50 chars; hard ceiling 72.

**Body rules** (optional, blank line after subject):

- Explain *why* the change exists. The diff already shows *what*.
- Wrap at ~72 chars (matters for `git log` readability).
- Reference issues/PRs/specs in footers, not the subject.

**Breaking changes:** put `BREAKING CHANGE: <explanation>` in a footer, OR append `!` after the type/scope (e.g. `feat(runtime)!: rename Application::Run`). Either signal is sufficient; both is fine.

## Procedure

Run these three commands in parallel first to understand the state of the tree:

1. `git status` (do NOT use `-uall` — too aggressive on large trees)
2. `git diff` (covers staged + unstaged; pipe through `head` if huge)
3. `git log -n 10 --oneline` (so the new message matches the project's existing style)

Then:

1. **Read the diff** and pick the conventional type that most accurately describes the *primary* intent. If a single commit truly spans two types (e.g. `feat` + `docs`), pick the dominant one and let the body mention the secondary; if the split is genuinely 50/50, propose splitting the commit.
2. **Pick the files to stage by name.** Do NOT use `git add -A` or `git add .` — those can sweep in `.env`, build artifacts, or unrelated work. List files explicitly.
3. **Compose the message.** Subject in Conventional Commits format, optional body focused on why. Per `docs/DesignDoc.md` §23.3: when a system changes, mention the docs that were updated.
4. **Commit with a HEREDOC** — no `Co-Authored-By`, no trailing AI signature:

   ```bash
   git commit -m "$(cat <<'EOF'
   feat(runtime): open SDL3 window from Application

   <optional body explaining why>
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

Do NOT push from this skill. Pushing is a separate decision handled by the `push` skill.

## What this skill MUST NOT produce

```
Co-Authored-By: Claude <noreply@anthropic.com>
Co-Authored-By: Claude Opus 4.7 <noreply@anthropic.com>
Co-Authored-By: GitHub Copilot <...>
🤖 Generated with Claude Code
Signed-off-by: <any AI tool>
```

None of these. The commit message ends at the last line of human-written content.
