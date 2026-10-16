/* DEMO MOCK DATA — NOT LIVE DATABASE. Loaded only when the URL has ?mock=1.
   Never used by default. Does not touch PostgreSQL. Simulation is intentionally unavailable. */
(function () {
  "use strict";
  var products = [
    { productId: 1, name: "MOCK Item A", price: 10, stock: 5 },
    { productId: 2, name: "MOCK Item B", price: 25, stock: 1 },
    { productId: 3, name: "MOCK Item C", price: 40, stock: 0 }
  ];
  var orders = [], nextId = 1;
  window.FLASHLOCK_MOCK = function (method, path, body) {
    var r = function (status, data) { return Promise.resolve({ ok: status < 400, status: status, data: data }); };
    if (path.indexOf("/api/products") === 0) return r(200, products.map(function (p) { return Object.assign({}, p); }));
    if (path.indexOf("/api/orders") === 0) return r(200, orders.slice());
    if (path.indexOf("/api/purchase") === 0) {
      var p = products.filter(function (x) { return x.productId === body.productId; })[0];
      if (!p) return r(404, { success: false, message: "Product not found (mock)" });
      if (p.stock < body.quantity) return r(409, { success: false, message: "Insufficient stock (mock)" });
      p.stock -= body.quantity;
      var o = { orderId: nextId++, productId: p.productId, userId: body.userId, quantity: body.quantity, status: "SUCCESS", createdAt: new Date().toISOString() };
      orders.unshift(o);
      return r(200, { success: true, message: "Purchase successful (mock)", orderId: o.orderId, remainingStock: p.stock });
    }
    return r(404, { message: "Not available in mock mode" });
  };
})();
