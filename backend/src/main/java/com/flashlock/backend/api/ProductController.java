package com.flashlock.backend.api;

import com.flashlock.backend.model.ProductResponse;
import com.flashlock.backend.service.PurchaseService;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

@RestController
@RequestMapping("/api/products")
public class ProductController {
    private final PurchaseService service;

    public ProductController(PurchaseService service) {
        this.service = service;
    }

    @GetMapping
    public List<ProductResponse> products() {
        return service.listProducts();
    }
}
