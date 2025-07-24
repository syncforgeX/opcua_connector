#!/bin/bash

# Load runtime environment variables
#source /etc/opcua_connector/config.sh

# Log start time (optional)
echo "🟢 Starting OPC UA MQTT Connector at $(date)"

# Run the installed binary
exec /usr/local/bin/iot_connector

