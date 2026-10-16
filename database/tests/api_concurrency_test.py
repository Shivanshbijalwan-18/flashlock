#!/usr/bin/env python3
"""
FlashLock - database/tests/api_concurrency_test.py   (Member 2)
TEST DB-06: fires many simultaneous purchase requests at ONE product through
the REAL backend API and reports what was OBSERVED. Python 3 stdlib only.

Prerequisites: backend running; product reset to a known stock
(psql ... -f database/reset_demo.sql); nobody else buying that product.

Example:
  python3 database/tests/api_concurrency_test.py \
      --base-url http://localhost:8080 --product-id 1 --requests 20 --initial-stock 5

Request/response field names follow the PROPOSED contract
(productId, userId, quantity -> {"success": true|false}). If Member 1's real
contract differs, override with --success-field / edit PAYLOAD below; do not
guess. Afterwards run database/test_queries.sql and record the DB state.
"""
import argparse, json, threading, time, urllib.request, urllib.error
from concurrent.futures import ThreadPoolExecutor

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base-url", default="http://localhost:8080")
    ap.add_argument("--path", default="/api/purchase")
    ap.add_argument("--product-id", type=int, default=1)
    ap.add_argument("--quantity", type=int, default=1)
    ap.add_argument("--requests", type=int, default=20)
    ap.add_argument("--initial-stock", type=int, default=5,
                    help="stock you reset the product to (used only for the verdict)")
    ap.add_argument("--success-field", default="success")
    ap.add_argument("--timeout", type=float, default=30)
    a = ap.parse_args()

    url = a.base_url.rstrip("/") + a.path
    barrier = threading.Barrier(a.requests)
    results = [None] * a.requests

    def fire(i):
        payload = json.dumps({"productId": a.product_id, "userId": 1000 + i,
                              "quantity": a.quantity}).encode()
        req = urllib.request.Request(url, data=payload, method="POST",
                                     headers={"Content-Type": "application/json"})
        barrier.wait()                       # release all threads together
        t0 = time.perf_counter()
        try:
            with urllib.request.urlopen(req, timeout=a.timeout) as r:
                status, body = r.status, r.read().decode()
        except urllib.error.HTTPError as e:  # 4xx/5xx still carry a body
            status, body = e.code, e.read().decode()
        except Exception as e:
            status, body = None, f"CLIENT_ERROR: {e}"
        results[i] = (status, body, time.perf_counter() - t0)

    with ThreadPoolExecutor(max_workers=a.requests) as ex:
        list(ex.map(fire, range(a.requests)))

    ok = rejected = errors = 0
    for status, body, _ in results:
        try:
            flag = json.loads(body).get(a.success_field)
        except Exception:
            flag = None
        if status is None or flag is None:
            errors += 1                      # unparseable / network / 5xx w/o JSON
        elif flag is True:
            ok += 1
        else:
            rejected += 1

    lat = sorted(r[2] for r in results)
    print(f"method          : {a.requests} simultaneous threads (barrier-released) -> POST {url}")
    print(f"product/qty     : productId={a.product_id}, quantity={a.quantity}")
    print(f"initial stock   : {a.initial_stock} (as declared by tester)")
    print(f"submitted       : {a.requests}")
    print(f"OBSERVED success: {ok}")
    print(f"OBSERVED reject : {rejected}")
    print(f"OBSERVED errors : {errors}  (anything not a clean success/rejection)")
    print(f"client latency  : min {lat[0]*1000:.0f} ms, max {lat[-1]*1000:.0f} ms (client-side, includes HTTP)")
    limit = a.initial_stock // a.quantity
    if ok * a.quantity > a.initial_stock:
        print(f"API-LEVEL VERDICT: FAIL - {ok*a.quantity} units reported sold > initial stock {a.initial_stock}")
    elif errors:
        print("API-LEVEL VERDICT: INCONCLUSIVE - some responses were errors; inspect them")
    else:
        print(f"API-LEVEL VERDICT: consistent so far (successes <= {limit}). "
              "NOT final: now run database/test_queries.sql and confirm final stock/orders.")
    print("sample responses:", [(r[0], r[1][:80]) for r in results[:3]])

if __name__ == "__main__":
    main()
