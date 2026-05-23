#include "server.hpp"
#include "protocol.hpp"
#include "../parser/parser.hpp"
#include "../execution/executor.hpp"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socklen_t = int;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define closesocket close
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace minidb {
namespace network {

// Maximum message size
static const int MAX_MSG = 8192;

// Read a newline-terminated message from a socket.
// Returns the message (without the newline).
static core::String recv_line(SOCKET sock) {
    char buf[MAX_MSG];
    int total = 0;
    while (total < MAX_MSG - 1) {
        char c;
        int n;
#ifdef _WIN32
        n = recv(sock, &c, 1, 0);
#else
        n = static_cast<int>(read(sock, &c, 1));
#endif
        if (n <= 0) break;
        if (c == '\n') break;
        if (c != '\r') {
            buf[total++] = c;
        }
    }
    buf[total] = '\0';
    return core::String(buf);
}

// Send a message followed by a newline.
static bool send_line(SOCKET sock, const char* msg) {
    std::size_t len = std::strlen(msg);
#ifdef _WIN32
    if (send(sock, msg, static_cast<int>(len), 0) == SOCKET_ERROR) return false;
    if (send(sock, "\n", 1, 0) == SOCKET_ERROR) return false;
#else
    if (write(sock, msg, len) < 0) return false;
    if (write(sock, "\n", 1) < 0) return false;
#endif
    return true;
}

int run_server(int port) {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }
#endif

    // Create socket
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        std::fprintf(stderr, "Failed to create socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR,
#ifdef _WIN32
               reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
               &opt, sizeof(opt));
#endif

    // Bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(listen_sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        std::fprintf(stderr, "Failed to bind to port %d\n", port);
        closesocket(listen_sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Listen
    if (listen(listen_sock, 5) == SOCKET_ERROR) {
        std::fprintf(stderr, "Failed to listen\n");
        closesocket(listen_sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::printf("MiniDB Server listening on port %d...\n", port);

    parser::Parser parser;
    execution::Executor executor;
    bool running = true;

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_sock = accept(listen_sock,
            reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);

        if (client_sock == INVALID_SOCKET) {
            continue;
        }

        // Handle one client connection
        bool client_connected = true;
        while (client_connected) {
            core::String request = recv_line(client_sock);
            if (request.empty()) {
                client_connected = false;
                break;
            }

            // Check for exit
            if (is_exit_request(request.c_str())) {
                core::String resp = serialize_status("ok", "goodbye");
                send_line(client_sock, resp.c_str());
                client_connected = false;
                running = false;
                break;
            }

            // Parse SQL from request
            core::String sql = parse_request_sql(request.c_str());
            core::String response;

            if (sql.empty()) {
                response = serialize_error("Invalid request: missing 'sql' field");
            } else {
                // Parse the SQL
                parser::SQLStatement stmt = parser.parse(sql.c_str());

                if (stmt.kind == parser::StmtKind::INVALID) {
                    response = serialize_error(stmt.error_msg.empty()
                        ? "Parse error" : stmt.error_msg.c_str());
                } else {
                    // Execute the SQL
                    execution::ExecResult result = executor.execute(stmt);
                    response = serialize_exec_result(result);
                }
            }

            if (!send_line(client_sock, response.c_str())) {
                client_connected = false;
            }
        }

        closesocket(client_sock);
    }

    executor.shutdown();
    closesocket(listen_sock);
#ifdef _WIN32
    WSACleanup();
#endif

    std::printf("MiniDB Server stopped.\n");
    return 0;
}

} // namespace network
} // namespace minidb
