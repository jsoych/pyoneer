# Pyoneer C Test Framework (unittest-style)

This repository includes a small C testing framework intentionally modeled after Python’s `unittest` module. The goal is to keep tests **easy to write**, **easy to read**, and **fast to run**, while fitting the rest of the system’s design conventions.

## Design Goals

- Mirror the mental model of Python `unittest`
- Keep tests self-contained (expected values live beside assertions)
- Avoid passing expected outputs through registration APIs
- Support running **all suites** or a **selected suite**
- Match project conventions:
  - **Objects are capitalized and opaque**
  - **Plain structs are lowercase and visible**

---

## Core Concepts

### Objects (opaque, capitalized)

- **TestCase**  
  A single runnable test (one test function + metadata).

- **TestSuite**  
  A collection of related TestCases (typically one suite per `test_*.c` file).

- **TestProgram**  
  A top-level orchestrator that holds multiple TestSuites and runs them (similar to `unittest.main()`).

> Note: A `TestRunner` object is intentionally deferred to a future cycle. `TestProgram` currently owns execution and reporting.

### Value Types (visible, lowercase)

- **`test_result_t`**  
  Aggregates results such as runs, failures, errors, and optional diagnostic details.

---

## Execution Hierarchy

