#include "ParseAMF.h"
#include <iostream>
#include "ParseControl.h"
#include <vector>
#include "ParseUtils.h"
#include "Parse.h"

void ParseAMF::handle_amf_command(const char* data, std::size_t length, SOCKET client_socket) {
    std::cout << "[handle_amf_command] Received AMF command with length: " << length << " bytes." << std::endl;

    if (!data || length == 0) {
        std::cerr << "[handle_amf_command] Error: Invalid input data" << std::endl;
        return;
    }

    // Print full bytes of packet for debugging
    std::cout << "[handle_amf_command] Full packet content (hex): ";
    for (size_t i = 0; i < length; ++i) {
        printf("%02X ", (unsigned char)data[i]);
    }
    std::cout << std::endl;

    size_t index = 0;  // Start from the beginning of the message body

    bool found_command = false;
    std::string command_name;
    double transaction_id = 0.0;

    // Read the command name
    if (index + 1 > length) {
        std::cerr << "[handle_amf_command] Error: Not enough data to read marker." << std::endl;
        return;
    }

    uint8_t marker = (unsigned char)data[index];
    std::cout << "[handle_amf_command] Marker: " << std::hex << (unsigned int)marker << std::dec << " at index: " << index << std::endl;

    if (marker == 0x02) { // AMF0 string marker
        index++;

        if (index + 2 > length) {
            std::cerr << "[handle_amf_command] Error: Not enough data to read string length." << std::endl;
            return;
        }

        // Read string length (2 bytes big-endian)
        uint16_t str_len = (static_cast<unsigned char>(data[index]) << 8) | static_cast<unsigned char>(data[index + 1]);
        std::cout << "[handle_amf_command] Read string length: " << str_len << " at index: " << index << std::endl;
        index += 2;

        if (index + str_len > length) {
            std::cerr << "[handle_amf_command] Error: String length exceeds available data. Str_len: " << str_len
                      << " Remaining length: " << (length - index) << std::endl;
            return;
        }

        // Extract the string (command name)
        command_name = std::string(data + index, str_len);
        std::cout << "[handle_amf_command] Extracted command name: '" << command_name << "'" << std::endl;
        index += str_len;

        found_command = true;
    } else {
        std::cerr << "[handle_amf_command] Error: Expected AMF0 string marker (0x02), got: " << std::hex << (unsigned int)marker << std::dec << std::endl;
        return;
    }

    // Read the transaction ID
    if (index + 1 > length) {
        std::cerr << "[handle_amf_command] Error: Not enough data to read transaction ID marker." << std::endl;
        return;
    }

    marker = (unsigned char)data[index];
    if (marker == 0x00) { // AMF0 number marker
        index++;
        if (index + 8 > length) {
            std::cerr << "[handle_amf_command] Error: Not enough data to read transaction ID." << std::endl;
            return;
        }

        transaction_id = Parses::read_amf_number(data + index - 1);
        std::cout << "[handle_amf_command] Extracted transaction ID: " << transaction_id << std::endl;
        index += 8;
    } else {
        std::cerr << "[handle_amf_command] Error: Expected AMF0 number marker (0x00) for transaction ID, got: " << std::hex << (unsigned int)marker << std::dec << std::endl;
        return;
    }

    // Handle the extracted command
    if (command_name == "connect") {
        ParseControl::send_window_ack_size(client_socket, 5000000);
        ParseControl::send_set_peer_bandwidth(client_socket, 5000000, 2);
        send_connect_response(client_socket, transaction_id);
    }
    else if (command_name == "createStream") {
        send_create_stream_response(client_socket, transaction_id);
    }
    else if (command_name == "publish") {
        send_on_status_publish(client_socket, transaction_id);
    }
    else if (command_name == "play") {
        send_on_status_play(client_socket, transaction_id);
    }
    else if (command_name == "pause") {
        send_on_status_pause(client_socket, transaction_id);
    }
    else {
        std::cerr << "[handle_amf_command] Unknown command: " << command_name << std::endl;
    }
}



