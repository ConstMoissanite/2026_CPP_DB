#ifndef MINIDB_NETWORK_SERVER_HPP
#define MINIDB_NETWORK_SERVER_HPP

namespace minidb {
namespace network {

// Start the MiniDB TCP server.
// Listens on the given port, accepts connections, parses SQL, returns JSON.
// Runs until an exit command is received or interrupted.
// Returns 0 on success, non-zero on failure.
int run_server(int port = 3307);

} // namespace network
} // namespace minidb

#endif // MINIDB_NETWORK_SERVER_HPP
