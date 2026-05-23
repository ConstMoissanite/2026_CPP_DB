// MiniDB Client — interactive REPL
// Usage: minidb_client [host] [port]
// Default: 127.0.0.1:3307

#include "../network/client.hpp"
#include "../core/string.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

using namespace minidb;

static const int MAX_INPUT = 8192;

// Read a line from stdin, trim trailing newline
static bool read_line(char* buf, int size) {
    if (!std::fgets(buf, size, stdin)) return false;
    std::size_t len = std::strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
        buf[--len] = '\0';
    }
    return true;
}

// Simple JSON pretty-print for display
static void display_response(const char* json) {
    const char* status_pos = std::strstr(json, "\"status\":\"");
    if (!status_pos) {
        std::printf("%s\n", json);
        return;
    }
    status_pos += 10;
    const char* status_end = std::strchr(status_pos, '"');
    if (!status_end) {
        std::printf("%s\n", json);
        return;
    }

    core::String status(status_pos, static_cast<std::size_t>(status_end - status_pos));

    if (status == core::String("error")) {
        const char* msg_pos = std::strstr(json, "\"message\":\"");
        if (msg_pos) {
            msg_pos += 11;
            const char* msg_end = std::strchr(msg_pos, '"');
            if (msg_end) {
                core::String msg(msg_pos, static_cast<std::size_t>(msg_end - msg_pos));
                std::printf("ERROR: %s\n", msg.c_str());
                return;
            }
        }
        std::printf("ERROR: (unknown)\n");
        return;
    }

    std::printf("OK\n");

    const char* kind_pos = std::strstr(json, "\"kind\":\"");
    if (kind_pos) {
        kind_pos += 8;
        const char* kind_end = std::strchr(kind_pos, '"');
        if (kind_end) {
            core::String kind(kind_pos, static_cast<std::size_t>(kind_end - kind_pos));
            std::printf("  Statement: %s\n", kind.c_str());
        }
    }

    const char* pos = json;
    while ((pos = std::strstr(pos, "\":\"")) != nullptr) {
        const char* key_start = pos;
        while (key_start > json && *(key_start - 1) != '"' && *(key_start - 1) != ',') {
            --key_start;
        }
        if (*key_start == ',' || *key_start == '{') ++key_start;
        const char* key_end = pos;
        core::String key(key_start, static_cast<std::size_t>(key_end - key_start));
        pos += 3;
        const char* val_end = std::strchr(pos, '"');
        if (val_end) {
            core::String val(pos, static_cast<std::size_t>(val_end - pos));
            if (!(key == core::String("status")) && !(key == core::String("kind"))) {
                std::printf("  %s: %s\n", key.c_str(), val.c_str());
            }
            pos = val_end + 1;
        } else {
            ++pos;
        }
    }
}

int main(int argc, char* argv[]) {
    const char* host = "127.0.0.1";
    int port = 3307;

    if (argc > 1) host = argv[1];
    if (argc > 2) {
        port = std::atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            std::fprintf(stderr, "Invalid port: %s\n", argv[2]);
            return 1;
        }
    }

    std::printf("MiniDB Client — connecting to %s:%d\n", host, port);
    std::printf("Type SQL statements or 'exit' to quit, 'help' for help.\n\n");

    char input[MAX_INPUT];
    bool connected = true;

    while (connected) {
        std::printf("minidb> ");
        if (!read_line(input, MAX_INPUT)) break;

        if (input[0] == '\0') continue;

        if (std::strcmp(input, "exit") == 0 || std::strcmp(input, "quit") == 0) {
            core::String resp = network::send_exit(host, port);
            if (!resp.empty()) {
                const char* msg_pos = std::strstr(resp.c_str(), "\"message\":\"");
                if (msg_pos) {
                    msg_pos += 11;
                    const char* msg_end = std::strchr(msg_pos, '"');
                    if (msg_end) {
                        core::String msg(msg_pos, static_cast<std::size_t>(msg_end - msg_pos));
                        std::printf("%s\n", msg.c_str());
                    }
                }
            }
            connected = false;
            break;
        }

        if (std::strcmp(input, "help") == 0) {
            std::printf("Commands:\n");
            std::printf("  <SQL>  — send SQL statement to server\n");
            std::printf("  exit   — quit\n");
            std::printf("  help   — show this message\n");
            std::printf("\nSupported SQL:\n");
            std::printf("  create database <name>\n");
            std::printf("  drop database <name>\n");
            std::printf("  use <name>\n");
            std::printf("  create table <name> (<col> <type> [primary], ...)\n");
            std::printf("  drop table <name>\n");
            std::printf("  select <col> from <table> [where <cond>]\n");
            std::printf("  delete from <table> [where <cond>]\n");
            std::printf("  insert into <table> values (<val>, ...)\n");
            std::printf("  update <table> set <col> = <val> [where <cond>]\n");
            continue;
        }

        core::String response = network::send_sql(host, port, input);
        if (response.empty()) {
            std::printf("ERROR: No response from server. Connection may be lost.\n");
            connected = false;
            break;
        }

        display_response(response.c_str());
    }

    std::printf("Goodbye.\n");
    return 0;
}
