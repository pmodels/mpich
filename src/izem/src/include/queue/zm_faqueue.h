/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_FAQUEUE_H
#define _ZM_FAQUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include "queue/zm_queue_types.h"

int zm_faqueue_init(zm_faqueue_t *);
int zm_faqueue_enqueue(zm_faqueue_t* q, void *data);
int zm_faqueue_dequeue(zm_faqueue_t* q, void **data);

#endif /* _ZM_FAQUEUE_H */
