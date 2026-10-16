# FlashLock — database/ (Member 2)

PostgreSQL data layer + verification scripts. **Additive only**: nothing here modifies `os-engine/`, `backend/`, or `frontend/`.

## Files
| File | Purpose | Destructive? |
|---|---|---|
| `schema.sql` | `products`, `orders`, keys, CHECKs, index | No (`IF NOT EXISTS`) |
| `seed.sql` | 3 deterministic demo products | No (`ON CONFLICT DO NOTHING`) |
| `reset_demo.sql` | Delete all orders, restore seed stock | **YES — demo DB only** (refuses unless DB is `flashlock_db`/`flashlock_test`) |
| `test_queries.sql` | Read-only checks A–H (C, D, E, G, H must return 0 rows) | No |
| `constraint_tests.sql` | DB-08 constraint proofs, always ROLLBACK | No (guarded) |
| `tests/concurrency_sql_reference.sh` | DB-06-REF: DB-level locking pattern, N parallel psql | Calls reset (demo only) |
| `tests/api_concurrency_test.py` | DB-06: N simultaneous requests to the real `POST /api/purchase` | No (reset is manual) |
| `MEMBER2_CONTEXT.md` | Paste-able handoff for another AI / teammate | — |

## Setup (fresh demo DB)
```bash
createdb -h localhost -p 5432 -U <user> flashlock_db      # or create in pgAdmin
psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/schema.sql
psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/seed.sql
psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/test_queries.sql
```
Passwords: use `PGPASSWORD` / `~/.pgpass`, never commit them. Backend should read credentials from env vars.

## Seed data
| product_id | name | price | stock | used for |
|---|---|---|---|---|
| 1 | Flash Sale Headphones | 1999.00 | 5 | DB-03, DB-06 |
| 2 | Smart Watch | 4999.00 | 10 | DB-07 |
| 3 | Limited Edition Speaker | 2999.00 | 1 | DB-04 |

## Database contract — what a successful purchase is
In ONE transaction: product exists AND `quantity > 0` AND `stock >= quantity` → `stock = stock - quantity` **and** one `orders` row (`status='SUCCESS'`) are written together, then commit. Otherwise: no stock change, no order row. Any unexpected exception → rollback. `CHECK (stock >= 0)` is only a last-resort guard.

## ASSUMPTIONS (proposed contract — NOT yet confirmed by Member 1)
1. Only successful orders are stored; rejected attempts are not persisted.
2. Request fields `productId`, `userId`, `quantity`; quantity may be > 1.
3. `user_id` is a plain demo number (no `users` table).
4. Either `SELECT … FOR UPDATE` or a single conditional `UPDATE … WHERE stock >= ?` is acceptable.
5. HTTP codes / JSON envelope undecided — `api_concurrency_test.py` reads a `success` boolean (`--success-field` to change).
If any of these change, update schema/tests together; do not rename columns silently.

## Questions for Member 1 (checklist)
- [ ] Exact column/field names confirmed? Failed attempts stored? Status codes + error JSON?
- [ ] Transaction strategy + rollback behavior (`@Transactional`)?
- [ ] DB name / host / port / user for local runs?
- [ ] Integration method (UNIX sockets vs subprocess) — not a DB concern, but affects metrics labelling.
