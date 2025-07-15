# opcua_connector

sudo dnf install opcua library
sudo dnf install libmicrohttpd-devel
sudo dnf install cJSON-devel
sudo dnf install rsyslog

CURRENT_LOG_LEVEL = LOG_LEVEL_DEBUG
This is the lowest level, so everything will print:
    ✅ log_debug()
    ✅ log_info()
    ✅ log_warn()
    ✅ log_error()
CURRENT_LOG_LEVEL = LOG_LEVEL_INFO
This disables debug, allows the rest:
    ❌ log_debug() — not shown
    ✅ log_info()
    ✅ log_warn()
    ✅ log_error()
CURRENT_LOG_LEVEL = LOG_LEVEL_WARN
    ❌ log_debug()
    ❌ log_info()
    ✅ log_warn()
    ✅ log_error()
CURRENT_LOG_LEVEL = LOG_LEVEL_ERROR
    ❌ log_debug()
    ❌ log_info()
    ❌ log_warn()
    ✅ log_error()

✅ If You Want to Log to a File (like /var/log/iot_connector.log)

If you want a traditional text file in /var/log, here’s how to do it:
🔧 Step 1: Create a rsyslog rule

Create a new file:

sudo vi /etc/rsyslog.d/iot_connector.conf

Paste this content:

if $programname == 'iot_connector' then /var/log/iot_connector.log
& stop

🔧 Step 2: Restart rsyslog

sudo systemctl restart rsyslog

✅ Step 3: Check log file

Now run your app, and then check:

tail -f /var/log/iot_connector.log

You’ll see the same logs appear there.



# Debug
journalctl -t iot_connector -f

# User input
curl -X POST http://localhost:8080/deviceconfigure \
  -H "Content-Type: application/json" \
  -d '{
    "device_name": "cnc_device_001",
    "opcua": {
      "endpoint_url": "opc.tcp://192.168.1.100:4840",
      "username": "opcuser",
      "password": "opcpass"
    },
    "mqtt": {
      "broker_url": "mqtts://broker.example.com:8883",
      "username": "mqttuser",
      "password": "mqttpass",
      "publish_interval_ms": 1000,
      "base_topic": "factory/cnc/device001",
      "tls_enabled": true,
      "certificate_path": "/tmp/device001-cert.pem",
      "certificate_content": "-----BEGIN CERTIFICATE-----\nMIIDXTCCAkWgAwIBAgI...YourCAHere...\n-----END CERTIFICATE-----"
    },
    "data_points": [
      {
        "namespace": 3,
        "identifier": 1001,
        "datatype": "int32",
        "alias": "spindle_speed"
      },
      {
        "namespace": 3,
        "identifier": 1002,
        "datatype": "float",
        "alias": "motor_temp"
      },
      {
        "namespace": 3,
        "identifier": 1003,
        "datatype": "boolean",
        "alias": "alarm_status"
      }
    ]
  }'

