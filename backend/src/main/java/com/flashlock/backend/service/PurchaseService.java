package com.flashlock.backend.service;

import com.flashlock.backend.model.OrderResponse;
import com.flashlock.backend.model.ProductResponse;
import com.flashlock.backend.model.PurchaseRequest;
import com.flashlock.backend.model.PurchaseResponse;
import com.flashlock.backend.repository.FlashLockRepository;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;

@Service
public class PurchaseService {
    private final FlashLockRepository repository;

    public PurchaseService(FlashLockRepository repository) {
        this.repository = repository;
    }

    public List<ProductResponse> listProducts() {
        return repository.findProducts();
    }

    public List<OrderResponse> listOrders() {
        return repository.findOrders();
    }

    @Transactional
    public PurchaseResponse purchase(PurchaseRequest request) {
        if (request == null || request.productId() == null || request.productId() <= 0 ||
                request.userId() == null || request.userId() <= 0 ||
                request.quantity() == null || request.quantity() <= 0) {
            throw new InvalidPurchaseException("productId, userId, and quantity must be positive integers");
        }

        int updated = repository.decrementStockIfAvailable(request.productId(), request.quantity());
        if (updated == 0) {
            if (!repository.productExists(request.productId())) {
                throw new ProductNotFoundException(request.productId());
            }
            throw new InsufficientStockException();
        }

        int remainingStock = repository.findStock(request.productId());
        long orderId = repository.insertSuccessfulOrder(
                request.productId(), request.userId(), request.quantity());
        return new PurchaseResponse(true, "Purchase successful", orderId, remainingStock);
    }
}
