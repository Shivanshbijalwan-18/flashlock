-- ============================================================================
-- FlashLock - database/test_queries.sql      (Member 2 owns this file)
-- Read-only verification queries. Run after any test:
--   psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/test_queries.sql
-- Queries C, D, E, G, H must return ZERO ROWS. If not -> record FAIL.
-- ============================================================================

\echo '--- A. Inventory (informational) ---'
SELECT product_id, name, price, stock FROM products ORDER BY product_id;

\echo '--- B. Orders, newest first (informational) ---'
SELECT order_id, product_id, user_id, quantity, status, created_at
FROM orders ORDER BY created_at DESC, order_id DESC;

\echo '--- C. Negative stock (MUST be empty) ---'
SELECT product_id, name, stock FROM products WHERE stock < 0;

\echo '--- D. Invalid order quantity (MUST be empty) ---'
SELECT order_id, quantity FROM orders WHERE quantity <= 0;

\echo '--- E. Orders referencing missing products (MUST be empty) ---'
SELECT o.order_id, o.product_id
FROM orders o LEFT JOIN products p ON p.product_id = o.product_id
WHERE p.product_id IS NULL;

\echo '--- F. Successful units per product (informational) ---'
SELECT p.product_id, p.name, p.stock AS current_stock,
       COALESCE(SUM(CASE WHEN o.status = 'SUCCESS' THEN o.quantity ELSE 0 END), 0)
         AS successful_units_ordered
FROM products p LEFT JOIN orders o ON o.product_id = p.product_id
GROUP BY p.product_id, p.name, p.stock ORDER BY p.product_id;

\echo '--- G. Conservation check vs SEED stock (MUST be empty right after a reset+test run) ---'
-- Valid ONLY when the run started from the seed values (reset_demo.sql) and
-- no manual stock edits were made. Rows returned = stock and orders disagree
-- (partial write / overselling / lost update).
WITH seed(product_id, initial_stock) AS (
    VALUES (1, 5), (2, 10), (3, 1)
)
SELECT p.product_id, s.initial_stock, p.stock AS current_stock,
       COALESCE(SUM(o.quantity) FILTER (WHERE o.status = 'SUCCESS'), 0) AS sold
FROM products p
JOIN seed s ON s.product_id = p.product_id
LEFT JOIN orders o ON o.product_id = p.product_id
GROUP BY p.product_id, s.initial_stock, p.stock
HAVING s.initial_stock - p.stock
       <> COALESCE(SUM(o.quantity) FILTER (WHERE o.status = 'SUCCESS'), 0);

\echo '--- H. Overselling check vs SEED stock (MUST be empty) ---'
WITH seed(product_id, initial_stock) AS (VALUES (1, 5), (2, 10), (3, 1))
SELECT o.product_id, s.initial_stock, SUM(o.quantity) AS sold
FROM orders o JOIN seed s ON s.product_id = o.product_id
WHERE o.status = 'SUCCESS'
GROUP BY o.product_id, s.initial_stock
HAVING SUM(o.quantity) > s.initial_stock;
