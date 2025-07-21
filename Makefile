# Compiler and flags
CC = gcc
CFLAGS = -I./include -DCURRENT_LOG_LEVEL=LOG_LEVEL_DEBUG
LIBS = -lmicrohttpd -lcjson -lpthread -lopen62541 -lpaho-mqtt3c -lrt

# Directories
SRC_DIR = src
BIN_DIR = bin
CONFIG_DIR = metadata

# Sources and objects
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/rest_server.c $(SRC_DIR)/json_utils.c \
       $(SRC_DIR)/opcua_client.c $(SRC_DIR)/mqtt.c $(SRC_DIR)/mqtt_queue.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SRCS))
TARGET = $(BIN_DIR)/iot_connector

.PHONY: all clean run install uninstall

# Build target
all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LIBS) -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)

# Run the built binary
run: $(TARGET)
	./$(TARGET)

# Define paths
PREFIX ?= /usr/local
ETCDIR ?= /etc/opcua_connector
SERVICE_FILE = systemd/opcua_connector.service
SYSTEMD_DIR = /etc/systemd/system
RSYSLOG_CONF=/etc/rsyslog.d/iot_connector.conf
LOG_FILE=/var/log/iot_connector.log


# Install the binary and config files
install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(ETCDIR)/metadata
	mkdir -p $(DESTDIR)$(ETCDIR)/certs
	cp bin/iot_connector $(DESTDIR)$(PREFIX)/bin/
	cp scripts/opcua_wrapper.sh $(DESTDIR)$(PREFIX)/bin/
	cp scripts/config.sh $(DESTDIR)$(ETCDIR)/
	chmod +x $(DESTDIR)$(PREFIX)/bin/opcua_wrapper.sh
	# Install systemd service
	cp $(SERVICE_FILE) $(SYSTEMD_DIR)/
	# Reload systemd and enable service
	systemctl daemon-reexec
	systemctl daemon-reload
	systemctl enable --now opcua_connector.service

	@echo "🔧 Setting up rsyslog for iot_connector logging..."
	@sudo sh -c 'echo "if \$programname == '\''iot_connector'\'' then $(LOG_FILE)\n& stop" > $(RSYSLOG_CONF)'
	@sudo touch $(LOG_FILE)
	@sudo chmod 644 $(LOG_FILE)
	@sudo systemctl restart rsyslog
	@echo "✅ Logging will appear in: $(LOG_FILE)"
	cat /etc/rsyslog.d/iot_connector.conf

# Uninstall everything
uninstall:
	# Stop and disable systemd service
	-systemctl stop opcua_connector.service
	-systemctl disable opcua_connector.service
	# Remove the systemd service file
	-rm -f $(SYSTEMD_DIR)/opcua_connector.service
	# Reload systemd daemon
	-systemctl daemon-reexec
	-systemctl daemon-reload
	# Remove installed files
	-rm -f $(PREFIX)/bin/iot_connector
	-rm -f $(PREFIX)/bin/opcua_wrapper.sh
	-rm -rf $(ETCDIR)
	# Remove rsyslog config and log file
	-rm -f /etc/rsyslog.d/iot_connector.conf
	-rm -f /var/log/iot_connector.log
