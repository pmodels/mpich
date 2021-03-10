/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#ifndef MPIT_MEM_H_INCLUDED
#define MPIT_MEM_H_INCLUDED

#include "mpichconf.h"

/* A simplified version from mpir_mem.h */

/* Rationale:
 *   push allocated pointers onto a stack
 *   free them on failure or
 *   commit (dismiss) the stack on success.
 */

#define MPIT_CHKPMEM_DECL(n_) \
    do { \
        void *(mpiu_chkpmem_stk_[n_]) = { NULL }; \
        int mpiu_chkpmem_stk_sp_=0; \
        const int mpiu_chkpmem_stk_sz_ = n_; \
    } while (0)

#define MPIT_CHKPMEM_COMMIT() \
    mpiu_chkpmem_stk_sp_ = 0

#define MPIT_CHKPMEM_REAP() \
    while (mpiu_chkpmem_stk_sp_ > 0) { \
        MPL_free(mpiu_chkpmem_stk_[--mpiu_chkpmem_stk_sp_]); \
    }

#define MPIT_CHKPMEM_MALLOC(pointer_,type_,nbytes_,class_) \
    do { \
        pointer_ = (type_)MPL_malloc(nbytes_,class_); \
        if (pointer_) { \
            MPIT_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_); \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_; \
        } else if (nbytes_ > 0) { \
            /* malloc returns NULL */ \
            mpi_errno = MPI_T_ERR_MEMORY; \
            goto fn_fail; \
        } \
    } while (0)

#define MPIT_CHKPMEM_CALLOC(pointer_,type_,nbytes_,class_) \
    do { \
        pointer_ = (type_)MPL_calloc(1, (nbytes_), (class_)); \
        if (pointer_) { \
            MPIT_Assert(mpiu_chkpmem_stk_sp_<mpiu_chkpmem_stk_sz_); \
            mpiu_chkpmem_stk_[mpiu_chkpmem_stk_sp_++] = pointer_; \
        } else if (nbytes_ > 0) { \
            mpi_errno = MPI_T_ERR_MEMORY; \
        } \
    } while (0)

#endif /* MPIT_MEM_H_INCLUDED */
