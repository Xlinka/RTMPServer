// Client.h
#ifndef CLIENT_H
#define CLIENT_H
#define BUFFER_SIZE 4096  // Define a buffer size, e.g., 4096 bytes

#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <winsock2.h>

class RTMPServer {
public:
    RTMPServer() : running_(false), server_fd(INVALID_SOCKET) {}
    ~RTMPServer();
    
    bool start(int port);
    void run();
    void stop();
    bool is_running() const;

private:
    void handle_client(SOCKET client_socket, const std::string& client_ip);

    SOCKET server_fd;
    bool running_;
    std::vector<std::thread> client_threads;
    std::mutex log_mutex;
};

#endif // CLIENT_H
