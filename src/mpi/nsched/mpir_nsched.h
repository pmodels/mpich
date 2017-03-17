/* -*- Mode: c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#ifndef MPIR_NSCHED_H_INCLUDED
#define MPIR_NSCHED_H_INCLUDED

#include "mpl_utlist.h"

struct MPIR_Comm;

typedef enum {
    MPII_NSCHED_ELM_TYPE__SYNC,
    MPII_NSCHED_ELM_TYPE__ISEND,
    MPII_NSCHED_ELM_TYPE__IRECV
} MPII_Nsched_elm_type_t;

typedef struct MPII_Nsched_elm {
    MPII_Nsched_elm_type_t type;

    union {
        struct {
            const void *buf;
            int count;
            MPI_Datatype datatype;
            int dest;
            int tag;
            struct MPIR_Comm *comm_ptr;
        } isend;
        struct {
            void *buf;
            int count;
            MPI_Datatype datatype;
            int dest;
            int tag;
            struct MPIR_Comm *comm_ptr;
        } irecv;
    } u;

    int issued;
    struct MPIR_Request *req;

    struct MPII_Nsched_elm *next;
} MPII_Nsched_elm;

typedef struct MPIR_Nsched {
    MPII_Nsched_elm *elm_list_head;
    MPII_Nsched_elm *elm_list_tail;
    struct MPIR_Request *req;
    struct MPIR_Nsched *next;
} MPIR_Nsched;

extern int MPIR_Nsched_progress_hook_id;

struct MPII_Nsched_active_comm {
    struct MPIR_Comm *comm;
    struct MPII_Nsched_active_comm *next;
};

extern struct MPII_Nsched_active_comm *MPII_Nsched_active_comm_list_head;
extern struct MPII_Nsched_active_comm *MPII_Nsched_active_comm_list_tail;

#define MPII_NSCHED_ACTIVE_COMM_ENQUEUE(ac_)                            \
    MPL_LL_APPEND(MPII_Nsched_active_comm_list_head, MPII_Nsched_active_comm_list_tail, (ac_))
#define MPII_NSCHED_ACTIVE_COMM_DEQUEUE(ac_)                            \
    MPL_LL_DELETE(MPII_Nsched_active_comm_list_head, MPII_Nsched_active_comm_list_tail, (ac_))

#define MPII_NSCHED_SCHED_ENQUEUE(comm_, s_)                            \
    MPL_LL_APPEND((comm_)->nsched_list_head, (comm_)->nsched_list_tail, (s_))
#define MPII_NSCHED_SCHED_DEQUEUE(comm_, s_)                            \
    MPL_LL_DELETE((comm_)->nsched_list_head, (comm_)->nsched_list_tail, (s_))

#define MPII_NSCHED_ELM_ENQUEUE(s_, elm_)                               \
    MPL_LL_APPEND((s_)->elm_list_head, (s_)->elm_list_tail, (elm_))
#define MPII_NSCHED_ELM_DEQUEUE(s_, elm_)                               \
    MPL_LL_DELETE((s_)->elm_list_head, (s_)->elm_list_tail, (elm_))

/* init */
int MPIR_Nsched_init(void);
int MPIR_Nsched_finalize(void);

/* ops management */
int MPIR_Nsched_create(MPIR_Nsched ** s, struct MPIR_Request **req);
int MPIR_Nsched_enqueue_sync(MPIR_Nsched * s);
int MPIR_Nsched_enqueue_isend(const void *buf, int count, MPI_Datatype datatype,
                              int dest, int tag, struct MPIR_Comm *comm_ptr, MPIR_Nsched * s);
int MPIR_Nsched_enqueue_irecv(void *buf, int count, MPI_Datatype datatype,
                              int dest, int tag, struct MPIR_Comm *comm_ptr, MPIR_Nsched * s);

/* progress management */
int MPIR_Nsched_kickoff(MPIR_Nsched * s, struct MPIR_Comm *comm_ptr);
int MPIR_Nsched_progress_hook(int *made_progress);
int MPIR_Nsched_are_pending(void);

#endif /* !defined(MPIR_NSCHED_H_INCLUDED) */
