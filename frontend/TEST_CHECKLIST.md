# FlashLock Frontend — UI Test Checklist
Record only what was actually run. Allowed results: PASS / FAIL / BLOCKED / NOT RUN.

| ID | Test | Expected | Result | Notes |
|----|------|----------|--------|-------|
| UI-01 | Page loads | No console errors; CSS and JS load | NOT RUN | |
| UI-02 | Inventory fetch succeeds | Names/prices/stock match API and DB; no mock banner | NOT RUN | |
| UI-03 | Inventory endpoint unavailable | Clear error state; no mock data shown as live | NOT RUN | |
| UI-04 | Valid purchase | Success text matches backend; stock refreshes; order appears (if contract lists it) | NOT RUN | |
| UI-05 | Insufficient stock | "Rejected" shown; no success; stock refreshed | NOT RUN | |
| UI-06 | Invalid quantity (empty/0/negative/decimal/text) | Rejected in UI; backend validates independently | NOT RUN | |
| UI-07 | Orders | Renders actual fields; empty list and failure handled | NOT RUN | |
| UI-08 | Simulation | Metrics match backend result; labelled as simulated; unavailable state if no endpoint | NOT RUN | |
| UI-09 | Double click / pending | Buttons locked while pending; correctness not dependent on it | NOT RUN | |
| UI-10 | Responsive layout | Usable at evaluation screen size | NOT RUN | |
