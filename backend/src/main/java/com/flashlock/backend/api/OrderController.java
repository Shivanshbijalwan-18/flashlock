package com.flashlock.backend.api;

import com.flashlock.backend.model.OrderResponse;
import com.flashlock.backend.service.PurchaseService;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
@RequestMapping("/api/orders")
public class OrderController {
    private final PurchaseService service;

    public OrderController(PurchaseService service) {
        this.service = service;
    }

    @GetMapping
    public List<OrderResponse> orders() {
        return service.listOrders();
    }
}
