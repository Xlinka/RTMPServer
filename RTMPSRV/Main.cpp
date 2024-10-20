#include <csignal>
#include <iostream>
#include "Client.h"

RTMPServer server;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "SIGINT received. Stopping server..." << std::endl;
        server.stop();  // Gracefully stop the server
    }
}

int main() {
    // Register signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);

    if (server.start(1935)) {
        std::cout << "RTMP server started on port 1935." << std::endl;
        server.run();  // Run the server
    } else {
        std::cerr << "Failed to start the server." << std::endl;
        return 1;
    }

    return 0;
}
