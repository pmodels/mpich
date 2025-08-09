/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "coll_impl.h"
#include "csel_container.h"
#include "csel_internal.h"
#include "mpl.h"

static void parse_container_params(struct json_object *obj, MPII_Csel_container_s * cnt)
{
    MPIR_Assert(obj != NULL);
    char *ckey;

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.ibcast.intra_tsp_tree.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.ibcast.intra_tsp_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.ibcast.intra_tsp_tree.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_ring:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.ibcast.intra_tsp_ring.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.bcast.intra_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.bcast.intra_tree.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "is_non_blocking=", strlen("is_non_blocking=")))
                        cnt->u.bcast.intra_tree.is_non_blocking =
                            atoi(ckey + strlen("is_non_blocking="));
                    else if (!strncmp(ckey, "topo_overhead=", strlen("topo_overhead=")))
                        cnt->u.bcast.intra_tree.topo_overhead =
                            atoi(ckey + strlen("topo_overhead="));
                    else if (!strncmp(ckey, "topo_diff_groups=", strlen("topo_diff_groups=")))
                        cnt->u.bcast.intra_tree.topo_diff_groups =
                            atoi(ckey + strlen("topo_diff_groups="));
                    else if (!strncmp(ckey, "topo_diff_switches=", strlen("topo_diff_switches=")))
                        cnt->u.bcast.intra_tree.topo_diff_switches =
                            atoi(ckey + strlen("topo_diff_switches="));
                    else if (!strncmp(ckey, "topo_same_switches=", strlen("topo_same_switches=")))
                        cnt->u.bcast.intra_tree.topo_same_switches =
                            atoi(ckey + strlen("topo_same_switches="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Bcast_intra_pipelined_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.bcast.intra_pipelined_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.bcast.intra_pipelined_tree.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "is_non_blocking=", strlen("is_non_blocking=")))
                        cnt->u.bcast.intra_pipelined_tree.is_non_blocking =
                            atoi(ckey + strlen("is_non_blocking="));
                    else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.bcast.intra_pipelined_tree.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    else if (!strncmp(ckey, "recv_pre_posted=", strlen("recv_pre_posted=")))
                        cnt->u.bcast.intra_pipelined_tree.recv_pre_posted =
                            atoi(ckey + strlen("recv_pre_posted="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                        cnt->u.ireduce.intra_tsp_tree.buffer_per_child =
                            atoi(ckey + strlen("buffer_per_child="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.ireduce.intra_tsp_tree.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.ireduce.intra_tsp_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.ireduce.intra_tsp_tree.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    else if (!strncmp(ckey, "topo_overhead=", strlen("topo_overhead=")))
                        cnt->u.ireduce.intra_tsp_tree.topo_overhead =
                            atoi(ckey + strlen("topo_overhead="));
                    else if (!strncmp(ckey, "topo_diff_groups=", strlen("topo_diff_groups=")))
                        cnt->u.ireduce.intra_tsp_tree.topo_diff_groups =
                            atoi(ckey + strlen("topo_diff_groups="));
                    else if (!strncmp(ckey, "topo_diff_switches=", strlen("topo_diff_switches=")))
                        cnt->u.ireduce.intra_tsp_tree.topo_diff_switches =
                            atoi(ckey + strlen("topo_diff_switches="));
                    else if (!strncmp(ckey, "topo_same_switches=", strlen("topo_same_switches=")))
                        cnt->u.ireduce.intra_tsp_tree.topo_same_switches =
                            atoi(ckey + strlen("topo_same_switches="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ireduce_intra_tsp_ring:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                        cnt->u.ireduce.intra_tsp_ring.buffer_per_child =
                            atoi(ckey + strlen("buffer_per_child="));
                    else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.ireduce.intra_tsp_tree.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                        cnt->u.iallreduce.intra_tsp_tree.buffer_per_child =
                            atoi(ckey + strlen("buffer_per_child="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.iallreduce.intra_tsp_tree.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.iallreduce.intra_tsp_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.iallreduce.intra_tsp_tree.chunk_size =
                            atoi(ckey + strlen("chunk_size="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_multiplying:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allreduce.intra_recursive_multiplying.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_tree:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "buffer_per_child=", strlen("buffer_per_child=")))
                        cnt->u.allreduce.intra_tree.buffer_per_child =
                            atoi(ckey + strlen("buffer_per_child="));
                    else if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allreduce.intra_tree.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "tree_type=", strlen("tree_type=")))
                        cnt->u.allreduce.intra_tree.tree_type =
                            get_tree_type_from_string(ckey + strlen("tree_type="));
                    else if (!strncmp(ckey, "chunk_size=", strlen("chunk_size=")))
                        cnt->u.allreduce.intra_tree.chunk_size = atoi(ckey + strlen("chunk_size="));
                    else if (!strncmp(ckey, "topo_overhead=", strlen("topo_overhead=")))
                        cnt->u.allreduce.intra_tree.topo_overhead =
                            atoi(ckey + strlen("topo_overhead="));
                    else if (!strncmp(ckey, "topo_diff_groups=", strlen("topo_diff_groups=")))
                        cnt->u.allreduce.intra_tree.topo_diff_groups =
                            atoi(ckey + strlen("topo_diff_groups="));
                    else if (!strncmp(ckey, "topo_diff_switches=", strlen("topo_diff_switches=")))
                        cnt->u.allreduce.intra_tree.topo_diff_switches =
                            atoi(ckey + strlen("topo_diff_switches="));
                    else if (!strncmp(ckey, "topo_same_switches=", strlen("topo_same_switches=")))
                        cnt->u.allreduce.intra_tree.topo_same_switches =
                            atoi(ckey + strlen("topo_same_switches="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recexch:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allreduce.intra_recexch.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                        cnt->u.allreduce.intra_recexch.single_phase_recv =
                            atoi(ckey + strlen("single_phase_recv="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_k_reduce_scatter_allgather:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allreduce.intra_k_reduce_scatter_allgather.k =
                            atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                        cnt->u.allreduce.intra_k_reduce_scatter_allgather.single_phase_recv =
                            atoi(ckey + strlen("single_phase_recv="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_ccl:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "ccl=", strlen("ccl=")))
                        cnt->u.allreduce.intra_ccl.ccl = get_ccl_from_string(ckey + strlen("ccl="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Ibcast_intra_tsp_scatterv_recexch_allgatherv:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "scatterv_k=", strlen("scatterv_k=")))
                        cnt->u.ibcast.intra_tsp_scatterv_recexch_allgatherv.scatterv_k =
                            atoi(ckey + strlen("scatterv_k="));
                    else if (!strncmp(ckey, "allgatherv_k=", strlen("allgatherv_k=")))
                        cnt->u.ibcast.intra_tsp_scatterv_recexch_allgatherv.allgatherv_k =
                            atoi(ckey + strlen("allgatherv_k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_single_buffer:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.iallreduce.intra_tsp_recexch_single_buffer.k =
                            atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_multiple_buffer:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.iallreduce.intra_tsp_recexch_multiple_buffer.k =
                            atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Iallreduce_intra_tsp_recexch_reduce_scatter_recexch_allgatherv:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.iallreduce.intra_tsp_recexch_reduce_scatter_recexch_allgatherv.k =
                            atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_k_brucks:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allgather.intra_k_brucks.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_doubling:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allgather.intra_recexch_doubling.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                        cnt->u.allgather.intra_recexch_doubling.single_phase_recv =
                            atoi(ckey + strlen("single_phase_recv="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allgather_intra_recexch_halving:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.allgather.intra_recexch_halving.k = atoi(ckey + strlen("k="));
                    else if (!strncmp(ckey, "single_phase_recv=", strlen("single_phase_recv=")))
                        cnt->u.allgather.intra_recexch_halving.single_phase_recv =
                            atoi(ckey + strlen("single_phase_recv="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Alltoall_intra_k_brucks:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.alltoall.intra_k_brucks.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_k_dissemination:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.barrier.intra_k_dissemination.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Barrier_intra_recexch:
            {
                json_object_object_foreach(obj, key, val) {
                    ckey = MPL_strdup_no_spaces(key);
                    if (!strncmp(ckey, "k=", strlen("k=")))
                        cnt->u.barrier.intra_recexch.k = atoi(ckey + strlen("k="));
                    MPL_free(ckey);
                }
            }
            break;

        default:
            /* Algorithm does not have parameters */
            break;
    }
}

void *MPII_Create_container(struct json_object *obj)
{
    MPII_Csel_container_s *cnt = MPL_malloc(sizeof(MPII_Csel_container_s), MPL_MEM_COLL);

    json_object_object_foreach(obj, key, val) {
        char *ckey = MPL_strdup_no_spaces(key);

        if (!strncmp(ckey, "algorithm=", strlen("algorithm="))) {
            char *str = ckey + strlen("algorithm=");

            bool matched = false;
            for (int idx = 0; idx < MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count; idx++) {
                if (strcmp(str, Csel_container_type_str[idx]) == 0) {
                    cnt->id = idx;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                MPIR_Assert(0);
            }
        } else if (!strncmp(ckey, "composition=", strlen("composition="))) {
            char *str = ckey + strlen("composition=");

            bool matched = false;
            for (int idx = 0; idx < MPII_CSEL_CONTAINER_TYPE__ALGORITHM__Algorithm_count; idx++) {
                if (strcmp(str, Csel_container_type_str[idx]) == 0) {
                    cnt->id = idx;
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                MPIR_Assert(0);
            }
        } else {
            fprintf(stderr, "unrecognized key %s\n", key);
            MPIR_Assert(0);
        }

        MPL_free(ckey);
    }

    /* process algorithm parameters */
    parse_container_params(json_object_object_get(obj, key), cnt);

    return (void *) cnt;
}
