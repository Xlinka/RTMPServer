#include "Client.h"
#include "Parse.h"  // Updated to match your RTMP parsing
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>

bool RTMPServer::start(int port) {
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket, error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Bind to address
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed, error: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return false;
    }

    // Start listening
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed, error: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return false;
    }

    std::cout << "RTMP server started on port " << port << std::endl;
    return true;
}

void RTMPServer::run() {
    running_ = true;

    while (running_) {
        sockaddr_in client_address;
        int client_len = sizeof(client_address);

        // Accept incoming connections
        SOCKET client_socket = accept(server_fd, (sockaddr*)&client_address, &client_len);
        if (client_socket == INVALID_SOCKET) {
            if (!running_) break;  // Exit if the server has been stopped
            std::cerr << "Failed to accept connection, error: " << WSAGetLastError() << std::endl;
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "New client connected" << std::endl;
        }

        // Handle the client in a separate thread
        client_threads.emplace_back([this, client_socket]() {
            handle_client(client_socket);
        });
    }

    // Join all threads when shutting down
    for (std::thread& t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void RTMPServer::handle_client(SOCKET client_socket) {
    // Perform RTMP handshake
    if (!Parses::perform_handshake(client_socket)) {
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cerr << "Handshake failed" << std::endl;
        }
        closesocket(client_socket);
        return;
    }

    std::vector<char> recv_buffer;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Continue processing RTMP packets after handshake
    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "Received " << read_size << " bytes from client" << std::endl;
        }

        // Append new data to the receive buffer
        recv_buffer.insert(recv_buffer.end(), buffer, buffer + read_size);

        // Parse as many RTMP packets as possible
        size_t parsed_bytes = 0;
        while (parsed_bytes < recv_buffer.size()) {
            size_t bytes_consumed = Parses::parse_rtmp_packet(recv_buffer.data() + parsed_bytes, recv_buffer.size() - parsed_bytes, client_socket);
            if (bytes_consumed == 0) {
                break;  // Not enough data to parse a complete packet
            }
            parsed_bytes += bytes_consumed;
        }

        // Remove parsed data from the buffer
        if (parsed_bytes > 0) {
            recv_buffer.erase(recv_buffer.begin(), recv_buffer.begin() + parsed_bytes);
        }
    }

    if (read_size == 0) {
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "Client disconnected" << std::endl;
        }
    } else {
        {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cerr << "Failed to read data from client, error: " << WSAGetLastError() << std::endl;
        }
    }

    // Close client socket
    closesocket(client_socket);
}

void RTMPServer::stop() {
    running_ = false;
    closesocket(server_fd);  // Close server socket to stop accepting new connections
}

RTMPServer::~RTMPServer() {
    if (server_fd != INVALID_SOCKET) {
        closesocket(server_fd);
    }
    WSACleanup();
}
