#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>  // For SOCKET and Winsock functions
#include <vector>
#include <thread>
#include <mutex>

#define BUFFER_SIZE 4096

class RTMPServer {
public:
    bool start(int port);
    void run();
    void stop();
    ~RTMPServer();

private:
    void handle_client(SOCKET client_socket);  // Handles a client connection
    SOCKET server_fd = INVALID_SOCKET;         // Server socket descriptor
    bool running_ = false;                     // Server running flag

    // Vector to hold client threads and a mutex for thread safety
    std::vector<std::thread> client_threads;
    std::mutex log_mutex;
};

#endif  // CLIENT_H
