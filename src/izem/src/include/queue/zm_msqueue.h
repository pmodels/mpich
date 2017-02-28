/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef _ZM_MSQUEUE_H
#define _ZM_MSQUEUE_H
#include <stdlib.h>
#include <stdio.h>
#include "queue/zm_queue_types.h"

int zm_msqueue_init(zm_msqueue_t *);
int zm_msqueue_enqueue(zm_msqueue_t* q, void *data);
int zm_msqueue_dequeue(zm_msqueue_t* q, void **data);

#endif /* _ZM_MSQUEUE_H */
