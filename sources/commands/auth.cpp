// #include "commands.hpp"
// #include "logger.hpp"
// #include "server.hpp"

// void    Commands::
// void    Commands::handle_capacities(const int& client_socket, const IRCMessage& msg)
// {
// 				if (msg.params.empty()) {
// 								send_to_client(client_socket, "CAP * LS :multi-prefix");
// 								Logger::info("CAP negotiation in progress", client_socket);
// 								return;
// 				}

// 				if (msg.params[0] == "LS") {
// 								send_to_client(client_socket, "CAP * LS :multi-prefix\r\n");
// 				}
// 				else if (msg.params[0] == "END") {
// 								Logger::info("CAP negotiation ended", client_socket);
// 								return;
// 				}
// 				else if (msg.params[0] == "REQ") {
// 								send_to_client(client_socket, "CAP * ACK :");
// 				}

// 				Logger::info("CAP negotiation in progress", client_socket);
// }
