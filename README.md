# Doubly-Linked Circular List & Transaction Sort Engine

A high-performance doubly-linked circular list data structure implemented from scratch in C, used to power a sorted bank transaction ledger with formatted output.
## Overview

This project has two components:

1. **`my402list.c`** — A generic doubly-linked circular list library supporting insertion, deletion, traversal, and search operations in O(1) to O(n) time
2. **`warmup1.c`** — A command-line transaction processor that parses bank records, sorts them chronologically using the list, and outputs a formatted financial ledger

## Architecture

### Circular List with Sentinel Node

The list uses an **anchor (sentinel) node** embedded directly in the list struct. This eliminates null-pointer edge cases for empty lists, head insertions, and tail insertions — all operations use the same 4-pointer update pattern:

```
         ┌──────────────────────────────────────────┐
         │                                          │
         ▼                                          │
    ┌─────────┐     ┌─────────┐     ┌─────────┐    │
    │ ANCHOR  │◄───►│ elem 1  │◄───►│ elem 2  │◄───┘
    │(sentinel)│     │  obj *──┼──►  │  obj *──┼──►
    └─────────┘     └─────────┘     └─────────┘
         ▲                                          │
         │                                          │
         └──────────────────────────────────────────┘
```

**Key design decision:** The `next`/`prev` pointers live **outside** the stored objects (in `My402ListElem` wrappers), so the same object can be inserted into multiple lists simultaneously — a pattern used extensively in OS kernels.

### API

| Function | Complexity | Description |
|----------|-----------|-------------|
| `Init()` | O(1) | Initialize anchor to point to itself |
| `Append(obj)` / `Prepend(obj)` | O(1) | Insert at tail/head |
| `InsertBefore(obj, elem)` / `InsertAfter(obj, elem)` | O(1) | Insert relative to an element |
| `Unlink(elem)` | O(1) | Remove and free a node |
| `UnlinkAll()` | O(n) | Remove all nodes |
| `First()` / `Last()` | O(1) | Access head/tail |
| `Next(elem)` / `Prev(elem)` | O(1) | Bidirectional traversal |
| `Find(obj)` | O(n) | Linear search by pointer equality |
| `Length()` / `Empty()` | O(1) | Cached member count |

### Transaction Processor

The sort engine reads bank transactions from a file or stdin, validates each record, and produces a fixed-width 80-column ledger:

```
+-----------------+--------------------------+----------------+----------------+
|       Date      | Description              |         Amount |        Balance |
+-----------------+--------------------------+----------------+----------------+
| Thu Aug 21 2008 | Initial deposit          |      1,723.00  |      1,723.00  |
| Thu Jan  1 2009 | Phone bill               | (       45.33) |      1,677.67  |
| Mon Jul 13 2009 | Dear parents             |     10,388.07  |     12,065.74  |
| Sun Jan 10 2010 | Beemer monthly payment - | (      654.32) |     11,411.42  |
+-----------------+--------------------------+----------------+----------------+
```

**Design highlights:**
- **Integer arithmetic for money** — all monetary values stored as `long` cents to eliminate floating-point rounding errors
- **Sorted insertion** — transactions are inserted in-order by timestamp as they're read, avoiding a separate sort pass
- **Strict input validation** — rejects malformed records (bad types, future timestamps, invalid amounts, duplicate timestamps, oversized lines) with descriptive error messages
- **Comma-formatted currency** — custom formatter handles comma grouping, parenthesized negatives, and overflow display for values exceeding $10M

## Build & Run

```bash
# Build the transaction processor
make warmup1

# Sort transactions from a file
./warmup1 sort transactions.tfile

# Sort transactions from stdin
cat transactions.tfile | ./warmup1 sort

# Build and run the list test suite
make listtest
./listtest              # no output = all tests pass
./listtest -debug       # verbose test output

# Clean build artifacts
make clean
```

## Input Format

Tab-delimited records with 4 fields per line:

```
+	1219356033	1723.00	Initial deposit
-	1230815233	45.33	Phone bill
```

| Field | Format | Description |
|-------|--------|-------------|
| Type | `+` or `-` | Deposit or withdrawal |
| Timestamp | Unix epoch (seconds) | Must be > 0 and in the past |
| Amount | `digits.dd` | Up to 7 digits before decimal, exactly 2 after |
| Description | Free text | Leading spaces stripped; must not be empty |

## Technical Details

- **Language:** C (C99)
- **Platform:** Linux (Ubuntu 20.04), also builds on macOS
- **Compiler:** GCC with `-g -Wall` (zero warnings)
- **Build system:** Make with separate compilation (each `.c` compiled independently, then linked)
- **Memory management:** All list nodes dynamically allocated; no arrays used for list operations; no memory leaks
- **Testing:** Verified against 50+ automated test cases including 20 randomized shuffle-and-sort trials and 30 transaction file tests

## Project Structure

```
├── my402list.h      # List API (provided header)
├── my402list.c      # List implementation (14 functions)
├── warmup1.c        # Transaction sort engine
├── cs402.h          # Shared constants (TRUE/FALSE/NULL)
├── listtest.c       # Automated list test suite
├── test.tfile       # Sample transaction file
├── Makefile         # Build rules for warmup1 + listtest
└── w1-README.txt    # Assignment submission notes
```
