#ifndef MINIDB_NETWORK_PROTOCOL_HPP
#define MINIDB_NETWORK_PROTOCOL_HPP

#include "../core/string.hpp"
#include "../parser/ast.hpp"
#include "../execution/executor.hpp"

namespace minidb {
namespace network {

// ============================================================
// JSON-based protocol for client/server communication.
//
// Request:  {"type":"sql","sql":"<sql string>"}
// Response: {"status":"ok","kind":"<stmt kind>","data":{...}}
//   or:     {"status":"error","message":"<error text>"}
// ============================================================

// Serialize an ExecResult into a JSON response string.
// This replaces serialize_response for the executor-based server.
core::String serialize_exec_result(const execution::ExecResult& result);

// Serialize a parsed SQLStatement into a JSON response string.
// (Kept for compatibility with parse-only mode.)
core::String serialize_response(const parser::SQLStatement& stmt);

// Serialize an error message into a JSON response string.
core::String serialize_error(const char* message);

// Serialize a raw status message (e.g. "ok", "goodbye").
core::String serialize_status(const char* status, const char* message);

// Extract the SQL string from a JSON request.
// Expects: {"type":"sql","sql":"..."}
// Returns the SQL string on success, or empty string on parse failure.
core::String parse_request_sql(const char* json);

// Check if a request is an "exit" command.
bool is_exit_request(const char* json);

// ============================================================
// JSON builder helpers (hand-rolled, no library)
// ============================================================

// Append a JSON key-value pair: "key":"value"
void json_append_kv(core::String& out, const char* key, const char* value);

// Append a JSON key-value pair with int value: "key":123
void json_append_kv_int(core::String& out, const char* key, int value);

// Append a JSON key-value pair with bool value: "key":true/false
void json_append_kv_bool(core::String& out, const char* key, bool value);

// Append a JSON key with an object value
void json_append_key_obj(core::String& out, const char* key);

// Append opening/closing braces
void json_append_open(core::String& out);
void json_append_close(core::String& out);

} // namespace network
} // namespace minidb

#endif // MINIDB_NETWORK_PROTOCOL_HPP
