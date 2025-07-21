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

# 🧠 OPC UA MQTT IoT Connector

This project provides an IoT connector that bridges **OPC UA devices** to an **MQTT broker**. It is designed to run as a systemd service on Linux systems, allowing automatic startup and monitoring.

---

## 📆 Prerequisites

* GCC / Make
* `systemd` (for running as a service)
* `mosquitto` or other MQTT broker (locally or remotely)
* Access to build tools and necessary libraries (e.g., `libpaho-mqtt`, `open62541`, etc.)

---

## ⚙️ Build and Install

Run the following commands to build and install the connector:

```bash
make clean
make
sudo make uninstall
sudo make install
```

### 🛠 What Each Command Does

* `make clean`
  Cleans any previous builds.

* `make`
  Compiles the application and generates the binary.

* `sudo make uninstall`
  Removes installed files (binary, config, systemd service).

* `sudo make install`
  Installs the binary, config, wrapper script, and:

  * Creates `/etc/opcua_connector/metadata/` and `/etc/opcua_connector/certs/` directories
  * Copies config files to `/etc/opcua_connector/`
  * Installs the systemd service file
  * Reloads systemd daemon
  * Enables and starts the service

---

## 📂 Installed Paths

| Component           | Location                                      |
| ------------------- | --------------------------------------------- |
| Binary              | `/usr/local/bin/iot_connector`                |
| Wrapper Script      | `/usr/local/bin/opcua_wrapper.sh`             |
| Config File         | `/etc/opcua_connector/config.sh`              |
| Metadata Folder     | `/etc/opcua_connector/metadata/`              |
| Certificates Folder | `/etc/opcua_connector/certs/`                 |
| Systemd Service     | `/etc/systemd/system/opcua_connector.service` |

---

## 🔁 Running as a Service

The `make install` process will automatically:

* Place the service file at `/etc/systemd/system/opcua_connector.service`
* Reload systemd
* Enable the service at boot
* Start the service immediately

To manually control the service:

```bash
# Check status
sudo systemctl status opcua_connector.service

# Restart the service
sudo systemctl restart opcua_connector.service

# Stop the service
sudo systemctl stop opcua_connector.service

# View logs
journalctl -fu opcua_connector.service
```

---

## 🧽 Uninstallation

To remove everything cleanly:

```bash
sudo make uninstall
```

This removes:

* `/usr/local/bin/iot_connector`
* `/usr/local/bin/opcua_wrapper.sh`
* `/etc/opcua_connector/`
* `/etc/systemd/system/opcua_connector.service`

## 🛠️ Debugging Section – Recommended Content
1. Enable Systemd Logging

```bash
# View service logs (real-time)
journalctl -u opcua_connector.service -f

# View full log history
journalctl -u opcua_connector.service

# Check if service is failing
systemctl status opcua_connector.service
```
## MQTT Debugge
```bash
mqttx sub -h localhost -p 1883 -t test/topic
```
