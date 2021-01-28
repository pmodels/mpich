/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "allreduce_group.h"

#ifndef COLL_IMPL_H_INCLUDED
#define COLL_IMPL_H_INCLUDED

#include "stubtran_impl.h"
#include "gentran_impl.h"

#include "../algorithms/stubalgo/stubalgo.h"
#include "../algorithms/treealgo/treealgo.h"
#include "../algorithms/recexchalgo/recexchalgo.h"

#include "csel_container.h"

#define MPII_COLLECTIVE_FALLBACK_CHECK(rank, check, mpi_errno, ...)     \
    do {                                                                \
        if ((check) == 0) {                                             \
            if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_error) { \
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**collalgo"); \
            } else if (MPIR_CVAR_COLLECTIVE_FALLBACK == MPIR_CVAR_COLLECTIVE_FALLBACK_print) { \
                if ((rank) == 0) {                                      \
                    fprintf(stderr, "User set collective algorithm is not usable for the provided arguments\n"); \
                    fprintf(stderr, ""  __VA_ARGS__);                   \
                    fflush(stderr);                                     \
                }                                                       \
                goto fallback;                                          \
            } else {                                                    \
                goto fallback;                                          \
            }                                                           \
        }                                                               \
    } while (0)

extern int MPIR_Nbc_progress_hook_id;

extern MPIR_Tree_type_t MPIR_Iallreduce_tree_type;
extern MPIR_Tree_type_t MPIR_Ireduce_tree_type;
extern MPIR_Tree_type_t MPIR_Ibcast_tree_type;
extern void *MPIR_Csel_root;
extern char MPII_coll_generic_json[];

/* Function to initialze communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm);

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm);

/* Hook for any collective algorithms related initialization */
int MPII_Coll_init(void);

int MPIR_Coll_safe_to_block(void);

int MPII_Coll_finalize(void);

#define MPII_SCHED_WRAPPER_EMPTY(fn, comm_ptr, request)                 \
    do {                                                                \
        int tag = -1;                                                   \
        MPIR_Sched_t s = MPIR_SCHED_NULL;                               \
                                                                        \
        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);                \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = MPIR_Sched_create(&s);                              \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = fn(comm_ptr, s);                                    \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);       \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
    } while (0)

#define MPII_SCHED_WRAPPER(fn, comm_ptr, request, ...)                  \
    do {                                                                \
        int tag = -1;                                                   \
        MPIR_Sched_t s = MPIR_SCHED_NULL;                               \
                                                                        \
        mpi_errno = MPIR_Sched_next_tag(comm_ptr, &tag);                \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = MPIR_Sched_create(&s);                              \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = fn(__VA_ARGS__, comm_ptr, s);                       \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
                                                                        \
        mpi_errno = MPIR_Sched_start(&s, comm_ptr, tag, request);       \
        if (mpi_errno)                                                  \
            MPIR_ERR_POP(mpi_errno);                                    \
    } while (0)

/* functions for supporting GPU buffers in reduce collectives */
void MPIR_Coll_host_buffer_alloc(const void *sendbuf, const void *recvbuf, MPI_Aint count,
                                 MPI_Datatype datatype, void **host_sendbuf, void **host_recvbuf);
void MPIR_Coll_host_buffer_free(void *host_sendbuf, void *host_recvbuf);
void MPIR_Coll_host_buffer_swap_back(void *host_sendbuf, void *host_recvbuf, void *in_recvbuf,
                                     MPI_Aint count, MPI_Datatype datatype, MPIR_Request * request);

#endif /* COLL_IMPL_H_INCLUDED */
