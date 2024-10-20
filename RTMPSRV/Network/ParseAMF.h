#ifndef PARSEAMF_H
#define PARSEAMF_H

#include <winsock2.h> // For SOCKET type
#include <cstddef>    // For std::size_t

class ParseAMF {
public:
    static void handle_amf_command(const char* data, std::size_t length, SOCKET client_socket);
    static void send_connect_response(SOCKET client_socket, double transaction_id);
    static void send_create_stream_response(SOCKET client_socket, double transaction_id);
    static void send_on_status_publish(SOCKET client_socket, double transaction_id);
};

#endif // PARSEAMF_H
