cmake_minimum_required(VERSION 3.16)
project(minidb VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Parser library (C++ with C ABI wrapper)
# ============================================================
add_library(minidb_parser STATIC
    src/parser/token.cpp
    src/parser/ast.cpp
    src/parser/lexer.cpp
    src/parser/parser.cpp
    src/parser/parser_c.cpp
)

target_include_directories(minidb_parser
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# ============================================================
# Storage library
# ============================================================
add_library(minidb_storage STATIC
    src/storage/column.cpp
    src/storage/row.cpp
    src/storage/table.cpp
    src/storage/index.cpp
    src/storage/database.cpp
)

target_include_directories(minidb_storage
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(minidb_storage PUBLIC minidb_parser)

# ============================================================
# Execution library
# ============================================================
add_library(minidb_execution STATIC
    src/execution/executor.cpp
)

target_include_directories(minidb_execution
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(minidb_execution PUBLIC minidb_storage minidb_parser)

# ============================================================
# Network library
# ============================================================
add_library(minidb_network STATIC
    src/network/protocol.cpp
    src/network/server.cpp
    src/network/client.cpp
)

target_include_directories(minidb_network
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(minidb_network PUBLIC minidb_parser minidb_execution)

# Platform-specific networking libraries
if(WIN32)
    target_link_libraries(minidb_network PUBLIC ws2_32)
endif()

# ============================================================
# Server executable
# ============================================================
add_executable(minidb_server
    src/cli/server_main.cpp
)

target_link_libraries(minidb_server PRIVATE minidb_network minidb_execution minidb_storage minidb_parser)

# ============================================================
# Client executable
# ============================================================
add_executable(minidb_client
    src/cli/client_main.cpp
)

target_link_libraries(minidb_client PRIVATE minidb_network minidb_execution minidb_storage minidb_parser)

# ============================================================
# Tests
# ============================================================
enable_testing()

add_executable(test_lexer
    tests/test_lexer.cpp
)
target_link_libraries(test_lexer PRIVATE minidb_parser)
add_test(NAME lexer COMMAND test_lexer)

add_executable(test_parser
    tests/test_parser.cpp
)
target_link_libraries(test_parser PRIVATE minidb_parser)
add_test(NAME parser COMMAND test_parser)

add_executable(test_c_api
    tests/test_c_api.cpp
)
target_link_libraries(test_c_api PRIVATE minidb_parser)
add_test(NAME c_api COMMAND test_c_api)

# ============================================================
# Storage tests
# ============================================================
add_executable(test_storage
    tests/test_storage.cpp
)
target_link_libraries(test_storage PRIVATE minidb_storage minidb_parser)
add_test(NAME storage COMMAND test_storage)

# ============================================================
# Executor tests
# ============================================================
add_executable(test_executor
    tests/test_executor.cpp
)
target_link_libraries(test_executor PRIVATE minidb_execution minidb_storage minidb_parser)
add_test(NAME executor COMMAND test_executor)
