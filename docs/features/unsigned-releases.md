# Feature: Unsigned CI releases

## State

- **Request (user):** "Change of plans: We are going to go without code signing for now." Full spec in [unsigned-releases.request.md](unsigned-releases.request.md).
- **Slug:** `unsigned-releases`
- **Starting point:** PR ginstrom/felix-cat#16 (merged as 807ccc0) already did most of Phase 1–2: committed Office headers, WTL from NuGet, Boost via b2, build.yml, installer in CI, release.yml with SignPath steps, root `CODE_SIGNING_POLICY.md` linked from README. This feature is the delta.
- **Path:** Medium. No architecture. Design (gap analysis against the spec), plan, implement, verify.
- **Phase:** 6. Report (PR open)
- **Next action:** user reviews and merges PR ginstrom/felix-cat#46, then the hand-offs in the PR description (tag v3.0.1, publish, winget, screenshot, Office test). Optional: re-run the clean-VM test on the CI installer once the user signs in to FelixTest.
- **Design:** `docs/designs/unsigned-releases.md` (gap analysis; open questions in §8.1). Orchestrator settled Q3 (`actions/attest@v4`, upstream-recommended; attest-build-provenance v4 wraps it) and Q4 (English install guide now, Japanese as follow-up); design approved 2026-11-29.
- **Branch:** `ci/unsigned-releases` in worktree `.claude/worktrees/unsigned-releases`, base 807ccc0 (origin/main).
- **Status thread:** none; the `recursive_slack` tools aren't available in this session. This State section is the status.
- **Environment:** no Python, so `runlog` is unavailable; gates record commands and CI run URLs instead of run IDs. Clean-install VM "FelixTest" (VirtualBox, snapshot "Clean Windows 11"). Host hypervisor is off.
- **Open questions:** none.
- **Tiers:** designer opus; planner sonnet; engineers 01 sonnet, 02 opus, 03 sonnet, 04 sonnet, 05 sonnet; reviewers match. Escalations: none.
- **Plan:** `docs/implementation-plans/unsigned-releases/index.md`, log `log.md` there. Scintilla issue: ginstrom/felix-cat#44.
- **Verification gates:** steps 01–05 reviews clean (see log.md); overall review (opus) clean at ffd48a9; Build CI success at ffd48a9 (run 46160127626), artifact hash and notes checked; follow-ups at 553c307 are docs only.
- **PR:** https://github.com/ginstrom/felix-cat/pull/46

## Decision log

| Decision | By | Alternatives | Reason |
|---|---|---|---|
| Treat this as a delta on PR #16's work, not a rebuild | orchestrator | redo phases 1–2 | They're merged and CI passes |
| Skip architecture | orchestrator | architect first | No new components; CI/docs changes |
| Q1: keep Boost via b2 (pinned SHA-256, cached), not vcpkg | user | vcpkg manifest mode | Already passing; matches versioned lib names and local setup |
| Q2: keep checked-in SciLexer.dll; file a GitHub issue for migrating to Scintilla 5 and building it from source | user | build 2.01 from source; migrate now | Migration is its own change |
| Q3: `actions/attest@v4` | orchestrator | `actions/attest-build-provenance@v4` | Upstream recommendation; the latter wraps it |
| Q4: English install guide now; Japanese as follow-up | orchestrator | Japanese now; none | Labels need checking on Japanese Windows |
| Q5: winget ID `Ginstrom.Felix` | user | `RyanGinstrom.Felix` | Matches GitHub owner |
| Design approved as written | user (2026-11-29) | changes | — |
