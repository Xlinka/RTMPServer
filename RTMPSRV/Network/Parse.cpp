#include "Parse.h"
#include "ParseControl.h"
#include "ParseAMF.h"
#include "ParseUtils.h"
#include <iostream>
#include <cstdlib>
#include <cstring> // for memcpy
#include <ctime>   // for time

// Function to perform the RTMP handshake with the client
bool Parse::perform_handshake(SOCKET client_socket) {
    char c0c1[1537]; // Buffer for C0 and C1
    char s0s1s2[3073]; // Buffer for S0, S1, S2
    char c2[1536]; // Buffer for C2

    // Receive C0 and C1
    int bytes_received = recv(client_socket, c0c1, 1537, 0);
    if (bytes_received != 1537) {
        std::cerr << "Failed to receive C0 and C1" << std::endl;
        return false;
    }

    // Prepare S0 (version byte)
    s0s1s2[0] = 0x03; // RTMP version 3

    // Prepare S1 and S2
    srand(static_cast<unsigned int>(time(NULL))); // Seed random number generator
    for (int i = 1; i <= 1536; ++i) {
        s0s1s2[i] = rand() % 256; // Fill with random data
    }
    memcpy(s0s1s2 + 1537, c0c1 + 1, 1536); // Copy C1 to S1 and S2

    // Send S0, S1, S2
    int bytes_sent = send(client_socket, s0s1s2, 3073, 0);
    if (bytes_sent != 3073) {
        std::cerr << "Failed to send S0, S1, S2" << std::endl;
        return false;
    }

    // Receive C2
    bytes_received = recv(client_socket, c2, 1536, 0);
    if (bytes_received != 1536) {
        std::cerr << "Failed to receive C2" << std::endl;
        return false;
    }

    std::cout << "RTMP handshake completed successfully." << std::endl;
    return true;
}

// Function to parse RTMP packet data
size_t Parse::parse_rtmp_packet(const char* data, std::size_t length, SOCKET client_socket) {
    if (length < 1) {
        std::cerr << "Packet too small to be an RTMP command." << std::endl;
        return 0;
    }
    size_t initial_length = length;

    // Debugging: Print the entire RTMP packet in hex
    Parses::dump_hex(data, length);

    // Extract the fmt and csid fields from the first byte
    unsigned char fmt = (data[0] & 0xC0) >> 6;
    unsigned int csid = (data[0] & 0x3F);

    int header_index = 1;
    if (csid == 0) {
        csid = 64 + (unsigned char)data[1];
        header_index++;
    } else if (csid == 1) {
        csid = 64 + (unsigned char)data[1] + ((unsigned char)data[2]) * 256;
        header_index += 2;
    }

    int header_size = 0;
    unsigned int timestamp = 0;
    unsigned int message_length = 0;
    unsigned char message_type_id = 0;
    unsigned int message_stream_id = 0;

    // Determine the header size and extract necessary fields
    if (fmt == 0) {
        header_size = header_index + 11;
        timestamp = ((unsigned char)data[header_index]) << 16 |
                    ((unsigned char)data[header_index + 1]) << 8 |
                    ((unsigned char)data[header_index + 2]);
        message_length = ((unsigned char)data[header_index + 3]) << 16 |
                         ((unsigned char)data[header_index + 4]) << 8 |
                         ((unsigned char)data[header_index + 5]);
        message_type_id = (unsigned char)data[header_index + 6];
        message_stream_id = ((unsigned char)data[header_index + 7]) |
                            ((unsigned char)data[header_index + 8]) << 8 |
                            ((unsigned char)data[header_index + 9]) << 16 |
                            ((unsigned char)data[header_index + 10]) << 24;

    } else if (fmt == 1) {
        header_size = header_index + 7;
        timestamp = ((unsigned char)data[header_index]) << 16 |
                    ((unsigned char)data[header_index + 1]) << 8 |
                    ((unsigned char)data[header_index + 2]);
        message_length = ((unsigned char)data[header_index + 3]) << 16 |
                         ((unsigned char)data[header_index + 4]) << 8 |
                         ((unsigned char)data[header_index + 5]);
        message_type_id = (unsigned char)data[header_index + 6];

    } else if (fmt == 2) {
        header_size = header_index + 3;
        timestamp = ((unsigned char)data[header_index]) << 16 |
                    ((unsigned char)data[header_index + 1]) << 8 |
                    ((unsigned char)data[header_index + 2]);

    } else if (fmt == 3) {
        header_size = header_index;
        std::cerr << "RTMP fmt: 3 (no header, continuation packet), skipping." << std::endl;
        return 0;
    }

    std::cout << "Timestamp: " << timestamp << ", Message length: " << message_length << std::endl;
    std::cout << "RTMP message type: " << std::hex << (int)message_type_id << std::dec << std::endl;
    std::cout << "Message Stream ID: " << message_stream_id << std::endl;

    if (length < header_size + message_length) {
        std::cerr << "Not enough data for the RTMP message body." << std::endl;
        return 0;
    }

    const char* message_body = data + header_size;

    // Correctly call methods from ParseControl and ParseAMF
    switch (message_type_id) {
        case 0x01:
            ParseControl::handle_set_chunk_size(message_body, message_length);
            break;
        case 0x03:
            ParseControl::handle_acknowledgement(message_body, message_length);
            break;
        case 0x04:
            ParseControl::handle_user_control_message(message_body, message_length);
            break;
        case 0x05:
            ParseControl::handle_window_ack_size(message_body, message_length);
            break;
        case 0x06:
            ParseControl::handle_set_peer_bandwidth(message_body, message_length);
            break;
        case 0x14:
            ParseAMF::handle_amf_command(message_body, message_length, client_socket);
            break;
        default:
            std::cerr << "Unknown RTMP message type: " << (int)message_type_id << ", skipping." << std::endl;
            break;
    }

    return initial_length;
}
