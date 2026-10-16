# FlashLock — Universal Coding Agent Rules

## 1. Project authority
- The shared Git repository is the source of truth.
- Read this file and relevant project documentation before editing.
- Inspect existing code, build files, and git status before creating or changing files.
- Never assume a proposed feature has already been implemented.
- Treat documented examples as proposals until confirmed by the team.

## 2. Technology restrictions
- OS engine: C11 and POSIX threads.
- Backend: Java + Spring Boot.
- Database: PostgreSQL.
- Frontend: plain HTML, CSS, and JavaScript.
- Do not introduce another framework, language, database, or service without team approval.

## 3. File ownership
- Member 1 owns os-engine/ and backend/.
- Member 2 owns database/.
- Member 3 owns frontend/.
- Shared documentation changes must be coordinated with affected owners.
- Do not edit outside your assigned scope without approval.
- Do not overwrite another member's work or reformat unrelated files.

## 4. API contract
- Treat docs/api-contract.md as the canonical API specification once approved.
- Do not invent endpoint paths, request fields, response fields, or error formats.
- If code and the documented contract disagree, report the discrepancy and coordinate before changing either.
- Record approved contract changes in documentation and notify affected members.

## 5. Database contract
- Treat docs/database-schema.md as canonical once approved.
- Do not independently rename tables/columns or change types, constraints, keys, or relationships.
- Coordinate schema changes with the database owner and backend owner.
- Never use mock data as proof that PostgreSQL integration works.

## 6. Dependencies and configuration
- Inspect existing dependency/build files before adding packages.
- Preserve compatible versions and existing project conventions.
- Do not add a dependency unless it is needed; explain why it is needed.
- Never commit passwords, tokens, API keys, or real secrets.
- Use environment variables or a documented local configuration method.
- Do not commit local IDE settings, build output, or generated files unless the team explicitly requires them.

## 7. Safe changes
- Make small, focused, reviewable changes.
- Do not delete working code simply to replace it with a new design.
- Do not overwrite uncommitted work; inspect git status first.
- Do not run destructive database commands or reset shared data without explicit approval.
- Do not silently change the architecture or the C-to-backend communication method.
- Do not claim code is complete or tested unless the evidence supports that claim.

## 8. Testing and reporting
- Run relevant tests that are possible in the current environment.
- Report exact commands executed and actual results.
- Label tests PASS, FAIL, BLOCKED, or NOT RUN.
- Explain environment-related blockers instead of hiding them.
- Do not fabricate test results, concurrency guarantees, or performance measurements.
- Distinguish simulated OS-engine metrics from real PostgreSQL/backend measurements.

## 9. Integration correctness
- C mutexes/semaphores coordinate threads in the C process; they do not protect PostgreSQL data.
- Persistent inventory updates and order creation must be handled safely in a database transaction.
- Successful stock decrement and order insertion must succeed or fail atomically.
- FIFO queue dispatch does not guarantee concurrent worker completion order.
- Do not present simulated purchase outcomes as actual database transactions.
- Confirm whether the approved integration method is UNIX domain sockets or a subprocess before implementing it.

## 10. Handoff requirements
Before handing off work, report:
1. Files created or modified.
2. Dependencies added and why.
3. Commands to build and run the module.
4. API/schema/architecture assumptions.
5. Tests executed, with exact results.
6. Known limitations and unresolved issues.
7. Any action required from another member.

## 11. When information is missing
- Do not guess silently or create a competing implementation.
- Ask the responsible member or record a blocker in docs/integration-status.md.
- If a change affects another module, propose it and wait for agreement before proceeding.
- Keep docs aligned with the actual code.

## 12. Definition of done
