# Pyoneer C Test Framework (unittest-style)

This repository includes a small C testing framework intentionally modeled after Python’s `unittest` module. The goal is to keep tests **easy to write**, **easy to read**, and **fast to run**, while fitting the rest of the system’s design conventions.

## Core componets

- **TestCase**  
  A single runnable test (one test function + metadata).

- **TestSuite**  
  A collection of related TestCases (typically one suite per `test_*.c` file).

- **TestProgram**  
  A top-level orchestrator that holds multiple TestSuites and runs them (similar to `unittest.main()`).
