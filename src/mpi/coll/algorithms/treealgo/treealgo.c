/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"
#include "treeutil.h"

static void dump_node(MPIR_Treealgo_tree_t * node, FILE * output_stream);
static void dump_tree(int tree_type, int rank, MPIR_Treealgo_tree_t * ct);

int MPII_Treealgo_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Treealgo_comm_init(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_Treealgo_comm_cleanup(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPIR_Treealgo_tree_create(int rank, int nranks, int tree_type, int k, int root,
                              MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (tree_type) {
        case MPIR_TREE_TYPE_KARY:
            mpi_errno = MPII_Treeutil_tree_kary_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_1:
            mpi_errno = MPII_Treeutil_tree_knomial_1_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_2:
            mpi_errno = MPII_Treeutil_tree_knomial_2_init(rank, nranks, k, root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**treetype", "**treetype %d",
                                 tree_type);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    if (MPIR_CVAR_COLL_TREE_DUMP) {
        dump_tree(tree_type, rank, ct);
    }
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


int MPIR_Treealgo_tree_create_topo_aware(MPIR_Comm * comm, int tree_type, int k, int root,
                                         bool enable_reorder, MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (tree_type) {
        case MPIR_TREE_TYPE_TOPOLOGY_AWARE:
            if (!comm->coll.topo_aware_tree || root != comm->coll.topo_aware_tree_root
                || k != comm->coll.topo_aware_tree_k) {
                if (comm->coll.topo_aware_tree) {
                    MPIR_Treealgo_tree_free(comm->coll.topo_aware_tree);
                } else {
                    comm->coll.topo_aware_tree =
                        (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t),
                                                            MPL_MEM_BUFFER);
                }
                mpi_errno =
                    MPII_Treeutil_tree_topology_aware_init(comm, k, root, enable_reorder,
                                                           comm->coll.topo_aware_tree);
                MPIR_ERR_CHECK(mpi_errno);
                *ct = *comm->coll.topo_aware_tree;
                comm->coll.topo_aware_tree_root = root;
                comm->coll.topo_aware_tree_k = k;
            }
            *ct = *comm->coll.topo_aware_tree;
            utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
            for (int i = 0; i < ct->num_children; i++) {
                utarray_push_back(ct->children,
                                  &ut_int_array(comm->coll.topo_aware_tree->children)[i],
                                  MPL_MEM_COLL);
            }
            break;

        case MPIR_TREE_TYPE_TOPOLOGY_AWARE_K:
            if (!comm->coll.topo_aware_k_tree || root != comm->coll.topo_aware_k_tree_root
                || k != comm->coll.topo_aware_k_tree_k) {
                if (comm->coll.topo_aware_k_tree) {
                    MPIR_Treealgo_tree_free(comm->coll.topo_aware_k_tree);
                } else {
                    comm->coll.topo_aware_k_tree =
                        (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t),
                                                            MPL_MEM_BUFFER);
                }
                mpi_errno =
                    MPII_Treeutil_tree_topology_aware_k_init(comm, k, root, enable_reorder,
                                                             comm->coll.topo_aware_k_tree);
                MPIR_ERR_CHECK(mpi_errno);
                *ct = *comm->coll.topo_aware_k_tree;
                comm->coll.topo_aware_k_tree_root = root;
                comm->coll.topo_aware_k_tree_k = k;
            }
            *ct = *comm->coll.topo_aware_k_tree;
            utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
            for (int i = 0; i < ct->num_children; i++) {
                utarray_push_back(ct->children,
                                  &ut_int_array(comm->coll.topo_aware_k_tree->children)[i],
                                  MPL_MEM_COLL);
            }
            break;

        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**treetype", "**treetype %d",
                                 tree_type);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    if (MPIR_CVAR_COLL_TREE_DUMP) {
        dump_tree(tree_type, comm->rank, ct);
    }
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


int MPIR_Treealgo_tree_create_topo_wave(MPIR_Comm * comm, int k, int root,
                                        bool enable_reorder, int overhead, int lat_diff_groups,
                                        int lat_diff_switches, int lat_same_switches,
                                        MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    if (!comm->coll.topo_wave_tree || root != comm->coll.topo_wave_tree_root
        || overhead != comm->coll.topo_wave_tree_overhead
        || lat_diff_groups != comm->coll.topo_wave_tree_lat_diff_groups
        || lat_diff_switches != comm->coll.topo_wave_tree_lat_diff_switches
        || lat_same_switches != comm->coll.topo_wave_tree_lat_same_switches) {
        if (comm->coll.topo_wave_tree) {
            MPIR_Treealgo_tree_free(comm->coll.topo_wave_tree);
        } else {
            comm->coll.topo_wave_tree =
                (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t), MPL_MEM_BUFFER);
        }
        mpi_errno = MPII_Treeutil_tree_topology_wave_init(comm, k, root, enable_reorder, overhead,
                                                          lat_diff_groups, lat_diff_switches,
                                                          lat_same_switches,
                                                          comm->coll.topo_wave_tree);
        MPIR_ERR_CHECK(mpi_errno);
        *ct = *comm->coll.topo_wave_tree;
        comm->coll.topo_wave_tree_root = root;
        comm->coll.topo_wave_tree_overhead = overhead;
        comm->coll.topo_wave_tree_lat_diff_groups = lat_diff_groups;
        comm->coll.topo_wave_tree_lat_diff_switches = lat_diff_switches;
        comm->coll.topo_wave_tree_lat_same_switches = lat_same_switches;
    }
    *ct = *comm->coll.topo_wave_tree;
    utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
    for (int i = 0; i < ct->num_children; i++) {
        utarray_push_back(ct->children,
                          &ut_int_array(comm->coll.topo_wave_tree->children)[i], MPL_MEM_COLL);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    if (MPIR_CVAR_COLL_TREE_DUMP) {
        dump_tree(MPIR_TREE_TYPE_TOPOLOGY_WAVE, comm->rank, ct);
    }
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}

void MPIR_Treealgo_tree_free(MPIR_Treealgo_tree_t * tree)
{
    utarray_free(tree->children);
}


static void dump_node(MPIR_Treealgo_tree_t * node, FILE * output_stream)
{
    fprintf(output_stream, "{ \"rank\": %d, \"nranks\": %d, \"parent\": %d, \"children\": [",
            node->rank, node->nranks, node->parent);
    for (int child = 0; child < node->num_children; child++) {
        if (child > 0) {
            fprintf(output_stream, ",");
        }
        fprintf(output_stream, "%d", tree_ut_int_elt(node->children, child));
    }
    fprintf(output_stream, "] }\n");
}

static void dump_tree(int tree_type, int rank, MPIR_Treealgo_tree_t * ct)
{
    char outfile_name[PATH_MAX];
    sprintf(outfile_name, "%s%d.json", "colltree", rank);
    fprintf(stdout, "tree_type=%d: dumping %s\n", tree_type, outfile_name);
    FILE *outfile = fopen(outfile_name, "w");
    dump_node(ct, outfile);
    fclose(outfile);
}
