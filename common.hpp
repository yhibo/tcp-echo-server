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
    uint16_t messageSize;
    uint8_t messageType;
    uint8_t messageSequence;
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
    uint16_t statusCode;
};

struct EchoRequest {
    Header header;
    uint16_t messageSize;
    std::string cipherMessage;
};

struct EchoResponse {
    Header header;
    uint16_t messageSize;
    std::string plainMessage;
};


uint8_t* serializeHeader(const Header& header, uint8_t* buffer);
uint8_t* serializeUserCredentials(const UserCredentials& credentials, uint8_t* buffer);
uint8_t* serializeLoginRequest(const LoginRequest& request, uint8_t* buffer);
uint8_t* serializeLoginResponse(const LoginResponse& response, uint8_t* buffer);
uint8_t* serializeEchoRequest(const EchoRequest& request, uint8_t* buffer);
uint8_t* serializeEchoResponse(const EchoResponse& response, uint8_t* buffer);

const uint8_t* deserializeHeader(Header& header, const uint8_t* buffer);
const uint8_t* deserializeUserCredentials(UserCredentials& credentials, const uint8_t* buffer);
const uint8_t* deserializeLoginRequest(LoginRequest& request, const uint8_t* buffer);
const uint8_t* deserializeLoginResponse(LoginResponse& response, const uint8_t* buffer);
const uint8_t* deserializeEchoRequest(EchoRequest& request, const uint8_t* buffer);
const uint8_t* deserializeEchoResponse(EchoResponse& response, const uint8_t* buffer);


void printHeader(const Header& header);
void printUserCredentials(const UserCredentials& credentials);
void printLoginRequest(const LoginRequest& request);
void printLoginResponse(const LoginResponse& response);
void printEchoRequest(const EchoRequest& request);
void printEchoResponse(const EchoResponse& response);
