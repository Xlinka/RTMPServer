#include <csignal>
#include <iostream>
#include <thread>
#include "Client.h"        // RTMP server

// Global instance of the RTMP server
RTMPServer server;

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "SIGINT received. Stopping server..." << std::endl;
        server.stop();  // Gracefully stop the server
    }
}



int main() {
    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, signal_handler);


    // Attempt to start the server
    std::cout << "Attempting to start RTMP server on port 1935..." << std::endl;
    if (server.start(1935)) {
        std::cout << "RTMP server successfully started on port 1935." << std::endl;

        // Main loop to run the server
        std::cout << "Running the RTMP server..." << std::endl;
        server.run();  // This will block until the server is stopped
    } else {
        std::cerr << "Failed to start the RTMP server on port 1935." << std::endl;
        return 1;  // Exit with error code
    }


    std::cout << "RTMP server stopped." << std::endl;
    return 0;  // Exit gracefully
}
