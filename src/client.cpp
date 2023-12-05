#include "common.hpp"

const char DEFAULT_SERVER_IP[] = "127.0.0.1";
const uint16_t MESSAGE_SEQUENCE = 10;

int connect_to_server(const char* server_ip, const char* port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(server_ip, port, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << "\n";
        return -1;
    }

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        return -1;
    }

    if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        close(sockfd);
        std::cerr << "Error connecting: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

void handle_login_response(int sockfd) {
    std::vector<uint8_t> buffer(LOGIN_RESPONSE_BYTE_SIZE);
    int count = recv(sockfd, buffer.data(), LOGIN_RESPONSE_BYTE_SIZE, 0);
    
    if (count != LOGIN_RESPONSE_BYTE_SIZE) {
        std::cerr << "Error receiving login response: " << strerror(errno) << "\n";
        return;
    }

    LoginResponse response;
    deserialize_login_response(response, buffer.data());

    std::cout << "Login Response Status: " << response.status_code << std::endl;
}

void handle_echo_response(int sockfd) {
    std::vector<uint8_t> buffer(HEADER_BYTE_SIZE);
    if (recv(sockfd, buffer.data(), HEADER_BYTE_SIZE, 0) != HEADER_BYTE_SIZE) {
        std::cerr << "Error receiving echo response header: " << strerror(errno) << "\n";
        return;
    }

    Header header;
    deserialize_header(header, buffer.data());

    buffer.resize(header.message_size);
    if (recv(sockfd, &buffer[HEADER_BYTE_SIZE], header.message_size, 0) != header.message_size - HEADER_BYTE_SIZE) {
        std::cerr << "Error receiving echo response message: " << strerror(errno) << "\n";
        return;
    }

    EchoResponse response;
    deserialize_echo_response(response, buffer.data());

    std::cout << "Echo Response Message: " << response.plain_message << std::endl;
}

void login(int sockfd, const UserCredentials &credentials) {
    LoginRequest request = {{LOGIN_REQUEST_BYTE_SIZE, LOGIN_REQUEST_TYPE, MESSAGE_SEQUENCE}, credentials};

    std::vector<uint8_t> buffer(1024);
    serialize_login_request(request, buffer.data());

    if (send(sockfd, buffer.data(), request.header.message_size, 0) == -1) {
        std::cerr << "Error sending login request: " << strerror(errno) << "\n";
    }

    handle_login_response(sockfd);
}

void send_echo_request(int sockfd, const UserCredentials &credentials, const std::string &message) {
    std::string cipher_message = encrypt_echo_message(credentials, MESSAGE_SEQUENCE, message);
    uint16_t totalSize = static_cast<uint16_t>(HEADER_BYTE_SIZE + SIZE_BYTE_SIZE + cipher_message.size());
    EchoRequest request = {{totalSize, ECHO_REQUEST_TYPE, MESSAGE_SEQUENCE}, static_cast<uint16_t>(cipher_message.size()), cipher_message};

    std::vector<uint8_t> buffer(1024);
    serialize_echo_request(request, buffer.data());

    if (send(sockfd, buffer.data(), request.header.message_size, 0) == -1) {
        std::cerr << "Error sending echo request: " << strerror(errno) << "\n";
    }

    handle_echo_response(sockfd);
}


int main(int argc, char* argv[]) {
    const char* server_ip = (argc > 1) ? argv[1] : DEFAULT_SERVER_IP;
    const char* port = (argc > 2) ? argv[2] : DEFAULT_PORT;

    int sockfd = connect_to_server(server_ip, port);
    if (sockfd < 0) {
        return 1;
    }

    UserCredentials credentials = {"admin", "12345"};
    login(sockfd, credentials);

    send_echo_request(sockfd, credentials, "Hello, server!");

    getc(stdin);

    send_echo_request(sockfd, credentials, "Bye, server!");

    close(sockfd);
    return 0;
}
