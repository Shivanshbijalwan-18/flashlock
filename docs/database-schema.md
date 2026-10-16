# FlashLock Database Schema

## Database

PostgreSQL

## Purpose

This document defines the canonical database schema used
by the backend and database-testing module.

## Proposed Products Table

Candidate fields:

- product_id: BIGINT primary key
- name: VARCHAR(150), not null
- price: NUMERIC(10,2), not null
- stock: INTEGER, not null, stock must be >= 0

## Proposed Orders Table

Candidate fields:

- order_id: generated primary key
- product_id: foreign key referencing products
- user_id: BIGINT, not null
- quantity: INTEGER, not null, quantity must be > 0
- status: VARCHAR(30), not null
- created_at: timestamp, default current timestamp

## Status

PROPOSED — MEMBER 1 AND MEMBER 2 MUST CONFIRM.

These are candidate fields, not proof that the schema has
already been created.

## Decisions Required

- Confirm table and column names.
- Confirm primary and foreign keys.
- Confirm data types and constraints.
- Confirm order status values.
- Confirm the policy for failed purchase attempts.
- Confirm the actual SQL files match this specification.

## Rules

- Do not independently rename columns or tables.
- Coordinate schema changes with the database and backend owners.
- Update this document after an approved schema change.
