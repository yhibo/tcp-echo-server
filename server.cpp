#include "common.hpp"
#include <fcntl.h>
#include <unordered_map>
#include <sys/epoll.h>


const int INITIAL_EVENT_LIST_SIZE = 10;
const int LISTEN_MAX_CONNECTIONS = 10;
const int INITIAL_BUFFER_SIZE = 1024;
const int EPOLL_FLAGS = EPOLLIN | EPOLLET;

std::unordered_map<int, UserCredentials> loggedUsers;

void printLoggedUsers(const std::unordered_map<int, UserCredentials>& loggedUsers) {
    std::cout << "Logged Users:" << std::endl;
    for (const auto& pair : loggedUsers) {
        std::cout << "User ID: " << pair.first << std::endl;
        std::cout << "  Username: " << pair.second.username << std::endl;
        std::cout << "  Password: " << pair.second.password << std::endl;
    }
}

std::string (&decryptEchoMessage)(const UserCredentials &credentials, uint8_t message_sequence, const std::string& cipherText) = encryptEchoMessage;
int setup_listener_socket();
void set_non_blocking(int socket_fd);
int handle_new_connection(int epoll_fd, int server_fd);
void handle_client_data(int epoll_fd, int client_fd);
void close_client_connection(int epoll_fd, int client_fd);
uint16_t user_login(int client_fd, const UserCredentials &userCredentials);

int main() {
    int server_fd = setup_listener_socket();
    if (server_fd < 0) {
        std::cout << "Error setting up the listener socket\n";
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cout << "Error creating epoll instance: " << strerror(errno) << "\n";
        close(server_fd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        std::cout << "Error adding server socket to epoll: " << strerror(errno) << "\n";
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    std::vector<struct epoll_event> events(INITIAL_EVENT_LIST_SIZE);
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (nfds == -1) {
            std::cout << "Error during epoll_wait: " << strerror(errno) << "\n";
            if (errno != EINTR) {
                break;
            }
            continue;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                if (handle_new_connection(epoll_fd, server_fd) == -1) {
                    std::cout << "Error handling new connection. Continuing with other connections.\n";
                }
            } else {
                handle_client_data(epoll_fd, events[n].data.fd);
            }
        }

        if (nfds == events.size()) {
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
        std::cout << "getaddrinfo error: " << gai_strerror(rv) << "\n";
        return -1;
    }

    int listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listener == -1) {
        std::cout << "Error creating socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        return -1;
    }

    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        std::cout << "Error setting socket options: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (bind(listener, res->ai_addr, res->ai_addrlen) == -1) {
        std::cout << "Error binding socket: " << strerror(errno) << "\n";
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (listen(listener, LISTEN_MAX_CONNECTIONS) == -1) {
        std::cout << "Error listening on socket: " << strerror(errno) << "\n";
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
        std::cout << "Error getting flags for socket: " << strerror(errno) << "\n";
        return;
    }

    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cout << "Error setting non-blocking flag for socket: " << strerror(errno) << "\n";
    }
}

int handle_new_connection(int epoll_fd, int server_fd) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1) {
        std::cout << "Error accepting new connection: " << strerror(errno) << "\n";
        return -1;
    }

    set_non_blocking(new_fd);

    struct epoll_event ev;
    ev.events = EPOLL_FLAGS;
    ev.data.fd = new_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
        std::cout << "Error adding new connection to epoll: " << strerror(errno) << "\n";
        close(new_fd);
        return -1;
    }

    return 0;
}

void handle_client_data(int epoll_fd, int client_fd) {

    std::vector<uint8_t> buffer(INITIAL_BUFFER_SIZE);
    int responseSize;

    int count = recv(client_fd, &buffer[0], HEADER_BYTE_SIZE, 0);
    if (count == -1) {
        if (errno != EAGAIN) {
            std::cout << "Error reading from client: " << strerror(errno) << "\n";
            close_client_connection(epoll_fd, client_fd);
        }
        return;
    } else if (count != HEADER_BYTE_SIZE) {
        close_client_connection(epoll_fd, client_fd);
        return;
    }

    Header header;
    deserializeHeader(header, &buffer[0]);

    if (header.messageSize > INITIAL_BUFFER_SIZE){
        buffer.resize(header.messageSize);
    }

    printLoggedUsers(loggedUsers);

    if (header.messageType != LOGIN_REQUEST_TYPE && header.messageType != ECHO_REQUEST_TYPE){
        std::cout << "Invalid request from client: " << client_fd << "\n";
        close_client_connection(epoll_fd, client_fd);
        return;
    }

    if (header.messageType == LOGIN_REQUEST_TYPE) {
        if(recv(client_fd, &buffer[0], USER_CREDENTIALS_BYTE_SIZE, 0) != USER_CREDENTIALS_BYTE_SIZE){
            close_client_connection(epoll_fd, client_fd);
            return;
        }

        UserCredentials userCredentials;
        deserializeUserCredentials(userCredentials, &buffer[0]);

        uint16_t statusCode = user_login(client_fd, userCredentials);

        if (statusCode == 0) {
            close_client_connection(epoll_fd, client_fd);
            return;
        }

        LoginResponse response = {{LOGIN_RESPONSE_BYTE_SIZE, LOGIN_RESPONSE_TYPE, header.messageSequence}, statusCode};

        printLoginResponse(response);        
        serializeLoginResponse(response, &buffer[0]);
        responseSize = response.header.messageSize;

    } else {
        auto it = loggedUsers.find(client_fd);
        if (it == loggedUsers.end()) {
            close_client_connection(epoll_fd, client_fd);
            return;
        }

        if(recv(client_fd, &buffer[0], SIZE_BYTE_SIZE, 0) != SIZE_BYTE_SIZE){
            close_client_connection(epoll_fd, client_fd);
            return;
        }

        uint16_t cipherMessageSize = ntohs(buffer[0] | (buffer[1] << 8));


        if(recv(client_fd, &buffer[0], cipherMessageSize, 0) != cipherMessageSize){
            close_client_connection(epoll_fd, client_fd);
            return;
        }


        std::string cipherMessage(buffer.begin(), buffer.begin() + cipherMessageSize);

        std::cout << "Cipher message: " << cipherMessage << std::endl;

        std::string plainMessage = decryptEchoMessage(it->second, header.messageSequence, cipherMessage);

        EchoResponse response = {{static_cast<uint16_t>(HEADER_BYTE_SIZE + SIZE_BYTE_SIZE + cipherMessageSize), 3, header.messageSequence}, cipherMessageSize, plainMessage};

        printEchoResponse(response);


        serializeEchoResponse(response, &buffer[0]);
        responseSize = response.header.messageSize;
    }

    if (send(client_fd, &buffer[0], responseSize, 0) == -1) {
        std::cout << "Error sending data to client: " << strerror(errno) << "\n";
        close_client_connection(epoll_fd, client_fd);
    }

    printLoggedUsers(loggedUsers);
}

void close_client_connection(int epoll_fd, int client_fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        std::cout << "Error removing client fd from epoll: " << strerror(errno) << "\n";
    }

    auto it = loggedUsers.find(client_fd);
    if (it != loggedUsers.end()) {
        loggedUsers.erase(it);
        std::cout << "Logged user removed, fd: " << client_fd << std::endl;
    }

    close(client_fd);
    std::cout << "Connection closed, fd: " << client_fd << std::endl;
}

uint16_t user_login(int client_fd, const UserCredentials &userCredentials){
    loggedUsers.insert({client_fd, userCredentials});

    return 1;
}
