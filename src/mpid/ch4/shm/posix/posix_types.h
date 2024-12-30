/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef POSIX_TYPES_H_INCLUDED
#define POSIX_TYPES_H_INCLUDED

#include "mpidu_init_shm.h"
#include "mpidu_genq.h"

enum {
    MPIDI_POSIX_OK,
    MPIDI_POSIX_NOK
};

enum {
    MPIDI_POSIX_DIST__LOCAL = 0,
    MPIDI_POSIX_DIST__NO_SHARED_CACHE,
    MPIDI_POSIX_DIST__INTER_NUMA
};

#define MPIDI_POSIX_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE            (1024)
#define MPIDI_POSIX_AM_HDR_POOL_NUM_CELLS_PER_CHUNK     (1024)

#define MPIDI_POSIX_AMREQUEST(req,field)      ((req)->dev.ch4.am.shm_am.posix.field)
#define MPIDI_POSIX_AMREQUEST_HDR(req, field) ((req)->dev.ch4.am.shm_am.posix.req_hdr->field)
#define MPIDI_POSIX_AMREQUEST_HDR_PTR(req)    ((req)->dev.ch4.am.shm_am.posix.req_hdr)
#define MPIDI_POSIX_REQUEST(req, field)       ((req)->dev.ch4.shm.posix.field)
#define MPIDI_POSIX_COMM(comm, field)         ((comm)->dev.ch4.shm.posix.field)

typedef struct {
    MPIDU_genq_private_pool_t am_hdr_buf_pool;
    MPIDI_POSIX_am_request_header_t *postponed_queue;
    MPIR_Request **active_rreq;
    char pad MPL_ATTR_ALIGNED(MPL_CACHELINE_SIZE);
} MPIDI_POSIX_per_vci_t;

typedef struct {
    int core_id;
    int l3_cache_id;
    int numa_id;
} MPIDI_POSIX_topo_info_t;

typedef struct {
    MPIDI_POSIX_per_vci_t per_vci[MPIDI_CH4_MAX_VCIS];
    void *shm_ptr;
    /* Keep track of all of the local processes in MPI_COMM_WORLD and what their original rank was
     * in that communicator. */
    int *local_ranks;
    int *local_procs;
    int local_rank_0;
    int num_vcis;               /* num_vcis in POSIX need >= MPIDI_global.n_total_vcis */
    int *local_rank_dist;
    MPIDI_POSIX_topo_info_t topo;
} MPIDI_POSIX_global_t;

extern MPIDI_POSIX_global_t MPIDI_POSIX_global;

MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_rma_outstanding_req_enqueu(MPIR_gpu_req yreq,
                                                                     MPIDI_POSIX_win_t * posix_win)
{
    MPIDI_POSIX_rma_req_t *req;
    req = MPL_malloc(sizeof(MPIDI_POSIX_rma_req_t), MPL_MEM_RMA);
    MPIR_Assert(req);

    req->yreq = yreq;
    req->next = NULL;
    LL_APPEND(posix_win->outstanding_reqs_head, posix_win->outstanding_reqs_tail, req);
}

/* Blocking wait until all queued requests are complete.
 * Using *flushall* to indicate that it does not distinguish targets. */
MPL_STATIC_INLINE_PREFIX void MPIDI_POSIX_rma_outstanding_req_flushall(MPIDI_POSIX_win_t *
                                                                       posix_win)
{
    MPIDI_POSIX_rma_req_t *req, *req_tmp;
    /* No dependency between requests, thus can safely wait one by one. */
    LL_FOREACH_SAFE(posix_win->outstanding_reqs_head, req, req_tmp) {
        int mpi_errno ATTRIBUTE((unused)) = MPI_SUCCESS;

        if (req->yreq.type == MPIR_TYPEREP_REQUEST) {
            mpi_errno = MPIR_Typerep_wait(req->yreq.u.y_req);
            MPIR_Assert(mpi_errno == MPI_SUCCESS);
        } else if (req->yreq.type == MPIR_GPU_REQUEST) {
            int completed = 0;
            while (!completed) {
                mpi_errno = MPL_gpu_test(&req->yreq.u.gpu_req, &completed);
                MPIR_Assert(mpi_errno == MPI_SUCCESS);
            }
        } else {
            MPIR_Assert(req->yreq.type == MPIR_NULL_REQUEST);
        }

        LL_DELETE(posix_win->outstanding_reqs_head, posix_win->outstanding_reqs_tail, req);
        MPL_free(req);
    }
}

int MPIDI_POSIX_init_vci(int vci);

#endif /* POSIX_TYPES_H_INCLUDED */
