# OPC UA Connector

An edge connector for ingesting data from OPC UA servers and publishing it over MQTT. Also includes a REST API for dynamic configuration.

---

## 🔧 Installation (Fedora)

Install the required libraries:

```bash
sudo dnf install opcua library
sudo dnf install libmicrohttpd-devel
sudo dnf install cJSON-devel
sudo dnf install rsyslog
```

---

## 🪵 Logging Levels

Set the desired logging level using `CURRENT_LOG_LEVEL`.

| Level              | Enabled Logs                             |
|-------------------|------------------------------------------|
| `LOG_LEVEL_DEBUG` | ✅ `log_debug()`<br>✅ `log_info()`<br>✅ `log_warn()`<br>✅ `log_error()` |
| `LOG_LEVEL_INFO`  | ❌ `log_debug()`<br>✅ `log_info()`<br>✅ `log_warn()`<br>✅ `log_error()` |
| `LOG_LEVEL_WARN`  | ❌ `log_debug()`<br>❌ `log_info()`<br>✅ `log_warn()`<br>✅ `log_error()` |
| `LOG_LEVEL_ERROR` | ❌ `log_debug()`<br>❌ `log_info()`<br>❌ `log_warn()`<br>✅ `log_error()` |

---

## 📁 Logging to File

If you want to store logs in a traditional log file such as `/var/log/iot_connector.log`:

### 1️⃣ Create rsyslog rule

```bash
sudo vi /etc/rsyslog.d/iot_connector.conf
```

Paste the following content:

```
if $programname == 'iot_connector' then /var/log/iot_connector.log
& stop
```

### 2️⃣ Restart rsyslog

```bash
sudo systemctl restart rsyslog
```

### 3️⃣ View Logs

```bash
tail -f /var/log/iot_connector.log
```

Alternatively, view via `journalctl`:

```bash
journalctl -t iot_connector -f
```

---

## 🧪 API Usage

### 🔹 POST: Configure Device

```bash
curl --location 'http://localhost:8080/deviceconfigure' \
--header 'Content-Type: application/json' \
--data '{
  "device_name": "cnc_device_002",
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
```

### 🔹 GET: View Configured Devices

```bash
curl --location --request GET 'http://10.20.32.132:8081/deviceconfig'
```

### 🔹 DELETE: Remove a Device

```bash
curl --location --request DELETE 'http://localhost:8082/deviceconfig/cnc_device_002'
```

