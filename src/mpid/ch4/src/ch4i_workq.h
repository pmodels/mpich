/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#ifndef CH4I_WORKQ_H_INCLUDED
#define CH4I_WORKQ_H_INCLUDED

/* Point-to-Point */
typedef enum MPIDI_pt2pt_op MPIDI_pt2pt_op_t;
typedef struct MPIDI_pt2pt_elemt MPIDI_pt2pt_elemt_t;

enum MPIDI_pt2pt_op {MPIDI_SEND, MPIDI_ISEND, MPIDI_IRECV};

struct MPIDI_pt2pt_elemt {
    MPIDI_pt2pt_op_t op;
    const void *send_buf;
    void *recv_buf;
    MPI_Aint count;
    MPI_Datatype datatype;
    int rank;
    int tag;
    MPIR_Comm *comm_ptr;
    int context_offset;
    MPIR_Request *request;
};

/* RMA */
typedef enum MPIDI_rma_op MPIDI_rma_op_t;
typedef struct MPIDI_rma_elemt MPIDI_rma_elemt_t;

enum MPIDI_rma_op {MPIDI_PUT};

struct MPIDI_rma_elemt {
    MPIDI_rma_op_t op;
    const void *origin_addr;
    int origin_count;
    MPI_Datatype origin_datatype;
    int target_rank;
    MPI_Aint target_disp;
    int target_count;
    MPI_Datatype target_datatype;
    MPIR_Win *win_ptr;
};

/* For profiling */
extern double MPIDI_pt2pt_enqueue_time;
extern double MPIDI_pt2pt_progress_time;
extern double MPIDI_pt2pt_issue_pend_time;
extern double MPIDI_rma_enqueue_time;
extern double MPIDI_rma_progress_time;
extern double MPIDI_rma_issue_pend_time;

#if defined(MPIDI_WORKQ_PROFILE)
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START  double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP                          \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_pt2pt_enqueue_time += (enqueue_t2 - enqueue_t1);  \
    }while(0)
#define MPIDI_WORKQ_PT2PT_PROGRESS_START double progress_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_PROGRESS_STOP                                 \
    do {                                                                \
        double progress_t2 = MPI_Wtime();                               \
        MPIDI_pt2pt_progress_time += (progress_t2 - progress_t1);       \
    }while(0)
#define MPIDI_WORKQ_PT2PT_ISSUE_START    double issue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_PT2PT_ISSUE_STOP                            \
    do {                                                        \
        double issue_t2 = MPI_Wtime();                          \
        MPIDI_pt2pt_issue_pend_time += (issue_t2 - issue_t1);   \
    }while(0)

#define MPIDI_WORKQ_RMA_ENQUEUE_START    double enqueue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP                            \
    do {                                                        \
        double enqueue_t2 = MPI_Wtime();                        \
        MPIDI_rma_enqueue_time += (enqueue_t2 - enqueue_t1);    \
    }while(0)
#define MPIDI_WORKQ_RMA_PROGRESS_START   double progress_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_PROGRESS_STOP                           \
    do {                                                        \
        double progress_t2 = MPI_Wtime();                       \
        MPIDI_rma_progress_time += (progress_t2 - progress_t1); \
    }while(0)
#define MPIDI_WORKQ_RMA_ISSUE_START      double issue_t1 = MPI_Wtime();
#define MPIDI_WORKQ_RMA_ISSUE_STOP                              \
    do {                                                        \
        double issue_t2 = MPI_Wtime();                          \
        MPIDI_rma_issue_pend_time += (issue_t2 - issue_t1);     \
    }while(0)
#else /*!defined(MPIDI_WORKQ_PROFILE)*/
#define MPIDI_WORKQ_PT2PT_ENQUEUE_START
#define MPIDI_WORKQ_PT2PT_ENQUEUE_STOP
#define MPIDI_WORKQ_PT2PT_PROGRESS_START
#define MPIDI_WORKQ_PT2PT_PROGRESS_STOP
#define MPIDI_WORKQ_PT2PT_ISSUE_START
#define MPIDI_WORKQ_PT2PT_ISSUE_STOP

#define MPIDI_WORKQ_RMA_ENQUEUE_START
#define MPIDI_WORKQ_RMA_ENQUEUE_STOP
#define MPIDI_WORKQ_RMA_PROGRESS_START
#define MPIDI_WORKQ_RMA_PROGRESS_STOP
#define MPIDI_WORKQ_RMA_ISSUE_START
#define MPIDI_WORKQ_RMA_ISSUE_STOP
#endif

