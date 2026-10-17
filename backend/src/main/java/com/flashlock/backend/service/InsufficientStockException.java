package com.flashlock.backend.service;

public class InsufficientStockException extends RuntimeException {
    public InsufficientStockException() {
        super("Insufficient stock");
    }
}
