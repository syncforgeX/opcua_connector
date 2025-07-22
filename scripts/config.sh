#!/bin/bash

# MQTT Timer Configuration
export MQTT_TIMER_INITIAL_START=2
export MQTT_INTERVAL=5  # 5 sec publish interval user need to provide minimum 1sec

# Reconnect Settings
export RECONNECT_CNT=4  # 20 attempts to reconnect

# FOCAS Timer Configuration
export OPCUA_TIMER_INITIAL_START=1
#export OPCUA_INTERVAL=1  # default 1 sec polling interval

export OPCUA_FILE_PATH="buffer_data/opcua_buffer.json"

# MQTT Configuration
export MQTT_BROKER="tcp://localhost:1883"
export MQTT_CLIENTID="LocalClient1234"
export MQTT_TOPIC="test/topic"
export MQTT_QOS=1
export MQTT_TIMEOUT=10000  # 10-second timeout
export MQTT_CERTS_PATH="/etc/opcua_connector/certs/mqtt_cert.pem"


#!/bin/bash

# Function to check if an environment variable is set
check_env_var() {
    if [ -z "${!1}" ]; then
        echo "❌ ERROR: $1 is not set!"
        MISSING_VARS=1
    else
        echo "✅ $1 = ${!1}"
    fi
}

echo "🔍 Checking required environment variables..."

# List of required environment variables
MISSING_VARS=0
check_env_var "MQTT_TIMER_INITIAL_START"
check_env_var "MQTT_INTERVAL"
check_env_var "RECONNECT_CNT"
check_env_var "OPCUA_FILE_PATH"
check_env_var "MQTT_BROKER"
check_env_var "MQTT_CLIENTID"
check_env_var "MQTT_TOPIC"
check_env_var "MQTT_QOS"
check_env_var "MQTT_TIMEOUT"

# If any variable is missing, exit the script
if [ "$MISSING_VARS" -ne 0 ]; then
    echo "❌ Exiting due to missing environment variables!"
    exit 1
fi

echo "✅ All required environment variables are set!"
echo "🚀 Starting the application..."


