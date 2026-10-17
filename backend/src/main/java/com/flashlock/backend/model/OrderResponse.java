package com.flashlock.backend.model;

import java.time.LocalDateTime;

public record OrderResponse(long orderId, long productId, long userId, int quantity,
                            String status, LocalDateTime createdAt) {
}
