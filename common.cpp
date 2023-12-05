#include "common.hpp"


uint8_t* serialize_header(const Header& header, uint8_t* buffer) {
    uint16_t netmessage_size = htons(header.message_size);

    buffer[0] = netmessage_size & 0xFF;
    buffer[1] = (netmessage_size >> 8) & 0xFF;
    buffer[2] = header.message_type;
    buffer[3] = header.message_sequence;

    return buffer + HEADER_BYTE_SIZE;
}

const uint8_t* deserialize_header(Header& header, const uint8_t* buffer) {
    header.message_size = ntohs(buffer[0] | (buffer[1] << 8));
    header.message_type = buffer[2];
    header.message_sequence = buffer[3];

    return buffer + sizeof(Header);
}

uint8_t* serialize_user_credentials(const UserCredentials& credentials, uint8_t* buffer) {
    memcpy(buffer, credentials.username, USER_BYTE_SIZE);
    memcpy(buffer + USER_BYTE_SIZE, credentials.password, PASS_BYTE_SIZE);

    return buffer + USER_CREDENTIALS_BYTE_SIZE;
}

const uint8_t* deserialize_user_credentials(UserCredentials& credentials, const uint8_t* buffer) {
    memcpy(credentials.username, buffer, USER_BYTE_SIZE);
    memcpy(credentials.password, buffer + USER_BYTE_SIZE, PASS_BYTE_SIZE);

    return buffer + 64;
}

uint8_t* serialize_login_request(const LoginRequest& request, uint8_t* buffer) {
    uint8_t* bufferHead = serialize_header(request.header, buffer);
    return serialize_user_credentials(request.credentials, bufferHead);
}

const uint8_t* deserialize_login_request(LoginRequest& request, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserialize_header(request.header, buffer);
    return deserialize_user_credentials(request.credentials, bufferHead);
}

uint8_t* serialize_login_response(const LoginResponse& response, uint8_t* buffer) {
    uint8_t* bufferHead = serialize_header(response.header, buffer);
    uint16_t netstatus_code = htons(response.status_code);

    bufferHead[0] = netstatus_code & 0xFF;
    bufferHead[1] = (netstatus_code >> 8) & 0xFF;

    return bufferHead + sizeof(uint16_t);
}

const uint8_t* deserialize_login_response(LoginResponse& response, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserialize_header(response.header, buffer);
    response.status_code = ntohs(bufferHead[0] | (bufferHead[1] << 8));

    return bufferHead + sizeof(uint16_t);
}

uint8_t* serialize_echo_request(const EchoRequest& request, uint8_t* buffer) {
    uint8_t* bufferHead = serialize_header(request.header, buffer);
    uint16_t netmessage_size = htons(request.message_size);

    bufferHead[0] = netmessage_size & 0xFF;
    bufferHead[1] = (netmessage_size >> 8) & 0xFF;
    memcpy(bufferHead + sizeof(uint16_t), request.cipher_message.data(), request.cipher_message.size());

    return bufferHead + sizeof(uint16_t) + request.cipher_message.size();
}

const uint8_t* deserialize_echo_request(EchoRequest& request, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserialize_header(request.header, buffer);
    request.message_size = ntohs(bufferHead[0] | (bufferHead[1] << 8));
    bufferHead += sizeof(uint16_t);

    request.cipher_message.resize(request.message_size);
    memcpy(request.cipher_message.data(), bufferHead, request.message_size);

    return bufferHead + request.message_size;
}

uint8_t* serialize_echo_response(const EchoResponse& response, uint8_t* buffer) {
    uint8_t* advanced_ptr = serialize_header(response.header, buffer);
    uint16_t netmessage_size = htons(response.message_size);

    advanced_ptr[0] = netmessage_size & 0xFF;
    advanced_ptr[1] = (netmessage_size >> 8) & 0xFF;
    memcpy(advanced_ptr + sizeof(uint16_t), response.plain_message.data(), response.plain_message.size());

    return advanced_ptr + sizeof(uint16_t) + response.plain_message.size();
}

