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

#define MPIDI_POSIX_AM_BUFF_SZ               (1 * 1024 * 1024)
#define MPIDI_POSIX_AM_HDR_POOL_CELL_SIZE            (1024)
#define MPIDI_POSIX_AM_HDR_POOL_NUM_CELLS_PER_CHUNK     (1024)
#define MPIDI_POSIX_AM_HDR_POOL_MAX_NUM_CELLS           (257 * 1024)

#define MPIDI_POSIX_AMREQUEST(req,field)      ((req)->dev.ch4.am.shm_am.posix.field)
#define MPIDI_POSIX_AMREQUEST_HDR(req, field) ((req)->dev.ch4.am.shm_am.posix.req_hdr->field)
#define MPIDI_POSIX_AMREQUEST_HDR_PTR(req)    ((req)->dev.ch4.am.shm_am.posix.req_hdr)
#define MPIDI_POSIX_REQUEST(req, field)       ((req)->dev.ch4.shm.posix.field)
#define MPIDI_POSIX_COMM(comm, field)         ((comm)->dev.ch4.shm.posix.field)

typedef struct {
    MPIDU_genq_private_pool_t am_hdr_buf_pool;

    /* Postponed queue */
    MPIDI_POSIX_am_request_header_t *postponed_queue;

    /* Active recv requests array */
    MPIR_Request **active_rreq;

    void *shm_ptr;

    /* Keep track of all of the local processes in MPI_COMM_WORLD and what their original rank was
     * in that communicator. */
    int num_local;
    int my_local_rank;
    int *local_ranks;
    int *local_procs;
    int local_rank_0;
} MPIDI_POSIX_global_t;

extern MPIDI_POSIX_global_t MPIDI_POSIX_global;

#endif /* POSIX_TYPES_H_INCLUDED */
