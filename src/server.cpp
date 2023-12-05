#include "common.hpp"
#include <fcntl.h>
#include <unordered_map>
#include <sys/epoll.h>


const int INITIAL_EVENT_LIST_SIZE = 10;
const int LISTEN_MAX_CONNECTIONS = 10;
const int INITIAL_BUFFER_SIZE = 1024;
const int EPOLL_FLAGS = EPOLLIN | EPOLLET;

std::unordered_map<int, UserCredentials> logged_users;

#ifndef NDEBUG
void printLogged_users(const std::unordered_map<int, UserCredentials>& logged_users) {
    std::cout << "Logged Users:" << std::endl;
    for (const auto& pair : logged_users) {
        std::cout << "User ID: " << pair.first << std::endl;
        std::cout << "  Username: " << pair.second.username << std::endl;
        std::cout << "  Password: " << pair.second.password << std::endl;
    }
}
#endif

std::string (&decrypt_echo_message)(const UserCredentials &credentials, uint8_t message_sequence, const std::string& cipher_text) = encrypt_echo_message;
int setup_listener_socket(const char* port);
void set_non_blocking(int socket_fd);
int handle_new_connection(int epoll_fd, int server_fd);
bool receive_header(int client_fd, std::vector<uint8_t>& buffer, Header& header);
bool is_valid_request(const Header& header);
bool handle_login_request(int client_fd, const Header& header, std::vector<uint8_t>& buffer);
bool handle_echo_request(int client_fd, const Header& header, std::vector<uint8_t>& buffer);
bool send_response(int client_fd, const std::vector<uint8_t>& buffer, int response_size);

void handle_client_data(int epoll_fd, int client_fd);
void close_client_connection(int epoll_fd, int client_fd);
uint16_t user_login(int client_fd, const UserCredentials &user_credentials);

int main(int argc, char* argv[]) {
    const char* port = (argc > 1) ? argv[1] : DEFAULT_PORT;

    int server_fd = setup_listener_socket(port);
    if (server_fd < 0) {
        #ifndef NDEBUG
        std::cout << "Error setting up the listener socket\n";
        #endif
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        #ifndef NDEBUG
        std::cout << "Error creating epoll instance: " << strerror(errno) << "\n";
        #endif
        close(server_fd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        #ifndef NDEBUG
        std::cout << "Error adding server socket to epoll: " << strerror(errno) << "\n";
        #endif
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    std::vector<struct epoll_event> events(INITIAL_EVENT_LIST_SIZE);
    while (true) {
        int nfds = epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (nfds == -1) {
            #ifndef NDEBUG
            std::cout << "Error during epoll_wait: " << strerror(errno) << "\n";
            #endif
            if (errno != EINTR) {
                break;
            }
            continue;
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == server_fd) {
                if (handle_new_connection(epoll_fd, server_fd) == -1) {
                    #ifndef NDEBUG
                    std::cout << "Error handling new connection. Continuing with other connections.\n";
                    #endif
                }
            } else {
                handle_client_data(epoll_fd, events[n].data.fd);
            }
        }

        if (nfds == static_cast<int>(events.size())) {
            events.resize(events.size() * 2);
        }
    }

    close(server_fd);
    close(epoll_fd);
    return 0;
}

int setup_listener_socket(const char* port) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int rv = getaddrinfo(NULL, port, &hints, &res);
    if (rv != 0) {
        #ifndef NDEBUG
        std::cout << "getaddrinfo error: " << gai_strerror(rv) << "\n";
        #endif
        return -1;
    }

    int listener = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listener == -1) {
        #ifndef NDEBUG
        std::cout << "Error creating socket: " << strerror(errno) << "\n";
        #endif
        freeaddrinfo(res);
        return -1;
    }

    int yes = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        #ifndef NDEBUG
        std::cout << "Error setting socket options: " << strerror(errno) << "\n";
        #endif
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (bind(listener, res->ai_addr, res->ai_addrlen) == -1) {
        #ifndef NDEBUG
        std::cout << "Error binding socket: " << strerror(errno) << "\n";
        #endif
        freeaddrinfo(res);
        close(listener);
        return -1;
    }

    if (listen(listener, LISTEN_MAX_CONNECTIONS) == -1) {
        #ifndef NDEBUG
        std::cout << "Error listening on socket: " << strerror(errno) << "\n";
        #endif
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
        #ifndef NDEBUG
        std::cout << "Error getting flags for socket: " << strerror(errno) << "\n";
        #endif
        return;
    }

    if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        #ifndef NDEBUG
        std::cout << "Error setting non-blocking flag for socket: " << strerror(errno) << "\n";
        #endif
    }
}

int handle_new_connection(int epoll_fd, int server_fd) {
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof their_addr;
    int new_fd = accept(server_fd, (struct sockaddr *)&their_addr, &addr_size);
    if (new_fd == -1) {
        #ifndef NDEBUG
        std::cout << "Error accepting new connection: " << strerror(errno) << "\n";
        #endif
        return -1;
    }

    set_non_blocking(new_fd);

    struct epoll_event ev;
    ev.events = EPOLL_FLAGS;
    ev.data.fd = new_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_fd, &ev) == -1) {
        #ifndef NDEBUG
        std::cout << "Error adding new connection to epoll: " << strerror(errno) << "\n";
        #endif
        close(new_fd);
        return -1;
    }

    return 0;
}

