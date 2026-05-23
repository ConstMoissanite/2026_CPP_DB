// MiniDB Server — main entry point
// Usage: minidb_server [port]
// Default port: 3307

#include "../network/server.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[]) {
    int port = 3307;
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }

    return minidb::network::run_server(port);
}
