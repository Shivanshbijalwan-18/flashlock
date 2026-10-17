package com.flashlock.backend.model;

import jakarta.validation.constraints.NotNull;
import jakarta.validation.constraints.Positive;

public record PurchaseRequest(
        @NotNull @Positive Long productId,
        @NotNull @Positive Long userId,
        @NotNull @Positive Integer quantity) {
}
