

/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "posix_noinline.h"

int MPIDU_SELECTION_MAX_POSIX_ALGORITHM_INDICES[MPIDU_SELECTION_COLLECTIVES_MAX]
    = {
    0,  /* ALLGATHER */
    0,  /* ALLGATHERV */
    0,  /* ALLREDUCE */
    0,  /* ALLTOALL */
    0,  /* ALLTOALLV */
    0,  /* ALLTOALLW */
    0,  /* BARRIER */
    0,  /* BCAST */
    0,  /* EXSCAN */
    0,  /* GATHER */
    0,  /* GATHERV */
    0,  /* REDUCE_SCATTER */
    0,  /* REDUCE */
    0,  /* SCAN */
    0,  /* SCATTER */
    0,  /* SCATTERV */
    0,  /* REDUCE_SCATTER_BLOCK */
    0,  /* IALLGATHER */
    0,  /* IALLGATHERV */
    0,  /* IALLREDUCE */
    0,  /* IALLTOALL */
    0,  /* IALLTOALLV */
    0,  /* IALLTOALLW */
    0,  /* IBARRIER */
    0,  /* IBCAST */
    0,  /* IEXSCAN */
    0,  /* IGATHER */
    0,  /* IGATHERV */
    0,  /* IREDUCE_SCATTER */
    0,  /* IREDUCE */
    0,  /* ISCAN */
    0,  /* ISCATTER */
    0,  /* ISCATTERV */
    0,  /* IREDUCE_SCATTER_BLOCK */
};

const char **MPIDU_SELECTION_POSIX_ALGORITHMS[MPIDU_SELECTION_COLLECTIVES_MAX] = {
    NULL,       /* ALLGATHER */
    NULL,       /* ALLGATHERV */
    NULL,       /* ALLREDUCE */
    NULL,       /* ALLTOALL */
    NULL,       /* ALLTOALLV */
    NULL,       /* ALLTOALLW */
    NULL,       /* BARRIER */
    NULL,       /* BCAST */
    NULL,       /* EXSCAN */
    NULL,       /* GATHER */
    NULL,       /* GATHERV */
    NULL,       /* REDUCE_SCATTER */
    NULL,       /* REDUCE */
    NULL,       /* SCAN */
    NULL,       /* SCATTER */
    NULL,       /* SCATTERV */
    NULL,       /* REDUCE_SCATTER_BLOCK */
    NULL,       /* IALLGATHER */
    NULL,       /* IALLGATHERV */
    NULL,       /* IALLREDUCE */
    NULL,       /* IALLTOALL */
    NULL,       /* IALLTOALLV */
    NULL,       /* IALLTOALLW */
    NULL,       /* IBARRIER */
    NULL,       /* IBCAST */
    NULL,       /* IEXSCAN */
    NULL,       /* IGATHER */
    NULL,       /* IGATHERV */
    NULL,       /* IREDUCE_SCATTER */
    NULL,       /* IREDUCE */
    NULL,       /* ISCAN */
    NULL,       /* ISCATTER */
    NULL,       /* ISCATTERV */
    NULL,       /* IREDUCE_SCATTER_BLOCK */
};

void MPIDI_POSIX_algorithm_parser(MPIDU_SELECTION_coll_id_t coll_id, int *cnt_num,
                                  MPIDIG_coll_algo_generic_container_t * cnt, char *value)
{
    int count = *cnt_num, posix_algo_id = 0;
    for (posix_algo_id = 0;
         posix_algo_id < MPIDU_SELECTION_MAX_POSIX_ALGORITHM_INDICES[MPIDU_SELECTION_BCAST];
         posix_algo_id++) {
        /* Find if the algorithm name specified matches any of the existing posix algorithm */
        if (strcasecmp(MPIDU_SELECTION_POSIX_ALGORITHMS[MPIDU_SELECTION_BCAST][posix_algo_id],
                       value) == 0) {
            break;
        }
    }
    if (posix_algo_id < MPIDU_SELECTION_MAX_POSIX_ALGORITHM_INDICES[MPIDU_SELECTION_BCAST]) {
        /* Assign the algo_id assosicated with the algorithm name (string) in the leaf node */
        cnt[count++].id = posix_algo_id;
    }
    *cnt_num = count;
}
