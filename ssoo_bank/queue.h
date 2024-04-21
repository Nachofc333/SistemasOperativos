#ifndef HEADER_FILE
#define HEADER_FILE
#include <pthread.h>

struct element {
char type[20];
int params[3];
};

typedef struct queue {
int leng;
int size;
int head;
int tail;

struct element *data;
pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct element* elem);
struct element * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif

