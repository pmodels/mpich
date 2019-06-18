/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2018 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#include "mpiimpl.h"
#include "ch4_coll_select_tree_build.h"

const char *MPIDU_SELECTION_Bcast_composition[] = {
    "alpha",
    "beta",
    "gamma",
};

const char *MPIDU_SELECTION_Barrier_composition[] = {
    "alpha",
    "beta",
};

const char *MPIDU_SELECTION_Allreduce_composition[] = {
    "alpha",
    "beta",
    "gamma",
};

const char *MPIDU_SELECTION_Allgather_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Allgatherv_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Scatter_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Scatterv_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Gather_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Gatherv_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Alltoall_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Alltoallv_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Alltoallw_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Reduce_composition[] = {
    "alpha",
    "beta",
    "gamma",
};

const char *MPIDU_SELECTION_Reduce_scatter_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Reduce_scatter_block_composition[] = {
    "alpha",
};

const char *MPIDU_SELECTION_Scan_composition[] = {
    "alpha",
    "beta",
};

const char *MPIDU_SELECTION_Exscan_composition[] = {
    "alpha",
};

int MPIDU_SELECTION_MAX_COMPOSITION_INDICES[MPIDU_SELECTION_COLLECTIVES_MAX] = {
    sizeof(MPIDU_SELECTION_Allgather_composition) / sizeof(char *),     /* ALLGATHER */
    sizeof(MPIDU_SELECTION_Allgatherv_composition) / sizeof(char *),    /* ALLGATHERV */
    sizeof(MPIDU_SELECTION_Allreduce_composition) / sizeof(char *),     /* ALLREDUCE */
    sizeof(MPIDU_SELECTION_Alltoall_composition) / sizeof(char *),      /* ALLTOALL */
    sizeof(MPIDU_SELECTION_Alltoallv_composition) / sizeof(char *),     /* ALLTOALLV */
    sizeof(MPIDU_SELECTION_Alltoallw_composition) / sizeof(char *),     /* ALLTOALLW */
    sizeof(MPIDU_SELECTION_Barrier_composition) / sizeof(char *),       /* BARRIER */
    sizeof(MPIDU_SELECTION_Bcast_composition) / sizeof(char *), /* BCAST */
    sizeof(MPIDU_SELECTION_Exscan_composition) / sizeof(char *),        /* EXSCAN */
    sizeof(MPIDU_SELECTION_Gather_composition) / sizeof(char *),        /* GATHER */
    sizeof(MPIDU_SELECTION_Gatherv_composition) / sizeof(char *),       /* GATHERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_composition) / sizeof(char *),        /* REDUCE_SCATTER */
    sizeof(MPIDU_SELECTION_Reduce_composition) / sizeof(char *),        /* REDUCE */
    sizeof(MPIDU_SELECTION_Scan_composition) / sizeof(char *),  /* SCAN */
    sizeof(MPIDU_SELECTION_Scatter_composition) / sizeof(char *),       /* SCATTER */
    sizeof(MPIDU_SELECTION_Scatterv_composition) / sizeof(char *),      /* SCATTERV */
    sizeof(MPIDU_SELECTION_Reduce_scatter_block_composition) / sizeof(char *),  /* REDUCE_SCATTER_BLOCK */
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

const char **MPIDU_SELECTION_COMPOSITIONS[MPIDU_SELECTION_COLLECTIVES_MAX] = {
    MPIDU_SELECTION_Allgather_composition,      /* ALLGATHER */
    MPIDU_SELECTION_Allgatherv_composition,     /* ALLGATHERV */
    MPIDU_SELECTION_Allreduce_composition,      /* ALLREDUCE */
    MPIDU_SELECTION_Alltoall_composition,       /* ALLTOALL */
    MPIDU_SELECTION_Alltoallv_composition,      /* ALLTOALLV */
    MPIDU_SELECTION_Alltoallw_composition,      /* ALLTOALLW */
    MPIDU_SELECTION_Barrier_composition,        /* BARRIER */
    MPIDU_SELECTION_Bcast_composition,  /* BCAST */
    MPIDU_SELECTION_Exscan_composition, /* EXSCAN */
    MPIDU_SELECTION_Gather_composition, /* GATHER */
    MPIDU_SELECTION_Gatherv_composition,        /* GATHERV */
    MPIDU_SELECTION_Reduce_scatter_composition, /* REDUCE_SCATTER */
    MPIDU_SELECTION_Reduce_composition, /* REDUCE */
    MPIDU_SELECTION_Scan_composition,   /* SCAN */
    MPIDU_SELECTION_Scatter_composition,        /* SCATTER */
    MPIDU_SELECTION_Scatterv_composition,       /* SCATTERV */
    MPIDU_SELECTION_Reduce_scatter_block_composition,   /* REDUCE_SCATTER_BLOCK */
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

void MPIDI_CH4_container_parser(MPIDU_SELECTION_json_node_t json_node, int *cnt_num,
                                MPIDIG_coll_algo_generic_container_t * cnt, int coll_id,
                                char *value)
{
    int composition_id = 0, i = 0, count = *cnt_num;
    char *key = NULL, *leaf_name = NULL;
    int is_known_token = 1;
    int algorithm_type = -1;
    for (composition_id = 0;
         composition_id < MPIDU_SELECTION_MAX_COMPOSITION_INDICES[coll_id]; composition_id++) {
        if (strcasecmp(MPIDU_SELECTION_COMPOSITIONS[coll_id][composition_id], value) == 0) {
            break;
        }
    }
    if (composition_id < MPIDU_SELECTION_MAX_COMPOSITION_INDICES[coll_id]) {
        cnt[count++].id = composition_id;

        json_node = MPIDU_SELECTION_tree_get_node_json_next(json_node, 0);

        if (json_node != MPIDU_SELECTION_NULL_TYPE) {
            /* Depending upon the composition, last level JSON node could have multiple nodes,
             * one for each algorithm that the composition will use.
             * Iterate over all the leaf nodes to assign proper algorithms to netmod and shm */
            for (i = 0; i < MPIDU_SELECTION_tree_get_node_json_peer_count(json_node); i++) {
                leaf_name = MPL_strdup(MPIDU_SELECTION_tree_get_node_json_name(json_node, i));

                is_known_token = 1;

                char buff[MPIDU_SELECTION_BUFFER_MAX_SIZE] = { 0 };

                if (leaf_name) {

                    MPL_strncpy(buff, leaf_name, MPIDU_SELECTION_BUFFER_MAX_SIZE);

                    key = NULL;
                    value = NULL;
                    algorithm_type = -1;

                    key = strtok(buff, MPIDU_SELECTION_DELIMITER_VALUE);
                    value = strtok(NULL, MPIDU_SELECTION_DELIMITER_VALUE);
                    algorithm_type = MPIDU_SELECTION_tree_get_algorithm_type(key);

                    /* check if the algorithm type specified by the current JSON leaf node is netmod or shm */
                    if ((algorithm_type != MPIDU_SELECTION_NETMOD) ||
                        (algorithm_type != MPIDU_SELECTION_SHM)) {
                        is_known_token = 0;
                    }

                    if (is_known_token) {
                        if (algorithm_type == MPIDU_SELECTION_NETMOD) {
                            MPIDI_NM_algorithm_parser(MPIDU_SELECTION_BCAST, &count, cnt, value);
                        }
#ifndef MPIDI_CH4_DIRECT_NETMOD
                        if (algorithm_type == MPIDU_SELECTION_SHM) {
                            MPIDI_SHM_algorithm_parser(MPIDU_SELECTION_BCAST, &count, cnt, value);
                        }
#endif
                    } else      /* abort if the composition children are not an algorithm */
                        MPIR_Assert(0);
                    MPL_free(leaf_name);
                }
            }
        }
    }
    *cnt_num = count;
}
