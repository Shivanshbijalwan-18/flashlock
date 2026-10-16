# FlashLock — MEMBER 2 CONTEXT HANDOFF (paste this whole file into another AI)

Last updated: 2026-10-16. Written so a fresh AI can continue my work with zero other context.

## 0. Your role
You are the **database + testing engineer (Member 2)** of a 3-person student project called FlashLock. You own PostgreSQL schema, seed data, SQL verification, and reproducible test scripts. You do NOT own or edit: `os-engine/`, `backend/` (Member 1), `frontend/` (Member 3). Work only inside `database/` only (the repo's existing `docs/*` files belong to the team: don't overwrite them). Never overwrite existing files — inspect the repo first.

## 1. Project in brief
Flash-sale system that must never oversell. Stack: C11 + POSIX threads (OS simulator), Java Spring Boot (REST API), PostgreSQL, plain HTML/CSS/JS.
Two SEPARATE mechanisms:
- A. C simulator: producers, bounded queue, worker threads, mutex/semaphores, metrics. Simulated numbers are NOT real DB outcomes.
- B. Real purchase: browser → `POST /api/purchase` → Spring Boot → PostgreSQL transaction/row lock → stock + order.
C mutexes do NOT protect PostgreSQL rows; DB transactions do. C-to-backend integration (UNIX sockets vs subprocess) is unconfirmed — don't decide it.

## 2. Team rules (AGENT_RULES.md summary)
Repo is source of truth; read docs before editing; never invent APIs/columns/endpoints; don't rename schema silently; no secrets in git (env vars, `.env` ignored); small focused changes; no destructive DB commands on shared data without approval; never claim a test passed unless run and observed; label PASS/FAIL/BLOCKED/NOT RUN; keep simulated metrics separate from real ones; report mismatches instead of guessing; handoff report = files changed, commands, actual results, assumptions, blockers.

## 3. Schema (proposed minimum contract, UNCONFIRMED by Member 1)
```
products(product_id BIGINT PK, name VARCHAR(150) NOT NULL, price NUMERIC(10,2) NOT NULL, stock INTEGER NOT NULL CHECK(stock>=0))
orders(order_id BIGINT GENERATED ALWAYS AS IDENTITY PK, product_id BIGINT NOT NULL FK→products, user_id BIGINT NOT NULL,
       quantity INTEGER NOT NULL CHECK(quantity>0), status VARCHAR(30) NOT NULL, created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)
```
Success status = `'SUCCESS'`. Extra: `ck_products_price_nonneg`, index `idx_orders_product_id`. No CHECK on status (failed-attempt storage undecided).
Seed: (1 "Flash Sale Headphones" 1999.00 stock 5), (2 "Smart Watch" 4999.00 stock 10), (3 "Limited Edition Speaker" 2999.00 stock 1).
Purchase contract: one transaction — validate quantity>0, product exists, stock>=quantity → decrement stock AND insert SUCCESS order atomically → commit; else no change, no order; exception → rollback. Either `SELECT…FOR UPDATE` or conditional `UPDATE…WHERE stock>=?` is fine.

## 4. Proposed API (UNCONFIRMED — don't hardcode as fact)
`GET /api/products`, `POST /api/purchase`, `GET /api/orders`, `POST /api/simulation/run`, `GET /api/simulation/stats`.
Request `{"productId":1,"userId":101,"quantity":1}`; success `{"success":true,"message":"Purchase successful","orderId":501,"remainingStock":4}`; failure `{"success":false,"message":"Insufficient stock"}`. Status codes / error envelope undecided.

