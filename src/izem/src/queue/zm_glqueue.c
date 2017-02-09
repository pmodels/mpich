/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "queue/zm_glqueue.h"

int zm_glqueue_init(zm_glqueue_t *q) {
    zm_glqnode_t* node = (zm_glqnode_t*) malloc(sizeof(zm_glqnode_t));
    node->data = NULL;
    node->next = ZM_NULL;
    pthread_mutex_init(&q->lock, NULL);
    q->head = (zm_ptr_t)node;
    q->tail = (zm_ptr_t)node;
    return 0;
}

int zm_glqueue_enqueue(zm_glqueue_t* q, void *data) {
    /* allocate a new node */
    zm_glqnode_t* node = (zm_glqnode_t*) malloc(sizeof(zm_glqnode_t));
    /* set the data and next pointers */
    node->data = data;
    node->next = ZM_NULL;
    /* acquire the global lock */
    pthread_mutex_lock(&q->lock);
    /* chain the new node to the tail */
    ((zm_glqnode_t*)q->tail)->next = (zm_ptr_t)node;
    /* update the tail to point to the new node */
    q->tail = (zm_ptr_t)node;
    /* release the global lock */
    pthread_mutex_unlock(&q->lock);
    return 0;
}

int zm_glqueue_dequeue(zm_glqueue_t* q, void **data) {
    zm_glqnode_t* head;
    /* acquire the global lock */
    pthread_mutex_lock(&q->lock);
    head = (zm_glqnode_t*)q->head;
    *data = NULL;
    /* move forward the head pointer in case the queue is not empty */
    if(head->next != ZM_NULL) {
        q->head = head->next;
         /* return the data pointer of the current head */
        *data = ((zm_glqnode_t*)q->head)->data;
        free(head);
    }
    /* release the global lock */
    pthread_mutex_unlock(&q->lock);
    return 1;
}
