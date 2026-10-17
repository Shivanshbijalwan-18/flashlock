package com.flashlock.backend.integration;

import com.flashlock.backend.model.PurchaseRequest;
import com.flashlock.backend.service.InsufficientStockException;
import com.flashlock.backend.service.PurchaseService;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.condition.EnabledIfSystemProperty;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.jdbc.core.JdbcTemplate;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicLong;

import static org.junit.jupiter.api.Assertions.assertEquals;

@SpringBootTest
@EnabledIfSystemProperty(named = "flashlock.integration", matches = "true")
class PurchaseConcurrencyIntegrationTest {
    private static final int INITIAL_STOCK = 5;
    private static final int ATTEMPTS = 20;
    private static final AtomicLong IDS = new AtomicLong(System.currentTimeMillis());

    @Autowired
    private JdbcTemplate jdbc;

    @Autowired
    private PurchaseService service;

    private long productId;

    @BeforeEach
    void createIsolatedProductOnTestDatabase() {
        String database = jdbc.queryForObject("SELECT current_database()", String.class);
        if (!"flashlock_test".equals(database)) {
            throw new IllegalStateException("Integration tests are restricted to database flashlock_test");
        }
        long candidateId = IDS.incrementAndGet();
        jdbc.update("INSERT INTO products (product_id, name, price, stock) VALUES (?, ?, 1.00, ?)",
                candidateId, "FlashLock concurrency test", INITIAL_STOCK);
        // Only clean up a row after this test has successfully inserted it.
        productId = candidateId;
    }

    @AfterEach
    void removeOnlyTheTestRows() {
        if (productId > 0) {
            jdbc.update("DELETE FROM orders WHERE product_id = ?", productId);
            jdbc.update("DELETE FROM products WHERE product_id = ?", productId);
        }
    }

    @Test
    void concurrentRequestsNeverSellMoreThanPersistedStock() throws Exception {
        ExecutorService executor = Executors.newFixedThreadPool(ATTEMPTS);
        CountDownLatch ready = new CountDownLatch(ATTEMPTS);
        CountDownLatch start = new CountDownLatch(1);
        try {
            List<Future<Boolean>> results = new ArrayList<>();
            for (int i = 0; i < ATTEMPTS; i++) {
                long userId = i + 1L;
                results.add(executor.submit(() -> {
                    ready.countDown();
                    start.await();
                    try {
                        service.purchase(new PurchaseRequest(productId, userId, 1));
                        return true;
                    } catch (InsufficientStockException rejected) {
                        return false;
                    }
                }));
            }
            ready.await();
            start.countDown();

            int successes = 0;
            for (Future<Boolean> result : results) {
                if (result.get()) successes++;
            }

            Integer finalStock = jdbc.queryForObject(
                    "SELECT stock FROM products WHERE product_id = ?", Integer.class, productId);
            Integer orderCount = jdbc.queryForObject(
                    "SELECT COUNT(*) FROM orders WHERE product_id = ? AND status = 'SUCCESS'",
                    Integer.class, productId);
            Integer unitsOrdered = jdbc.queryForObject(
                    "SELECT COALESCE(SUM(quantity), 0) FROM orders " +
                            "WHERE product_id = ? AND status = 'SUCCESS'", Integer.class, productId);

            assertEquals(INITIAL_STOCK, successes);
            assertEquals(0, finalStock);
            assertEquals(INITIAL_STOCK, orderCount);
            assertEquals(INITIAL_STOCK, unitsOrdered);
        } finally {
            executor.shutdownNow();
        }
    }
}
