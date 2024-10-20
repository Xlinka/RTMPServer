#include "ParseControl.h"
#include <iostream>
#include "Buffer.h"
#include "Parse.h"
#include "ParseUtils.h"
// Default chunk size is 128 bytes for RTMP as per the spec
unsigned int current_chunk_size = 128;

// Function to handle the 'Set Chunk Size' control message
void ParseControl::handle_set_chunk_size(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Set Chunk Size message is too short." << std::endl;
        return;
    }

    // Extract the new chunk size from the message (big-endian)
    unsigned int new_chunk_size = ((unsigned char)data[0] << 24) |
                                  ((unsigned char)data[1] << 16) |
                                  ((unsigned char)data[2] << 8) |
                                  (unsigned char)data[3];

    // Update the global chunk size
    std::cout << "Setting new chunk size: " << new_chunk_size << std::endl;
    current_chunk_size = new_chunk_size;
}

// Function to handle 'Window Acknowledgement Size' message
void ParseControl::handle_window_ack_size(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Window Acknowledgement Size message too short." << std::endl;
        return;
    }

    // Extract window size (big-endian)
    unsigned int window_size = ((unsigned char)data[0] << 24) |
                               ((unsigned char)data[1] << 16) |
                               ((unsigned char)data[2] << 8) |
                               (unsigned char)data[3];

    std::cout << "Window Acknowledgement Size set to: " << window_size << std::endl;
}

// Function to handle 'Acknowledgement' message
void ParseControl::handle_acknowledgement(const char* data, std::size_t length) {
    if (length < 4) {
        std::cerr << "Acknowledgement message too short." << std::endl;
        return;
    }

    // Extract the acknowledgment value (big-endian)
    unsigned int ack_value = ((unsigned char)data[0] << 24) |
                             ((unsigned char)data[1] << 16) |
                             ((unsigned char)data[2] << 8) |
                             (unsigned char)data[3];

    std::cout << "Acknowledgement received for: " << ack_value << std::endl;
}

// Function to handle 'Set Peer Bandwidth' message
void ParseControl::handle_set_peer_bandwidth(const char* data, std::size_t length) {
    if (length < 5) {
        std::cerr << "Set Peer Bandwidth message too short." << std::endl;
        return;
    }

    // Extract the peer bandwidth and limit type (big-endian)
    unsigned int bandwidth = ((unsigned char)data[0] << 24) |
                             ((unsigned char)data[1] << 16) |
                             ((unsigned char)data[2] << 8) |
                             (unsigned char)data[3];
    unsigned char limit_type = (unsigned char)data[4];

    std::cout << "Peer Bandwidth set to: " << bandwidth 
              << ", Limit Type: " << (int)limit_type << std::endl;
}

// Function to handle 'User Control Message'
void ParseControl::handle_user_control_message(const char* data, std::size_t length) {
    if (length < 2) {
        std::cerr << "User Control Message too short." << std::endl;
        return;
    }

    // Extract event type (big-endian)
    unsigned short event_type = ((unsigned char)data[0] << 8) | (unsigned char)data[1];
    std::cout << "User Control Message Event Type: " << event_type << std::endl;

    switch (event_type) {
        case 0x00:
            std::cout << "Stream Begin event received." << std::endl;
            break;
        case 0x01:
            std::cout << "Stream EOF event received." << std::endl;
            break;
        case 0x02:
            std::cout << "Stream Dry event received." << std::endl;
            break;
        // Add other cases for different events if needed
        default:
            std::cout << "Unknown User Control Message event." << std::endl;
            break;
    }
}

// Function to send 'Window Acknowledgement Size' message to the client
void ParseControl::send_window_ack_size(SOCKET client_socket, unsigned int size) {
    std::vector<char> message(16);  // 12-byte header + 4-byte window size

    // Prepare the RTMP header
    message[0] = 0x02;  // fmt=0, csid=2
    message[1] = 0x00;  // Timestamp (3 bytes)
    message[2] = 0x00;
    message[3] = 0x00;
    message[4] = 0x00;  // Message length (4 bytes)
    message[5] = 0x04;
    message[6] = 0x05;  // Message Type ID: Window Acknowledgement Size
    message[7] = 0x00;  // Message Stream ID (always 0 for control)
    message[8] = 0x00;
    message[9] = 0x00;
    message[10] = 0x00;
    message[11] = 0x00;

    // Window size (4 bytes, big-endian)
    message[12] = (size >> 24) & 0xFF;
    message[13] = (size >> 16) & 0xFF;
    message[14] = (size >> 8) & 0xFF;
    message[15] = size & 0xFF;

    // Send the message using the send utility function with retries
    if (!Parses::send_rtmp_message(client_socket, message)) {
        std::cerr << "Failed to send Window Acknowledgement Size." << std::endl;
    } else {
        std::cout << "Sent Window Acknowledgement Size to client." << std::endl;
    }
}

// Function to send 'Set Peer Bandwidth' message to the client
void ParseControl::send_set_peer_bandwidth(SOCKET client_socket, unsigned int bandwidth, unsigned char limit_type) {
    std::vector<char> message(17);  // 12-byte header + 4-byte bandwidth + 1-byte limit type

    // Prepare the RTMP header
    message[0] = 0x02;  // fmt=0, csid=2
    message[1] = 0x00;  // Timestamp (3 bytes)
    message[2] = 0x00;
    message[3] = 0x00;
    message[4] = 0x00;  // Message length (5 bytes)
    message[5] = 0x05;
    message[6] = 0x06;  // Message Type ID: Set Peer Bandwidth
    message[7] = 0x00;  // Message Stream ID (always 0 for control)
    message[8] = 0x00;
    message[9] = 0x00;
    message[10] = 0x00;
    message[11] = 0x00;

    // Bandwidth (4 bytes, big-endian)
    message[12] = (bandwidth >> 24) & 0xFF;
    message[13] = (bandwidth >> 16) & 0xFF;
    message[14] = (bandwidth >> 8) & 0xFF;
    message[15] = bandwidth & 0xFF;

    // Limit type (1 byte)
    message[16] = limit_type;

    // Send the message using the send utility function with retries
    if (!Parses::send_rtmp_message(client_socket, message)) {
        std::cerr << "Failed to send Set Peer Bandwidth." << std::endl;
    } else {
        std::cout << "Sent Set Peer Bandwidth to client." << std::endl;
    }
}
