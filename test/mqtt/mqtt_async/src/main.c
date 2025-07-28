#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client.h"  // Your .h file
#include <unistd.h>			  //
#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    DeviceConfig config = {0};

    // Initialize with test values
    config.device_name = "cnc_device_002";
    config.mqtt.messagebus.protocol = "ssl";
    config.mqtt.messagebus.host = "10.20.32.132";
    config.mqtt.messagebus.port = 8883;
    config.mqtt.messagebus.clientid = "LocalClient1234";
    config.mqtt.messagebus.qos = 1;
    config.mqtt.messagebus.keepalive = 60;
    config.mqtt.messagebus.retained = false;
    config.mqtt.messagebus.certfile = "./certs/cacert.pem";
    config.mqtt.messagebus.keyfile = "./certs/clientkey.pem";
    config.mqtt.messagebus.skipverify = false;
    config.mqtt.messagebus.basetopicprefix = "test/topic";
    config.mqtt.messagebus.type = "mqtt";
    config.mqtt.messagebus.authmode = "usernamepassword";
    config.mqtt.messagebus.secretname = "mqtt-secret";
	
    printf("hello\n");
    // Initialize MQTT
    edgex_bus_t *bus = mqtt_client_init(&config, "test-service");
    if (bus == NULL) {
        fprintf(stderr, "❌ MQTT client initialization failed.\n");
        return EXIT_FAILURE;
    }

    printf("✅ MQTT client initialized successfully.\n");

    // Example usage:
    while(1){
    bus->postfn(bus, "test/topic/hello", "{\"msg\": \"Hello from test client!\"}");
   usleep(50000); // 50ms
    }
    // Clean up
    bus->freefn(bus);

    return EXIT_SUCCESS;
}

