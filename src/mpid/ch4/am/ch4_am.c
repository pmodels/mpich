/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

#define MAX_COMPLETION_CB 16
#define MAX_HANDLER_FN    256

static int num_completion_cb = MPIDI_AM_COMPLETION_NUM_RESERVED;
static MPIDI_AM_completion_cb completion_cb_table[MAX_COMPLETION_CB] = {
    (MPIDI_am_completion_cb) am_completion_null,        /* 0 */
    (MPIDI_am_completion_cb) am_completion_tmpbuf,      /* 1 */
    0
};

static int num_handler_fn = 0;
static MPIDI_AM_handeler_fn handler_fn_table[MAX_HANDLER_FN];

/* common completion handlers */
static int am_completion_null(void *context)
{
    return 0;
}

static int am_completion_tmpbuf(MPIDI_AM_COMPLETION_TMPBUF_CTX_t * ctx)
{
    MPL_free(ctx->tmpbuf);
    if (ctx->next_completion_id) {
        MPIDI_am_send_complete(ctx->next_completion_id, ctx->next_context);
    }
    MPL_free(ctx);
    return 0;
}

/* AM registration API -- for protocol layer */
int MPIDI_AM_register_completion(MPIDI_AM_completion_cb fn)
{
    MPIR_Assert(num_completion_cb < MAX_COMPLETION_CB);
    int i = num_completion_cb;
    completion_cb_table[i] = fn;
    num_completion_cb++;
    return i;
}

int MPIDI_AM_register_handler(MPIDI_AM_handeler_fn fn)
{
    MPIR_Assert(num_handler_fn < MAX_HANDLER_FN);
    int i = num_handler_fn;
    handler_fn_table[i] = fn;
    num_handler_fn++;
    return i;
}

/* AM callback API -- for device layer */
void MPIDI_AM_send_complete(int id, void *context)
{
    MPIR_Assert(id >= 0 && id < num_completion_cb);
    completion_cb_table[id] (context);
}

void MPIDI_AM_run_handler(int id, void *header, void *payload)
{
    MPIR_Assert(id >= 0 && id < num_handler_fn);
    handler_fn_table[id] (header, payload);
}
