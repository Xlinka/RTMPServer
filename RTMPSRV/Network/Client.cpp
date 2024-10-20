// Client.cpp
#include "Client.h"
#include "Parse.h"      // For RTMP parsing and handshake
#include "ParseAMF.h"   // For handling AMF commands (connect, createStream, publish)
#include "Buffer.h"     // For managing the data buffer
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

// Utility function to get current timestamp for logging
std::string current_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    localtime_s(&local_time, &now_c);  // Thread-safe localtime function

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool RTMPServer::is_running() const {
    return running_;
}

bool RTMPServer::start(int port) {
    std::cout << "[" << current_timestamp() << "] [start] Attempting to start RTMP server on port " << port << "..." << std::endl;

    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "[" << current_timestamp() << "] [start] WSAStartup failed with error: " << result << std::endl;
        return false;
    }

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[" << current_timestamp() << "] [start] Failed to create socket. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    // Bind to address
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "[" << current_timestamp() << "] [start] Bind failed. Error: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return false;
    }

    // Start listening
    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[" << current_timestamp() << "] [start] Listen failed. Error: " << WSAGetLastError() << std::endl;
        closesocket(server_fd);
        WSACleanup();
        return false;
    }

    std::cout << "[" << current_timestamp() << "] [start] RTMP server started successfully on port " << port << "." << std::endl;
    return true;
}

void RTMPServer::run() {
    std::cout << "[" << current_timestamp() << "] [run] RTMP server is now running..." << std::endl;
    running_ = true;

    while (running_) {
        sockaddr_in client_address;
        int client_len = sizeof(client_address);

        // Accept incoming connections
        SOCKET client_socket = accept(server_fd, (sockaddr*)&client_address, &client_len);
        if (client_socket == INVALID_SOCKET) {
            if (!running_) break;  // Exit if the server has been stopped
            std::cerr << "[" << current_timestamp() << "] [run] Failed to accept connection. Error: " << WSAGetLastError() << std::endl;
            continue;
        }

        std::lock_guard<std::mutex> lock(log_mutex);
        std::string client_ip = inet_ntoa(client_address.sin_addr);
        std::cout << "[" << current_timestamp() << "] [run] New client connected from " << client_ip << std::endl;

        // Handle the client in a separate thread
        client_threads.emplace_back([this, client_socket, client_ip]() {
            handle_client(client_socket, client_ip);
        });
    }

    std::cout << "[" << current_timestamp() << "] [run] Shutting down server..." << std::endl;

    // Join all threads when shutting down
    for (std::thread& t : client_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void RTMPServer::handle_client(SOCKET client_socket, const std::string& client_ip) {
    std::cout << "[handle_client] Client connected from IP: " << client_ip << std::endl;

    // Perform RTMP handshake
    if (!Parse::perform_handshake(client_socket)) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << "[handle_client] RTMP handshake failed for IP: " << client_ip << std::endl;
        closesocket(client_socket);
        return;
    }

    std::cout << "[handle_client] RTMP handshake completed successfully for IP: " << client_ip << std::endl;

    char buffer[BUFFER_SIZE];
    int read_size;

    // Process RTMP packets after handshake
    while ((read_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[handle_client] Received " << read_size << " bytes from client IP: " << client_ip << std::endl;

        // Store received data in the buffer
        Buffer::store_data(buffer, read_size);

        // Retrieve data from the buffer
        const std::vector<char>& data = Buffer::get_data();
        std::cout << "[handle_client] Buffer contains " << data.size() << " bytes before parsing." << std::endl;

        // Ensure there is enough data for AMF command handling
        if (data.size() < 3) {
            std::cerr << "[handle_client] Not enough data for AMF command, skipping." << std::endl;
        } else {
            // Parse AMF commands from the client
            ParseAMF::handle_amf_command(data.data(), data.size(), client_socket);

            // Clear the buffer after handling
            Buffer::clear();
        }
    }

    if (read_size == 0) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[handle_client] Client from IP: " << client_ip << " disconnected." << std::endl;
    } else {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << "[handle_client] Error receiving data from client IP: " << client_ip << ", error: " << WSAGetLastError() << std::endl;
    }

    // Close client socket
    closesocket(client_socket);
    std::cout << "[handle_client] Closed client socket for IP: " << client_ip << std::endl;
}

void RTMPServer::stop() {
    std::cout << "[" << current_timestamp() << "] [stop] Stopping RTMP server..." << std::endl;
    running_ = false;
    closesocket(server_fd);  // Close server socket to stop accepting new connections
    std::cout << "[" << current_timestamp() << "] [stop] Server has stopped accepting new connections." << std::endl;
}

RTMPServer::~RTMPServer() {
    if (server_fd != INVALID_SOCKET) {
        std::cout << "[" << current_timestamp() << "] [~RTMPServer] Closing server socket." << std::endl;
        closesocket(server_fd);
    }
    WSACleanup();
    std::cout << "[" << current_timestamp() << "] [~RTMPServer] Winsock cleanup completed." << std::endl;
}
