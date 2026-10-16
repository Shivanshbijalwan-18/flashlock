-- ============================================================================
-- FlashLock - database/schema.sql            (Member 2 owns this file)
-- PostgreSQL schema: products + orders (proposed minimum contract).
--
-- SAFE / NON-DESTRUCTIVE: uses CREATE TABLE IF NOT EXISTS. It never drops
-- anything. If a table already exists with a different shape it is NOT
-- altered - compare against docs/database-schema.md and talk to Member 2.
--
-- Run on a dedicated demo database (e.g. flashlock_db):
--   psql -h localhost -p 5432 -U <user> -d flashlock_db -f database/schema.sql
-- ============================================================================

CREATE TABLE IF NOT EXISTS products (
    product_id BIGINT        NOT NULL,
    name       VARCHAR(150)  NOT NULL,
    price      NUMERIC(10,2) NOT NULL,
    stock      INTEGER       NOT NULL,
    CONSTRAINT pk_products        PRIMARY KEY (product_id),
    -- Final defensive guard only. The backend transaction must still be
    -- correct; a CHECK alone is not a concurrency strategy.
    CONSTRAINT ck_products_stock_nonneg CHECK (stock >= 0),
    CONSTRAINT ck_products_price_nonneg CHECK (price >= 0)
);

CREATE TABLE IF NOT EXISTS orders (
    order_id   BIGINT      GENERATED ALWAYS AS IDENTITY,
    product_id BIGINT      NOT NULL,
    user_id    BIGINT      NOT NULL,
    quantity   INTEGER     NOT NULL,
    status     VARCHAR(30) NOT NULL,           -- 'SUCCESS' for completed purchases
    created_at TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT pk_orders         PRIMARY KEY (order_id),
    CONSTRAINT fk_orders_product FOREIGN KEY (product_id) REFERENCES products (product_id),
    CONSTRAINT ck_orders_quantity_positive CHECK (quantity > 0)
);

-- Speeds up per-product aggregation used by the verification queries.
CREATE INDEX IF NOT EXISTS idx_orders_product_id ON orders (product_id);

-- NOTE: status is intentionally NOT restricted by a CHECK. Whether failed
-- attempts are stored is still an open decision with Member 1 (see README).