const uint8_t* deserialize_echo_response(EchoResponse& response, const uint8_t* buffer) {
    const uint8_t* advanced_ptr = deserialize_header(response.header, buffer);
    response.message_size = ntohs(advanced_ptr[0] | (advanced_ptr[1] << 8));
    advanced_ptr += sizeof(uint16_t);

    response.plain_message.resize(response.message_size);
    memcpy(response.plain_message.data(), advanced_ptr, response.message_size);

    return advanced_ptr + response.message_size;
}

uint32_t next_key(uint32_t key) {
return (key * 1103515245 + 12345) % 0x7FFFFFFF;
}

uint8_t calculate_check_sum(const std::string& text) {
    uint8_t checksum = 0;
    for (char ch : text) {
        checksum += static_cast<uint8_t>(ch);
    }
    return ~checksum;
}

std::string encrypt_echo_message(const UserCredentials &credentials, uint8_t message_sequence, const std::string& cipher_text) {
    uint8_t username_sum = calculate_check_sum(credentials.username);
    uint8_t password_sum = calculate_check_sum(credentials.password);
    uint32_t key = (static_cast<uint32_t>(message_sequence) << 16) | (static_cast<uint32_t>(username_sum) << 8) | password_sum;

    std::string plain_text;
    plain_text.reserve(cipher_text.size());

    for (char ch : cipher_text) {
        key = next_key(key); 
        plain_text.push_back(ch ^ static_cast<uint8_t>(key % 256));
    }

    return plain_text;
}

#ifndef NDEBUG

int indent_level = 0;

void increaseIndent() {
    indent_level += 2;
}

void decreaseIndent() {
    if (indent_level >= 2) {
        indent_level -= 2;
    }
}

std::string getIndent() {
    return std::string(indent_level, ' ');
}

void print_header(const Header& header) {
    std::cout << getIndent() << "Header Details:" << std::endl;
    increaseIndent();
    std::cout << getIndent() << "Message Size: " << header.message_size << std::endl;
    std::cout << getIndent() << "Message Type: " << static_cast<int>(header.message_type) << std::endl;
    std::cout << getIndent() << "Message Sequence: " << static_cast<int>(header.message_sequence) << std::endl;
    decreaseIndent();
}

void print_user_credentials(const UserCredentials& credentials) {
    std::cout << getIndent() << "User Credentials:" << std::endl;
    increaseIndent();
    std::cout << getIndent() << "Username: " << credentials.username << std::endl;
    std::cout << getIndent() << "Password: " << credentials.password << std::endl;
    decreaseIndent();
}

void print_login_request(const LoginRequest& request) {
    std::cout << getIndent() << "Login Request:" << std::endl;
    increaseIndent();
    print_header(request.header);
    print_user_credentials(request.credentials);
    decreaseIndent();
}

void print_login_response(const LoginResponse& response) {
    std::cout << getIndent() << "Login Response:" << std::endl;
    increaseIndent();
    print_header(response.header);
    std::cout << getIndent() << "Status Code: " << response.status_code << std::endl;
    decreaseIndent();
}

void print_echo_request(const EchoRequest& request) {
    std::cout << getIndent() << "Echo Request:" << std::endl;
    increaseIndent();
    print_header(request.header);
    std::cout << getIndent() << "Message Size: " << request.message_size << std::endl;
    std::cout << getIndent() << "Cipher Message: " << request.cipher_message << std::endl;
    decreaseIndent();
}

void print_echo_response(const EchoResponse& response) {
    std::cout << getIndent() << "Echo Response:" << std::endl;
    increaseIndent();
    print_header(response.header);
    std::cout << getIndent() << "Message Size: " << response.message_size << std::endl;
    std::cout << getIndent() << "Plain Message: " << response.plain_message << std::endl;
    decreaseIndent();
}
#endif