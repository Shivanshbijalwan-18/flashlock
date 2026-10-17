package com.flashlock.backend.repository;

import com.flashlock.backend.model.OrderResponse;
import com.flashlock.backend.model.ProductResponse;
import org.springframework.jdbc.core.JdbcTemplate;
import org.springframework.jdbc.support.GeneratedKeyHolder;
import org.springframework.jdbc.support.KeyHolder;
import org.springframework.stereotype.Repository;

import java.sql.PreparedStatement;
import java.sql.Timestamp;
import java.util.List;

@Repository
public class FlashLockRepository {
    private final JdbcTemplate jdbc;

    public FlashLockRepository(JdbcTemplate jdbc) {
        this.jdbc = jdbc;
    }

    public List<ProductResponse> findProducts() {
        return jdbc.query(
                "SELECT product_id, name, price, stock FROM products ORDER BY product_id",
                (rs, rowNum) -> new ProductResponse(
                        rs.getLong("product_id"),
                        rs.getString("name"),
                        rs.getBigDecimal("price"),
                        rs.getInt("stock")
                )
        );
    }

    public int decrementStockIfAvailable(long productId, int quantity) {
        return jdbc.update(
                "UPDATE products SET stock = stock - ? " +
                        "WHERE product_id = ? AND stock >= ?",
                quantity, productId, quantity
        );
    }

    public boolean productExists(long productId) {
        Boolean exists = jdbc.queryForObject(
                "SELECT EXISTS (SELECT 1 FROM products WHERE product_id = ?)",
                Boolean.class,
                productId
        );
        return Boolean.TRUE.equals(exists);
    }

    public int findStock(long productId) {
        Integer stock = jdbc.queryForObject(
                "SELECT stock FROM products WHERE product_id = ?",
                Integer.class,
                productId
        );

        if (stock == null) {
            throw new IllegalStateException(
                    "Stock query returned no value for an updated product"
            );
        }

        return stock;
    }

    public long insertSuccessfulOrder(long productId, long userId, int quantity) {
        KeyHolder keys = new GeneratedKeyHolder();

        jdbc.update(connection -> {
            PreparedStatement statement = connection.prepareStatement(
                    "INSERT INTO orders (product_id, user_id, quantity, status) " +
                            "VALUES (?, ?, ?, 'SUCCESS')",
                    new String[]{"order_id"}
            );

            statement.setLong(1, productId);
            statement.setLong(2, userId);
            statement.setInt(3, quantity);

            return statement;
        }, keys);

        Number key = keys.getKey();

        if (key == null) {
            throw new IllegalStateException(
                    "Database did not return an order ID"
            );
        }

        return key.longValue();
    }

    public List<OrderResponse> findOrders() {
        return jdbc.query(
                "SELECT order_id, product_id, user_id, quantity, status, created_at " +
                        "FROM orders ORDER BY created_at DESC, order_id DESC",
                (rs, rowNum) -> {
                    Timestamp createdAt = rs.getTimestamp("created_at");

                    return new OrderResponse(
                            rs.getLong("order_id"),
                            rs.getLong("product_id"),
                            rs.getLong("user_id"),
                            rs.getInt("quantity"),
                            rs.getString("status"),
                            createdAt == null ? null : createdAt.toLocalDateTime()
                    );
                }
        );
    }
}