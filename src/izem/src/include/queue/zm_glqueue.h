/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_GLQUEUE_H
#define _ZM_GLQUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include "queue/zm_queue_types.h"

/* glqueue: concurrent queue where both enqueue and dequeue operations
 * are protected with the same global lock (thus, the gl prefix) */

int zm_glqueue_init(zm_glqueue_t *);
int zm_glqueue_enqueue(zm_glqueue_t* q, void *data);
int zm_glqueue_dequeue(zm_glqueue_t* q, void **data);

#endif /* _ZM_GLQUEUE_H */
