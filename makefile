# Compiler
CC = g++

# Flags
CFLAGS = -Wall -Wextra -std=c++17
RELEASEFLAGS = -O2 -DNDEBUG

# Directories
BUILDDIR = build

# Source files
COMMON_SRCS = src/common.cpp
CLIENT_SRCS = src/client.cpp $(COMMON_SRCS)
SERVER_SRCS = src/server.cpp $(COMMON_SRCS)

# Targets
.PHONY: all client server clean release run

all: client server

client: | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/client $(CLIENT_SRCS)

server: | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $(BUILDDIR)/server $(SERVER_SRCS)

release: CFLAGS += $(RELEASEFLAGS)
release: all

clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

run:
	@bash -c '(gnome-terminal -e "./build/server 8080" & disown) || \
	          (konsole -e "./build/server 8080" & disown) || \
	          (xterm -e "./build/server 8080" & disown) || \
	          echo "No known terminal emulator found." &'
	@bash -c '(gnome-terminal -e "./build/client 127.0.0.1 8080" & disown) || \
	          (konsole -e "./build/client 127.0.0.1 8080" & disown) || \
	          (xterm -e "./build/client 127.0.0.1 8080" & disown) || \
	          echo "No known terminal emulator found." &'
	@bash -c '(gnome-terminal -e "./build/client 127.0.0.1 8080" & disown) || \
	          (konsole -e "./build/client 127.0.0.1 8080" & disown) || \
	          (xterm -e "./build/client 127.0.0.1 8080" & disown) || \
	          echo "No known terminal emulator found." &'
