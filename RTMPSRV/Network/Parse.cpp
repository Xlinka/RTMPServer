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

size_t Parse::parse_rtmp_packet(const char* data, std::size_t length, SOCKET client_socket) {
    if (length < 1) {
        std::cerr << "Packet too small to be an RTMP command." << std::endl;
        return 0;
    }

    size_t initial_length = length;
    size_t total_consumed = 0;

    // Loop to handle multiple chunks in the data
    while (total_consumed < length) {
        const char* current_data = data + total_consumed;
        size_t remaining_length = length - total_consumed;

        // Check if we have enough data for at least the basic header
        if (remaining_length < 1) {
            break; // Not enough data
        }

        // Extract fmt and csid from the basic header
        unsigned char fmt = (current_data[0] & 0xC0) >> 6;
        unsigned int csid = (current_data[0] & 0x3F);
        size_t header_index = 1;

        if (csid == 0) {
            if (remaining_length < 2) {
                break; // Not enough data
            }
            csid = 64 + (unsigned char)current_data[1];
            header_index++;
        } else if (csid == 1) {
            if (remaining_length < 3) {
                break; // Not enough data
            }
            csid = 64 + (unsigned char)current_data[1] + ((unsigned char)current_data[2]) * 256;
            header_index += 2;
        }

        // Determine the header size and fields based on fmt
        unsigned int timestamp = 0;
        unsigned int message_length = 0;
        unsigned char message_type_id = 0;
        unsigned int message_stream_id = 0;
        size_t header_size = header_index;

        if (fmt <= 2) {
            size_t required_header_size = header_index + (fmt == 0 ? 11 : (fmt == 1 ? 7 : 3));
            if (remaining_length < required_header_size) {
                break; // Not enough data
            }

            // Parse timestamp, message length, message type ID
            timestamp = ((unsigned char)current_data[header_index]) << 16 |
                        ((unsigned char)current_data[header_index + 1]) << 8 |
                        ((unsigned char)current_data[header_index + 2]);
            header_index += 3;

            if (fmt <= 1) {
                message_length = ((unsigned char)current_data[header_index]) << 16 |
                                 ((unsigned char)current_data[header_index + 1]) << 8 |
                                 ((unsigned char)current_data[header_index + 2]);
                message_type_id = (unsigned char)current_data[header_index + 3];
                header_index += 4;

                if (fmt == 0) {
                    message_stream_id = ((unsigned char)current_data[header_index]) |
                                        ((unsigned char)current_data[header_index + 1]) << 8 |
                                        ((unsigned char)current_data[header_index + 2]) << 16 |
                                        ((unsigned char)current_data[header_index + 3]) << 24;
                    header_index += 4;
                }
            }

            header_size = header_index;
        } else if (fmt == 3) {
            // No header, use cached values (not implemented in this example)
            header_size = header_index;
            std::cerr << "RTMP fmt: 3 (no header, continuation packet), skipping." << std::endl;
            // For a full implementation, you need to maintain previous chunk headers per chunk stream
            return 0;
        }

        // Handle extended timestamp
        if (timestamp == 0xFFFFFF) {
            if (remaining_length < header_size + 4) {
                break; // Not enough data
            }
            timestamp = ((unsigned char)current_data[header_size]) << 24 |
                        ((unsigned char)current_data[header_size + 1]) << 16 |
                        ((unsigned char)current_data[header_size + 2]) << 8 |
                        ((unsigned char)current_data[header_size + 3]);
            header_size += 4;
        }

        // At this point, header_size bytes have been consumed for the header
        size_t total_message_size = header_size + message_length;

        if (remaining_length < total_message_size) {
            break; // Not enough data to parse the full message
        }

        const char* message_body = current_data + header_size;

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

        total_consumed += total_message_size;
    }

    return total_consumed;
}