static inline void MPIDI_workq_pt2pt_enqueue_body(MPIDI_pt2pt_op_t op,
                                                  const void *send_buf,
                                                  void *recv_buf,
                                                  MPI_Aint count,
                                                  MPI_Datatype datatype,
                                                  int rank,
                                                  int tag,
                                                  MPIR_Comm *comm_ptr,
                                                  int context_offset,
                                                  int ep_idx,
                                                  MPIR_Request *request)
{
    MPIDI_pt2pt_elemt_t* pt2pt_elemt = NULL;
    pt2pt_elemt = MPL_malloc(sizeof (*pt2pt_elemt));
    pt2pt_elemt->op       = op;
    pt2pt_elemt->send_buf = send_buf;
    pt2pt_elemt->recv_buf = recv_buf;
    pt2pt_elemt->count    = count;
    pt2pt_elemt->datatype = datatype;
    pt2pt_elemt->rank     = rank;
    pt2pt_elemt->tag      = tag;
    pt2pt_elemt->comm_ptr = comm_ptr;
    pt2pt_elemt->context_offset = context_offset;
    pt2pt_elemt->request  = request;
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.ep_pt2pt_pend_ops[ep_idx], pt2pt_elemt);
}

static inline void MPIDI_workq_rma_enqueue_body(MPIDI_rma_op_t op,
                                                const void *origin_addr,
                                                int origin_count,
                                                MPI_Datatype origin_datatype,
                                                int target_rank,
                                                MPI_Aint target_disp,
                                                int target_count,
                                                MPI_Datatype target_datatype,
                                                MPIR_Win *win_ptr,
                                                int ep_idx)
{
    MPIDI_rma_elemt_t* rma_elemt = NULL;
    rma_elemt = MPL_malloc(sizeof (*rma_elemt));
    rma_elemt->op               = op;
    rma_elemt->origin_addr      = origin_addr;
    rma_elemt->origin_count     = origin_count;
    rma_elemt->origin_datatype  = origin_datatype;
    rma_elemt->target_rank      = target_rank;
    rma_elemt->target_disp      = target_disp;
    rma_elemt->target_count     = target_count;
    rma_elemt->target_datatype  = target_datatype;
    rma_elemt->win_ptr          = win_ptr;
    MPIDI_workq_enqueue(&MPIDI_CH4_Global.ep_rma_pend_ops[ep_idx], rma_elemt);
}

