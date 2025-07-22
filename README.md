# OPC UA MQTT IoT Connector

An edge connector for ingesting data from OPC UA servers and publishing it over MQTT. Includes a REST API for dynamic configuration.

---

## ✨ Features

* OPC UA Client integration using `open62541`
* MQTT publishing using Eclipse Paho
* REST API using `libmicrohttpd`
* TLS support for MQTT
* Systemd integration for auto-start
* Centralized logging with rsyslog

---

## 🔧 Installation

### Fedora

```bash
sudo dnf install libmicrohttpd-devel cJSON-devel rsyslog
```

### Ubuntu / Debian

#### Step 1: Install System Dependencies

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake \
  git \
  libssl-dev \
  libmicrohttpd-dev
```

#### Step 2: Build and Install open62541

```bash
git clone https://github.com/open62541/open62541.git
cd open62541
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

#### Step 3: Build and Install Paho MQTT C Client

```bash
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git checkout v1.3.13
mkdir build && cd build
cmake -DPAHO_WITH_SSL=TRUE ..
make -j$(nproc)
sudo make install
sudo ldconfig
```

---

## 🎮 Build and Install Connector

```bash
make clean
make
sudo make uninstall
sudo make install
```

This will:

* Build the binary
* Copy files to appropriate system paths
* Register and start the systemd service

---

## 📁 Installed Paths

| Component           | Location                                      |
| ------------------- | --------------------------------------------- |
| Binary              | `/usr/local/bin/iot_connector`                |
| Wrapper Script      | `/usr/local/bin/opcua_wrapper.sh`             |
| Config File         | `/etc/opcua_connector/config.sh`              |
| Metadata Folder     | `/etc/opcua_connector/metadata/`              |
| Certificates Folder | `/etc/opcua_connector/certs/`                 |
| Systemd Service     | `/etc/systemd/system/opcua_connector.service` |

---

## 🌐 REST API Usage

### Configure Device (POST)

```bash
curl -X POST http://localhost:8080/deviceconfigure \
  -H 'Content-Type: application/json' \
  -d '{
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
      "certificate_content": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----"
    },
    "data_points": [
      {"namespace": 3, "identifier": 1001, "datatype": "int32", "alias": "spindle_speed"},
      {"namespace": 3, "identifier": 1002, "datatype": "float", "alias": "motor_temp"},
      {"namespace": 3, "identifier": 1003, "datatype": "boolean", "alias": "alarm_status"}
    ]
  }'
```

### View Devices (GET)

```bash
curl http://localhost:8080/deviceconfig
```

### Delete Device (DELETE)

```bash
curl -X DELETE http://localhost:8080/deviceconfig/cnc_device_002
```

---

## 🎓 Logging

### Log Levels

Set in `config.h`:

| Level             | Includes Logs            |
| ----------------- | ------------------------ |
| LOG\_LEVEL\_DEBUG | debug, info, warn, error |
| LOG\_LEVEL\_INFO  | info, warn, error        |
| LOG\_LEVEL\_WARN  | warn, error              |
| LOG\_LEVEL\_ERROR | error only               |

### Log to File via rsyslog

#### Step 1: Create Log File and Set Permissions

```bash
sudo rm /var/log/iot_connector.log
sudo touch /var/log/iot_connector.log
sudo chown syslog:adm /var/log/iot_connector.log
sudo chmod 644 /var/log/iot_connector.logLog to File via rsyslog
```

#### Step 2: Create rsyslog Rule

```bash
sudo vi /etc/rsyslog.d/iot_connector.conf
```

Add:

```
if $programname == 'iot_connector' then /var/log/iot_connector.log
& stop
```

#### Step 3: Restart rsyslog

```bash
sudo systemctl restart rsyslog
```

#### Step 4: View Logs

```bash
tail -f /var/log/iot_connector.log
# or
journalctl -t iot_connector -f
```

---

## 🔄 Systemd Service

### Control Commands

```bash
sudo systemctl status opcua_connector.service
sudo systemctl restart opcua_connector.service
sudo systemctl stop opcua_connector.service
journalctl -u opcua_connector.service -f
```

---

## 🗑 Uninstall

```bash
sudo make uninstall
```

This removes:

* Binary and wrapper
* Config and metadata
* Systemd service file

---

## 🔎 MQTT Debugging

```bash
mqttx sub -h localhost -p 1883 -t test/topic
```
