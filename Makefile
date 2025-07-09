# Variables
CC = gcc
CFLAGS = -I./include -DCURRENT_LOG_LEVEL=LOG_LEVEL_DEBUG
LIBS = -lmicrohttpd -lcjson -lpthread

# Directories
SRC_DIR = src
BIN_DIR = bin

# Source and object files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/rest_server.c $(SRC_DIR)/json_utils.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

# Target executable
TARGET = $(BIN_DIR)/iot_connector

# Default rule
all: $(BIN_DIR) $(TARGET)

# Build target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@

# Build object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean up build files
clean:
	rm -rf $(BIN_DIR)

# Run the binary
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

