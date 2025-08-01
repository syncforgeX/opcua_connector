# OPC UA MQTT IoT Connector

An edge connector for ingesting data from OPC UA servers and publishing it over MQTT. Includes a REST API for dynamic configuration.

---

## ✨ Features

* OPC UA Client integration using `open62541` open62541 v1.4.12.
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
curl --location 'http://10.20.32.132:8080/deviceconfigure' \
--header 'Content-Type: application/json' \
--data '{
    "device_name": "cnc_device_002",
    "opcua": {
        "endpoint_url": "opc.tcp://10.20.32.132:53530/OPCUA/SimulationServer",
        "username": "opcuser",
        "password": "opcpass"
    },
    "messagebus": {
        "protocol": "ssl",
        "host": "10.20.32.132",
        "port": 8883,
        "clientid": "LocalClient1234",
        "qos": 1,
        "keepalive": 15,
        "cleansession": false,
        "retained": true,
        "certfile": "./certs/cacert.pem", 
        "keyfile": "./certs/clientcert.pem",
        "privateKey": "./certs/clientkey.pem",
        "skipverify": false,
        "basetopicprefix": "test/topic",
        "authmode": "usernamepassword",
        "buffer_msg": 1000
    },
    "data_points": [
        {
            "namespace": 3,
            "identifier": 1007,
            "datatype": "Double",
            "alias": "Constant"
        },
        {
            "namespace": 3,
            "identifier": 1001,
            "datatype": "int32",
            "alias": "Count"
        },
        {
            "namespace": 3,
            "identifier": 1002,
            "datatype": "Double",
            "alias": "Random"
        }
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
mqttx pub -h localhost -p 1883 -t test/topic -m '{"status": "running", "ts": 1721211234567}'
```

```bash
mqttx sub -h localhost -p 1883 -t test/topic
```

export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH
