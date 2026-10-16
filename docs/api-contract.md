# FlashLock API Contract

## Purpose

This document defines the agreed API between the frontend
and the Spring Boot backend.

## Candidate Endpoints

The following endpoints are proposed and must be confirmed
against the actual backend implementation.

### GET /api/products

Purpose: Retrieve the available products and their stock.

Status: PENDING IMPLEMENTATION/VERIFICATION.

### POST /api/purchase

Purpose: Attempt to purchase a product.

Illustrative request:

{
  "productId": 1,
  "userId": 101,
  "quantity": 1
}

Status: PENDING TEAM CONFIRMATION.

### GET /api/orders

Purpose: Retrieve orders, if required by the final project scope.

Status: PENDING TEAM CONFIRMATION.

### POST /api/simulation/run

Purpose: Run the OS concurrency simulation, if required.

Status: PENDING TEAM CONFIRMATION.

### GET /api/simulation/stats

Purpose: Retrieve OS simulation metrics, if required.

Status: PENDING TEAM CONFIRMATION.

## Decisions Required

Before integration, agree on:

- Exact endpoint paths and HTTP methods.
- Request field names and data types.
- Success response structure.
- Error response structure.
- HTTP status codes.
- Whether quantities greater than one are supported.
- Whether failed purchase attempts are persisted.
- How simulation metrics are exposed.

## Rules

- The frontend must not invent endpoint paths or response fields.
- The backend owner must communicate API changes to affected members.
- Update this document whenever an approved API contract changes.
- Examples in this document are proposals until confirmed.
