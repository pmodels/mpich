/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_SWPQUEUE_H
#define _ZM_SWPQUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include "queue/zm_queue_types.h"

int zm_swpqueue_init(zm_swpqueue_t *);
int zm_swpqueue_enqueue(zm_swpqueue_t* q, void *data);
int zm_swpqueue_dequeue(zm_swpqueue_t* q, void **data);

#endif /* _ZM_SWPQUEUE_H */
