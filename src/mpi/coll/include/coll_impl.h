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

#include "allreduce_group.h"

#ifndef COLL_IMPL_H_INCLUDED
#define COLL_IMPL_H_INCLUDED

#include "stubtran_impl.h"
#include "gentran_impl.h"

#include "../algorithms/stubalgo/stubalgo.h"
#include "../algorithms/treealgo/treealgo.h"
#include "../algorithms/recexchalgo/recexchalgo.h"

#define MPII_COLLECTIVE_FALLBACK_CHECK(check)                           \
    do {                                                                \
        if ((check) == 0) {                                             \
            if (MPIR_CVAR_COLLECTIVE_FALLBACK == 0) {                   \
                MPIR_ERR_SETANDJUMP(mpi_errno, MPI_ERR_OTHER, "**collalgo"); \
            } else if (MPIR_CVAR_COLLECTIVE_FALLBACK == 1) {            \
                fprintf(stderr, "User set collective algorithm is not usable for the provided arguments\n"); \
                fflush(stderr);                                         \
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

/* Function to initialze communicators for collectives */
int MPIR_Coll_comm_init(MPIR_Comm * comm);

/* Function to cleanup any communicators for collectives */
int MPII_Coll_comm_cleanup(MPIR_Comm * comm);

/* Hook for any collective algorithms related initialization */
int MPII_Coll_init(void);

int MPIR_Coll_safe_to_block(void);

int MPII_Coll_finalize(void);

#endif /* COLL_IMPL_H_INCLUDED */
