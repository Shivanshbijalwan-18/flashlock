#!/usr/bin/env bash
# ============================================================================
# FlashLock - database/tests/concurrency_sql_reference.sh   (Member 2)
# DB-06-REF: DATABASE-LEVEL reference test. N parallel psql sessions each run
# the "SELECT ... FOR UPDATE, check, decrement, insert order" pattern against
# product 1. This proves the PostgreSQL locking pattern is sound.
# IT IS NOT A SUBSTITUTE for DB-06, which must go through Member 1's real
# POST /api/purchase endpoint (tests/api_concurrency_test.py).
#
# DEMO-ONLY: calls reset_demo.sql (destructive) first.
# Usage: PGHOST=localhost PGPORT=5432 PGUSER=me PGDATABASE=flashlock_db \
#        PGPASSWORD=... ./concurrency_sql_reference.sh [N=20]
# ============================================================================
set -euo pipefail
N="${1:-20}"
DIR="$(cd "$(dirname "$0")/.." && pwd)"
PSQL="psql -X -q -v ON_ERROR_STOP=1"

$PSQL -f "$DIR/reset_demo.sql" >/dev/null
INITIAL=$($PSQL -At -c "SELECT stock FROM products WHERE product_id=1")

ATTEMPT="DO \$\$ DECLARE s int; BEGIN
  SELECT stock INTO s FROM products WHERE product_id = 1 FOR UPDATE;
  IF s >= 1 THEN
    UPDATE products SET stock = stock - 1 WHERE product_id = 1;
    INSERT INTO orders (product_id, user_id, quantity, status) VALUES (1, 100 + (random()*1000)::int, 1, 'SUCCESS');
  END IF; END \$\$;"

for i in $(seq 1 "$N"); do $PSQL -c "$ATTEMPT" & done
wait

echo "method=$N parallel psql sessions, SELECT FOR UPDATE pattern"
echo "initial_stock=$INITIAL"
echo "final_stock=$($PSQL -At -c "SELECT stock FROM products WHERE product_id=1")"
echo "orders_created=$($PSQL -At -c "SELECT count(*) FROM orders WHERE product_id=1")"
echo "units_sold=$($PSQL -At -c "SELECT COALESCE(sum(quantity),0) FROM orders WHERE product_id=1 AND status='SUCCESS'")"
