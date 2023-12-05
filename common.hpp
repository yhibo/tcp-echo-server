#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <vector>

const char PORT[] = "8080";

const uint8_t LOGIN_REQUEST_TYPE = 0;
const uint8_t LOGIN_RESPONSE_TYPE = 1;
const uint8_t ECHO_REQUEST_TYPE = 2;
const uint8_t ECHO_RESPONSE_TYPE = 3;

const uint16_t HEADER_BYTE_SIZE = 4;
const uint16_t SIZE_BYTE_SIZE = 2;
const uint16_t USER_BYTE_SIZE = 32;
const uint16_t PASS_BYTE_SIZE = 32;
const uint16_t USER_CREDENTIALS_BYTE_SIZE = USER_BYTE_SIZE + PASS_BYTE_SIZE;
const uint16_t LOGIN_REQUEST_BYTE_SIZE = HEADER_BYTE_SIZE + USER_CREDENTIALS_BYTE_SIZE;
const uint16_t LOGIN_RESPONSE_BYTE_SIZE = HEADER_BYTE_SIZE + SIZE_BYTE_SIZE;

struct Header {
    uint16_t message_size;
    uint8_t message_type;
    uint8_t message_sequence;
};

struct UserCredentials
{
    char username[USER_BYTE_SIZE];
    char password[PASS_BYTE_SIZE];
};

struct LoginRequest {
    Header header;
    UserCredentials credentials;
};

struct LoginResponse {
    Header header;
    uint16_t status_code;
};

struct EchoRequest {
    Header header;
    uint16_t message_size;
    std::string cipher_message;
};

struct EchoResponse {
    Header header;
    uint16_t message_size;
    std::string plain_message;
};


uint8_t* serialize_header(const Header& header, uint8_t* buffer);
uint8_t* serialize_user_credentials(const UserCredentials& credentials, uint8_t* buffer);
uint8_t* serialize_login_request(const LoginRequest& request, uint8_t* buffer);
uint8_t* serialize_login_response(const LoginResponse& response, uint8_t* buffer);
uint8_t* serialize_echo_request(const EchoRequest& request, uint8_t* buffer);
uint8_t* serialize_echo_response(const EchoResponse& response, uint8_t* buffer);

const uint8_t* deserialize_header(Header& header, const uint8_t* buffer);
const uint8_t* deserialize_user_credentials(UserCredentials& credentials, const uint8_t* buffer);
const uint8_t* deserialize_login_request(LoginRequest& request, const uint8_t* buffer);
const uint8_t* deserialize_login_response(LoginResponse& response, const uint8_t* buffer);
const uint8_t* deserialize_echo_request(EchoRequest& request, const uint8_t* buffer);
const uint8_t* deserialize_echo_response(EchoResponse& response, const uint8_t* buffer);

std::string encrypt_echo_message(const UserCredentials &credentials, uint8_t message_sequence, const std::string& cipher_text);

#ifndef NDEBUG
void print_header(const Header& header);
void print_user_credentials(const UserCredentials& credentials);
void print_login_request(const LoginRequest& request);
void print_login_response(const LoginResponse& response);
void print_echo_request(const EchoRequest& request);
void print_echo_response(const EchoResponse& response);
#endif