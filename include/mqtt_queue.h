#ifndef _MQTT_QUEUE_H_
#define _MQTT_QUEUE_H_

/* Define the structure for a queue element */
typedef struct {
    char json_data[MAX_JSON_SIZE];
} QueueElement;

/* Define the structure for the queue */
typedef struct {
    QueueElement elements[QUEUE_CAPACITY];
    int front;
    int rear;
    int size;
} Queue;

/* declared variable to hold mqtt queue data */
extern Queue queue;

/* Function prototype */
void MqttQueue_init(Queue *queue);
static int isFull(Queue *queue);
static int isEmpty(Queue *queue);
int enqueue(Queue *queue, const char *json_data);
int dequeue(Queue *queue, char *buffer);


#endif
