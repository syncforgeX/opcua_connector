#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_config_default.h>
#include <stdio.h>

int main(void) {
	// Create and configure client
	UA_Client *client = UA_Client_new();
	UA_ClientConfig_setDefault(UA_Client_getConfig(client));

	// Connect to server
	UA_StatusCode status = UA_Client_connect(client, "opc.tcp://fedora:53530/OPCUA/SimulationServer");
	if(status != UA_STATUSCODE_GOOD) {
		printf("Failed to connect: %s\n", UA_StatusCode_name(status));
		UA_Client_delete(client);
		return 1;
	}

	printf("Connected to OPC UA server successfully!\n");

	// Prepare to read NodeId: ns=3;i=1001
	UA_NodeId nodeId = UA_NODEID_NUMERIC(3, 1001);
	UA_Variant value;
	UA_Variant_init(&value);

	status = UA_Client_readValueAttribute(client, nodeId, &value);
	if(status == UA_STATUSCODE_GOOD && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
		UA_Int32 val = *(UA_Int32*)value.data;
		printf("Read ns=3;i=1001 → Int32 value: %d\n", val);
	} else {
		printf("Failed to read or incorrect type: %s\n", UA_StatusCode_name(status));
	}

	// Cleanup
	UA_Variant_clear(&value);
	UA_Client_disconnect(client);
	UA_Client_delete(client);
	return 0;
}
