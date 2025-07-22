#include"mqtt.h"
#include"log_utils.h"

/* define variable to handle mqtt queue data */
Queue queue ={0};

/**
 * @brief: Initializes the mqtt queue to its default empty state.
 *
 * @param: Queue *queue Pointer to the mqtt queue structure to initialize.
 */
void MqttQueue_init(Queue *queue) {
        queue->front = CLEAR;
        queue->rear = -1; 
        queue->size = CLEAR;
}
/**
 * @brief: Checks if the mqtt queue is full.
 *
 * @param: Queue *queue Pointer to the queue structure to check.
 * @return: 1 if the queue is full, 0 otherwise.
 */
static int isFull(Queue *queue) {
        return queue->size == QUEUE_CAPACITY;
}
/**
 * @brief: Checks if the mqtt queue is empty.
 *
 * @param: queue Pointer to the mqtt queue structure to check.
 * @return: 1 if the queue is empty, 0 otherwise.
 */
int isEmpty(Queue *queue) {
        return queue->size == 0;
}
/**
 * @brief: Adds a new JSON record to the mqtt queue.
 *
 * @param: Queue *queue Pointer to the mqtt queue structure.
 const char *json_data JSON string to enqueue.
 * @return: 0 on success, -1 if the queue is full.
 */
int enqueue(Queue *queue, const char *json_data) {
        if (isFull(queue)) {
                //write_payloads_to_file(queue);
                MqttQueue_init(queue);//reset 
                log_debug("Queue is full...to write in the file\n");
                //return -1;
        }   
        log_debug("current queue size%d\n ",queue->size);

	queue->rear = (queue->rear + 1) % QUEUE_CAPACITY;
        strncpy(queue->elements[queue->rear].json_data, json_data, MAX_JSON_SIZE - 1); 
//        log_debug("enqeue data %s\n",queue->elements[queue->rear].json_data);
        queue->elements[queue->rear].json_data[MAX_JSON_SIZE - 1] = '\0';
        queue->size++;
        return E_OK;
}
/**
 * @brief: Removes the oldest JSON record from the mqtt queue.
 *
 * @param: Queue *queue Pointer to the mqtt queue structure.
 char *buffer Buffer to store the dequeued JSON string.
 * @return: 0 on success, -1 if the mqtt queue is empty.
 */
int dequeue(Queue *queue, char *buffer) {
        if (isEmpty(queue)) {
                log_error("MQ_ERR: Queue is empty...\n");
                return ENOT_OK;
        }
  //      log_debug("dequeue queue=%s\n",queue->elements[queue->front].json_data);
        strncpy(buffer, queue->elements[queue->front].json_data, MAX_JSON_SIZE);
        buffer[MAX_JSON_SIZE - 1] = '\0';
        queue->front = (queue->front + 1) % QUEUE_CAPACITY;
        queue->size--;
        return E_OK;
}
 
