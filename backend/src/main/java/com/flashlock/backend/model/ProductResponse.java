package com.flashlock.backend.model;

import java.math.BigDecimal;

public record ProductResponse(long productId, String name, BigDecimal price, int stock) {
}
