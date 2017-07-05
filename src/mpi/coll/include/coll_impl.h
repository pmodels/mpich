/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_PROGRESS_MAX_COLLS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of collective operations at a time that the progress engine should make progress on

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/


#ifndef MPIC_COLL_IMPL_H_INCLUDED
#define MPIC_COLL_IMPL_H_INCLUDED

#include "queue.h"

extern MPIC_progress_global_t MPIC_progress_global;
extern MPIC_global_t MPIC_global_instance;

#define GLOBAL_NAME MPIC_

#include "../transports/stub/transport.h"
#include "../transports/mpich/transport.h"

/* Algorithms with STUB transport */
#include "../transports/stub/api_def.h"
#include "../algorithms/stub/post.h"
#include "../algorithms/tree/post.h"
#include "../algorithms/ring/post.h"
#include "../algorithms/dissem/post.h"
#include "../algorithms/recexch/post.h"
#include "../src/tsp_namespace_undef.h"


/* Algorithms with MPICH transport */
#include "../transports/mpich/api_def.h"
#include "../algorithms/stub/post.h"
#include "../algorithms/tree/post.h"
#include "../algorithms/ring/post.h"
#include "../algorithms/dissem/post.h"
#include "../algorithms/recexch/post.h"
#include "../src/tsp_namespace_undef.h"

/*Default Algorithms*/
#include "mpic_default.h"

#undef GLOBAL_NAME

struct MPIR_Request;

/* Hook to make progress on non-blocking collective operations */
MPL_STATIC_INLINE_PREFIX int MPIC_progress_hook()
{
    int count = 0;
    int mpi_errno = MPI_SUCCESS;
    MPIC_queue_elem_t *s;

    /* Go over up to MPIC_PROGRESS_MAX_COLLS collecives in the queue and make progress on them */
    for (s = MPIC_progress_global.head.tqh_first;
            ((s != NULL) && (count < MPIR_CVAR_PROGRESS_MAX_COLLS));
            s = s->list_data.tqe_next) {
        /* kick_fn makes progress on the collective operation */
        if (s->kick_fn(s)) {
            /* If the collective operation has completed, mark it as complete */
            MPIC_req_t *base = (MPIC_req_t *) s;
            MPIR_Request *req = container_of(base, MPIR_Request, coll);
            MPID_Request_complete(req);
            count++;
        }
    }

    return mpi_errno;
}

/* Function to initialze communicators for collectives */
MPL_STATIC_INLINE_PREFIX int MPIC_comm_init(struct MPIR_Comm *comm)
{
    int rank, size, mpi_errno = MPI_SUCCESS;
    int *tag = &(MPIC_COMM(comm)->use_tag);
    *tag = 0;
    rank = comm->rank;
    size = comm->local_size;

    MPIC_STUB_STUB_comm_init(&(MPIC_COMM(comm)->stub_stub), tag, rank, size);
    MPIC_STUB_TREE_comm_init(&(MPIC_COMM(comm)->stub_tree), tag, rank, size);
    MPIC_STUB_RING_comm_init(&(MPIC_COMM(comm)->stub_ring), tag, rank, size);
    MPIC_STUB_DISSEM_comm_init(&(MPIC_COMM(comm)->stub_dissem), tag, rank, size);
    MPIC_STUB_RECEXCH_comm_init(&(MPIC_COMM(comm)->stub_recexch),  tag, rank, size);
    MPIC_MPICH_STUB_comm_init(&(MPIC_COMM(comm)->mpich_stub), tag, rank, size);
    MPIC_MPICH_TREE_comm_init(&(MPIC_COMM(comm)->mpich_tree), tag, rank, size);
    MPIC_MPICH_RING_comm_init(&(MPIC_COMM(comm)->mpich_ring), tag, rank, size);
    MPIC_MPICH_DISSEM_comm_init(&(MPIC_COMM(comm)->mpich_dissem), tag, rank, size);
    MPIC_MPICH_RECEXCH_comm_init(&(MPIC_COMM(comm)->mpich_recexch),  tag, rank, size);

    /*hook for device level collective communicator initialization */
#ifdef MPID_COLL_COMM_INIT_HOOK
    MPID_COLL_COMM_INIT_HOOK;
#endif
    return mpi_errno;
}

/* Function to cleanup any communicators for collectives */
MPL_STATIC_INLINE_PREFIX int MPIC_comm_cleanup(struct MPIR_Comm *comm)
{
    int mpi_errno = MPI_SUCCESS;
    /* cleanup device level collective communicators */
#ifdef MPID_COLL_COMM_CLEANUP_HOOK
    MPID_COLL_COMM_CLEANUP_HOOK;
#endif
    /* cleanup all collective communicators */
    MPIC_STUB_STUB_comm_cleanup(&(MPIC_COMM(comm)->stub_stub));
    MPIC_STUB_TREE_comm_cleanup(&(MPIC_COMM(comm)->stub_tree));
    MPIC_STUB_RING_comm_cleanup(&(MPIC_COMM(comm)->stub_ring));
    MPIC_STUB_DISSEM_comm_cleanup(&(MPIC_COMM(comm)->stub_dissem));
    MPIC_STUB_RECEXCH_comm_cleanup(&(MPIC_COMM(comm)->stub_recexch));
    MPIC_MPICH_STUB_comm_cleanup(&(MPIC_COMM(comm)->mpich_stub));
    MPIC_MPICH_TREE_comm_cleanup(&(MPIC_COMM(comm)->mpich_tree));
    MPIC_MPICH_RING_comm_cleanup(&(MPIC_COMM(comm)->mpich_ring));
    MPIC_MPICH_DISSEM_comm_cleanup(&(MPIC_COMM(comm)->mpich_dissem));
    MPIC_MPICH_RECEXCH_comm_cleanup(&(MPIC_COMM(comm)->mpich_recexch));

    return mpi_errno;
}

/* Hook for any collective algorithms related initialization */
MPL_STATIC_INLINE_PREFIX int MPIC_init()
{
    int mpi_errno = MPI_SUCCESS;
    MPIC_progress_global.progress_fn = MPID_Progress_test;

    MPIC_STUB_init();
    MPIC_MPICH_init();

    MPIC_STUB_STUB_init();
    MPIC_STUB_TREE_init();
    MPIC_STUB_RING_init();
    MPIC_STUB_DISSEM_init();
    MPIC_STUB_RECEXCH_init();
    MPIC_MPICH_STUB_init();
    MPIC_MPICH_TREE_init();
    MPIC_MPICH_RING_init();
    MPIC_MPICH_DISSEM_init();
    MPIC_MPICH_RECEXCH_init();

#ifdef MPIDI_NM_COLL_INIT_HOOK
    MPIDI_NM_COLL_INIT_HOOK;
#endif
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIC_finalize()
{
    int mpi_errno = MPI_SUCCESS;
    return mpi_errno;
}
#endif /* MPIC_COLL_IMPL_H_INCLUDED */
