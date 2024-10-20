#include "ParseAMF.h"
#include <iostream>
#include "ParseControl.h"
#include <vector>
#include "ParseUtils.h"


void ParseAMF::handle_amf_command(const char* data, std::size_t length, SOCKET client_socket) {
    if (length < 1) {
        std::cerr << "AMF command too short." << std::endl;
        return;
    }
    unsigned int index = 0;
    if ((unsigned char)data[index] != 0x02) {
        std::cerr << "Expected AMF0 String at the start of the command." << std::endl;
        return;
    }
    index++;
    unsigned short string_length = ((unsigned char)data[index] << 8) | (unsigned char)data[index + 1];
    index += 2;
    std::string command_name(data + index, string_length);
    index += string_length;
    std::cout << "AMF command: " << command_name << std::endl;
    if (command_name == "connect") {
        ParseControl::send_window_ack_size(client_socket, 5000000);
        ParseControl::send_set_peer_bandwidth(client_socket, 5000000, 2);
        ParseAMF::send_connect_response(client_socket, 1);
    }
    else if (command_name == "createStream") {
        ParseAMF::send_create_stream_response(client_socket, 1);
    }
    else if (command_name == "publish") {
        ParseAMF::send_on_status_publish(client_socket, 1);
    }
}

void ParseAMF::send_connect_response(SOCKET client_socket, double transaction_id) {
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };
    // AMF-encoded response
    std::vector<char> body;
    // Command name '_result'
    Parse::write_amf_string("_result", body);
    // Transaction ID
    Parse::write_amf_number(transaction_id, body);
    // Command object (an empty object)
    body.push_back(0x03); // AMF0 Object marker
    Parse::write_amf_string("fmsVer", body);
    Parse::write_amf_string("FMS/3,0,1,123", body);
    Parse::write_amf_string("capabilities", body);
    Parse::write_amf_number(31.0, body);
    body.push_back(0x00);
    body.push_back(0x00);
    body.push_back(0x09); // End of object

    // Calculate message length
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send response to the client
    int result = send(client_socket, response.data(), response.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send connect response. Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent 'connect' response to client." << std::endl;
    }
}

void ParseAMF::send_create_stream_response(SOCKET client_socket, double transaction_id) {
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };
    // AMF-encoded response
    std::vector<char> body;
    // Command name '_result'
    Parse::write_amf_string("_result", body);
    // Transaction ID
    Parse::write_amf_number(transaction_id, body);
    // Stream ID (we'll use 1 for simplicity)
    Parse::write_amf_number(1.0, body);

    // Calculate message length
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send response to the client
    int result = send(client_socket, response.data(), response.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send createStream response. Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent 'createStream' response to client." << std::endl;
    }
}

void ParseAMF::send_on_status_publish(SOCKET client_socket, double transaction_id) {
    // Prepare RTMP header
    char header[12] = {
        0x02,               // fmt=0, csid=2
        0x00, 0x00, 0x00,   // Timestamp (0)
        0x00, 0x00, 0x00,   // Message length (to be filled later)
        0x14,               // Message Type ID: 0x14 (Command Message AMF0)
        0x00, 0x00, 0x00, 0x00  // Message Stream ID (usually 0 for control messages)
    };
    // AMF-encoded response
    std::vector<char> body;
    // Command name 'onStatus'
    Parse::write_amf_string("onStatus", body);
    // Transaction ID
    Parse::write_amf_number(transaction_id, body);
    // Command object
    body.push_back(0x03); // AMF0 Object marker
    Parse::write_amf_string("level", body);
    Parse::write_amf_string("status", body);
    Parse::write_amf_string("code", body);
    Parse::write_amf_string("NetStream.Publish.Start", body);
    body.push_back(0x00);
    body.push_back(0x00);
    body.push_back(0x09); // End of object

    // Calculate message length
    unsigned int message_length = body.size();
    header[3] = (message_length >> 16) & 0xFF;
    header[4] = (message_length >> 8) & 0xFF;
    header[5] = message_length & 0xFF;

    // Combine header and body
    std::vector<char> response(header, header + sizeof(header));
    response.insert(response.end(), body.begin(), body.end());

    // Send response to the client
    int result = send(client_socket, response.data(), response.size(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Failed to send onStatus publish response. Error: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Sent 'onStatus' publish response to client." << std::endl;
    }
}

