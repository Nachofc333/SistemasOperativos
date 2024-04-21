#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"

// Crea la cola
queue *queue_init(int size)
{
  queue *q = (queue *)malloc(sizeof(queue));
  q->size = size;
  q->leng = 0;
  q->head = 0;
  q->tail = 0;
  q->data = malloc(q->size * sizeof(struct element));
  return q;
}

// Agregar un element a la cola
int queue_put(queue *q, struct element* x) {
    int next;
    next = q->head+1;
    if (next >= q->size){
        next=0;
    }
    if ((q->leng)==q->size){
        return -1;
    }
    q->leng = q->leng+1;
    q->data[q->head]=*x;
    q->head = next;
    return 0;
}


// Retira un elemento de la cola
struct element *queue_get(queue *q) {
    struct element *elem;
    if (queue_empty(q) == 0) {
        elem = &(q->data[q->tail]);
    if (q->tail+1 == q->size){
      q->tail=0;
    }
    else{
      q->tail = q->tail+1;
    }
        q->leng = q->leng - 1;
    }
    return elem;
}

// Cromprueba que la cola esté vacía
int queue_empty(queue *q) {
  if (q->leng == 0){
      return -1;
  }
  return 0;
}
int queue_full(queue *q){
  if (q->size == q->leng){
      return -1;
  }
  return 0;
}
// Elimina la memoria
int queue_destroy(queue *q){
  free(q->data);
  free(q);
  return 0;
}

