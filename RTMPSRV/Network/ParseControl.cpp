#include "ParseControl.h"
#include <iostream>

void ParseControl::handle_set_chunk_size(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Set Chunk Size message too short." << std::endl;
        return;
    }
    unsigned int chunk_size = ((unsigned char)data[0] << 24) |
                              ((unsigned char)data[1] << 16) |
                              ((unsigned char)data[2] << 8) |
                              (unsigned char)data[3];
    std::cout << "New chunk size set to: " << chunk_size << std::endl;
}

void ParseControl::handle_window_ack_size(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Window Acknowledgement Size message too short." << std::endl;
        return;
    }
    unsigned int window_size = ((unsigned char)data[0] << 24) |
                               ((unsigned char)data[1] << 16) |
                               ((unsigned char)data[2] << 8) |
                               (unsigned char)data[3];
    std::cout << "Window Acknowledgement Size: " << window_size << std::endl;
}

void ParseControl::handle_acknowledgement(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Acknowledgment message too short." << std::endl;
        return;
    }
    unsigned int ack_value = ((unsigned char)data[0] << 24) |
                             ((unsigned char)data[1] << 16) |
                             ((unsigned char)data[2] << 8) |
                             (unsigned char)data[3];
    std::cout << "Acknowledgment received: " << ack_value << std::endl;
}

void ParseControl::handle_set_peer_bandwidth(const char* data, std::size_t length) {
    if (length < 5) {
        std::cerr << "Set Peer Bandwidth message too short." << std::endl;
        return;
    }
    unsigned int bandwidth = ((unsigned char)data[0] << 24) |
                             ((unsigned char)data[1] << 16) |
                             ((unsigned char)data[2] << 8) |
                             (unsigned char)data[3];
    unsigned char limit_type = (unsigned char)data[4];
    std::cout << "Set Peer Bandwidth: " << bandwidth << ", Limit Type: " << (int)limit_type << std::endl;
}

void ParseControl::handle_user_control_message(const char* data, std::size_t length) {
    if (length < 2) {
        std::cerr << "User Control Message too short." << std::endl;
        return;
    }
    unsigned short event_type = ((unsigned char)data[0] << 8) | (unsigned char)data[1];
    std::cout << "User Control Message Event Type: " << event_type << std::endl;
    // Handle different event types as needed
}

void ParseControl::send_window_ack_size(SOCKET client_socket, unsigned int size) {
    char message[12 + 4];
    message[0] = 0x02;  // Chunk basic header: fmt=0, csid=2
    message[1] = 0x00;
    message[2] = 0x00;
    message[3] = 0x00;
    message[4] = 0x00;
    message[5] = 0x00;
    message[6] = 0x04;
    message[7] = 0x05;  // Message Type ID: Window Acknowledgement Size
    message[8] = 0x00;
    message[9] = 0x00;
    message[10] = 0x00;
    message[11] = 0x00;

    // Window size (4 bytes)
    message[12] = (size >> 24) & 0xFF;
    message[13] = (size >> 16) & 0xFF;
    message[14] = (size >> 8) & 0xFF;
    message[15] = size & 0xFF;

    int result = send(client_socket, message, sizeof(message), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send Window Acknowledgement Size. Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent Window Acknowledgement Size to client." << std::endl;
    }
}

void ParseControl::send_set_peer_bandwidth(SOCKET client_socket, unsigned int bandwidth, unsigned char limit_type) {
    char message[12 + 5];
    message[0] = 0x02;  // Chunk basic header: fmt=0, csid=2
    message[1] = 0x00;
    message[2] = 0x00;
    message[3] = 0x00;
    message[4] = 0x00;
    message[5] = 0x00;
    message[6] = 0x05;
    message[7] = 0x06;  // Message Type ID: Set Peer Bandwidth
    message[8] = 0x00;
    message[9] = 0x00;
    message[10] = 0x00;
    message[11] = 0x00;

    // Bandwidth (4 bytes)
    message[12] = (bandwidth >> 24) & 0xFF;
    message[13] = (bandwidth >> 16) & 0xFF;
    message[14] = (bandwidth >> 8) & 0xFF;
    message[15] = bandwidth & 0xFF;

    // Limit type (1 byte)
    message[16] = limit_type;

    int result = send(client_socket, message, sizeof(message), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send Set Peer Bandwidth. Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent Set Peer Bandwidth to client." << std::endl;
    }
}
