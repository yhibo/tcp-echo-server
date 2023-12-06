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
	@command -v tmux >/dev/null 2>&1 || { echo >&2 "tmux not installed. Aborting."; exit 1; }
	@tmux new-session -d -s my_session './build/server 8080; read'
	@tmux split-window -t my_session -h './build/client 127.0.0.1 8080; read'
	@tmux split-window -t my_session -h './build/client 127.0.0.1 8080; read'
	@tmux -2 attach-session -t my_session
