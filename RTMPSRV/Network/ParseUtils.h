#ifndef PARSEUTILS_H
#define PARSEUTILS_H

#include <vector>
#include <string>
#include <cstddef>
#include <winsock2.h> // For SOCKET

class Parses {
public:
    static void write_amf_string(const std::string& str, std::vector<char>& buffer);
    static void write_amf_number(double value, std::vector<char>& buffer);
    static std::vector<char> build_rtmp_header(unsigned char fmt, unsigned int csid, 
                                               unsigned int timestamp, unsigned int message_length, 
                                               unsigned char message_type_id, unsigned int stream_id);
    static bool send_rtmp_message(SOCKET client_socket, const std::vector<char>& message, int retry_count = 3);
    static void dump_hex(const char* data, std::size_t length);

    static double read_amf_number(const char* data);
    static std::string read_amf_string(const char* data, std::size_t& offset);
};

#endif // PARSEUTILS_H
