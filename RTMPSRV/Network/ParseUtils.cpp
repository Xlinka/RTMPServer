#include "ParseUtils.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

void Parses::write_amf_string(const std::string& str, std::vector<char>& buffer) {
    buffer.push_back(0x02); // AMF0 string type marker
    uint16_t len = htons((uint16_t)str.size());
    buffer.insert(buffer.end(), (char*)&len, (char*)&len + 2); // Insert string length
    buffer.insert(buffer.end(), str.begin(), str.end()); // Insert string content
}


void Parses::write_amf_number(double value, std::vector<char>& buffer) {
    buffer.push_back(0x00); // AMF0 number type marker

    uint64_t host_double;
    std::memcpy(&host_double, &value, sizeof(double));

    // Check system endianness
    uint16_t endian_test = 0x1;
    bool is_little_endian = *(reinterpret_cast<uint8_t*>(&endian_test)) == 0x1;

    uint64_t net_double;
    if (is_little_endian) {
        // Swap bytes from little-endian to big-endian
        net_double = ((host_double & 0xFF00000000000000ULL) >> 56) |
                     ((host_double & 0x00FF000000000000ULL) >> 40) |
                     ((host_double & 0x0000FF0000000000ULL) >> 24) |
                     ((host_double & 0x000000FF00000000ULL) >> 8) |
                     ((host_double & 0x00000000FF000000ULL) << 8) |
                     ((host_double & 0x0000000000FF0000ULL) << 24) |
                     ((host_double & 0x000000000000FF00ULL) << 40) |
                     ((host_double & 0x00000000000000FFULL) << 56);
    } else {
        net_double = host_double;
    }

    // Write the 8 bytes to the buffer
    for (int i = 0; i < 8; ++i) {
        buffer.push_back(static_cast<char>((net_double >> ((7 - i) * 8)) & 0xFF));
    }
}


std::vector<char> Parses::build_rtmp_header(unsigned char fmt, unsigned int csid, 
                                           unsigned int timestamp, unsigned int message_length, 
                                           unsigned char message_type_id, unsigned int stream_id) {
    std::vector<char> header(12);
    header[0] = (fmt << 6) | (csid & 0x3F);  // Fmt and csid
    header[1] = (timestamp >> 16) & 0xFF;
    header[2] = (timestamp >> 8) & 0xFF;
    header[3] = timestamp & 0xFF;
    header[4] = (message_length >> 16) & 0xFF;
    header[5] = (message_length >> 8) & 0xFF;
    header[6] = message_length & 0xFF;
    header[7] = message_type_id;
    header[8] = stream_id & 0xFF;
    header[9] = (stream_id >> 8) & 0xFF;
    header[10] = (stream_id >> 16) & 0xFF;
    header[11] = (stream_id >> 24) & 0xFF;

    return header;
}

bool Parses::send_rtmp_message(SOCKET client_socket, const std::vector<char>& message, int retry_count) {
    int result;
    for (int attempt = 0; attempt < retry_count; ++attempt) {
        result = send(client_socket, message.data(), message.size(), 0);
        if (result != SOCKET_ERROR) {
            return true;
        }
        std::cerr << "Send failed, attempt " << (attempt + 1) << " of " << retry_count 
                  << ". Error: " << WSAGetLastError() << std::endl;
    }
    return false;
}

void Parses::dump_hex(const char* data, std::size_t length) {
    std::cout << "Hex dump (" << length << " bytes):" << std::endl;
    for (std::size_t i = 0; i < length; ++i) {
        printf("%02X ", (unsigned char)data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

double Parses::read_amf_number(const char* data) {
    if (data[0] != 0x00) {
        throw std::runtime_error("Expected AMF0 Number");
    }

    // Read the 8 bytes of the double
    uint64_t net_double;
    std::memcpy(&net_double, data + 1, sizeof(uint64_t));

    // Check system endianness
    uint16_t endian_test = 0x1;
    bool is_little_endian = *(reinterpret_cast<uint8_t*>(&endian_test)) == 0x1;

    uint64_t host_double;
    if (is_little_endian) {
        // Swap bytes from big-endian to little-endian
        host_double = ((net_double & 0xFF00000000000000ULL) >> 56) |
                      ((net_double & 0x00FF000000000000ULL) >> 40) |
                      ((net_double & 0x0000FF0000000000ULL) >> 24) |
                      ((net_double & 0x000000FF00000000ULL) >> 8) |
                      ((net_double & 0x00000000FF000000ULL) << 8) |
                      ((net_double & 0x0000000000FF0000ULL) << 24) |
                      ((net_double & 0x000000000000FF00ULL) << 40) |
                      ((net_double & 0x00000000000000FFULL) << 56);
    } else {
        host_double = net_double;
    }

    double number;
    std::memcpy(&number, &host_double, sizeof(double));
    return number;
}


//  read AMF0 string from the data stream
std::string Parses::read_amf_string(const char* data, std::size_t& offset) {
    if (data[0] != 0x02) {
        throw std::runtime_error("Expected AMF0 String");
    }

    // Extract the string length (2 bytes, big-endian)
    unsigned short string_length = (unsigned char(data[1]) << 8) | unsigned char(data[2]);
    std::string str(data + 3, string_length);

    // Update the offset to reflect the bytes read (1 byte marker + 2 bytes length + string content)
    offset += 3 + string_length;

    return str;
}
