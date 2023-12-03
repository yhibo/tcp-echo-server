#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

struct Header {
    uint16_t messageSize;
    uint8_t messageType;
    uint8_t messageSequence;
};

struct LoginRequest {
    Header header;
    char username[32];
    char password[32];
};

struct LoginResponse {
    Header header;
    uint16_t statusCode;
};

struct EchoRequest {
    Header header;
    uint16_t messageSize;
    std::vector<uint8_t> cipherMessage;
};

struct EchoResponse {
    Header header;
    uint16_t messageSize;
    std::vector<uint8_t> plainMessage;
};

const int PORT = 8080;
const int MAX_EVENTS = 10; // Maximum number of events to be returned from a single epoll_wait call

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Create an epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;

    // Add server socket to the epoll instance
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                // Accept new connection
                new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
                if (new_socket == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }

                // Set new socket to non-blocking mode
                int flags = fcntl(new_socket, F_GETFL, 0);
                if (flags == -1) {
                    perror("fcntl");
                    exit(EXIT_FAILURE);
                }
                flags |= O_NONBLOCK;
                if (fcntl(new_socket, F_SETFL, flags) == -1) {
                    perror("fcntl");
                    exit(EXIT_FAILURE);
                }

                // Add the new socket to the epoll instance
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = new_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                    perror("epoll_ctl: new_socket");
                    exit(EXIT_FAILURE);
                }

            } else {
                // Handle data from clients
                new_socket = events[n].data.fd;
                int count = read(new_socket, buffer, sizeof(buffer));
                if (count == -1) {
                    // If errno == EAGAIN, that means we have read all data
                    if (errno != EAGAIN) {
                        perror("read");
                        close(new_socket);
                    }
                } else if (count == 0) {
                    // End of file. The remote has closed the connection.
                    close(new_socket);
                } else {
                    // Write the buffer to standard output
                    std::cout << "Received message: " << buffer << std::endl;

                    // Echo back the message
                    send(new_socket, buffer, count, 0);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
