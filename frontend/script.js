/* FlashLock frontend — plain JS. Talks to Spring Boot over HTTP only.
   !! Every API path / field name below is a PROPOSAL from the team guide, NOT a confirmed contract.
   !! When Member 1 / Member 2 confirm the real contract, edit ONLY the CONFIG block. */
(function () {
  "use strict";
  var q = new URLSearchParams(location.search);

  var CONFIG = {
    API_BASE: (q.get("api") || "").replace(/\/$/, ""),   // "" = same origin. Use ?api=http://localhost:8080 for a separate dev server (needs CORS).
    MOCK: q.get("mock") === "1",
    TIMEOUT_MS: 10000,
    SIM_MAX: 10000,
    PATHS: { products: "/api/products", purchase: "/api/purchase", orders: "/api/orders", simRun: "/api/simulation/run", simStats: "/api/simulation/stats" },
    SIM_FLOW: "run",                                      // "run" = POST run returns metrics; "stats" = POST run, then GET stats
    WRAPPER_KEYS: ["data", "items", "content", "results"],// used only if a list comes wrapped in an object
    REQUEST: { productId: "productId", userId: "userId", quantity: "quantity", simCount: "requestCount" },
    FIELDS: {                                             // first alias found wins
      product: { id: ["productId", "product_id", "id"], name: ["name", "productName", "product_name"], price: ["price", "unitPrice"], stock: ["stock", "availableStock", "quantity", "available_stock"] },
      order: { id: ["orderId", "order_id", "id"], productId: ["productId", "product_id"], productName: ["productName", "product_name"], userId: ["userId", "user_id"], qty: ["quantity", "qty"], status: ["status"], time: ["createdAt", "created_at", "timestamp", "orderTime"] },
      result: { message: ["message", "error"], orderId: ["orderId", "order_id"], remaining: ["remainingStock", "remaining_stock"] }
    },
    METRICS: [                                            // label + unit for known simulator metrics; unknown ones are shown raw
      { keys: ["submitted"], label: "Submitted" },
      { keys: ["processed"], label: "Processed" },
      { keys: ["simulated_success", "simulatedSuccess", "success", "succeeded"], label: "Simulated success" },
      { keys: ["simulated_rejected", "simulatedRejected", "rejected"], label: "Simulated rejected" },
      { keys: ["max_queue_depth", "maxQueueDepth"], label: "Max queue depth" },
      { keys: ["avg_queue_wait_ms", "avgQueueWaitMs"], label: "Avg queue wait", unit: " ms" },
      { keys: ["avg_service_ms", "avgServiceMs"], label: "Avg simulated service time", unit: " ms" }
    ]
  };

  var $ = function (id) { return document.getElementById(id); };
  var state = { products: [], pending: false };

  function pick(o, keys) { if (!o) return undefined; for (var i = 0; i < keys.length; i++) if (o[keys[i]] !== undefined && o[keys[i]] !== null) return o[keys[i]]; return undefined; }
  function toList(d) {
    if (Array.isArray(d)) return d;
    var k = d && CONFIG.WRAPPER_KEYS.filter(function (w) { return Array.isArray(d[w]); })[0];
    return k ? d[k] : null;
  }
  function node(tag, text, cls) { var n = document.createElement(tag); if (text !== undefined) n.textContent = text; if (cls) n.className = cls; return n; }
  function show(id, text, cls) { var n = $(id); n.textContent = text; n.className = "state" + (cls ? " " + cls : ""); n.hidden = false; }
  function errText(res) { var m = res && res.data && pick(res.data, CONFIG.FIELDS.result.message); return m ? String(m) : "HTTP " + (res ? res.status : "?"); }
  function setConn(kind, text) { var c = $("conn"); c.className = "pill pill-" + kind; c.textContent = text; }
  function say(kind, text) { var r = $("result"); r.className = "msg " + kind; r.textContent = text; r.hidden = false; }

  function api(method, path, body) {
    if (CONFIG.MOCK) {
      return window.FLASHLOCK_MOCK ? window.FLASHLOCK_MOCK(method, path, body) : Promise.reject(new Error("Mock data failed to load"));
    }
    var ctl = new AbortController(), t = setTimeout(function () { ctl.abort(); }, CONFIG.TIMEOUT_MS);
    var opt = { method: method, signal: ctl.signal };
    if (body) { opt.headers = { "Content-Type": "application/json" }; opt.body = JSON.stringify(body); }
    return fetch(CONFIG.API_BASE + path, opt).then(function (res) {
      return res.text().then(function (txt) {
        var data = null;
        if (txt) { try { data = JSON.parse(txt); } catch (e) { if (res.ok) throw new Error("Server returned invalid JSON"); } }
        return { ok: res.ok, status: res.status, data: data };
      });
    }).catch(function (e) {
      throw e.name === "AbortError" ? new Error("Request timed out") : e;
    }).finally(function () { clearTimeout(t); });
  }

  /* ---------- Inventory ---------- */
  function loadProducts() {
    show("inv-state", "Loading products…");
    return api("GET", CONFIG.PATHS.products).then(function (res) {
      if (!res.ok) throw new Error("Could not load products: " + errText(res));
      var list = toList(res.data);
      if (!list) throw new Error("Unexpected products response shape (expected a list). Check the API contract.");
      state.products = list; setConn(CONFIG.MOCK ? "wait" : "ok", CONFIG.MOCK ? "Mock mode" : "Backend connected");
      renderProducts();
    }).catch(function (e) {
      state.products = []; $("inv-table").hidden = true;
      setConn("bad", "Backend unavailable"); show("inv-state", e.message + ". No inventory is shown.", "err");
    });
  }
  function renderProducts() {
    var F = CONFIG.FIELDS.product, body = $("inv-body"); body.replaceChildren();
    $("inv-table").hidden = !state.products.length;
    if (!state.products.length) { show("inv-state", "No products yet. Ask Member 2 whether seed data has been loaded."); return; }
    $("inv-state").hidden = true;
    state.products.forEach(function (p) {
      var id = pick(p, F.id), stock = Number(pick(p, F.stock)), price = Number(pick(p, F.price));
      var tr = node("tr"), qty = node("input"), btn = node("button", "Purchase");
      tr.appendChild(node("td", String(pick(p, F.name) !== undefined ? pick(p, F.name) : "(unnamed)")));
      tr.appendChild(node("td", isNaN(price) ? "—" : price.toFixed(2), "num"));
      tr.appendChild(node("td", isNaN(stock) ? "—" : String(stock), "num" + (stock === 0 ? " soldout" : "")));
      qty.type = "number"; qty.min = "1"; qty.step = "1"; qty.value = "1"; qty.setAttribute("aria-label", "Quantity");
      var tdQ = node("td"); tdQ.appendChild(qty); tr.appendChild(tdQ);
      var tdB = node("td"); btn.type = "button"; btn.className = "buy";
      if (stock === 0) { btn.disabled = true; btn.textContent = "Sold out"; qty.disabled = true; }   // UX only; backend enforces stock
      btn.addEventListener("click", function () { purchase(id, qty.value, pick(p, F.name)); });
      tdB.appendChild(btn); tr.appendChild(tdB); body.appendChild(tr);
    });
    syncButtons();
  }
  function syncButtons() { document.querySelectorAll("button.buy").forEach(function (b) { if (b.textContent === "Purchase") b.disabled = state.pending; }); }

  /* ---------- Purchase ---------- */
  function purchase(productId, rawQty, name) {
    if (state.pending) return;
    var uid = $("user-id").value.trim();
    if (!/^[1-9]\d*$/.test(String(rawQty).trim())) return say("bad", "Rejected: quantity must be a whole number of 1 or more.");
    if (!/^[1-9]\d*$/.test(uid)) return say("bad", "Rejected: user ID must be a positive whole number.");
    if (productId === undefined) return say("bad", "Rejected: this product has no ID in the API response. Check the API contract.");
    var R = CONFIG.REQUEST, body = {};
    body[R.productId] = productId; body[R.userId] = Number(uid); body[R.quantity] = Number(rawQty);
    state.pending = true; syncButtons(); say("warn", "Sending purchase request…");
    api("POST", CONFIG.PATHS.purchase, body).then(function (res) {
      var d = res.data || {}, f = CONFIG.FIELDS.result, msg = pick(d, f.message), oid = pick(d, f.orderId), rem = pick(d, f.remaining);
      if (res.ok && d.success === true) {
        say("ok", "Success: " + (msg || "Purchase confirmed") + (oid !== undefined ? " (order " + oid + ")" : "") + (rem !== undefined ? ", remaining stock " + rem : "") + ".");
      } else if (res.ok && d.success === undefined) {
        say("warn", "Unconfirmed: the server replied OK but did not say the purchase succeeded, so it is not counted as one. Check the API contract.");
      } else {
        say("bad", "Rejected: " + (msg || errText(res)) + (name ? " (" + name + ")" : "") + ".");
      }
    }).catch(function (e) {
      say("bad", "Failed: " + e.message + ". The purchase result is unknown; check Orders.");
    }).finally(function () {
      state.pending = false; syncButtons(); loadProducts(); loadOrders();   // stock may have changed either way
    });
  }

  /* ---------- Orders ---------- */
  function loadOrders() {
    var F = CONFIG.FIELDS.order, body = $("ord-body");
    return api("GET", CONFIG.PATHS.orders).then(function (res) {
      if (!res.ok) throw new Error("Could not load orders: " + errText(res));
      var list = toList(res.data);
      if (!list) throw new Error("Unexpected orders response shape (expected a list)");
      body.replaceChildren(); $("ord-table").hidden = !list.length;
      if (!list.length) return show("ord-state", "No orders yet.");
      $("ord-state").hidden = true;
      list.forEach(function (o) {
        var pid = pick(o, F.productId), pname = pick(o, F.productName);
        if (pname === undefined && pid !== undefined) {
          var m = state.products.filter(function (p) { return pick(p, CONFIG.FIELDS.product.id) === pid; })[0];
          pname = m ? pick(m, CONFIG.FIELDS.product.name) : "Product " + pid;
        }
        var t = pick(o, F.time), d = t ? new Date(t) : null, tr = node("tr");
        [pick(o, F.id), pname, pick(o, F.userId), pick(o, F.qty), pick(o, F.status), t === undefined ? undefined : (isNaN(d) ? t : d.toLocaleString())]
          .forEach(function (v, i) { tr.appendChild(node("td", v === undefined ? "—" : String(v), i === 3 ? "num" : "")); });
        body.appendChild(tr);
      });
    }).catch(function (e) { body.replaceChildren(); $("ord-table").hidden = true; show("ord-state", e.message, "err"); });
  }

  /* ---------- OS simulation ---------- */
  function runSim() {
    var n = $("sim-count").value.trim(), btn = $("sim-run");
    if (!/^[1-9]\d*$/.test(n) || Number(n) > CONFIG.SIM_MAX) return show("sim-state", "Enter a whole number from 1 to " + CONFIG.SIM_MAX + ".", "err");
    var b = {}; b[CONFIG.REQUEST.simCount] = Number(n);
    btn.disabled = true; show("sim-state", "Running simulation…"); $("sim-metrics").replaceChildren();
    api("POST", CONFIG.PATHS.simRun, b).then(function (res) {
      if (!res.ok) throw new Error(res.status === 404 || res.status === 501 ? "Simulation endpoint is not available on the backend yet (HTTP " + res.status + ")" : "Simulation failed: " + errText(res));
      if (CONFIG.SIM_FLOW === "stats") return api("GET", CONFIG.PATHS.simStats).then(function (r2) { if (!r2.ok) throw new Error("Could not load simulation stats: " + errText(r2)); return r2.data; });
      return res.data;
    }).then(function (data) {
      var m = data && (data.metrics && typeof data.metrics === "object" ? data.metrics : data);
      if (!m || typeof m !== "object" || Array.isArray(m)) throw new Error("Simulation returned no metrics");
      renderMetrics(m); show("sim-state", "Simulation finished. Values below are simulated, not real purchases.");
    }).catch(function (e) { show("sim-state", e.message + ". No metrics shown.", "err"); })
      .finally(function () { btn.disabled = false; });
  }
  function renderMetrics(m) {
    var box = $("sim-metrics"), used = {}; box.replaceChildren();
    function card(label, val) { var c = node("div", undefined, "metric"); c.appendChild(node("b", String(val))); c.appendChild(node("span", label)); box.appendChild(c); }
    CONFIG.METRICS.forEach(function (d) {
      d.keys.forEach(function (k) { used[k] = 1; });
      var v = pick(m, d.keys);
      card(d.label, v === undefined ? "Unavailable" : (typeof v === "number" && d.unit ? Math.round(v * 100) / 100 + d.unit : v));
    });
    Object.keys(m).forEach(function (k) { if (!used[k] && ["string", "number", "boolean"].indexOf(typeof m[k]) >= 0) card(k, m[k]); });
  }

  /* ---------- Start ---------- */
  function start() {
    $("refresh").addEventListener("click", function () { loadProducts(); loadOrders(); });
    $("sim-run").addEventListener("click", runSim);
    if (CONFIG.MOCK) {
      $("mock-banner").hidden = false; document.title = "[MOCK] FlashLock Dashboard"; $("sim-run").disabled = true;
      show("sim-state", "Simulation is not available in mock mode.");
    }
    loadProducts().then(loadOrders);
  }
  if (CONFIG.MOCK) {
    var s = document.createElement("script"); s.src = "mock.js"; s.onload = start; s.onerror = start; document.head.appendChild(s);
  } else start();
})();
