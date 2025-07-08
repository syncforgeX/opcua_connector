/*
compile:
gcc sample_data_dynamic.c -o sample_data -lopen62541

execution:
./sample_data <ns> <id> <datatype>

ex:
./sample_data 3 1001 int32
*/

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if(argc < 4) {
        printf("Usage: %s <namespace> <identifier> <datatype>\n", argv[0]);
        printf("Example: %s 3 1001 int32\n", argv[0]);
        return 1;
    }

    int ns = atoi(argv[1]);
    int id = atoi(argv[2]);
    const char *datatype = argv[3];

    UA_Client *client = UA_Client_new();
    UA_ClientConfig *config = UA_Client_getConfig(client);
   // *config = UA_ClientConfig_default;

    UA_StatusCode status = UA_Client_connect(client, "opc.tcp://fedora:53530/OPCUA/SimulationServer");
    if(status != UA_STATUSCODE_GOOD) {
        printf("Failed to connect: %s\n", UA_StatusCode_name(status));
        UA_Client_delete(client);
        return 1;
    }

    printf("Connected to OPC UA server successfully!\n");

    UA_NodeId nodeId = UA_NODEID_NUMERIC(ns, id);
    UA_Variant value;
    UA_Variant_init(&value);

    status = UA_Client_readValueAttribute(client, nodeId, &value);
    if(status != UA_STATUSCODE_GOOD) {
        printf("Read failed: %s\n", UA_StatusCode_name(status));
    } else if(strcmp(datatype, "int32") == 0 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_INT32])) {
        printf("Int32 value: %d\n", *(UA_Int32 *)value.data);
    } else if(strcmp(datatype, "float") == 0 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_FLOAT])) {
        printf("Float value: %f\n", *(UA_Float *)value.data);
    } else if(strcmp(datatype, "double") == 0 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_DOUBLE])) {
        printf("Double value: %lf\n", *(UA_Double *)value.data);
    } else if(strcmp(datatype, "string") == 0 && UA_Variant_hasScalarType(&value, &UA_TYPES[UA_TYPES_STRING])) {
        UA_String str = *(UA_String *)value.data;
        printf("String value: %.*s\n", (int)str.length, str.data);
    } else {
        printf("Data type mismatch or unsupported type: %s\n", datatype);
    }

    UA_Variant_clear(&value);
    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return 0;
}

