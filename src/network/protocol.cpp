#include "protocol.hpp"
#include "../storage/column.hpp"
#include <cstdio>
#include <cstring>

namespace minidb {
namespace network {

// ============================================================
// JSON builder helpers
// ============================================================

static void append_char(core::String& out, char c) {
    char buf[2] = {c, '\0'};
    // Manual string concatenation since we don't have operator+
    core::String tmp(buf);
    // Build via manual construction from existing data
    std::size_t old_len = out.length();
    char* new_data = new char[old_len + 2];
    if (old_len > 0) {
        std::memcpy(new_data, out.c_str(), old_len);
    }
    new_data[old_len] = c;
    new_data[old_len + 1] = '\0';
    out = core::String(new_data);
    delete[] new_data;
}

static void append_str(core::String& out, const char* s) {
    std::size_t old_len = out.length();
    std::size_t add_len = std::strlen(s);
    char* new_data = new char[old_len + add_len + 1];
    if (old_len > 0) {
        std::memcpy(new_data, out.c_str(), old_len);
    }
    std::memcpy(new_data + old_len, s, add_len);
    new_data[old_len + add_len] = '\0';
    core::String tmp(new_data);
    out = std::move(tmp);
    delete[] new_data;
}

void json_append_open(core::String& out) {
    append_char(out, '{');
}

void json_append_close(core::String& out) {
    append_char(out, '}');
}

void json_append_kv(core::String& out, const char* key, const char* value) {
    if (out.length() > 0 && out.c_str()[out.length() - 1] != '{' && out.c_str()[out.length() - 1] != ',') {
        append_char(out, ',');
    }
    append_char(out, '"');
    append_str(out, key);
    append_str(out, "\":\"");
    append_str(out, value);
    append_char(out, '"');
}

void json_append_kv_int(core::String& out, const char* key, int value) {
    if (out.length() > 0 && out.c_str()[out.length() - 1] != '{' && out.c_str()[out.length() - 1] != ',') {
        append_char(out, ',');
    }
    append_char(out, '"');
    append_str(out, key);
    append_str(out, "\":");
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", value);
    append_str(out, buf);
}

void json_append_kv_bool(core::String& out, const char* key, bool value) {
    if (out.length() > 0 && out.c_str()[out.length() - 1] != '{' && out.c_str()[out.length() - 1] != ',') {
        append_char(out, ',');
    }
    append_char(out, '"');
    append_str(out, key);
    append_str(out, "\":");
    append_str(out, value ? "true" : "false");
}

// ============================================================
// Serialize WHERE clause as a JSON object
// ============================================================
static void serialize_where(core::String& out, const parser::WhereClause& where) {
    append_str(out, "\"where\":{");
    core::String tmp;
    json_append_kv(tmp, "column", where.column.c_str());
    append_str(out, tmp.c_str() + 1); // skip leading '{'
    core::String tmp2;
    json_append_kv(tmp2, "op", where.op.c_str());
    append_str(out, tmp2.c_str()); // adds comma before
    core::String tmp3;
    json_append_kv(tmp3, "value", where.const_value.c_str());
    append_str(out, tmp3.c_str());
    core::String tmp4;
    json_append_kv_bool(tmp4, "is_int", where.is_int_literal);
    append_str(out, tmp4.c_str());
    append_str(out, "}");
}

static void append_comma_if_needed(core::String& out) {
    if (out.length() > 0 && out.c_str()[out.length() - 1] != '{' && out.c_str()[out.length() - 1] != ',') {
        append_char(out, ',');
    }
}

// ============================================================
// Main serialize_response
// ============================================================
core::String serialize_response(const parser::SQLStatement& stmt) {
    core::String out;
    json_append_open(out);

    // Status
    if (stmt.kind == parser::StmtKind::INVALID) {
        return serialize_error(stmt.error_msg.c_str());
    }

    json_append_kv(out, "status", "ok");

    // Kind
    json_append_kv(out, "kind", parser::stmt_kind_name(stmt.kind));

    switch (stmt.kind) {
    case parser::StmtKind::CREATE_DATABASE:
    case parser::StmtKind::DROP_DATABASE:
    case parser::StmtKind::USE:
    {
        const char* dbname = nullptr;
        if (stmt.kind == parser::StmtKind::CREATE_DATABASE)
            dbname = stmt.create_database.database_name.c_str();
        else if (stmt.kind == parser::StmtKind::DROP_DATABASE)
            dbname = stmt.drop_database.database_name.c_str();
        else
            dbname = stmt.use_stmt.database_name.c_str();

        json_append_kv(out, "name", dbname);
        break;
    }

    case parser::StmtKind::CREATE_TABLE:
    {
        json_append_kv(out, "table", stmt.create_table.table_name.c_str());
        json_append_kv_int(out, "column_count", static_cast<int>(stmt.create_table.columns.size()));
        append_char(out, ',');
        append_str(out, "\"columns\":[");
        for (std::size_t i = 0; i < stmt.create_table.columns.size(); ++i) {
            if (i > 0) append_char(out, ',');
            append_char(out, '{');
            core::String col;
            json_append_kv(col, "name", stmt.create_table.columns[i].name.c_str());
            append_str(out, col.c_str() + 1);
            core::String col2;
            json_append_kv(col2, "type", stmt.create_table.columns[i].type.c_str());
            append_str(out, col2.c_str());
            core::String col3;
            json_append_kv_bool(col3, "primary", stmt.create_table.columns[i].is_primary);
            append_str(out, col3.c_str());
            append_char(out, '}');
        }
        append_str(out, "]");
        break;
    }

    case parser::StmtKind::DROP_TABLE:
    {
        json_append_kv(out, "table", stmt.drop_table.table_name.c_str());
        break;
    }

    case parser::StmtKind::SELECT:
    {
        json_append_kv(out, "column", stmt.select_stmt.column_name.c_str());
        json_append_kv(out, "table", stmt.select_stmt.table_name.c_str());
        json_append_kv_bool(out, "has_where", stmt.select_stmt.has_where);
        if (stmt.select_stmt.has_where) {
            append_char(out, ',');
            serialize_where(out, stmt.select_stmt.where);
        }
        break;
    }

    case parser::StmtKind::DELETE:
    {
        json_append_kv(out, "table", stmt.delete_stmt.table_name.c_str());
        json_append_kv_bool(out, "has_where", stmt.delete_stmt.has_where);
        if (stmt.delete_stmt.has_where) {
            append_char(out, ',');
            serialize_where(out, stmt.delete_stmt.where);
        }
        break;
    }

    case parser::StmtKind::INSERT:
    {
        json_append_kv(out, "table", stmt.insert_stmt.table_name.c_str());
        json_append_kv_int(out, "value_count", static_cast<int>(stmt.insert_stmt.values.size()));
        append_char(out, ',');
        append_str(out, "\"values\":[");
        for (std::size_t i = 0; i < stmt.insert_stmt.values.size(); ++i) {
            if (i > 0) append_char(out, ',');
            append_char(out, '{');
            core::String v;
            json_append_kv(v, "value", stmt.insert_stmt.values[i].c_str());
            append_str(out, v.c_str() + 1);
            core::String v2;
            json_append_kv_bool(v2, "is_int", stmt.insert_stmt.is_int[i]);
            append_str(out, v2.c_str());
            append_char(out, '}');
        }
        append_str(out, "]");
        break;
    }

    case parser::StmtKind::UPDATE:
    {
        json_append_kv(out, "table", stmt.update_stmt.table_name.c_str());
        json_append_kv(out, "set_column", stmt.update_stmt.set_column.c_str());
        json_append_kv(out, "set_value", stmt.update_stmt.set_value.c_str());
        json_append_kv_bool(out, "set_is_int", stmt.update_stmt.set_is_int);
        json_append_kv_bool(out, "has_where", stmt.update_stmt.has_where);
        if (stmt.update_stmt.has_where) {
            append_char(out, ',');
            serialize_where(out, stmt.update_stmt.where);
        }
        break;
    }

    default:
        break;
    }

    json_append_close(out);
    return out;
}

core::String serialize_error(const char* message) {
    core::String out;
    json_append_open(out);
    json_append_kv(out, "status", "error");
    json_append_kv(out, "message", message);
    json_append_close(out);
    return out;
}

core::String serialize_status(const char* status, const char* message) {
    core::String out;
    json_append_open(out);
    json_append_kv(out, "status", status);
    json_append_kv(out, "message", message);
    json_append_close(out);
    return out;
}

// ============================================================
// JSON request parsing (simple substring search)
// ============================================================

// Find the value of a JSON string key. Returns empty string if not found.
// Handles: "key":"value"  (but not nested objects or escaped quotes)
static core::String json_get_string(const char* json, const char* key) {
    // Build search pattern: "key":"
    char pattern[256];
    std::snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
    const char* pos = std::strstr(json, pattern);
    if (!pos) return core::String();

    pos += std::strlen(pattern);
    const char* end = std::strchr(pos, '"');
    if (!end) return core::String();

    return core::String(pos, static_cast<std::size_t>(end - pos));
}

core::String parse_request_sql(const char* json) {
    // Check for {"type":"sql","sql":"..."}
    core::String type = json_get_string(json, "type");
    if (type.empty() || !(type == core::String("sql"))) {
        return core::String();
    }
    return json_get_string(json, "sql");
}

bool is_exit_request(const char* json) {
    core::String type = json_get_string(json, "type");
    return type == core::String("exit");
}

// ============================================================
// Serialize ExecResult (executor output)
// ============================================================
core::String serialize_exec_result(const execution::ExecResult& result) {
    if (result.status == execution::ExecResult::ERROR) {
        return serialize_error(result.message.c_str());
    }

    core::String out;
    json_append_open(out);
    json_append_kv(out, "status", "ok");
    json_append_kv(out, "message", result.message.c_str());

    // If there's a result table (SELECT), include it
    if (result.result_table && result.result_table->column_count() > 0) {
        append_char(out, ',');
        json_append_kv_int(out, "column_count",
            static_cast<int>(result.result_table->column_count()));
        json_append_kv_int(out, "row_count",
            static_cast<int>(result.result_table->row_count()));

        // Columns
        append_char(out, ',');
        append_str(out, "\"columns\":[");
        for (std::size_t i = 0; i < result.result_table->column_count(); ++i) {
            if (i > 0) append_char(out, ',');
            core::String col;
            json_append_kv(col, "name", result.result_table->columns()[i].name().c_str());
            // Remove leading '{'
            const char* col_cstr = col.c_str();
            append_str(out, col_cstr + 1);
            core::String ct;
            json_append_kv(ct, "type",
                result.result_table->columns()[i].type() == storage::ColumnType::INT
                ? "int" : "string");
            append_str(out, ct.c_str());
        }
        append_str(out, "]");

        // Rows
        append_char(out, ',');
        append_str(out, "\"rows\":[");
        for (std::size_t i = 0; i < result.result_table->row_count(); ++i) {
            if (i > 0) append_char(out, ',');
            append_char(out, '[');
            const storage::Row& row = result.result_table->row_at(i);
            for (std::size_t j = 0; j < row.count(); ++j) {
                if (j > 0) append_char(out, ',');
                const storage::Value& v = row.get_value(j);
                if (v.is_int()) {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "%d", v.int_value());
                    append_str(out, buf);
                } else if (v.is_string()) {
                    append_char(out, '"');
                    append_str(out, v.string_value().c_str());
                    append_char(out, '"');
                } else {
                    append_str(out, "null");
                }
            }
            append_char(out, ']');
        }
        append_str(out, "]");
    }

    json_append_close(out);
    return out;
}

} // namespace network
} // namespace minidb
