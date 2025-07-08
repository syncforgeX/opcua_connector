
# Variables
CC = gcc
CFLAGS = -I./include/
LIBS = -lmicrohttpd -lpthread -lcjson -ltaos

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source and object files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/thread.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

# Target executable
TARGET = $(BIN_DIR)/data_collector

# Docker image name and version
DOCKER_IMAGE = data_collector
DOCKER_VERSION = $(shell cat VERSION)
DOCKER_TAG = $(DOCKER_IMAGE):$(DOCKER_VERSION)

# Default rule
all: $(BIN_DIR) $(TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LIBS) -o $@

# Build object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Build Docker image with version
docker:
	docker build -t $(DOCKER_TAG) .

# Clean up build files
clean:
	rm -rf $(BIN_DIR) $(TARGET)

# Run the binary
run: $(TARGET)
	./$(TARGET)
.PHONY: all clean docker

