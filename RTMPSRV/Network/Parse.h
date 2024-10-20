#ifndef PARSE_H
#define PARSE_H

#include <winsock2.h> // For SOCKET type
#include <cstddef>    // For std::size_t

class Parses {
public:
    static bool perform_handshake(SOCKET client_socket);
    static size_t parse_rtmp_packet(const char* data, std::size_t length, SOCKET client_socket);
};

#endif // PARSE_H
