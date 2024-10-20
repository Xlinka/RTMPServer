#pragma once
#include <string>
#include <vector>
#include <WinSock2.h>

class ParseAMF {
public:
    static void handle_amf_command(const char* data, std::size_t length, SOCKET client_socket);
    static void send_connect_response(SOCKET client_socket, double transaction_id);
    static void send_create_stream_response(SOCKET client_socket, double transaction_id);
    static void send_on_status_publish(SOCKET client_socket, double transaction_id);
    static void send_on_status_play(SOCKET client_socket, double transaction_id);
    static void send_on_status_pause(SOCKET client_socket, double transaction_id);
    static double network_to_host_double(uint64_t net_double); // renamed the function for clarity

private:
    static bool send_rtmp_message_safe(SOCKET client_socket, const std::vector<char>& message);
};
