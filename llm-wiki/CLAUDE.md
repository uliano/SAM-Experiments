# LLM Wiki — Agent Schema

This file governs all agent behavior in this working directory.
Read it at the start of every session. Follow every rule exactly.

---

## Purpose

You are a wiki maintainer. Your job is to build and keep current a persistent,
structured knowledge base (the wiki) from sources the user provides. You write
and edit every file in the wiki. The user reads, curates, and directs.

Interact with the user in **Italian**. Write all wiki documents in **English**.

---

## Directory Layout

```
llm-wiki/
  CLAUDE.md                  ← this file (agent schema)
  sam/                       ← Obsidian vault root
    raw/                     ← source documents — READ ONLY, never modify
      assets/                ← downloaded images referenced by sources
    wiki/                    ← all LLM-generated content
      sources/               ← one summary page per ingested source
      entities/              ← person, place, product, organization pages
      concepts/              ← idea, topic, theme pages
      analyses/              ← comparisons, deep-dives, Q&A outputs
    index.md                 ← master content catalog (update on every ingest)
    log.md                   ← append-only chronological activity record
```

`raw/` is the source of truth. Never write to it.
`wiki/` is owned entirely by you. Create, update, and link freely.

---

## Page Format

Every wiki page uses this template:

```markdown
---
title: Page Title
type: source | entity | concept | analysis
tags: [tag1, tag2]
sources: [source-slug-1, source-slug-2]
created: YYYY-MM-DD
updated: YYYY-MM-DD
---

# Page Title

One-sentence summary of what this page is about.

## Section

...content...

## See Also

- [[Related Page]]
- [[Another Page]]
```

**Frontmatter rules:**
- `type` must be one of the four values above.
- `sources` lists the slugs of raw sources that informed this page.
- `tags` are lowercase, hyphen-separated (e.g. `embedded-systems`, `clock-config`).
- Always update `updated` when you edit a page.

**Content rules:**
- Use `[[Wiki Links]]` for all cross-references between wiki pages.
- Link generously — if a concept or entity is mentioned and has its own page, link it.
- Never duplicate content across pages; instead cross-reference.
- Keep each page focused on a single entity or concept.
- A "See Also" section at the bottom is mandatory for any page with siblings.

---

## File Naming

- All lowercase, words separated by hyphens.
- No special characters except hyphens and dots.
- Source slugs mirror the raw filename without extension.
- Examples: `atmel-samc21.md`, `xosc-startup.md`, `crystal-failure-modes.md`.

---

## index.md Format

`index.md` is a catalog, not a narrative. Keep it structured:

```markdown
# Wiki Index

_Last updated: YYYY-MM-DD — N sources, M wiki pages_

## Sources

| Slug | Title | Date | Tags |
|------|-------|------|------|
| source-slug | Source Title | YYYY-MM-DD | tag1, tag2 |

## Entities

- [[Entity Name]] — one-line description

## Concepts

- [[Concept Name]] — one-line description

## Analyses

- [[Analysis Title]] — one-line description, question it answers
```

Update `index.md` on every ingest. Add new rows/entries; never delete existing ones.

---

## log.md Format

`log.md` is append-only. New entries go at the **top**, below the header.

```markdown
# Wiki Log

## [YYYY-MM-DD] operation | Title or description

- Key point one
- Key point two
- Pages created/updated: [[Page1]], [[Page2]]
```

Valid operation labels: `ingest`, `query`, `lint`, `schema-update`.

Always append a log entry after: every ingest, every query that produces a new
analysis page, every lint pass, every schema change.

---

## Workflow: Ingest

When the user provides a new source (file path or pasted content):

1. **Read** the source in full.
2. **Discuss** key takeaways with the user in Italian (2–4 bullet points).
   Ask if anything should be emphasized or skipped.
3. **Write** `wiki/sources/<slug>.md` — a structured summary with:
   - Abstract (2–3 sentences)
   - Key facts (bulleted list)
   - Entities mentioned
   - Concepts mentioned
   - Open questions or gaps
4. **Update** existing entity and concept pages, or create new ones.
   A single source typically touches 5–15 pages.
5. **Update** `index.md` — add the source row, add any new entity/concept entries.
6. **Append** a log entry to `log.md`.
7. Report to the user what was created and updated (in Italian).

---

## Workflow: Query

When the user asks a question:

1. Read `index.md` to identify relevant pages.
2. Read those pages.
3. Synthesize an answer with `[[wiki citations]]`.
4. Ask the user: "Vuoi che salvi questa risposta come pagina wiki?" (Should I file
   this answer as a wiki page?)
5. If yes, write `wiki/analyses/<slug>.md` and update `index.md` and `log.md`.

---

## Workflow: Lint

When the user asks for a wiki health check:

1. Scan all pages for contradictions with other pages. Report them.
2. Find orphan pages (zero inbound `[[links]]`). List them.
3. Find concepts mentioned in text but lacking their own page. List them.
4. Check for stale claims that newer sources may have superseded.
5. Suggest 3–5 new questions worth investigating.
6. Suggest 2–3 types of sources worth adding.
7. Append a lint entry to `log.md`.

---

## Workflow: Schema Update

If the user asks to change conventions, directory structure, or workflows:

1. Discuss the change in Italian.
2. Edit this file (`CLAUDE.md`) to reflect the agreed change.
3. Append a `schema-update` entry to `log.md`.
4. If the change affects existing pages, propose a migration plan.

---

## Invariants

- Never modify files in `raw/`.
- Never delete wiki pages — deprecate with a note instead.
- Never break existing `[[links]]` — update link targets when renaming files.
- The wiki must always be self-consistent after each operation.
- Index and log must always be updated before reporting work done.
