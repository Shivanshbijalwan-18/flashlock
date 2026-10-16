-- ============================================================================
-- FlashLock - database/constraint_tests.sql   (Member 2 owns this file)
-- TEST DB-08: proves PostgreSQL itself rejects invalid data.
-- Runs inside one transaction that is ROLLED BACK, so no test rows remain.
-- Refuses to run outside the demo database.
--   psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/constraint_tests.sql
-- Read the NOTICE lines: each must say PASS.
-- ============================================================================
DO $$
BEGIN
    IF current_database() NOT IN ('flashlock_db', 'flashlock_test') THEN
        RAISE EXCEPTION 'constraint_tests.sql refused: "%" is not a FlashLock demo database', current_database();
    END IF;
END $$;

BEGIN;

DO $$
BEGIN
    BEGIN
        INSERT INTO orders (product_id, user_id, quantity, status) VALUES (999999, 1, 1, 'SUCCESS');
        RAISE NOTICE 'DB-08a FAIL: order for nonexistent product was ACCEPTED';
    EXCEPTION WHEN foreign_key_violation THEN
        RAISE NOTICE 'DB-08a PASS: foreign key rejected nonexistent product_id';
    END;

    BEGIN
        INSERT INTO orders (product_id, user_id, quantity, status) VALUES (1, 1, 0, 'SUCCESS');
        RAISE NOTICE 'DB-08b FAIL: quantity 0 was ACCEPTED';
    EXCEPTION WHEN check_violation THEN
        RAISE NOTICE 'DB-08b PASS: CHECK rejected quantity 0';
    END;

    BEGIN
        INSERT INTO orders (product_id, user_id, quantity, status) VALUES (1, 1, -1, 'SUCCESS');
        RAISE NOTICE 'DB-08c FAIL: quantity -1 was ACCEPTED';
    EXCEPTION WHEN check_violation THEN
        RAISE NOTICE 'DB-08c PASS: CHECK rejected quantity -1';
    END;

    BEGIN
        UPDATE products SET stock = -1 WHERE product_id = 1;
        RAISE NOTICE 'DB-08d FAIL: negative stock was ACCEPTED';
    EXCEPTION WHEN check_violation THEN
        RAISE NOTICE 'DB-08d PASS: CHECK rejected stock = -1';
    END;

    BEGIN
        INSERT INTO orders (product_id, user_id, quantity, status) VALUES (1, 1, 1, NULL);
        RAISE NOTICE 'DB-08e FAIL: NULL status was ACCEPTED';
    EXCEPTION WHEN not_null_violation THEN
        RAISE NOTICE 'DB-08e PASS: NOT NULL rejected NULL status';
    END;
END $$;

ROLLBACK;  -- nothing from this file is ever kept