static inline int MPIDI_workq_pt2pt_progress(int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_pt2pt_elemt_t* pt2pt_elemt = NULL;
    MPIDI_WORKQ_PT2PT_PROGRESS_START;
    MPIDI_workq_dequeue(&MPIDI_CH4_Global.ep_pt2pt_pend_ops[ep_idx], (void**)&pt2pt_elemt);
    while(pt2pt_elemt != NULL) {
        MPIDI_WORKQ_PT2PT_ISSUE_START;
        switch(pt2pt_elemt->op) {
        case MPIDI_SEND:
            mpi_errno = MPIDI_NM_mpi_send(pt2pt_elemt->send_buf,
                                          pt2pt_elemt->count,
                                          pt2pt_elemt->datatype,
                                          pt2pt_elemt->rank,
                                          pt2pt_elemt->tag,
                                          pt2pt_elemt->comm_ptr,
                                          pt2pt_elemt->context_offset,
                                          ep_idx,
                                          &pt2pt_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case MPIDI_ISEND:
            mpi_errno = MPIDI_NM_mpi_isend(pt2pt_elemt->send_buf,
                                           pt2pt_elemt->count,
                                           pt2pt_elemt->datatype,
                                           pt2pt_elemt->rank,
                                           pt2pt_elemt->tag,
                                           pt2pt_elemt->comm_ptr,
                                           pt2pt_elemt->context_offset,
                                           ep_idx,
                                           &pt2pt_elemt->request);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        case MPIDI_IRECV:
            /* FIXME: fill in */
            break;
        }
        MPIDI_WORKQ_PT2PT_ISSUE_STOP;
        MPL_free(pt2pt_elemt);
        MPIDI_workq_dequeue(&MPIDI_CH4_Global.ep_pt2pt_pend_ops[ep_idx], (void**)&pt2pt_elemt);
    }
    MPIDI_WORKQ_PT2PT_PROGRESS_STOP;
fn_fail:
    return mpi_errno;
}

static inline int MPIDI_workq_rma_progress(int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_rma_elemt_t* rma_elemt = NULL;
    MPIDI_WORKQ_RMA_PROGRESS_START;
    MPIDI_workq_dequeue(&MPIDI_CH4_Global.ep_rma_pend_ops[ep_idx], (void**)&rma_elemt);
    while(rma_elemt != NULL) {
        MPIDI_WORKQ_RMA_ISSUE_START;
        switch(rma_elemt->op) {
        case MPIDI_PUT:
            mpi_errno = MPIDI_NM_mpi_put(rma_elemt->origin_addr,
                                         rma_elemt->origin_count,
                                         rma_elemt->origin_datatype,
                                         rma_elemt->target_rank,
                                         rma_elemt->target_disp,
                                         rma_elemt->target_count,
                                         rma_elemt->target_datatype,
                                         rma_elemt->win_ptr,
                                         ep_idx);
            if (mpi_errno != MPI_SUCCESS) goto fn_fail;
            break;
        }
        MPIDI_WORKQ_RMA_ISSUE_STOP;
        MPL_free(rma_elemt);
        MPIDI_workq_dequeue(&MPIDI_CH4_Global.ep_rma_pend_ops[ep_idx], (void**)&rma_elemt);
    }
    MPIDI_WORKQ_RMA_PROGRESS_STOP;
fn_fail:
    return mpi_errno;
}

static inline int MPIDI_workq_ep_progress_body(int ep_idx)
{
    int mpi_errno = MPI_SUCCESS;
    mpi_errno =  MPIDI_workq_pt2pt_progress(ep_idx);
    if (mpi_errno != MPI_SUCCESS) goto fn_fail;
    mpi_errno =  MPIDI_workq_rma_progress(ep_idx);
fn_fail:
    return mpi_errno;
}

static inline void MPIDI_workq_pt2pt_enqueue(MPIDI_pt2pt_op_t op,
                                             const void *send_buf,
                                             void *recv_buf,
                                             MPI_Aint count,
                                             MPI_Datatype datatype,
                                             int rank,
                                             int tag,
                                             MPIR_Comm *comm_ptr,
                                             int context_offset,
                                             int ep_idx,
                                             MPIR_Request *request)
{
    MPIDI_WORKQ_PT2PT_ENQUEUE_START;
    MPIDI_workq_pt2pt_enqueue_body(op, send_buf, recv_buf, count, datatype,
                                   rank, tag, comm_ptr, context_offset, ep_idx, request);
    MPIDI_WORKQ_PT2PT_ENQUEUE_STOP;
}

static inline void MPIDI_workq_rma_enqueue(MPIDI_rma_op_t op,
                                           const void *origin_addr,
                                           int origin_count,
                                           MPI_Datatype origin_datatype,
                                           int target_rank,
                                           MPI_Aint target_disp,
                                           int target_count,
                                           MPI_Datatype target_datatype,
                                           MPIR_Win *win_ptr,
                                           int ep_idx)
{
    MPIDI_WORKQ_RMA_ENQUEUE_START;
    MPIDI_workq_rma_enqueue_body(op, origin_addr, origin_count, origin_datatype,
                                 target_rank, target_disp, target_count, target_datatype,
                                 win_ptr, ep_idx);
    MPIDI_WORKQ_RMA_ENQUEUE_STOP;
}

static inline int MPIDI_workq_ep_progress(int ep_idx)
{
    int mpi_errno;
    mpi_errno = MPIDI_workq_ep_progress_body(ep_idx);
    return mpi_errno;
}

static inline int MPIDI_workq_global_progress(int* made_progress)
{
    int mpi_errno, ep_idx;
    *made_progress = 1;
    for( ep_idx = 0; ep_idx < MPIDI_CH4_Global.n_netmod_eps; ep_idx++) {
        mpi_errno = MPIDI_workq_ep_progress(ep_idx);
        if(unlikely(mpi_errno != MPI_SUCCESS))
            break;
    }
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_dispatch_send(int (*func)(const void *buf, int count,
                                                             MPI_Datatype datatype, int rank,
                                                             int tag, MPIR_Comm * comm,
                                                             int context_offset,
                                                             int ep_idx,
                                                             MPIR_Request ** request),
                                                 MPIDI_pt2pt_op_t op,
                                                 const void *buf, int count,
                                                 MPI_Datatype datatype, int rank,
                                                 int tag, MPIR_Comm * comm,
                                                 int context_offset,
                                                 int ep_idx,
                                                 MPIR_Request ** request)
{
    int mpi_errno = MPI_SUCCESS;

#ifdef MPIDI_CH4_MT_DIRECT
    mpi_errno = func(buf, count, datatype, rank, tag, comm, context_offset, ep_idx, request);
#else
#  ifdef MPIDI_CH4_MT_TRYLOCK
    /* FIXME: Implement trylock-enqueue */
#  else
    {
        /* Enqueue and hand-off */
        *request = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);
        /* FIXME: do we need to add a refcount while holding ownership in the queue? */
        MPIDI_workq_pt2pt_enqueue(op, buf, NULL /* recv_buf */, count, datatype,
                                  rank, tag, comm, context_offset, ep_idx, *request);
    }
#  endif
#endif
    return mpi_errno;
}

#endif /* CH4I_WORKQ_H_INCLUDED */
