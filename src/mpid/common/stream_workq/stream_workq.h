/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef MPIDU_STREAM_WORKQ_H_INCLUDED
#define MPIDU_STREAM_WORKQ_H_INCLUDED

#include "mpl.h"
#include "mpidu_genq_private_pool.h"
#include "utarray.h"

typedef void (MPIDU_stream_workq_cb_t) (void *data);

typedef struct MPIDU_stream_workq_op {
    MPIDU_stream_workq_cb_t *cb;
    void *data;
    MPL_gpu_event_t *trigger_event;
    MPL_gpu_event_t *done_event;
    MPIR_Request **request;     /* if not NULL, *request will be filled by cb */
    MPI_Status *status;
    struct MPIDU_stream_workq_op *next;
} MPIDU_stream_workq_op_t;

typedef struct MPIDU_stream_workq_wait_item {
    MPL_gpu_event_t *done_event;
    MPIR_Request *request;
    MPI_Status *status;
} MPIDU_stream_workq_wait_item_t;

typedef struct MPIDU_stream_workq {
    int vci;
    MPIDU_stream_workq_op_t *op_head;   /* FIFO, use singly linked list */
    MPIDU_stream_workq_op_t *op_tail;
    UT_array *wait_list;        /* utarray, since it will be frequently traversed */
    struct MPIDU_stream_workq *next;
} MPIDU_stream_workq_t;

int MPIDU_stream_workq_init(void);
int MPIDU_stream_workq_finalize(void);
int MPIDU_stream_workq_alloc(MPIDU_stream_workq_t ** workq_out, int vci);
int MPIDU_stream_workq_dealloc(MPIDU_stream_workq_t * workq);
int MPIDU_stream_workq_alloc_event(MPL_gpu_event_t ** event);
int MPIDU_stream_workq_free_event(MPL_gpu_event_t * event);
int MPIDU_stream_workq_enqueue(MPIDU_stream_workq_t * workq, MPIDU_stream_workq_op_t * op);
void MPIDU_stream_workq_progress_ops(int vci);
void MPIDU_stream_workq_progress_wait_list(int vci);

#endif /* MPIDU_STREAM_WORKQ_H_INCLUDED */
