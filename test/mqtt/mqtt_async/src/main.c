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
    config.mqtt.messagebus.retained = true;
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


    char msg[256];
    time_t now;
    struct tm *tm_info;

    // Example usage:

    while (1)
    {
	    now = time(NULL);
	    tm_info = localtime(&now);

	    char timestamp[64];
	    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

	    snprintf(msg, sizeof(msg), "{\"msg\": \"Hello from test client!\", \"timestamp\": \"%s\"}", timestamp);

	    bus->postfn(bus, "test/topic/hello", msg);

	    sleep(1);
    }
    // Clean up
    bus->freefn(bus);
    return EXIT_SUCCESS;
}

