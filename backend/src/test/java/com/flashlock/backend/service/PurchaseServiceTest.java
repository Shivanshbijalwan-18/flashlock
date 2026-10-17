package com.flashlock.backend.service;

import com.flashlock.backend.model.PurchaseRequest;
import com.flashlock.backend.model.PurchaseResponse;
import com.flashlock.backend.repository.FlashLockRepository;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InOrder;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class PurchaseServiceTest {
    @Mock
    private FlashLockRepository repository;

    @InjectMocks
    private PurchaseService service;

    @Test
    void successfulPurchaseDecrementsThenCreatesOrder() {
        when(repository.decrementStockIfAvailable(1L, 2)).thenReturn(1);
        when(repository.findStock(1L)).thenReturn(3);
        when(repository.insertSuccessfulOrder(1L, 101L, 2)).thenReturn(456L);

        PurchaseResponse response = service.purchase(new PurchaseRequest(1L, 101L, 2));

        assertEquals(new PurchaseResponse(true, "Purchase successful", 456L, 3), response);
        InOrder order = inOrder(repository);
        order.verify(repository).decrementStockIfAvailable(1L, 2);
        order.verify(repository).findStock(1L);
        order.verify(repository).insertSuccessfulOrder(1L, 101L, 2);
    }

    @Test
    void invalidQuantityIsRejectedBeforeDatabaseWork() {
        assertThrows(InvalidPurchaseException.class,
                () -> service.purchase(new PurchaseRequest(1L, 101L, 0)));
        verify(repository, never()).decrementStockIfAvailable(1L, 0);
    }

    @Test
    void unknownProductIsDistinguishedFromInsufficientStock() {
        when(repository.decrementStockIfAvailable(999L, 1)).thenReturn(0);
        when(repository.productExists(999L)).thenReturn(false);

        assertThrows(ProductNotFoundException.class,
                () -> service.purchase(new PurchaseRequest(999L, 101L, 1)));
        verify(repository, never()).insertSuccessfulOrder(999L, 101L, 1);
    }

    @Test
    void insufficientStockDoesNotInsertOrder() {
        when(repository.decrementStockIfAvailable(1L, 2)).thenReturn(0);
        when(repository.productExists(1L)).thenReturn(true);

        assertThrows(InsufficientStockException.class,
                () -> service.purchase(new PurchaseRequest(1L, 101L, 2)));
        verify(repository, never()).insertSuccessfulOrder(1L, 101L, 2);
    }
}
