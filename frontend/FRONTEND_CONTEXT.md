# FlashLock — Member 3 (Frontend) Context File
Paste this whole file into any AI assistant to continue the work. Last updated: 2026-10-16.

## 1. Project
FlashLock = flash-sale inventory dashboard. Stack: C11 + POSIX threads (OS simulation engine, Member 1), Java Spring Boot (REST API + real purchase transactions, Member 1), PostgreSQL (Member 2), plain HTML/CSS/JS frontend (Member 3 = me). No React, Node/Express, bundlers or frameworks unless the team approves.
Browser -> Spring Boot over HTTP via fetch(). Browser NEVER touches PostgreSQL and holds no credentials.
Open decision (not mine): C-to-backend link is either UNIX domain sockets or Spring Boot launching the C exe as a subprocess. Ask Member 1; never assume.

## 2. Ownership
- Member 1: os-engine/, backend/ (API, transactions, C integration).
- Member 2: database/ (schema.sql, seed.sql, DB tests, concurrency test plan).
- Member 3 (me): frontend/index.html, style.css, script.js (+ mock.js, docs in frontend/).
- Never edit other members' folders without their agreement. Shared docs/ edits must be coordinated.
Repo layout (recommended, may differ in reality — inspect first): README.md, AGENT_RULES.md, os-engine/, backend/, database/, frontend/, docs/{architecture,api-contract,database-schema,integration-status,test-results}.md

## 3. Universal agent rules (summary of AGENT_RULES.md)
1. Repo is source of truth; read AGENT_RULES.md + docs/api-contract.md + docs/database-schema.md if present. Inspect code and `git status` before editing.
2. Do not invent endpoints, JSON fields, schema, dependencies or architecture. If missing/conflicting: stop, ask the owner, record blocker in docs/integration-status.md.
3. Small focused changes; don't overwrite or delete working/uncommitted work; no force-push; stage only own files (`git add frontend/`).
4. No secrets/passwords/tokens in files. No destructive DB commands.
5. Test honestly: report exact commands and results as PASS / FAIL / BLOCKED / NOT RUN. Never fabricate results or metrics.
6. Simulated OS metrics are not real purchases or DB/PostgreSQL latency. C mutexes don't protect PostgreSQL data; DB transactions do.
7. Frontend rules: check response.ok; handle network/non-2xx/invalid JSON; show success ONLY if backend confirms; refresh stock+orders after every attempt (even rejections); validate quantity as positive integer (backend remains authoritative); button disabling is UX only, not concurrency protection; render with textContent (no raw HTML); mock data must be visibly labelled and never mixed with live data; show "Unavailable/Not run" instead of fake numbers.
8. Handoff report: files changed, deps added, run commands, assumptions, tests actually run, limitations, actions needed from others.

## 4. API contract — PROPOSED ONLY, NOT CONFIRMED
GET /api/products | POST /api/purchase | GET /api/orders | POST /api/simulation/run (optional requestCount) | GET /api/simulation/stats
Purchase request (illustrative): {"productId":1,"userId":101,"quantity":1}
Success (illustrative): {"success":true,"message":"Purchase successful","orderId":501,"remainingStock":4}
Failure (illustrative): {"success":false,"message":"Insufficient stock"}
Unconfirmed: productId vs product_id; array vs {data:[...]} wrapper; field names for name/price/stock/order/timestamps; whether quantity>1 allowed; HTTP codes for rejection; stats path; metric names/units; same-origin vs CORS; whether /api/orders includes failed attempts.
Possible metrics: submitted, processed, simulated success/rejected, max_queue_depth, avg_queue_wait_ms, avg_service_ms.

## 5. What has been built (state of my work)
Files in frontend/ (all self-contained, no external assets/CDNs):
- index.html — header + connection pill, mock banner (hidden), Inventory table (user ID input, Refresh, result message area), Orders table, "OS Simulation Metrics" panel (request count + Run simulation, explanatory note, metric cards).
- style.css — responsive, text+colour distinguished messages, visible focus, tables scroll horizontally on small screens.
- script.js — ALL contract assumptions live in the `CONFIG` object at the top (paths, request field names, response field alias lists, metric labels, wrapper keys, SIM_FLOW "run"|"stats", API_BASE). When the real contract is known, edit only CONFIG. Features: products fetch+render, purchase with validation and pending-lock, orders fetch+render, simulation run+metric cards (unknown metric keys shown raw, missing ones "Unavailable"), loading/empty/error states, 10s timeout, no raw-HTML insertion.
- mock.js — isolated demo mock, loaded only via `?mock=1`; shows a yellow "DEMO MOCK DATA — NOT LIVE DATABASE" banner and "[MOCK]" title; simulation intentionally unavailable in mock mode.
- TEST_CHECKLIST.md — UI-01..UI-10, all currently NOT RUN.
- FRONTEND_CONTEXT.md — this file.

Behaviour notes: a purchase counts as success only if HTTP ok AND body.success === true. HTTP ok with no `success` field shows an "Unconfirmed" warning. Network failure on purchase says result unknown, then refreshes. Product shows "Sold out" when stock is 0 (UX only). userId defaults to 101 in an input (placeholder until real user handling is agreed).
NOT verified: nothing has been run against the real backend or database; the code has not been browser-tested in this session. All tests are NOT RUN.

## 6. How to run
- Mock demo: `cd frontend && python3 -m http.server 5500` then open http://localhost:5500/?mock=1
- Live, separate dev server: open http://localhost:5500/?api=http://localhost:8080 (needs CORS enabled by Member 1; port is an assumption).
- Live, same origin: if Member 1 serves static files from Spring Boot, copy/point to frontend/ files as they decide, and use relative URLs (no ?api).

## 7. Open questions / next steps
Ask Member 1: real endpoints + JSON fields; array vs wrapper; status codes for rejection; how to start backend; who serves the frontend / CORS; simulation endpoint readiness + metric names/units.
Ask Member 2: product IDs, seed stock, order/product field names, order statuses, whether failed attempts are stored, DB reset procedure for the demo.
Then: (1) update CONFIG; (2) run UI-01..UI-10 against the live backend and record results honestly; (3) verify stock in UI matches PostgreSQL; (4) report mismatches to owners; (5) `git add frontend/` only and commit.

## 8. Prompt to give another AI
"You are the frontend engineer (Member 3) on the FlashLock student project. Read FRONTEND_CONTEXT.md fully. Inspect the repo and the existing frontend/ files before editing; work only inside frontend/. Do not invent API paths or fields — update only the CONFIG block in script.js once the real contract is known, and ask me when something is unknown. Keep plain HTML/CSS/JS. Never claim a test passed unless it was run. First tell me: what frontend files exist, what API/seed data actually exist, what is missing, and the safest next step."
