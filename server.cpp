#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <vector>

const char PORT[] = "8080";
const int INITIAL_EVENT_LIST_SIZE = 10;
const int LISTEN_MAX_CONNECTIONS = 10;
const int BUFFER_SIZE = 1024;
const int EPOLL_FLAGS = EPOLLIN | EPOLLET; // Edge Triggered flag

// Function prototypes
int setup_listener_socket();
void set_non_blocking(int socket_fd);
int handle_new_connection(int epoll_fd, int server_fd);
void handle_client_data(int epoll_fd, int client_fd);
void close_client_connection(int epoll_fd, int client_fd);

int main() {
    int server_fd = setup_listener_socket();
    if (server_fd < 0) {
        std::cerr << "Error setting up the listener socket\n";
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Error creating epoll instance: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cerr << "Error adding server socket to epoll: " << strerror(errno) << "\n";
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    std::vector<struct epoll_event> events(INITIAL_EVENT_LIST_SIZE);
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (nfds == -1) {
            std::cerr << "Error during epoll_wait: " << strerror(errno) << "\n";
            if (errno != EINTR) {
                break;
            }
            continue;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                if (handle_new_connection(epoll_fd, server_fd) == -1) {
                    std::cerr << "Error handling new connection. Continuing with other connections.\n";
                }
            } else {
                handle_client_data(epoll_fd, events[n].data.fd);
            }
        }

        if (nfds == events.size()) {
            // Resize the event array if full
            events.resize(events.size() * 2);
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}

int setup_listener_socket() {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(NULL, PORT, &hints, &res);
    if (rv != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(rv) << "\n";
        return -1;
    }

    int listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listener == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        return -1;
    }

    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        std::cerr << "Error setting socket options: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (bind(listener, res->ai_addr, res->ai_addrlen) == -1) {
        std::cerr << "Error binding socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (listen(listener, LISTEN_MAX_CONNECTIONS) == -1) {
        std::cerr << "Error listening on socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    freeaddrinfo(res);
    return listener;
}

void set_non_blocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error getting flags for socket: " << strerror(errno) << "\n";
        return;
    }

    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error setting non-blocking flag for socket: " << strerror(errno) << "\n";
    }
}

int handle_new_connection(int epoll_fd, int server_fd) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1) {
        std::cerr << "Error accepting new connection: " << strerror(errno) << "\n";
        return -1;
    }

    set_non_blocking(new_fd);

    struct epoll_event ev;
    ev.events = EPOLL_FLAGS;
    ev.data.fd = new_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
        std::cerr << "Error adding new connection to epoll: " << strerror(errno) << "\n";
        close(new_fd);
        return -1;
    }

    return 0;
}

void handle_client_data(int epoll_fd, int client_fd) {
    char buffer[BUFFER_SIZE];
    int count = read(client_fd, buffer, BUFFER_SIZE);
    if (count == -1) {
        if (errno != EAGAIN) {
            std::cerr << "Error reading from client: " << strerror(errno) << "\n";
            close_client_connection(epoll_fd, client_fd);
        }
        return;
    } else if (count == 0) {
        close_client_connection(epoll_fd, client_fd);
        return;
    }

    std::cout << "Received message from fd " << client_fd << ": " << buffer << std::endl;

    if (send(client_fd, buffer, count, 0) == -1) {
        std::cerr << "Error sending data to client: " << strerror(errno) << "\n";
        close_client_connection(epoll_fd, client_fd);
    }
}

void close_client_connection(int epoll_fd, int client_fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        std::cerr << "Error removing client fd from epoll: " << strerror(errno) << "\n";
    }
    close(client_fd);
    std::cout << "Connection closed, fd: " << client_fd << std::endl;
}
