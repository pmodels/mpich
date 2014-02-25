/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

/* this file should be included by the using device's mpidimpl.h */

#ifndef MPID_SCHED_H_INCLUDED
#define MPID_SCHED_H_INCLUDED

/* FIXME open questions:
 * - Should the schedule hold a pointer to the nbc request and the nbc request
 *   hold a pointer to the schedule?  This could cause MT issues.
 */


enum MPIDU_Sched_entry_type {
    MPIDU_SCHED_ENTRY_INVALID_LB = 0,
    MPIDU_SCHED_ENTRY_SEND,
    /* _SEND_DEFER is easily implemented via _SEND */
    MPIDU_SCHED_ENTRY_RECV,
    /* _RECV_STATUS is easily implemented via _RECV */
    MPIDU_SCHED_ENTRY_REDUCE,
    MPIDU_SCHED_ENTRY_COPY,
    MPIDU_SCHED_ENTRY_NOP,
    MPIDU_SCHED_ENTRY_CB,
    MPIDU_SCHED_ENTRY_INVALID_UB
};

struct MPIDU_Sched_send {
    const void *buf;
    int count;
    const int *count_p;
    MPI_Datatype datatype;
    int dest;
    MPID_Comm *comm;
    MPID_Request *sreq;
    int is_sync; /* TRUE iff this send is an ssend */
};

struct MPIDU_Sched_recv {
    void *buf;
    int count;
    MPI_Datatype datatype;
    int src;
    MPID_Comm *comm;
    MPID_Request *rreq;
    MPI_Status *status;
};

struct MPIDU_Sched_reduce {
    const void *inbuf;
    void *inoutbuf;
    int count;
    MPI_Datatype datatype;
    MPI_Op op;
};

struct MPIDU_Sched_copy {
    const void *inbuf;
    int incount;
    MPI_Datatype intype;
    void *outbuf;
    int outcount;
    MPI_Datatype outtype;
};

/* nop entries have no args, so no structure is needed */

enum MPIDU_Sched_cb_type {
    MPIDU_SCHED_CB_TYPE_1 = 0, /* single state arg type --> MPID_Sched_cb_t */
    MPIDU_SCHED_CB_TYPE_2      /* double state arg type --> MPID_Sched_cb2_t */
};

struct MPIDU_Sched_cb {
    enum MPIDU_Sched_cb_type cb_type;
    union {
        MPID_Sched_cb_t *cb_p;
        MPID_Sched_cb2_t *cb2_p;
    } u;
    void *cb_state;
    void *cb_state2; /* unused for single-param callbacks */
};

enum MPIDU_Sched_entry_status {
    MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED = 0,
    MPIDU_SCHED_ENTRY_STATUS_STARTED,
    MPIDU_SCHED_ENTRY_STATUS_COMPLETE,
    MPIDU_SCHED_ENTRY_STATUS_FAILED, /* indicates a failure occurred while executing the entry */
    MPIDU_SCHED_ENTRY_STATUS_INVALID /* indicates an invalid entry, or invalid status value */
};

/* Use a tagged union for schedule entries.  Not always space optimal, but saves
 * lots of error-prone pointer arithmetic and makes scanning the schedule easy. */
struct MPIDU_Sched_entry {
    enum MPIDU_Sched_entry_type type;
    enum MPIDU_Sched_entry_status status;
    int is_barrier;
    union {
        struct MPIDU_Sched_send send;
        struct MPIDU_Sched_recv recv;
        struct MPIDU_Sched_reduce reduce;
        struct MPIDU_Sched_copy copy;
        /* nop entries have no args */
        struct MPIDU_Sched_cb cb;
    } u;
};

struct MPIDU_Sched {
    size_t size; /* capacity (in entries) of the entries array */
    size_t idx;  /* index into entries array of first yet-outstanding entry */
    int num_entries; /* number of populated entries, num_entries <= size */
    int tag;
    MPID_Request *req; /* really needed? could cause MT problems... */
    struct MPIDU_Sched_entry *entries;

    struct MPIDU_Sched *next; /* linked-list next pointer */
    struct MPIDU_Sched *prev; /* linked-list next pointer */
};

/* prototypes */
int MPIDU_Sched_progress(int *made_progress);
int MPIDU_Sched_are_pending(void);

#endif /* !defined(MPID_SCHED_H_INCLUDED) */
