-- ============================================================================
-- FlashLock - database/seed.sql              (Member 2 owns this file)
-- Deterministic demo inventory. Safe to re-run: existing product rows are
-- left untouched (ON CONFLICT DO NOTHING). To force stock back to these
-- values use database/reset_demo.sql (destructive, DEMO-ONLY).
--
--   product_id | name                      | price   | initial stock | purpose
--   1          | Flash Sale Headphones     |  1999.00 | 5            | concurrency test (DB-03/06)
--   2          | Smart Watch               |  4999.00 | 10           | quantity > 1 test (DB-07)
--   3          | Limited Edition Speaker   |  2999.00 | 1            | insufficient stock (DB-04)
-- Names/prices/stock are demo data, not project requirements.
-- ============================================================================

INSERT INTO products (product_id, name, price, stock) VALUES
    (1, 'Flash Sale Headphones',    1999.00,  5),
    (2, 'Smart Watch',              4999.00, 10),
    (3, 'Limited Edition Speaker',  2999.00,  1)
ON CONFLICT (product_id) DO NOTHING;
