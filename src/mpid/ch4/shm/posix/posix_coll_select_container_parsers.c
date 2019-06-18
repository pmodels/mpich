

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

const char *MPIDU_SELECTION_Bcast_posix[] = {
    "binomial",
    "scatter_recursive_doubling_allgather",
    "scatter_ring_allgatherv",
    "auto",
    "invalid",
    "release_gather",
};

const char *MPIDU_SELECTION_Barrier_posix[] = {
    "dissemination",
};

const char *MPIDU_SELECTION_Allreduce_posix[] = {
    "recursive_doubling",
    "reduce_scatter_allgather",
    "auto",
    "invalid",
    "release_gather",
};

const char *MPIDU_SELECTION_Allgather_posix[] = {
    "recursive_doubling",
    "brucks",
    "ring",
};

const char *MPIDU_SELECTION_Allgatherv_posix[] = {
    "recursive_doubling",
    "brucks",
    "ring",
};

const char *MPIDU_SELECTION_Gather_posix[] = {
    "binomial",
};

const char *MPIDU_SELECTION_Gatherv_posix[] = {
    "allcomm_linear",
};

const char *MPIDU_SELECTION_Scatter_posix[] = {
    "binomial",
};

const char *MPIDU_SELECTION_Scatterv_posix[] = {
    "allcomm_linear",
};

const char *MPIDU_SELECTION_Alltoall_posix[] = {
    "brucks",
    "scattered",
    "pairwise",
    "pairwise_sendrecv_replace",
};

const char *MPIDU_SELECTION_Alltoallv_posix[] = {
    "pairwise_sendrecv_replace",
    "scattered",
};

const char *MPIDU_SELECTION_Alltoallw_posix[] = {
    "pairwise_sendrecv_replace",
    "scattered",
};

const char *MPIDU_SELECTION_Reduce_posix[] = {
    "reduce_scatter_gather",
    "binomial",
    "auto",
    "invalid",
    "release_gather",
};

const char *MPIDU_SELECTION_Reduce_scatter_posix[] = {
    "noncommutative",
    "pairwise",
    "recursive_doubling",
    "recursive_halving",
};

const char *MPIDU_SELECTION_Reduce_scatter_block_posix[] = {
    "noncommutative",
    "pairwise",
    "recursive_doubling",
    "recursive_halving",
};

const char *MPIDU_SELECTION_Scan_posix[] = {
    "recursive_doubling",
};

const char *MPIDU_SELECTION_Exscan_posix[] = {
    "recursive_doubling",
};

int MPIDU_SELECTION_MAX_POSIX_ALGORITHM_INDICES[MPIDU_SELECTION_COLLECTIVES_MAX]
    = {
    sizeof(MPIDU_SELECTION_Allgather_posix) / sizeof(char *),   /* ALLGATHER */
    sizeof(MPIDU_SELECTION_Allgatherv_posix) / sizeof(char *),  /* ALLGATHERV */
    sizeof(MPIDU_SELECTION_Allreduce_posix) / sizeof(char *),   /* ALLREDUCE */
    sizeof(MPIDU_SELECTION_Alltoall_posix) / sizeof(char *),    /* ALLTOALL */
    sizeof(MPIDU_SELECTION_Alltoallv_posix) / sizeof(char *),   /* ALLTOALLV */
    sizeof(MPIDU_SELECTION_Alltoallw_posix) / sizeof(char *),   /* ALLTOALLW */
    sizeof(MPIDU_SELECTION_Barrier_posix) / sizeof(char *),     /* BARRIER */
    sizeof(MPIDU_SELECTION_Bcast_posix) / sizeof(char *),       /* BCAST */
    sizeof(MPIDU_SELECTION_Exscan_posix) / sizeof(char *),      /* EXSCAN */
    sizeof(MPIDU_SELECTION_Gather_posix) / sizeof(char *),      /* GATHER */
    sizeof(MPIDU_SELECTION_Gatherv_posix) / sizeof(char *),     /* GATHERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_posix) / sizeof(char *),      /* REDUCE_SCATTER */
    sizeof(MPIDU_SELECTION_Reduce_posix) / sizeof(char *),      /* REDUCE */
    sizeof(MPIDU_SELECTION_Scan_posix) / sizeof(char *),        /* SCAN */
    sizeof(MPIDU_SELECTION_Scatter_posix) / sizeof(char *),     /* SCATTER */
    sizeof(MPIDU_SELECTION_Scatterv_posix) / sizeof(char *),    /* SCATTERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_block_posix) / sizeof(char *),        /* REDUCE_SCATTER_BLOCK */
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
    MPIDU_SELECTION_Allgather_posix,    /* ALLGATHER */
    MPIDU_SELECTION_Allgatherv_posix,   /* ALLGATHERV */
    MPIDU_SELECTION_Allreduce_posix,    /* ALLREDUCE */
    MPIDU_SELECTION_Alltoall_posix,     /* ALLTOALL */
    MPIDU_SELECTION_Alltoallv_posix,    /* ALLTOALLV */
    MPIDU_SELECTION_Alltoallw_posix,    /* ALLTOALLW */
    MPIDU_SELECTION_Barrier_posix,      /* BARRIER */
    MPIDU_SELECTION_Bcast_posix,        /* BCAST */
    MPIDU_SELECTION_Exscan_posix,       /* EXSCAN */
    MPIDU_SELECTION_Gather_posix,       /* GATHER */
    MPIDU_SELECTION_Gatherv_posix,      /* GATHERV */
    MPIDU_SELECTION_Reduce_scatter_posix,       /* REDUCE_SCATTER */
    MPIDU_SELECTION_Reduce_posix,       /* REDUCE */
    MPIDU_SELECTION_Scan_posix, /* SCAN */
    MPIDU_SELECTION_Scatter_posix,      /* SCATTER */
    MPIDU_SELECTION_Scatterv_posix,     /* SCATTERV */
    MPIDU_SELECTION_Reduce_scatter_block_posix, /* REDUCE_SCATTER_BLOCK */
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
