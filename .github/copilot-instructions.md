# MiniDB Copilot Instructions

## Build, Test, and Lint

```bash
# Configure (from repo root)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Build everything
cmake --build build

# Run all tests via CTest
cd build && ctest --output-on-failure

# Run a single test binary directly
./build/test_lexer
./build/test_parser
./build/test_c_api
```

The project uses **C++20** (`CMAKE_CXX_STANDARD 20`) but targets **C++23** where available. The compiler is clang/clang++ on both Linux and Windows.

No linting or formatting tools are currently configured. Compiler warnings are enabled in `.vscode/settings.json`: `-Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion -Wnull-dereference`.

## Architecture

**Project**: MiniDB — a miniature relational database management system (course project). Client/server architecture with TCP/IP communication is planned but not yet implemented. Only the **SQL parser** has been built so far.

### Current Module Layout

```
include/minidb/parser.h     # Public C ABI (extern "C" wrapper for the parser)
src/core/
  string.hpp                 # Custom String class (heap-allocated C-string, no STL)
  vector.hpp                 # Custom Vector<T> template (raw memory management, no STL)
src/parser/
  token.hpp/.cpp             # Token types (keywords, identifiers, literals, operators)
  lexer.hpp/.cpp             # Hand-written lexer, case-insensitive keyword matching
  ast.hpp/.cpp               # AST node types: SQLStatement + 9 statement variants
  parser.hpp/.cpp            # Recursive-descent parser (lex+parse in one call)
  parser_c.cpp               # C ABI implementation — flattens C++ AST to C structs
tests/
  test_lexer.cpp             # Lexer unit tests (hand-rolled test framework, no library)
  test_parser.cpp            # Parser unit tests
  test_c_api.cpp             # C ABI unit tests
```

### Reference Documents

- `req export.md` — the formal course project requirements (DDL/DML specs, C/S architecture mandate, grading criteria)

### Pipeline

```
SQL string → Lexer (tokenize) → Vector<Token> → Parser (parse) → SQLStatement (AST)
                                                                    ↓
                                                           parser_c.cpp (flatten)
                                                                    ↓
                                                           MiniDB_Stmt (C struct)
```

- `Parser::parse(const char* sql)` is the all-in-one entry point — it internally creates a Lexer, tokenizes, then parses.
- The C ABI (`minidb_parse`, `minidb_stmt_free`, and type-specific accessors) is designed for FFI consumers that can't use C++.

### Planned but Not Yet Built

- **Storage engine** — file-based tables under `data/<dbname>/<tablename>.dat`, B+ tree indexes for primary keys at `data/<dbname>/<tablename>.idx`
- **Execution engine** — DDL/DML execution; interprets the AST and performs operations on the storage engine
- **Network engine** — TCP/IP client/server; server and client are separate binaries; data exchanged via serialized format (e.g., JSON); server parses SQL and executes, client sends SQL and displays formatted results
- **Interactive CLI** — prompt-based REPL imitating MySQL's shell:
  ```
  > create table person (id int primary, name string)
  [feedback]
  > insert into person values (1001, "peter")
  [feedback]
  > select name from person where id = 1001
  [result set]
  > exit
  ```

### Recommended Class Hierarchy (from Requirements)

The course project requirements suggest at least these classes; actual design may vary:

| Class | Role |
|---|---|
| `column` | Column metadata (name, type, primary key flag) |
| `row` | Container of column values |
| `table` | Container of rows; result sets are tables too |
| `index` | B+ tree index on primary key columns |
| `database` | Manages tables and indexes for one database |

### SQL Operator Limitations

WHERE clause operators are limited to exactly `=`, `<`, `>`. No `>=`, `<=`, `!=`, `LIKE`, `IN`, etc.

### Value Representation Pattern

Literal values (from INSERT, UPDATE, WHERE clauses) are stored as strings paired with a parallel `is_int` flag:
- `InsertStmt::values` (Vector&lt;String&gt;) + `InsertStmt::is_int` (Vector&lt;bool&gt;) — parallel arrays
- `WhereClause::const_value` (String) + `WhereClause::is_int_literal` (bool)
- `UpdateStmt::set_value` (String) + `UpdateStmt::set_is_int` (bool)

The int flag determines whether the string should be parsed as an integer at execution time.

## Key Conventions

### No STL Containers

**Do NOT use any C++ standard containers** (`std::vector`, `std::map`, `std::string`, `std::unordered_map`, `std::list`, `std::span`, container adapters, etc.). This is a hard project requirement. Use the custom replacements instead:

| STL Container | Replacement |
|---|---|
| `std::vector<T>` | `minidb::core::Vector<T>` |
| `std::string` | `minidb::core::String` |

Other standard library components (algorithms, threading, I/O) are allowed. Third-party network libraries are allowed.

### Namespaces

- `minidb::core` — custom data structures (Vector, String)
- `minidb::parser` — lexer, parser, AST types

### Header Guards

Use `MINIDB_MODULE_FILE_HPP` style. Include paths use relative paths from `src/` (e.g., `#include "../core/vector.hpp"` for internal sources) or from `include/` (e.g., `#include "minidb/parser.h"` for the public C header).

### Include Conventions

- **Public C header**: `#include "minidb/parser.h"` (for C ABI consumers)
- **Internal C++ headers**: `#include "parser/token.hpp"` or `#include "../core/string.hpp"` (since `src/` is in the include path)
- The CMake build adds both `${CMAKE_SOURCE_DIR}/include` and `${CMAKE_SOURCE_DIR}/src` as include directories for the `minidb_parser` target.

### Naming

- **Private members**: Prefixed with underscore (e.g., `_data`, `_len`, `_size`, `_pos`, `_col`, `_has_error`). Used consistently across all classes in `lexer.hpp`, `parser.hpp`, `string.hpp`, `vector.hpp`.
- SQL keywords in code: `KW_CREATE`, `KW_SELECT`, etc. (matching the SQL spec)
- SQL keywords are **case-insensitive** (the lexer lowercases identifiers before keyword matching)
- Database names, table names, column names: **lowercase only, no underscores, no special characters**
- String literals in SQL use **double quotes** (e.g., `"peter"`), not single quotes
- Semicolons at the end of SQL statements are **optional** (the parser handles both)

### Test Framework

Tests use a **hand-rolled macro-based framework** — no external test library:
- `TEST("name") { ... } END_TEST()` wraps each test
- `ASSERT_TRUE(cond, msg)` for boolean assertions
- `ASSERT_EQ_STR(a, b, msg)` for C-string comparison
- `ASSERT_STMT_KIND(stmt, expected)` for checking parsed statement kind (parser tests only)
- `FAIL(msg)` for early termination with message
- Test files include `<cstdio>`, `<cstring>`, `<cstdlib>` (C headers, not C++ wrappers)
- Test functions are `static void` — no test classes or fixtures
- Each test binary has a `main()` that calls all test functions in order, then returns 0 on success, 1 on failure
- Tests use `using namespace minidb::parser;` and `using namespace minidb::core;`

### Target Platform

Code must **build and run on Linux** (it's the grading platform). The current development environment is Windows with clang, but final verification must be on Linux.

### Data Type Restrictions

SQL types are limited to:
- `int` — C++ default integer width
- `string` — fixed-length 256-char maximum, UTF-8 encoded
