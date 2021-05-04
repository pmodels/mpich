/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_DYNPROC_H_INCLUDED
#define OFI_DYNPROC_H_INCLUDED

enum {
    MPIDI_OFI_DYNPROC_DISCONNECTED = 0,
    MPIDI_OFI_DYNPROC_CONNECTED_CHILD,
    MPIDI_OFI_DYNPROC_CONNECTED_PARENT
};

/* This is compatible with MPIDI_OFI_dynamic_process_request_t
 * when using MPIDI_OFI_EVENT_DYNPROC_DONE
 */
typedef struct {
    struct fi_context context[MPIDI_OFI_CONTEXT_STRUCTS];
    int event_id;
    int done;
    int data;
} MPIDI_OFI_dynproc_done_req_t;

typedef struct {
    fi_addr_t dest;
    int rank;
    int state;
    /* Parent need prepost recv for disconnect message */
    MPIDI_OFI_dynproc_done_req_t *req;
} MPIDI_OFI_conn_t;

/* allocated in MPIDI_OFI_global_t */
typedef struct MPIDI_OFI_conn_manager_t {
    int n_conn;                 /* Current number of open connections */
    int max_n_conn;             /* size of connection table */
    MPIDI_OFI_conn_t *conn_table;       /* connection table */
    /* maintain free conn_id in a stack, n_free is the stack pointer.
     * free_stack has the same size as max_n_conn since ever connection may
     * disconnect and pushed to the free_stack.
     */
    int n_free;
    int *free_stack;
} MPIDI_OFI_conn_manager_t;

int MPIDI_OFI_dynproc_init(void);
int MPIDI_OFI_dynproc_finalize(void);
int MPIDI_OFI_dynproc_insert_conn(fi_addr_t conn, int rank, int state, int *conn_id_out);

#endif /* OFI_DYNPROC_H_INCLUDED */
