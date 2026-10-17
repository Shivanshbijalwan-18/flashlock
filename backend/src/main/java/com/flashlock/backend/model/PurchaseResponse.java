package com.flashlock.backend.model;

public record PurchaseResponse(boolean success, String message, long orderId, int remainingStock) {
}
