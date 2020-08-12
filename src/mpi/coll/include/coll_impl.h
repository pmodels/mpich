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

#define MPIR_COLL_CHKLMEM_DECL(n_)                      \
    void *(chklmem_stk_[n_]) = { NULL };                \
    int chklmem_type_[n_] = { 0 };                      \
    int chklmem_stk_sp_=0;                              \
    MPIR_AssertDeclValue(const int chklmem_stk_sz_,n_)

#define MPIR_COLL_CHKLMEM_MALLOC_HOST(pointer_,nbytes_,rc_,name_,class_)   \
    do {                                                                   \
        pointer_ = MPL_malloc(nbytes_,class_);                             \
                                                                           \
        if (pointer_) {                                                    \
            MPIR_Assert(chklmem_stk_sp_<chklmem_stk_sz_);                  \
            chklmem_type_[chklmem_stk_sp_] = MPL_GPU_POINTER_UNREGISTERED_HOST; \
            chklmem_stk_[chklmem_stk_sp_++] = pointer_;                    \
        } else if (nbytes_ > 0) {                                          \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                         \
            goto fn_fail;                                                  \
        }                                                                  \
    } while (0)

#define MPIR_COLL_CHKLMEM_MALLOC(pointer_,nbytes_,attr_,rc_,name_,class_)  \
    do {                                                                   \
        if (attr_.type == MPL_GPU_POINTER_DEV)                             \
            MPL_gpu_malloc((void **) &pointer_, nbytes_, attr_.device);    \
        else                                                               \
            pointer_ = MPL_malloc(nbytes_,class_);                         \
                                                                           \
        if (pointer_) {                                                    \
            MPIR_Assert(chklmem_stk_sp_<chklmem_stk_sz_);                  \
            chklmem_type_[chklmem_stk_sp_] = attr_.type;                   \
            chklmem_stk_[chklmem_stk_sp_++] = pointer_;                    \
        } else if (nbytes_ > 0) {                                          \
            MPIR_CHKMEM_SETERR(rc_,nbytes_,name_);                         \
            goto fn_fail;                                                  \
        }                                                                  \
    } while (0)

#define MPIR_COLL_CHKLMEM_FREEALL()                                        \
    do {                                                                   \
        while (chklmem_stk_sp_ > 0) {                                      \
            if (chklmem_type_[chklmem_stk_sp_ - 1] == MPL_GPU_POINTER_DEV) \
                MPL_gpu_free(chklmem_stk_[--chklmem_stk_sp_]);             \
            else                                                           \
                MPL_free(chklmem_stk_[--chklmem_stk_sp_]);                 \
        }                                                                  \
    } while (0)

#endif /* COLL_IMPL_H_INCLUDED */
