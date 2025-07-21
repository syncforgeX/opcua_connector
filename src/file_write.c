#include"mqtt.h"
#include <stdio.h>

static pthread_t file_tid;
extern volatile char file_flag ;

void write_payloads_to_file(Queue *queue) {
    FILE *file = fopen(FILE_PATH, "r+"); // Open file in read/write mode
    if (file == NULL) {
        perror("Failed to open JSON file");
        return;
    }   

    // Check if the file is empty or already contains a JSON array
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    if (file_size == 0) {
        // If the file is empty, write the opening square bracket
        fseek(file, 0, SEEK_SET);
        fprintf(file, "[\n");
    } else {
        // If the file already contains data, go back to the last character (the closing bracket)
        fseek(file, -2, SEEK_END); // Move back two characters from EOF
        fprintf(file, ",\n");
    }   

    // Write the new payloads
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
        fprintf(file, "  %s%s\n", queue->elements[i].json_data, (i < QUEUE_CAPACITY - 1) ? "," : "");
    }   

    // Close the JSON array if this is the last write
    fprintf(file, "]\n");
    fclose(file);
    printf("%d JSON payloads written to file.\n", QUEUE_CAPACITY);
}
// Function to read JSON payloads from the file
json_object* read_payloads_from_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Memory allocation error");
        fclose(file);
        return NULL;
    }
/*
        // Allocate a fixed-size buffer to hold the file contents
    char buffer[MAX_FILE_SIZE];
    
    // Read the file into the buffer
    size_t bytes_read = fread(buffer, 1, MAX_FILE_SIZE - 1, file);
    if (bytes_read == 0) {
        fclose(file);
        return NULL;
    }
        
*/
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0';
    fclose(file);

    json_object *data = json_tokener_parse(buffer);
    free(buffer);

    if (data == NULL) {
        fprintf(stderr, "Error parsing JSON or file empty\n");
    }

    return data;
}
int delete_file_content(const char *file_path) {
    // Open the file in write mode ("w") to truncate it
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error opening file");
        return -1;  // Return an error if the file cannot be opened
    }

    // No need to write anything, just truncating the file
    fclose(file);

    printf("File content deleted successfully.\n");
    return 0;  // Success
}

// Function to publish payloads to the MQTT broker
void publish_payloads_to_mqtt(json_object *payloads) {
        if (payloads == NULL) {
                printf("No payloads to publish.\n");
                return;
        }

        MQTTClient_message pubmsg = MQTTClient_message_initializer;
        char ret;

        // Loop through each JSON object in the array
        size_t num_payloads = json_object_array_length(payloads);
        for (size_t i = 0; i < num_payloads; i++) {
                printf("buffer ");
                json_object *payload = json_object_array_get_idx(payloads, i);
                const char *json_payload = json_object_to_json_string(payload);
                usleep(1000);
                publish_message(json_payload);
        }
                printf("All payloads published successfully\n");
}

void *file_handler(void *arg){
        while(1){
                if(file_flag){
                        printf("to readddddd file content\n");
                        // Read payloads from the file
                        json_object *payloads = read_payloads_from_file(FILE_PATH);

                        // Publish the payloads to MQTT
                        if (payloads) {
                                publish_payloads_to_mqtt(payloads);
                                json_object_put(payloads); // Free JSON object
                                delete_file_content(FILE_PATH);
                        }
                        file_flag = CLEAR;

                }
        }
}

bool readfile_publish_init(){

        if (pthread_create(&file_tid, NULL, file_handler,NULL) != 0) {
                perror("Failed to create focas_tid");
                return ENOT_OK;
        }
        return E_OK;
}
