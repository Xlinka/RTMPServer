#ifndef PARSECONTROL_H
#define PARSECONTROL_H

#include <cstddef>    // For std::size_t
#include <winsock2.h> // For SOCKET type

class ParseControl {
public:
    static void handle_set_chunk_size(const char* data, std::size_t length);
    static void handle_acknowledgement(const char* data, std::size_t length);
    static void handle_user_control_message(const char* data, std::size_t length);
    static void handle_window_ack_size(const char* data, std::size_t length);
    static void handle_set_peer_bandwidth(const char* data, std::size_t length);

    static void send_window_ack_size(SOCKET client_socket, unsigned int size);
    static void send_set_peer_bandwidth(SOCKET client_socket, unsigned int bandwidth, unsigned char limit_type);
};

#endif // PARSECONTROL_H
