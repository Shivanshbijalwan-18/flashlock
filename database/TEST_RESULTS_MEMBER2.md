# FlashLock — Database Test Results (Member 2 detail log)

Kept in `database/` so it does NOT overwrite the team's `docs/test-results.md`. A summary in that file's format is at the bottom, ready to paste into it by hand.

**Rule:** Observed results only. Expected outcomes live in `database/README.md` / the test plan, never in the "Observed" column.

## Environment of the runs recorded below
- Run by: Claude (AI assistant) in a **sandbox**, NOT on the team's machines.
- PostgreSQL 16.15 (Ubuntu), database `flashlock_db`, throwaway local cluster.
- **Team must re-run these on their own PostgreSQL and replace/append evidence before claiming them in the evaluation.**
- Backend (Spring Boot) was not available → every API test is BLOCKED.

| ID | Test | Method | Observed result | Status |
|---|---|---|---|---|
| DB-01 | Schema setup | `psql -f schema.sql`, then `\d orders`; ran file twice | Tables, PK, FK, CHECK, index created; 2nd run only "already exists, skipping" notices, no errors | PASS (sandbox) |
| DB-02 | Seed data | `psql -f seed.sql` twice, `test_queries.sql` A | 1st run `INSERT 0 3`, 2nd `INSERT 0 0`; stock 5 / 10 / 1 for IDs 1 / 2 / 3 | PASS (sandbox) |
| DB-03 | Basic API purchase | needs `POST /api/purchase` | Backend not available | BLOCKED |
| DB-04 | Insufficient stock via API | needs backend | Backend not available | BLOCKED |
| DB-05 | Invalid quantity via API | needs backend | Backend not available | BLOCKED |
| DB-06 | Concurrent API purchases | `tests/api_concurrency_test.py` | Not run against real backend. (Script mechanics only checked against a throwaway mock: 20 requests → 5 success / 15 reject. **Mock result is not evidence about the project.**) | BLOCKED |
| DB-06-REF | DB-level lock pattern, stock 5, 20 parallel psql sessions | `tests/concurrency_sql_reference.sh 20`, 3 repetitions | Each run: final_stock=0, units_sold=5 | PASS (sandbox, DB-level only) |
| DB-07 | Quantity > 1 via API | needs backend | Backend not available | BLOCKED |
| DB-08 | FK / CHECK / NOT NULL | `psql -f constraint_tests.sql` | 08a–08e all printed PASS; transaction rolled back, orders table empty afterwards | PASS (sandbox) |
| DB-09 | Repeatability | reset → run DB-06-REF three times | Same final state every time (stock 0, sold 5) | PASS (sandbox, DB-level only) |
| DB-10 | UI ↔ API ↔ DB end to end | needs frontend + backend | Not available | BLOCKED |
| EXTRA | Guard + detector checks | `reset_demo.sql` on db `postgres`; injected a fake oversell then ran queries G/H | Reset refused with clear error; G and H each returned the injected bad row (sold 7 vs initial 5) | PASS (sandbox) |

## Evidence to attach for DB-06 once backend exists
Command line used, submitted count, observed successes/rejections/errors, `test_queries.sql` output after the run, date/time, backend commit hash.

## Open blockers
1. Member 1's backend purchase endpoint + confirmed contract.
2. Team-machine re-run of DB-01/02/08/09.

## Rows to paste into docs/test-results.md (team format: Date | Component | Test | Command/Method | Result | Notes)
Replace the existing `Database | Schema creation | TBD | NOT RUN` row with these. Re-run on a team machine first, then change the date/notes.

| Date | Component | Test | Command/Method | Result | Notes |
|---|---|---|---|---|---|
| 2026-10-16 | Database | Schema creation (DB-01) | `psql -f database/schema.sql` (run twice) | PASS | AI sandbox, PostgreSQL 16.15; NOT yet run on team machine |
| 2026-10-16 | Database | Seed data (DB-02) | `psql -f database/seed.sql` (run twice) | PASS | Stock 5/10/1 for IDs 1/2/3; sandbox |
| 2026-10-16 | Database | Constraints (DB-08) | `psql -f database/constraint_tests.sql` | PASS | FK, CHECK qty, CHECK stock, NOT NULL; sandbox |
| 2026-10-16 | Database | Lock-pattern reference (DB-06-REF) | `database/tests/concurrency_sql_reference.sh 20` x3 | PASS | DB-level only, not the backend; sandbox |
| TBD | Backend/DB | Purchase transaction (DB-03/04/05/07) | `POST /api/purchase` | BLOCKED | Backend not available |
| TBD | Backend/DB | Concurrent purchase (DB-06) | `database/tests/api_concurrency_test.py` | BLOCKED | Backend not available |
