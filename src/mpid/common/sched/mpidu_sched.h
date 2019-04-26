/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2011 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIDU_SCHED_H_INCLUDED
#define MPIDU_SCHED_H_INCLUDED

//#include "mpidu_pre.h"
/* FIXME open questions:
 * - Should the schedule hold a pointer to the nbc request and the nbc request
 *   hold a pointer to the schedule?  This could cause MT issues.
 */
#include "mpidu_pre.h"

enum MPIDU_Sched_element_entry_type {
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

struct MPIDU_Sched_element_send {
    const void *buf;
    MPI_Aint count;
    const MPI_Aint *count_p;
    MPI_Datatype datatype;
    int dest;
    struct MPIR_Comm *comm;
    struct MPIR_Request *sreq;
    int is_sync;                /* TRUE iff this send is an ssend */
};

struct MPIDU_Sched_element_recv {
    void *buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int src;
    struct MPIR_Comm *comm;
    struct MPIR_Request *rreq;
    MPI_Status *status;
};

struct MPIDU_Sched_element_reduce {
    const void *inbuf;
    void *inoutbuf;
    MPI_Aint count;
    MPI_Datatype datatype;
    MPI_Op op;
};

struct MPIDU_Sched_element_copy {
    const void *inbuf;
    MPI_Aint incount;
    MPI_Datatype intype;
    void *outbuf;
    MPI_Aint outcount;
    MPI_Datatype outtype;
};

/* nop entries have no args, so no structure is needed */

enum MPIDU_Sched_element_cb_type {
    MPIDU_SCHED_CB_TYPE_1 = 0,  /* single state arg type --> MPIR_Sched_element_cb_t */
    MPIDU_SCHED_CB_TYPE_2       /* double state arg type --> MPIR_Sched_element_cb2_t */
};

struct MPIDU_Sched_element_cb {
    enum MPIDU_Sched_element_cb_type cb_type;
    union {
        MPIR_Sched_element_cb_t *cb_p;
        MPIR_Sched_element_cb2_t *cb2_p;
    } u;
    void *cb_state;
    void *cb_state2;            /* unused for single-param callbacks */
};

enum MPIDU_Sched_element_entry_status {
    MPIDU_SCHED_ENTRY_STATUS_NOT_STARTED = 0,
    MPIDU_SCHED_ENTRY_STATUS_STARTED,
    MPIDU_SCHED_ENTRY_STATUS_COMPLETE,
    MPIDU_SCHED_ENTRY_STATUS_FAILED,    /* indicates a failure occurred while executing the entry */
    MPIDU_SCHED_ENTRY_STATUS_INVALID    /* indicates an invalid entry, or invalid status value */
};

/* Use a tagged union for schedule entries.  Not always space optimal, but saves
 * lots of error-prone pointer arithmetic and makes scanning the schedule easy. */
struct MPIDU_Sched_element_entry {
    enum MPIDU_Sched_element_entry_type type;
    enum MPIDU_Sched_element_entry_status status;
    int is_barrier;
    union {
        struct MPIDU_Sched_element_send send;
        struct MPIDU_Sched_element_recv recv;
        struct MPIDU_Sched_element_reduce reduce;
        struct MPIDU_Sched_element_copy copy;
        /* nop entries have no args */
        struct MPIDU_Sched_element_cb cb;
    } u;
};

struct MPIDU_Sched_element {
    size_t size;                /* capacity (in entries) of the entries array */
    size_t idx;                 /* index into entries array of first yet-outstanding entry */
    int num_entries;            /* number of populated entries, num_entries <= size */
    int tag;
    struct MPIR_Request *req;   /* really needed? could cause MT problems... */
    struct MPIDU_Sched_element_entry *entries;

    struct MPIDU_Sched_element *next;   /* linked-list next pointer */
    struct MPIDU_Sched_element *prev;   /* linked-list next pointer */
};

/* prototypes */
int MPIDU_Sched_list_progress(int *made_progress);
int MPIDU_Sched_list_has_pending_sched(void);
int MPIDU_Sched_list_get_next_tag(struct MPIR_Comm *comm_ptr, int *tag);
int MPIDU_Sched_element_create(MPIR_Sched_element_t * sp);
int MPIDU_Sched_element_clone(MPIR_Sched_element_t orig, MPIR_Sched_element_t * cloned);
int MPIDU_Sched_list_enqueue_sched(MPIR_Sched_element_t * sp, struct MPIR_Comm *comm, int tag,
                                   struct MPIR_Request **req);
int MPIDU_Sched_element_send(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                             struct MPIR_Comm *comm, MPIR_Sched_element_t s);
int MPIDU_Sched_element_recv(void *buf, MPI_Aint count, MPI_Datatype datatype, int src,
                             struct MPIR_Comm *comm, MPIR_Sched_element_t s);
int MPIR_Sched_element_ssend(const void *buf, MPI_Aint count, MPI_Datatype datatype, int dest,
                             struct MPIR_Comm *comm, MPIR_Sched_element_t s);
int MPIR_Sched_element_reduce(const void *inbuf, void *inoutbuf, MPI_Aint count,
                              MPI_Datatype datatype, MPI_Op op, MPIR_Sched_element_t s);
int MPIDU_Sched_element_copy(const void *inbuf, MPI_Aint incount, MPI_Datatype intype, void *outbuf,
                             MPI_Aint outcount, MPI_Datatype outtype, MPIR_Sched_element_t s);
int MPIDU_Sched_element_barrier(MPIR_Sched_element_t s);

#endif /* MPIDU_SCHED_H_INCLUDED */
