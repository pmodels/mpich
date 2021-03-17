/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef OFI_DYNPROC_H_INCLUDED
#define OFI_DYNPROC_H_INCLUDED

enum {
    MPIDI_OFI_DYNPROC_DISCONNECTED = 0,
    MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD,
    MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT,
    MPIDI_OFI_DYNPROC_CONNECTED_CHILD,
    MPIDI_OFI_DYNPROC_CONNECTED_PARENT
};

typedef struct {
    fi_addr_t dest;
    int rank;
    int state;
} MPIDI_OFI_conn_t;

/* allocated in MPIDI_OFI_global_t */
typedef struct MPIDI_OFI_conn_manager_t {
    int max_n_conn;             /* Maximum number of connections up to this point */
    int n_conn;                 /* Current number of open connections */
    int next_conn_id;           /* The next connection id to be used. */
    int *free_conn_id;          /* The list of the next connection id to be used so we
                                 * can garbage collect as we go. */
    MPIDI_OFI_conn_t *conn_list;        /* The list of connection structs to track the
                                         * outstanding dynamic process connections. */
} MPIDI_OFI_conn_manager_t;

int MPIDI_OFI_dynproc_init(void);
int MPIDI_OFI_dynproc_finalize(void);
int MPIDI_OFI_dynproc_insert_conn(fi_addr_t conn, int rank, int state);

#endif /* OFI_DYNPROC_H_INCLUDED */
