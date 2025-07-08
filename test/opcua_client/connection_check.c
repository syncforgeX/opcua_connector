#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <stdio.h>

int main() {
    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    UA_StatusCode status = UA_Client_connect(client, "opc.tcp://fedora:53530/OPCUA/SimulationServer");
    if (status != UA_STATUSCODE_GOOD) {
        printf("Connection failed: %s\n", UA_StatusCode_name(status));
        UA_Client_delete(client);
        return 1;
    }

    printf("Connected to OPC UA server successfully!\n");

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}