bool receive_header(int client_fd, std::vector<uint8_t>& buffer, Header& header) {
    int count = recv(client_fd, &buffer[0], HEADER_BYTE_SIZE, 0);
    if (count == -1 && errno != EAGAIN) {
        #ifndef NDEBUG
        std::cout << "Error reading from client: " << strerror(errno) << "\n";
        #endif
        return false;
    } else if (count != HEADER_BYTE_SIZE) {
        return false;
    }

    deserialize_header(header, &buffer[0]);
    if (header.message_size > INITIAL_BUFFER_SIZE) {
        buffer.resize(header.message_size);
    }
    return true;
}

bool is_valid_request(const Header& header) {
    if (header.message_type != LOGIN_REQUEST_TYPE && header.message_type != ECHO_REQUEST_TYPE) {
        #ifndef NDEBUG
        std::cout << "Invalid request from client.\n";
        #endif
        return false;
    }
    return true;
}

bool handle_login_request(int client_fd, const Header& header, std::vector<uint8_t>& buffer) {
    if (recv(client_fd, &buffer[0], USER_CREDENTIALS_BYTE_SIZE, 0) != USER_CREDENTIALS_BYTE_SIZE) {
        return false;
    }

    UserCredentials userCredentials;
    deserialize_user_credentials(userCredentials, &buffer[0]);
    uint16_t status_code = user_login(client_fd, userCredentials);

    if (status_code == 0) {
        return false;
    }

    LoginResponse response = {{LOGIN_RESPONSE_BYTE_SIZE, LOGIN_RESPONSE_TYPE, header.message_sequence}, status_code};
    print_login_response(response);
    serialize_login_response(response, &buffer[0]);

    return send_response(client_fd, buffer, response.header.message_size);
}

bool handle_echo_request(int client_fd, const Header& header, std::vector<uint8_t>& buffer) {
    auto it = logged_users.find(client_fd);
    if (it == logged_users.end()) {
        return false;
    }

    if (recv(client_fd, &buffer[0], SIZE_BYTE_SIZE, 0) != SIZE_BYTE_SIZE) {
        return false;
    }

    uint16_t cipher_message_size = ntohs(buffer[0] | (buffer[1] << 8));
    if (recv(client_fd, &buffer[0], cipher_message_size, 0) != cipher_message_size) {
        return false;
    }

    std::string cipher_message(buffer.begin(), buffer.begin() + cipher_message_size);
    #ifndef NDEBUG
    std::cout << "Cipher message: " << cipher_message << std::endl;
    #endif
    std::string plain_message = decrypt_echo_message(it->second, header.message_sequence, cipher_message);

    EchoResponse response = {{static_cast<uint16_t>(HEADER_BYTE_SIZE + SIZE_BYTE_SIZE + cipher_message_size), ECHO_RESPONSE_TYPE, header.message_sequence}, cipher_message_size, plain_message};
    print_echo_response(response);

    serialize_echo_response(response, &buffer[0]);
    return send_response(client_fd, buffer, response.header.message_size);
}

bool send_response(int client_fd, const std::vector<uint8_t>& buffer, int response_size) {
    if (send(client_fd, &buffer[0], response_size, 0) == -1) {
        #ifndef NDEBUG
        std::cout << "Error sending data to client: " << strerror(errno) << "\n";
        #endif
        return false;
    }
    return true;
}


void handle_client_data(int epoll_fd, int client_fd) {
    std::vector<uint8_t> buffer(INITIAL_BUFFER_SIZE);
    Header header;
    if (!receive_header(client_fd, buffer, header)) {
        close_client_connection(epoll_fd, client_fd);
        return;
    }

    if (!is_valid_request(header)) {
        close_client_connection(epoll_fd, client_fd);
        return;
    }

    if (header.message_type == LOGIN_REQUEST_TYPE) {
        if (!handle_login_request(client_fd, header, buffer)) {
            close_client_connection(epoll_fd, client_fd);
        }
    } else {
        if (!handle_echo_request(client_fd, header, buffer)) {
            close_client_connection(epoll_fd, client_fd);
        }
    }
}


void close_client_connection(int epoll_fd, int client_fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        #ifndef NDEBUG
        std::cout << "Error removing client fd from epoll: " << strerror(errno) << "\n";
        #endif
    }

    auto it = logged_users.find(client_fd);
    if (it != logged_users.end()) {
        logged_users.erase(it);
        #ifndef NDEBUG
        std::cout << "Logged user removed, fd: " << client_fd << std::endl;
        #endif
    }

    close(client_fd);
    #ifndef NDEBUG
    std::cout << "Connection closed, fd: " << client_fd << std::endl;
    #endif
}

uint16_t user_login(int client_fd, const UserCredentials &user_credentials){
    logged_users.insert({client_fd, user_credentials});

    return 1;
}
