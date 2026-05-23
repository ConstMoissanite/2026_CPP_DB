#ifndef MINIDB_NETWORK_CLIENT_HPP
#define MINIDB_NETWORK_CLIENT_HPP

#include "../core/string.hpp"

namespace minidb {
namespace network {

// Connect to a MiniDB server and send a single SQL command.
// Returns the server's JSON response.
core::String send_sql(const char* host, int port, const char* sql);

// Send an exit command to the server.
core::String send_exit(const char* host, int port);

} // namespace network
} // namespace minidb

#endif // MINIDB_NETWORK_CLIENT_HPP
