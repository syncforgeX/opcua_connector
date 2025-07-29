#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mqtt_client.h"  // Your .h file
#include <unistd.h>			  //
#include "mqtt_client.h"
#include <stdio.h>
#include <stdlib.h>

#define CA_CERT     "./certs/cacert.pem"
#define CLIENT_CERT "./certs/clientcert.pem"
#define CLIENT_KEY  "./certs/clientkey.pem"

int main() {
    DeviceConfig config = {0};

    // Initialize with test values
    config.device_name = "cnc_device_002";
    config.mqtt.messagebus.protocol = "ssl";
    config.mqtt.messagebus.host = "10.20.32.132";
    config.mqtt.messagebus.port = 8883;
    config.mqtt.messagebus.clientid = "LocalClient1234";
    config.mqtt.messagebus.qos = 1;
    config.mqtt.messagebus.keepalive = 15;
    config.mqtt.messagebus.cleansession = false;
    config.mqtt.messagebus.retained = true;
    config.mqtt.messagebus.certfile = CA_CERT;
    config.mqtt.messagebus.keyfile = CLIENT_CERT;
    config.mqtt.messagebus.privateKey = CLIENT_KEY;
    config.mqtt.messagebus.skipverify = false;
    config.mqtt.messagebus.basetopicprefix = "test/topic";
    config.mqtt.messagebus.authmode = "usernamepassword";
    config.mqtt.messagebus.buffer_msg = 1000;

	
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

	    usleep(100);
    }
    // Clean up
    bus->freefn(bus);
    return EXIT_SUCCESS;
}

