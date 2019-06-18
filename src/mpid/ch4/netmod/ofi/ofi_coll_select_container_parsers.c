
/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include <mpidimpl.h>
#include "ofi_noinline.h"

const char *MPIDU_SELECTION_Bcast_ofi[] = {
    "binomial",
    "scatter_recursive_doubling_allgather",
    "scatter_ring_allgatherv",
};

const char *MPIDU_SELECTION_Barrier_ofi[] = {
    "dissemination",
};

const char *MPIDU_SELECTION_Allreduce_ofi[] = {
    "recursive_doubling",
    "reduce_scatter_allgather",
};

const char *MPIDU_SELECTION_Allgather_ofi[] = {
    "recursive_doubling",
    "brucks",
    "ring",
};

const char *MPIDU_SELECTION_Allgatherv_ofi[] = {
    "recursive_doubling",
    "brucks",
    "ring",
};

const char *MPIDU_SELECTION_Gather_ofi[] = {
    "binomial",
};

const char *MPIDU_SELECTION_Gatherv_ofi[] = {
    "allcomm_linear",
};

const char *MPIDU_SELECTION_Scatter_ofi[] = {
    "binomial",
};

const char *MPIDU_SELECTION_Scatterv_ofi[] = {
    "allcomm_linear",
};

const char *MPIDU_SELECTION_Alltoall_ofi[] = {
    "brucks",
    "scattered",
    "pairwise",
    "pairwise_sendrecv_replace",
};

const char *MPIDU_SELECTION_Alltoallv_ofi[] = {
    "pairwise_sendrecv_replace",
    "scattered",
};

const char *MPIDU_SELECTION_Alltoallw_ofi[] = {
    "pairwise_sendrecv_replace",
    "scattered",
};

const char *MPIDU_SELECTION_Reduce_ofi[] = {
    "reduce_scatter_gather",
    "binomial",
};

const char *MPIDU_SELECTION_Reduce_scatter_ofi[] = {
    "noncommutative",
    "pairwise",
    "recursive_doubling",
    "recursive_halving",
};

const char *MPIDU_SELECTION_Reduce_scatter_block_ofi[] = {
    "noncommutative",
    "pairwise",
    "recursive_doubling",
    "recursive_halving",
};

const char *MPIDU_SELECTION_Scan_ofi[] = {
    "recursive_doubling",
};

const char *MPIDU_SELECTION_Exscan_ofi[] = {
    "recursive_doubling",
};

int MPIDU_SELECTION_MAX_OFI_ALGORITHM_INDICES[MPIDU_SELECTION_COLLECTIVES_MAX]
    = {
    sizeof(MPIDU_SELECTION_Allgather_ofi) / sizeof(char *),     /* ALLGATHER */
    sizeof(MPIDU_SELECTION_Allgatherv_ofi) / sizeof(char *),    /* ALLGATHERV */
    sizeof(MPIDU_SELECTION_Allreduce_ofi) / sizeof(char *),     /* ALLREDUCE */
    sizeof(MPIDU_SELECTION_Alltoall_ofi) / sizeof(char *),      /* ALLTOALL */
    sizeof(MPIDU_SELECTION_Alltoallv_ofi) / sizeof(char *),     /* ALLTOALLV */
    sizeof(MPIDU_SELECTION_Alltoallw_ofi) / sizeof(char *),     /* ALLTOALLW */
    sizeof(MPIDU_SELECTION_Barrier_ofi) / sizeof(char *),       /* BARRIER */
    sizeof(MPIDU_SELECTION_Bcast_ofi) / sizeof(char *), /* BCAST */
    sizeof(MPIDU_SELECTION_Exscan_ofi) / sizeof(char *),        /* EXSCAN */
    sizeof(MPIDU_SELECTION_Gather_ofi) / sizeof(char *),        /* GATHER */
    sizeof(MPIDU_SELECTION_Gatherv_ofi) / sizeof(char *),       /* GATHERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_ofi) / sizeof(char *),        /* REDUCE_SCATTER */
    sizeof(MPIDU_SELECTION_Reduce_ofi) / sizeof(char *),        /* REDUCE */
    sizeof(MPIDU_SELECTION_Scan_ofi) / sizeof(char *),  /* SCAN */
    sizeof(MPIDU_SELECTION_Scatter_ofi) / sizeof(char *),       /* SCATTER */
    sizeof(MPIDU_SELECTION_Scatterv_ofi) / sizeof(char *),      /* SCATTERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_block_ofi) / sizeof(char *),  /* REDUCE_SCATTER_BLOCK */
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

const char **MPIDU_SELECTION_OFI_ALGORITHMS[MPIDU_SELECTION_COLLECTIVES_MAX] = {
    MPIDU_SELECTION_Allgather_ofi,      /* ALLGATHER */
    MPIDU_SELECTION_Allgatherv_ofi,     /* ALLGATHERV */
    MPIDU_SELECTION_Allreduce_ofi,      /* ALLREDUCE */
    MPIDU_SELECTION_Alltoall_ofi,       /* ALLTOALL */
    MPIDU_SELECTION_Alltoallv_ofi,      /* ALLTOALLV */
    MPIDU_SELECTION_Alltoallw_ofi,      /* ALLTOALLW */
    MPIDU_SELECTION_Barrier_ofi,        /* BARRIER */
    MPIDU_SELECTION_Bcast_ofi,  /* BCAST */
    MPIDU_SELECTION_Exscan_ofi, /* EXSCAN */
    MPIDU_SELECTION_Gather_ofi, /* GATHER */
    MPIDU_SELECTION_Gatherv_ofi,        /* GATHERV */
    MPIDU_SELECTION_Reduce_scatter_ofi, /* REDUCE_SCATTER */
    MPIDU_SELECTION_Reduce_ofi, /* REDUCE */
    MPIDU_SELECTION_Scan_ofi,   /* SCAN */
    MPIDU_SELECTION_Scatter_ofi,        /* SCATTER */
    MPIDU_SELECTION_Scatterv_ofi,       /* SCATTERV */
    MPIDU_SELECTION_Reduce_scatter_block_ofi,   /* REDUCE_SCATTER_BLOCK */
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

void MPIDI_OFI_algorithm_parser(MPIDU_SELECTION_coll_id_t coll_id, int *cnt_num,
                                MPIDIG_coll_algo_generic_container_t * cnt, char *value)
{
    int count = *cnt_num, ofi_algo_id = 0;
    for (ofi_algo_id = 0;
         ofi_algo_id < MPIDU_SELECTION_MAX_OFI_ALGORITHM_INDICES[MPIDU_SELECTION_BCAST];
         ofi_algo_id++) {
        /* Find if the algorithm name specified matches any of the existing ofi algorithm */
        if (strcasecmp(MPIDU_SELECTION_OFI_ALGORITHMS[MPIDU_SELECTION_BCAST][ofi_algo_id],
                       value) == 0) {
            break;
        }
    }
    if (ofi_algo_id < MPIDU_SELECTION_MAX_OFI_ALGORITHM_INDICES[MPIDU_SELECTION_BCAST]) {
        /* Assign the algo_id assosicated with the algorithm name (string) in the leaf node */
        cnt[count++].id = ofi_algo_id;
    }
    *cnt_num = count;
}
