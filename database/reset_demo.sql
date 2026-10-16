-- ============================================================================
-- FlashLock - database/reset_demo.sql        (Member 2 owns this file)
--
-- !!! DESTRUCTIVE - DEMO DATABASE ONLY !!!
-- Deletes ALL rows from orders and restores products 1-3 to the seed values.
-- Guarded: aborts unless the current database is named 'flashlock_db'
-- (or 'flashlock_test'). It never drops tables.
--
--   psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/reset_demo.sql
-- Do NOT run while another teammate is mid-test on the same database.
-- ============================================================================

DO $$
BEGIN
    IF current_database() NOT IN ('flashlock_db', 'flashlock_test') THEN
        RAISE EXCEPTION 'reset_demo.sql refused: database "%" is not a FlashLock demo database', current_database();
    END IF;
END $$;

BEGIN;
DELETE FROM orders;
INSERT INTO products (product_id, name, price, stock) VALUES
    (1, 'Flash Sale Headphones',    1999.00,  5),
    (2, 'Smart Watch',              4999.00, 10),
    (3, 'Limited Edition Speaker',  2999.00,  1)
ON CONFLICT (product_id) DO UPDATE
    SET name = EXCLUDED.name, price = EXCLUDED.price, stock = EXCLUDED.stock;
-- Restart order ids so every test run starts from 1.
ALTER TABLE orders ALTER COLUMN order_id RESTART WITH 1;
COMMIT;

SELECT product_id, name, stock FROM products ORDER BY product_id;
