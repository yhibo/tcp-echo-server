#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/epoll.h>

const int MAX_EVENTS = 10;
const char PORT[] = "8080";

// Function prototypes
int setup_listener_socket();
void set_non_blocking(int socket_fd);
void handle_new_connection(int epoll_fd, int server_fd);
void handle_client_data(int client_fd);

int main() {
    int server_fd = setup_listener_socket();
    int epoll_fd = epoll_create1(0);

    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                handle_new_connection(epoll_fd, server_fd);
            } else {
                handle_client_data(events[n].data.fd);
            }
        }
    }

    close(server_fd);
    return 0;
}

int setup_listener_socket() {
    struct addrinfo hints, *res;
    int listener;
    int yes = 1; // For setsockopt SO_REUSEADDR

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &hints, &res);
    listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // Corrected setsockopt call
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    bind(listener, res->ai_addr, res->ai_addrlen);
    listen(listener, 10);
    freeaddrinfo(res);

    return listener;
}

void set_non_blocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

void handle_new_connection(int epoll_fd, int server_fd) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &addr_size);

    set_non_blocking(new_fd);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = new_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev);
}

void handle_client_data(int client_fd) {
    char buffer[1024];
    int count = read(client_fd, buffer, sizeof(buffer));

    if (count > 0) {
        std::cout << "Received message: " << buffer << std::endl;
        send(client_fd, buffer, count, 0);
    } else if (count == 0 || (count == -1 && errno != EAGAIN)) {
        close(client_fd);
    }
}
