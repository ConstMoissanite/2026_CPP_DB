#include "client.hpp"
#include <cstdio>
#include <cstring>

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

namespace minidb {
namespace network {

static const int MAX_MSG = 8192;

// Connect to server, return socket or INVALID_SOCKET on failure
static SOCKET connect_to_server(const char* host, int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<unsigned short>(port));

    // Resolve host
    struct hostent* he = gethostbyname(host);
    if (he) {
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], static_cast<std::size_t>(he->h_length));
    } else {
        addr.sin_addr.s_addr = inet_addr(host);
    }

    if (connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock;
}

// Send a line (message + newline)
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

// Receive a line from socket
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

core::String send_sql(const char* host, int port, const char* sql) {
    core::String result;

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    SOCKET sock = connect_to_server(host, port);
    if (sock == INVALID_SOCKET) {
        result = core::String("{\"status\":\"error\",\"message\":\"Failed to connect to server\"}");
#ifdef _WIN32
        WSACleanup();
#endif
        return result;
    }

    // Build request JSON
    char request[MAX_MSG];
    std::snprintf(request, sizeof(request), "{\"type\":\"sql\",\"sql\":\"%s\"}", sql);
    if (!send_line(sock, request)) {
        result = core::String("{\"status\":\"error\",\"message\":\"Failed to send request\"}");
    } else {
        result = recv_line(sock);
    }

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return result;
}

core::String send_exit(const char* host, int port) {
    core::String result;

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    SOCKET sock = connect_to_server(host, port);
    if (sock == INVALID_SOCKET) {
#ifdef _WIN32
        WSACleanup();
#endif
        return result;
    }

    send_line(sock, "{\"type\":\"exit\"}");
    result = recv_line(sock);

    closesocket(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return result;
}

} // namespace network
} // namespace minidb