double ParseAMF::network_to_host_double(uint64_t net_double) {
    uint64_t host_double = ntohl((uint32_t)(net_double >> 32)) | ((uint64_t)ntohl((uint32_t)net_double) << 32);
    double result;
    memcpy(&result, &host_double, sizeof(double));
    return result;
}



bool ParseAMF::send_rtmp_message_safe(SOCKET client_socket, const std::vector<char>& message) {
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "[send_rtmp_message_safe] Invalid socket" << std::endl;
        return false;
    }
    
    try {
        size_t total_sent = 0;
        while (total_sent < message.size()) {
            int sent = send(client_socket, 
                          message.data() + total_sent, 
                          static_cast<int>(message.size() - total_sent), 
                          0);
            
            if (sent == SOCKET_ERROR) {
                std::cerr << "[send_rtmp_message_safe] Send failed with error: " 
                         << WSAGetLastError() << std::endl;
                return false;
            }
            
            total_sent += sent;
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[send_rtmp_message_safe] Exception while sending: " << e.what() << std::endl;
        return false;
    }
}

// Modified connect response function with additional safety
void ParseAMF::send_connect_response(SOCKET client_socket, double transaction_id) {
    try {
        std::cout << "[send_connect_response] Preparing response" << std::endl;
        
        std::vector<char> body;
        
        // Write command name
        Parses::write_amf_string("_result", body);
        
        // Write transaction ID
        Parses::write_amf_number(transaction_id, body);
        
        // Write properties
        body.push_back(0x03); // Object marker
        Parses::write_amf_string("fmsVer", body);
        Parses::write_amf_string("FMS/3,0,1,123", body);
        Parses::write_amf_string("capabilities", body);
        Parses::write_amf_number(31.0, body);
        body.push_back(0x00);
        body.push_back(0x00);
        body.push_back(0x09);
        
        // Write information object
        body.push_back(0x03); // Object marker
        Parses::write_amf_string("level", body);
        Parses::write_amf_string("status", body);
        Parses::write_amf_string("code", body);
        Parses::write_amf_string("NetConnection.Connect.Success", body);
        Parses::write_amf_string("description", body);
        Parses::write_amf_string("Connection succeeded.", body);
        body.push_back(0x00);
        body.push_back(0x00);
        body.push_back(0x09);
        
        // Build header
        std::vector<char> message = Parses::build_rtmp_header(0x03, 0, 0, body.size(), 0x14, 0);
        message.insert(message.end(), body.begin(), body.end());
        
        if (send_rtmp_message_safe(client_socket, message)) {
            std::cout << "[send_connect_response] Response sent successfully" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[send_connect_response] Exception: " << e.what() << std::endl;
    }
}
// Send a response for the 'createStream' command with logging
void ParseAMF::send_create_stream_response(SOCKET client_socket, double transaction_id) {
    std::cout << "[send_create_stream_response] Preparing '_result' response for 'createStream' command." << std::endl;
    
    // Prepare AMF-encoded response
    std::vector<char> body;
    Parses::write_amf_string("_result", body);
    Parses::write_amf_number(transaction_id, body);
    Parses::write_amf_number(1.0, body);  // Stream ID (we'll use 1 for simplicity)

    // Calculate message length and build RTMP header
    std::vector<char> header = Parses::build_rtmp_header(0x02, 2, 0, body.size(), 0x14, 0);
    header.insert(header.end(), body.begin(), body.end());

    // Send the response and log the result
    if (!Parses::send_rtmp_message(client_socket, header)) {
        std::cerr << "[send_create_stream_response] Failed to send 'createStream' response." << std::endl;
    } else {
        std::cout << "[send_create_stream_response] Successfully sent 'createStream' response." << std::endl;
    }
}

// Send 'onStatus' publish response with detailed logging
void ParseAMF::send_on_status_publish(SOCKET client_socket, double transaction_id) {
    std::cout << "[send_on_status_publish] Start preparing 'onStatus' publish response." << std::endl;
    
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };

    // Prepare the AMF-encoded response
    std::vector<char> body;
    Parses::write_amf_string("onStatus", body);
    Parses::write_amf_number(transaction_id, body);

    // Command object (AMF0 Object marker)
    body.push_back(0x03); 
    Parses::write_amf_string("level", body);
    Parses::write_amf_string("status", body);
    Parses::write_amf_string("code", body);
    Parses::write_amf_string("NetStream.Publish.Start", body);
    body.push_back(0x00);
    body.push_back(0x00);
    body.push_back(0x09);  // End of object

    // Calculate message length and build RTMP header
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body into one message
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send the response and log the result
    if (!Parses::send_rtmp_message(client_socket, response)) {
        std::cerr << "[send_on_status_publish] Failed to send 'onStatus' publish response." << std::endl;
    } else {
        std::cout << "[send_on_status_publish] Successfully sent 'onStatus' publish response." << std::endl;
        std::cout << "[send_on_status_publish] Sent " << response.size() << " bytes." << std::endl;
    }

    std::cout << "[send_on_status_publish] End of 'onStatus' publish response preparation and sending." << std::endl;
}
// Send 'onStatus' play response
void ParseAMF::send_on_status_play(SOCKET client_socket, double transaction_id) {
    std::cout << "[send_on_status_play] Start preparing 'onStatus' play response." << std::endl;
    
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };

    // Prepare the AMF-encoded response
    std::vector<char> body;
    Parses::write_amf_string("onStatus", body);
    Parses::write_amf_number(transaction_id, body);

    // Command object (AMF0 Object marker)
    body.push_back(0x03); 
    Parses::write_amf_string("level", body);
    Parses::write_amf_string("status", body);
    Parses::write_amf_string("code", body);
    Parses::write_amf_string("NetStream.Play.Start", body);
    body.push_back(0x00);
    body.push_back(0x00);
    body.push_back(0x09);  // End of object

    // Calculate message length and build RTMP header
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body into one message
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send the response and log the result
    if (!Parses::send_rtmp_message(client_socket, response)) {
        std::cerr << "[send_on_status_play] Failed to send 'onStatus' play response." << std::endl;
    } else {
        std::cout << "[send_on_status_play] Successfully sent 'onStatus' play response." << std::endl;
    }

    std::cout << "[send_on_status_play] End of 'onStatus' play response preparation and sending." << std::endl;
}

// Send 'onStatus' pause response
void ParseAMF::send_on_status_pause(SOCKET client_socket, double transaction_id) {
    std::cout << "[send_on_status_pause] Start preparing 'onStatus' pause response." << std::endl;
    
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };

    // Prepare the AMF-encoded response
    std::vector<char> body;
    Parses::write_amf_string("onStatus", body);
    Parses::write_amf_number(transaction_id, body);

    // Command object (AMF0 Object marker)
    body.push_back(0x03); 
    Parses::write_amf_string("level", body);
    Parses::write_amf_string("status", body);
    Parses::write_amf_string("code", body);
    Parses::write_amf_string("NetStream.Pause.Notify", body);
    body.push_back(0x00);
    body.push_back(0x00);
    body.push_back(0x09);  // End of object

    // Calculate message length and build RTMP header
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body into one message
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send the response and log the result
    if (!Parses::send_rtmp_message(client_socket, response)) {
        std::cerr << "[send_on_status_pause] Failed to send 'onStatus' pause response." << std::endl;
    } else {
        std::cout << "[send_on_status_pause] Successfully sent 'onStatus' pause response." << std::endl;
    }

    std::cout << "[send_on_status_pause] End of 'onStatus' pause response preparation and sending." << std::endl;
}
