# FlashLock Architecture

## Project Overview

FlashLock is a high-concurrency flash-sale inventory and
transaction management system.

## Technology Stack

- OS Engine: C11 and POSIX threads
- Backend: Java and Spring Boot
- Database: PostgreSQL
- Frontend: HTML, CSS, and JavaScript

## Main Components

1. Frontend: Displays products, purchase controls, and system metrics.
2. Backend: Provides API endpoints and coordinates purchase operations.
3. Database: Stores product inventory and order records.
4. OS Engine: Simulates concurrent request processing using threads,
   synchronization primitives, a bounded queue, and worker threads.

## Important Design Distinction

The OS engine simulation and the actual database purchase flow
are separate parts of the project.

C mutexes and semaphores coordinate threads within the C process.
PostgreSQL transactions and appropriate locking protect persistent
inventory and order data.

## C-to-Backend Integration

Status: PENDING TEAM CONFIRMATION.

The team must confirm whether the C engine will communicate with
the backend using UNIX domain sockets or a subprocess.

Do not implement a different integration method without agreement.

## Status

This document is a starter specification. Update it when the
team agrees on the final architecture.
