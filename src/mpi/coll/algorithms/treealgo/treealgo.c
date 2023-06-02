/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "treealgo.h"
#include "treeutil.h"

static void dump_node(MPIR_Treealgo_tree_t * node, FILE * output_stream);


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


int MPIR_Treealgo_tree_create(MPIR_Comm * comm, MPIR_Treealgo_params_t * params,
                              MPIR_Treealgo_tree_t * ct)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    switch (params->tree_type) {
        case MPIR_TREE_TYPE_KARY:
            mpi_errno =
                MPII_Treeutil_tree_kary_init(params->rank, params->nranks, params->k, params->root,
                                             ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_1:
            mpi_errno =
                MPII_Treeutil_tree_knomial_1_init(params->rank, params->nranks, params->k,
                                                  params->root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_KNOMIAL_2:
            mpi_errno =
                MPII_Treeutil_tree_knomial_2_init(params->rank, params->nranks, params->k,
                                                  params->root, ct);
            MPIR_ERR_CHECK(mpi_errno);
            break;

        case MPIR_TREE_TYPE_TOPOLOGY_AWARE:
            if (!comm->coll.topo_aware_tree || params->root != comm->coll.topo_aware_tree_root) {
                if (comm->coll.topo_aware_tree) {
                    MPIR_Treealgo_tree_free(comm->coll.topo_aware_tree);
                } else {
                    comm->coll.topo_aware_tree =
                        (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t),
                                                            MPL_MEM_BUFFER);
                }
                mpi_errno =
                    MPII_Treeutil_tree_topology_aware_init(comm, params,
                                                           comm->coll.topo_aware_tree);
                MPIR_ERR_CHECK(mpi_errno);
                *ct = *comm->coll.topo_aware_tree;
                comm->coll.topo_aware_tree_root = params->root;
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
            if (!comm->coll.topo_aware_tree || params->root != comm->coll.topo_aware_tree_root) {
                if (comm->coll.topo_aware_tree) {
                    MPIR_Treealgo_tree_free(comm->coll.topo_aware_tree);
                } else {
                    comm->coll.topo_aware_tree =
                        (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t),
                                                            MPL_MEM_BUFFER);
                }
                mpi_errno =
                    MPII_Treeutil_tree_topology_aware_k_init(comm, params,
                                                             comm->coll.topo_aware_tree);
                MPIR_ERR_CHECK(mpi_errno);
                *ct = *comm->coll.topo_aware_tree;
                comm->coll.topo_aware_tree_root = params->root;
            }
            *ct = *comm->coll.topo_aware_tree;
            utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
            for (int i = 0; i < ct->num_children; i++) {
                utarray_push_back(ct->children,
                                  &ut_int_array(comm->coll.topo_aware_tree->children)[i],
                                  MPL_MEM_COLL);
            }
            break;

        case MPIR_TREE_TYPE_TOPOLOGY_WAVE:
            if (!comm->coll.topo_wave_tree || params->root != comm->coll.topo_wave_tree_root) {
                if (comm->coll.topo_wave_tree) {
                    MPIR_Treealgo_tree_free(comm->coll.topo_wave_tree);
                } else {
                    comm->coll.topo_wave_tree =
                        (MPIR_Treealgo_tree_t *) MPL_malloc(sizeof(MPIR_Treealgo_tree_t),
                                                            MPL_MEM_BUFFER);
                }
                mpi_errno = MPII_Treeutil_tree_topology_wave_init(comm, params,
                                                                  comm->coll.topo_wave_tree);
                MPIR_ERR_CHECK(mpi_errno);
                *ct = *comm->coll.topo_wave_tree;
                comm->coll.topo_wave_tree_root = params->root;
            }
            *ct = *comm->coll.topo_wave_tree;
            utarray_new(ct->children, &ut_int_icd, MPL_MEM_COLL);
            for (int i = 0; i < ct->num_children; i++) {
                utarray_push_back(ct->children,
                                  &ut_int_array(comm->coll.topo_wave_tree->children)[i],
                                  MPL_MEM_COLL);
            }
            break;

        default:
            MPIR_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**treetype", "**treetype %d",
                                 params->tree_type);
    }

    MPIR_FUNC_EXIT;

  fn_exit:
    {
        if (0 < strlen(MPIR_CVAR_TREE_DUMP_FILE_PREFIX)) {
            char outfile_name[PATH_MAX];
            sprintf(outfile_name, "%s%d.json", MPIR_CVAR_TREE_DUMP_FILE_PREFIX, params->rank);
            FILE *outfile = fopen(outfile_name, "w");
            dump_node(ct, outfile);
            fclose(outfile);
        }
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
