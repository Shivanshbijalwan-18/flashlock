package com.flashlock.backend.api;

import com.flashlock.backend.model.ApiError;
import com.flashlock.backend.service.InsufficientStockException;
import com.flashlock.backend.service.InvalidPurchaseException;
import com.flashlock.backend.service.ProductNotFoundException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.dao.DataAccessException;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.http.converter.HttpMessageNotReadableException;
import org.springframework.web.bind.MethodArgumentNotValidException;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.RestControllerAdvice;

@RestControllerAdvice
public class ApiExceptionHandler {
    private static final Logger logger = LoggerFactory.getLogger(ApiExceptionHandler.class);

    @ExceptionHandler({MethodArgumentNotValidException.class, HttpMessageNotReadableException.class,
            InvalidPurchaseException.class})
    public ResponseEntity<ApiError> badRequest(Exception ignored) {
        return ResponseEntity.badRequest().body(new ApiError(false, "Invalid request data"));
    }

    @ExceptionHandler(ProductNotFoundException.class)
    public ResponseEntity<ApiError> productNotFound(ProductNotFoundException exception) {
        return ResponseEntity.status(HttpStatus.NOT_FOUND)
                .body(new ApiError(false, exception.getMessage()));
    }

    @ExceptionHandler(InsufficientStockException.class)
    public ResponseEntity<ApiError> insufficientStock(InsufficientStockException exception) {
        return ResponseEntity.status(HttpStatus.CONFLICT)
                .body(new ApiError(false, exception.getMessage()));
    }

    @ExceptionHandler(DataAccessException.class)
    public ResponseEntity<ApiError> databaseFailure(DataAccessException exception) {
        logger.error("Database operation failed while handling an API request", exception);
        return internalError();
    }

    @ExceptionHandler(Exception.class)
    public ResponseEntity<ApiError> unexpectedFailure(Exception exception) {
        logger.error("Unexpected error while handling an API request", exception);
        return internalError();
    }

    private ResponseEntity<ApiError> internalError() {
        return ResponseEntity.status(HttpStatus.INTERNAL_SERVER_ERROR)
                .body(new ApiError(false, "Internal server error"));
    }
}
