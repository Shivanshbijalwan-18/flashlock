# FlashLock backend

Java 17 / Spring Boot 3.5.16 REST backend using JDBC and PostgreSQL. It uses the existing `products` and `orders` schema; it does not create or migrate tables.

## Build and run on Windows

Install a Java 17 JDK, Maven 3.6.3 or later, and PostgreSQL separately. From PowerShell:

```powershell
cd C:\Users\Admin\Documents\flashlock\backend
$env:FLASHLOCK_DB_URL = 'jdbc:postgresql://localhost:5432/flashlock_db'
$env:FLASHLOCK_DB_USERNAME = 'your_local_database_user'
$env:FLASHLOCK_DB_PASSWORD = 'your_local_database_password'
mvn spring-boot:run
```

The URL defaults to localhost port 5432 and database `flashlock_db`; that default does not mean PostgreSQL is installed or running. Username and password default to empty. Set credentials in the current PowerShell session or another local secret store; do not commit them.

Build and unit tests:

```powershell
mvn test
mvn package
```

No Maven Wrapper is included because Maven is not installed in the implementation environment, so a correctly generated wrapper could not be verified.

The schema and seed must be applied separately using the existing database instructions. This backend does not execute schema creation or reset scripts.

## API

`GET /api/products` returns an array of `{ productId, name, price, stock }`.

`POST /api/purchase` accepts `{"productId":1,"userId":101,"quantity":1}`. A successful purchase returns HTTP 201 and `{"success":true,"message":"Purchase successful","orderId":123,"remainingStock":4}`. Invalid requests return 400, unknown products 404, insufficient stock 409, and unexpected failures a generic 500 response.

`GET /api/orders` returns an array of `{ orderId, productId, userId, quantity, status, createdAt }`.

The purchase endpoint conditionally decrements stock and inserts an order in one Spring-managed transaction. Failed purchases are not written to `orders`.

## Frontend CORS

The frontend context documents a local server at `http://localhost:5500`; that exact origin is allowed for `/api/**`. Other origins are not enabled by default.

## PostgreSQL concurrency integration test

Unit tests run with `mvn test`. The persistent concurrency test is opt-in and refuses to run unless the connected database is named `flashlock_test`. Create that dedicated database, apply the existing schema, set the same connection variables above with its URL, then run:

```powershell
mvn "-Dflashlock.integration=true" test
```

The test creates a uniquely identified product with stock 5, launches 20 concurrent service calls, checks the persisted stock and orders, then deletes only rows for that test product. Do not point it at a database containing unrelated data. It does not reset the database.
