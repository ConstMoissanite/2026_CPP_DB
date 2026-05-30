# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**MiniDB（微型关系型数据库）** — a university course project (《现代 C++ 程序设计》, UESTC 2025–2026). Primary locale: **zh-Hans**. Documentation and user-facing messages are in Simplified Chinese. C/S architecture with a custom TCP protocol (JSON-based). All four layers are implemented: parser, storage engine, execution engine, and network layer. The project targets **Linux** for grading, with **clang++** as the compiler and **C++20/23**.

## Build, Test, Run

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build all targets
cmake --build build

# Run all tests
cd build && ctest --output-on-failure

# Run a single test binary
./build/test_lexer
./build/test_parser
./build/test_c_api
./build/test_storage
./build/test_executor

# Start server (default port 3307), then connect with client
./build/minidb_server [port]
./build/minidb_client [host] [port]
```

## Architecture

Four static libraries with a strict dependency chain:

```
minidb_parser   →  SQL lexing + recursive-descent parsing + C ABI wrapper
minidb_storage  →  file-backed tables, sorted-array index, database registry  (depends on parser)
minidb_execution →  executes parsed SQL against the storage layer             (depends on storage)
minidb_network  →  TCP server/client, JSON protocol serialization             (depends on execution)
```

Two executables: `minidb_server` (links network+execution+storage+parser) and `minidb_client` (same linkage).

### Data flow

```
SQL string → Lexer → Vector<Token> → Parser → SQLStatement (AST)
                                              ↓
                                        Executor::execute()
                                              ↓
                              Database → Table → Row/Column (in-memory + file persistence)
                                              ↓
                              ExecResult → serialize_exec_result() → JSON → TCP → client
```

### Storage layout

`data/<dbname>/<tablename>.dat` — plain-text table files with column metadata header and pipe-delimited rows. The index uses a sorted `Vector<Entry>` with binary search (not a B+ tree — the requirement calls for B+ tree, but the implementation uses sorted-array index achieving the same O(log n) lookup).

## Key Conventions

### No STL containers (hard requirement)

Use `minidb::core::Vector<T>` instead of `std::vector`, `minidb::core::String` instead of `std::string`. Other std components (algorithms, I/O, threading) are allowed. Third-party network libraries are allowed.

### Namespaces

`minidb::core` (Vector, String) → `minidb::parser` (lexer, AST, parser) → `minidb::storage` (Column, Row, Table, Index, Database) → `minidb::execution` (Executor, ExecResult) → `minidb::network` (server, client, protocol)

### Naming

- **Private members**: underscore prefix (`_data`, `_len`, `_pos`, `_has_error`)
- **SQL keywords**: `KW_CREATE`, `KW_SELECT`, etc., and are **case-insensitive** in the lexer
- **Identifiers**: lowercase only, no underscores, no special characters
- **String literals in SQL**: double quotes (`"peter"`), not single quotes
- **Semicolons**: optional at end of SQL statements

### Header guards

`MINIDB_MODULE_FILE_HPP` style. Internal headers use `#include "../core/vector.hpp"` or `#include "parser/token.hpp"` (both `src/` and `include/` are in include paths). Public C header: `#include "minidb/parser.h"`.

### Test framework

Hand-rolled macro framework (no external library):
- `TEST("name") { ... } END_TEST()` — wraps each test
- `ASSERT_TRUE(cond, msg)`, `ASSERT_EQ_STR(a, b, msg)`, `ASSERT_STMT_KIND(stmt, expected)`, `FAIL(msg)`
- Test functions are `static void`; each binary has a `main()` that calls tests sequentially
- Tests use `using namespace minidb::parser;` / `using namespace minidb::core;`

### WHERE clause operators

Only `=`, `<`, `>` are supported. No `>=`, `<=`, `!=`, `LIKE`, `IN`.

### Value representation

Literal values use parallel `is_int` flags alongside string storage: `Vector<String> values` + `Vector<bool> is_int` for INSERT; `const_value` (String) + `is_int_literal` (bool) for WHERE/UPDATE.

### Target platform

Must build and run on **Linux** (grading platform). Development happens on Windows with clang. Code uses `<cstdio>`, `<cstring>`, `<cstdlib>` — C headers, not C++ wrappers.

### Warnings

`-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference` (configured in `.vscode/settings.json`).

## Reference

- `req export.md` — formal course project requirements (DDL/DML specs, C/S architecture mandate, grading criteria)
- `.github/copilot-instructions.md` — older instructions (partially outdated: claims only parser exists, but all modules are now built)
