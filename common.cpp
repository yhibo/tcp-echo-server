#include "common.hpp"


uint8_t* serializeHeader(const Header& header, uint8_t* buffer) {
    uint16_t netMessageSize = htons(header.messageSize);

    buffer[0] = netMessageSize & 0xFF;
    buffer[1] = (netMessageSize >> 8) & 0xFF;
    buffer[2] = header.messageType;
    buffer[3] = header.messageSequence;

    return buffer + HEADER_BYTE_SIZE;
}

const uint8_t* deserializeHeader(Header& header, const uint8_t* buffer) {
    header.messageSize = ntohs(buffer[0] | (buffer[1] << 8));
    header.messageType = buffer[2];
    header.messageSequence = buffer[3];

    return buffer + sizeof(Header);
}

uint8_t* serializeUserCredentials(const UserCredentials& credentials, uint8_t* buffer) {
    memcpy(buffer, credentials.username, USER_BYTE_SIZE);
    memcpy(buffer + USER_BYTE_SIZE, credentials.password, PASS_BYTE_SIZE);

    return buffer + USER_CREDENTIALS_BYTE_SIZE;
}

const uint8_t* deserializeUserCredentials(UserCredentials& credentials, const uint8_t* buffer) {
    memcpy(credentials.username, buffer, USER_BYTE_SIZE);
    memcpy(credentials.password, buffer + USER_BYTE_SIZE, PASS_BYTE_SIZE);

    return buffer + 64;
}

uint8_t* serializeLoginRequest(const LoginRequest& request, uint8_t* buffer) {
    uint8_t* bufferHead = serializeHeader(request.header, buffer);
    return serializeUserCredentials(request.credentials, bufferHead);
}

const uint8_t* deserializeLoginRequest(LoginRequest& request, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserializeHeader(request.header, buffer);
    return deserializeUserCredentials(request.credentials, bufferHead);
}

uint8_t* serializeLoginResponse(const LoginResponse& response, uint8_t* buffer) {
    uint8_t* bufferHead = serializeHeader(response.header, buffer);
    uint16_t netStatusCode = htons(response.statusCode);

    bufferHead[0] = netStatusCode & 0xFF;
    bufferHead[1] = (netStatusCode >> 8) & 0xFF;

    return bufferHead + sizeof(uint16_t);
}

const uint8_t* deserializeLoginResponse(LoginResponse& response, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserializeHeader(response.header, buffer);
    response.statusCode = ntohs(bufferHead[0] | (bufferHead[1] << 8));

    return bufferHead + sizeof(uint16_t);
}

uint8_t* serializeEchoRequest(const EchoRequest& request, uint8_t* buffer) {
    uint8_t* bufferHead = serializeHeader(request.header, buffer);
    uint16_t netMessageSize = htons(request.messageSize);

    bufferHead[0] = netMessageSize & 0xFF;
    bufferHead[1] = (netMessageSize >> 8) & 0xFF;
    memcpy(bufferHead + sizeof(uint16_t), request.cipherMessage.data(), request.cipherMessage.size());

    return bufferHead + sizeof(uint16_t) + request.cipherMessage.size();
}

const uint8_t* deserializeEchoRequest(EchoRequest& request, const uint8_t* buffer) {
    const uint8_t* bufferHead = deserializeHeader(request.header, buffer);
    request.messageSize = ntohs(bufferHead[0] | (bufferHead[1] << 8));
    bufferHead += sizeof(uint16_t);

    request.cipherMessage.resize(request.messageSize);
    memcpy(request.cipherMessage.data(), bufferHead, request.messageSize);

    return bufferHead + request.messageSize;
}

uint8_t* serializeEchoResponse(const EchoResponse& response, uint8_t* buffer) {
    uint8_t* advancedPtr = serializeHeader(response.header, buffer);
    uint16_t netMessageSize = htons(response.messageSize);

    advancedPtr[0] = netMessageSize & 0xFF;
    advancedPtr[1] = (netMessageSize >> 8) & 0xFF;
    memcpy(advancedPtr + sizeof(uint16_t), response.plainMessage.data(), response.plainMessage.size());

    return advancedPtr + sizeof(uint16_t) + response.plainMessage.size();
}

const uint8_t* deserializeEchoResponse(EchoResponse& response, const uint8_t* buffer) {
    const uint8_t* advancedPtr = deserializeHeader(response.header, buffer);
    response.messageSize = ntohs(advancedPtr[0] | (advancedPtr[1] << 8));
    advancedPtr += sizeof(uint16_t);

    response.plainMessage.resize(response.messageSize);
    memcpy(response.plainMessage.data(), advancedPtr, response.messageSize);

    return advancedPtr + response.messageSize;
}

int indentLevel = 0;

void increaseIndent() {
    indentLevel += 2;
}

void decreaseIndent() {
    if (indentLevel >= 2) {
        indentLevel -= 2;
    }
}

std::string getIndent() {
    return std::string(indentLevel, ' ');
}

void printHeader(const Header& header) {
    std::cout << getIndent() << "Header Details:" << std::endl;
    increaseIndent();
    std::cout << getIndent() << "Message Size: " << header.messageSize << std::endl;
    std::cout << getIndent() << "Message Type: " << static_cast<int>(header.messageType) << std::endl;
    std::cout << getIndent() << "Message Sequence: " << static_cast<int>(header.messageSequence) << std::endl;
    decreaseIndent();
}

void printUserCredentials(const UserCredentials& credentials) {
    std::cout << getIndent() << "User Credentials:" << std::endl;
    increaseIndent();
    std::cout << getIndent() << "Username: " << credentials.username << std::endl;
    std::cout << getIndent() << "Password: " << credentials.password << std::endl;
    decreaseIndent();
}

void printLoginRequest(const LoginRequest& request) {
    std::cout << getIndent() << "Login Request:" << std::endl;
    increaseIndent();
    printHeader(request.header);
    printUserCredentials(request.credentials);
    decreaseIndent();
}

void printLoginResponse(const LoginResponse& response) {
    std::cout << getIndent() << "Login Response:" << std::endl;
    increaseIndent();
    printHeader(response.header);
    std::cout << getIndent() << "Status Code: " << response.statusCode << std::endl;
    decreaseIndent();
}

void printEchoRequest(const EchoRequest& request) {
    std::cout << getIndent() << "Echo Request:" << std::endl;
    increaseIndent();
    printHeader(request.header);
    std::cout << getIndent() << "Message Size: " << request.messageSize << std::endl;
    std::cout << getIndent() << "Cipher Message: " << request.cipherMessage << std::endl;
    decreaseIndent();
}

void printEchoResponse(const EchoResponse& response) {
    std::cout << getIndent() << "Echo Response:" << std::endl;
    increaseIndent();
    printHeader(response.header);
    std::cout << getIndent() << "Message Size: " << response.messageSize << std::endl;
    std::cout << getIndent() << "Plain Message: " << response.plainMessage << std::endl;
    decreaseIndent();
}
