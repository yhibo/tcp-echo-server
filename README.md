# TCP Echo Client-Server Project

This project consists of a TCP echo client and server. The provided Makefile simplifies the process of compiling and running these applications.

## Directory Structure

- `src/`: Contains the source files for the client and server.
- `build/`: Destination directory for the compiled executables.

## Building the Applications

Use the following commands to build the client and server applications:

```bash
# Compile both the client and server
make

# Compile only the client
make client

# Compile only the server
make server

# Compile for release (with optimizations)
# Note: Compiling the server in release mode suppresses the server-side output of sent responses.
make release

# Clean the build (remove compiled files)
make clean
```

## Running the Applications

### Server

The server accepts one optional command-line argument for the port. If no argument is provided, the default port is `8080`.

```bash
# Run the server on the default or specified port
./build/server [port]
```

### Client

The client accepts two optional command-line arguments: the server IP and port. If no arguments are passed, the default server IP is `127.0.0.1`, and the default port is `8080`.

```bash
# Connect the client to the specified server IP and port
./build/client [server_ip] [port]
```

### Testing with `make run`

You can easily test the server and clients by using the `make run` command. This will open the server and two client instances in separate terminal windows:

```bash
make run
```

## Example Usage

```bash
# Running the server on port 8080
./build/server 8080

# Connecting the client to server at 127.0.0.1 on port 8080
./build/client 127.0.0.1 8080
```
