#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int SERVER_PORT = 8080;        // Port number of the server
const int BUFFER_SIZE = 1024;        // Size of the buffer for incoming messages

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    const char* SERVER_IP = argv[1];  // IP address of the server from command line
    struct sockaddr_in serv_addr;
    int sock = 0;
    char buffer[BUFFER_SIZE] = {0};

    // Create the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // Convert IPv4 or IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    std::string message = "Hello from client!";
    send(sock, message.c_str(), message.length(), 0);
    std::cout << "Message sent" << std::endl;

    int valread = recv(sock, buffer, BUFFER_SIZE, 0);
    std::cout << "Server: " << buffer << std::endl;

    close(sock);
    return 0;
}