## 5. What is DONE (files in `flashlock/`)
- `database/schema.sql` — non-destructive, `IF NOT EXISTS`, named constraints.
- `database/seed.sql` — idempotent (`ON CONFLICT DO NOTHING`).
- `database/reset_demo.sql` — DESTRUCTIVE, guarded: aborts unless DB is `flashlock_db` or `flashlock_test`; deletes orders, restores seed stock, restarts order_id.
- `database/test_queries.sql` — queries A–H; C, D, E, G, H must return 0 rows (G/H compare to seed stock 5/10/1, valid only after a reset run).
- `database/constraint_tests.sql` — DB-08, five PASS/FAIL notices, always ROLLBACK.
- `database/tests/concurrency_sql_reference.sh [N]` — DB-06-REF (DB-level only, N parallel psql with FOR UPDATE). Uses PG* env vars.
- `database/tests/api_concurrency_test.py` — DB-06 against real API (stdlib only; flags `--base-url --path --product-id --quantity --requests --initial-stock --success-field`). Barrier-released threads. Prints observed counts only.
- `database/README.md`, `database/TEST_RESULTS_MEMBER2.md` (detail log + rows to paste into the team's docs/test-results.md), this file.

## 6. Test status (see database/TEST_RESULTS_MEMBER2.md for detail)
Executed in an AI SANDBOX (PostgreSQL 16.15), not on team machines — team must re-run for their own evidence:
- PASS: DB-01 schema (+idempotent re-run), DB-02 seed, DB-08 constraints, DB-06-REF (3 runs: stock 5 → final 0, sold 5), DB-09 repeatability (DB-level), guard/detector sanity checks.
- BLOCKED (no backend available): DB-03, 04, 05, 06 (API), 07, 10.
The API script was only dry-run against a throwaway mock; that is not project evidence.

## 7. Commands
```
psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/schema.sql
psql ... -f database/seed.sql
psql ... -f database/test_queries.sql
psql ... -f database/constraint_tests.sql
psql ... -f database/reset_demo.sql        # DEMO ONLY
PGHOST=localhost PGPORT=5432 PGUSER=<u> PGDATABASE=flashlock_db PGPASSWORD=<p> database/tests/concurrency_sql_reference.sh 20
python3 database/tests/api_concurrency_test.py --base-url http://localhost:8080 --product-id 1 --requests 20 --initial-stock 5
```
Manual API test order once backend exists: reset → GET /api/products → DB-03 → DB-04 (product 3, qty 2 / stock 0) → DB-05 (qty 0, -1) → DB-07 (product 2, qty 3 then qty 8) → DB-06 → run `test_queries.sql` after EACH and record actual output.

## 8. Remaining work / next steps
1. Inspect real repo; compare `backend/` entities/DTOs/controllers and `docs/api-contract.md`, `docs/database-schema.md` against §3–4; report mismatches, don't rename silently.
2. Get Member 1's answers: names, failed attempts stored?, status codes/envelope, transaction strategy, DB connection details.
3. Run DB-03…DB-07, DB-06 (API), DB-10 when backend is ready; update `database/TEST_RESULTS_MEMBER2.md` and add rows to the team's existing `docs/test-results.md` (edit, never replace) with ONLY observed output.
4. Re-run DB-01/02/08/09 on the team's own PostgreSQL.
5. Send Member 3 the agreed contract + seed IDs; send Member 1 schema + commands.
6. Commit only: `git add database/` (edit docs/test-results.md and docs/integration-status.md by hand, small diffs). Suggested messages: "Add PostgreSQL schema and constraints", "Add FlashLock seed data", "Add database verification queries", "Document purchase concurrency test plan", "Record database test results".

## 9. Hard rules for you
Don't edit Member 1/3 files; don't implement backend transaction code unless asked (write a test instead); PostgreSQL syntax only; no passwords in files; no invented results; separate expected vs observed; state BLOCKED with the missing dependency; never claim "race-condition free" from a single purchase; never call C-simulator numbers real DB metrics.

## 10. FIRST ACTION when you take over
Inspect the repository and report: (a) what database/backend files exist, (b) what schema/API details are agreed, (c) what is missing, (d) the safest next milestone. Don't regenerate files that already exist.

## 11. Real repo state (checked 2026-10-16: github.com/Shivanshbijalwan-18/flashlock, branch main, 7 commits)
Existing: `README.md` (2 lines), `AGENT_RULES.md`, `docs/{api-contract,architecture,database-schema,integration-status,test-results}.md`. NO `database/`, `backend/`, `frontend/`, `os-engine/` yet.
- `docs/database-schema.md` lists the same candidate schema as §3 (PROPOSED, needs Member 1+2 confirmation) → my SQL matches it; no mismatch found.
- `docs/api-contract.md`: all endpoints PENDING CONFIRMATION → don't treat §4 as agreed.
- `docs/test-results.md` is a team template (columns: Date | Component | Test | Command/Method | Result | Notes). Edit rows by hand; never overwrite.
- `docs/integration-status.md`: "Create database schema | Member 2 | NOT STARTED" → update to IN PROGRESS/COMPLETE only after the SQL is run on a team machine.
- Only path collision with my package was docs/test-results.md → resolved by moving my log to database/TEST_RESULTS_MEMBER2.md.
