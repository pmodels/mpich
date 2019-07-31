/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2018 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */

#include "mpidimpl.h"
#include "mpidu_shm.h"
#include "mpl.h"
#include "mpidu_bc.h"

static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;
static size_t *indices;
static int local_size;
static char *segment;

#undef FUNCNAME
#define FUNCNAME MPIDU_bc_table_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_bc_table_destroy(void *bc_table)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIDU_shm_seg_destroy(&memory, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    MPL_free(indices);

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPIDU_bc_allgather
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_bc_allgather(MPIR_Comm * comm, int *nodemap, void *bc, int bc_len, int same_len,
                       void **bc_table, size_t ** bc_indices)
{
    int mpi_errno = MPI_SUCCESS;
    int local_rank = -1, local_leader = -1;
    int rank = MPIR_Comm_rank(comm), size = MPIR_Comm_size(comm);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (!same_len) {
        bc_len *= 2;
        *bc_indices = indices;
    }

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &local_rank, &local_leader);
    if (rank != local_leader) {
        size_t start = local_leader - nodemap[comm->rank] + (local_rank - 1);

        memcpy(&segment[start * bc_len], bc, bc_len);
    }

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (rank == local_leader) {
        MPIR_Errflag_t errflag = MPIR_ERR_NONE;
        MPIR_Comm *allgather_comm = comm->node_roots_comm ? comm->node_roots_comm : comm;

        MPIR_Allgather_impl(MPI_IN_PLACE, 0, MPI_DATATYPE_NULL, segment, (local_size - 1) * bc_len,
                            MPI_BYTE, allgather_comm, &errflag);
    }

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    *bc_table = segment;

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#ifdef USE_PMIX_API
#include <pmix.h>

#define VALLEN 1024
#define KEYLEN 64

#undef FUNCNAME
#define FUNCNAME MPIDU_bc_table_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, size_t ** bc_indices)
{
    int rc, mpi_errno = MPI_SUCCESS;
    int start, end, i;
    char *val = NULL, *val_p;
    int out_len, val_len, rem, flag;
    pmix_value_t value, *pvalue;
    pmix_info_t *info;
    pmix_proc_t proc;
    int local_rank, local_leader;
    size_t my_bc_len = bc_len;

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &local_rank, &local_leader);

    /* if business cards can be different length, use the max value length */
    if (!same_len)
        bc_len = VALLEN;
    mpi_errno = MPIDU_shm_seg_alloc(bc_len * size, (void **) &segment, MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, local_size, local_rank, local_leader, rank,
                             MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (size == 1) {
        memcpy(segment, bc, my_bc_len);
        goto single;
    }

    val = MPL_malloc(VALLEN, MPL_MEM_ADDRESS);
    memset(val, 0, VALLEN);
    val_p = val;
    rem = VALLEN;
    rc = MPL_str_add_binary_arg(&val_p, &rem, "mpi", (char *) bc, my_bc_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
    MPIR_Assert(rem >= 0);

    if (!roots_only || rank == local_leader) {
        value.type = PMIX_STRING;
        value.data.string = val;
        rc = PMIx_Put(PMIX_LOCAL, "bc", &value);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_put");
        rc = PMIx_Put(PMIX_REMOTE, "bc", &value);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_put");
        rc = PMIx_Commit();
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_commit");
    }

    PMIX_INFO_CREATE(info, 1);
    PMIX_INFO_LOAD(info, PMIX_COLLECT_DATA, &flag, PMIX_BOOL);
    rc = PMIx_Fence(&MPIR_Process.pmix_wcproc, 1, info, 1);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_fence");
    PMIX_INFO_FREE(info, 1);

    if (!roots_only) {
        start = local_rank * (size / local_size);
        end = start + (size / local_size);
        if (local_rank == local_size - 1)
            end += size % local_size;
        for (i = start; i < end; i++) {
            PMIX_PROC_CONSTRUCT(&proc);
            MPL_strncpy(proc.nspace, MPIR_Process.pmix_proc.nspace, PMIX_MAX_NSLEN);
            proc.rank = i;
            rc = PMIx_Get(&proc, "bc", NULL, 0, &pvalue);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_get");
            rc = MPL_str_get_binary_arg(pvalue->data.string, "mpi", &segment[i * bc_len], bc_len,
                                        &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
            PMIX_VALUE_RELEASE(pvalue);
        }
    } else {
        int num_nodes, *node_roots;
        MPIR_NODEMAP_get_node_roots(nodemap, size, &node_roots, &num_nodes);

        start = local_rank * (num_nodes / local_size);
        end = start + (num_nodes / local_size);
        if (local_rank == local_size - 1)
            end += num_nodes % local_size;
        for (i = start; i < end; i++) {
            PMIX_PROC_CONSTRUCT(&proc);
            MPL_strncpy(proc.nspace, MPIR_Process.pmix_proc.nspace, PMIX_MAX_NSLEN);
            proc.rank = i;
            rc = PMIx_Get(&proc, "bc", NULL, 0, &pvalue);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmix_get");
            rc = MPL_str_get_binary_arg(pvalue->data.string, "mpi", &segment[i * bc_len], bc_len,
                                        &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
            PMIX_VALUE_RELEASE(pvalue);
        }
    }
    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  single:
    if (!same_len) {
        indices = MPL_malloc(size * sizeof(size_t), MPL_MEM_ADDRESS);
        for (i = 0; i < size; i++)
            indices[i] = bc_len * i;
        *bc_indices = indices;
    }

  fn_exit:
    MPL_free(val);
    *bc_table = segment;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#elif defined(USE_PMI2_API)
#include <pmi2.h>

#undef FUNCNAME
#define FUNCNAME MPIDU_bc_table_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, size_t ** bc_indices)
{
    int rc, mpi_errno = MPI_SUCCESS;
    int start, end, i;
    int out_len, val_len, rem;
    char *key = NULL, *val = NULL, *val_p;
    int local_rank, local_leader;
    size_t my_bc_len = bc_len;

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &local_rank, &local_leader);

    /* if business cards can be different length, use the max value length */
    if (!same_len)
        bc_len = PMI2_MAX_VALLEN;
    mpi_errno = MPIDU_shm_seg_alloc(bc_len * size, (void **) &segment, MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, local_size, local_rank, local_leader, rank,
                             MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (size == 1) {
        memcpy(segment, bc, my_bc_len);
        goto single;
    }

    val = MPL_malloc(PMI2_MAX_VALLEN, MPL_MEM_ADDRESS);
    memset(val, 0, PMI2_MAX_VALLEN);
    val_p = val;
    rem = PMI2_MAX_VALLEN;
    rc = MPL_str_add_binary_arg(&val_p, &rem, "mpi", (char *) bc, my_bc_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
    MPIR_Assert(rem >= 0);

    key = MPL_malloc(PMI2_MAX_KEYLEN, MPL_MEM_ADDRESS);
    MPIR_Assert(key);
    if (!roots_only || rank == local_leader) {
        sprintf(key, "bc-%d", rank);
        rc = PMI2_KVS_Put(key, val);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsput");
    }
    rc = PMI2_KVS_Fence();
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsfence");

    if (!roots_only) {
        start = local_rank * (size / local_size);
        end = start + (size / local_size);
        if (local_rank == local_size - 1)
            end += size % local_size;
        for (i = start; i < end; i++) {
            sprintf(key, "bc-%d", i);
            rc = PMI2_KVS_Get(NULL, -1, key, val, PMI2_MAX_VALLEN, &val_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsget");
            rc = MPL_str_get_binary_arg(val, "mpi", &segment[i * bc_len], bc_len, &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
        }
    } else {
        int num_nodes, *node_roots;
        MPIR_NODEMAP_get_node_roots(nodemap, size, &node_roots, &num_nodes);

        start = local_rank * (num_nodes / local_size);
        end = start + (num_nodes / local_size);
        if (local_rank == local_size - 1)
            end += num_nodes % local_size;
        for (i = start; i < end; i++) {
            sprintf(key, "bc-%d", node_roots[i]);
            rc = PMI2_KVS_Get(NULL, -1, key, val, PMI2_MAX_VALLEN, &val_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvsget");
            rc = MPL_str_get_binary_arg(val, "mpi", &segment[i * bc_len], bc_len, &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
        }
        MPL_free(node_roots);
    }
    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  single:
    if (!same_len) {
        indices = MPL_malloc(size * sizeof(size_t), MPL_MEM_ADDRESS);
        for (i = 0; i < size; i++)
            indices[i] = bc_len * i;
        *bc_indices = indices;
    }

  fn_exit:
    MPL_free(key);
    MPL_free(val);
    *bc_table = segment;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#else
#include <pmi.h>

#undef FUNCNAME
#define FUNCNAME MPIDU_bc_table_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDU_bc_table_create(int rank, int size, int *nodemap, void *bc, int bc_len, int same_len,
                          int roots_only, void **bc_table, size_t ** bc_indices)
{
    int rc, mpi_errno = MPI_SUCCESS;
    int start, end, i;
    int key_max, val_max, name_max, out_len, rem;
    char *kvsname = NULL, *key = NULL, *val = NULL, *val_p;
    int local_rank = -1, local_leader = -1;
    size_t my_bc_len = bc_len;

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &local_rank, &local_leader);

    rc = PMI_KVS_Get_name_length_max(&name_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max");
    rc = PMI_KVS_Get_key_length_max(&key_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max");
    rc = PMI_KVS_Get_value_length_max(&val_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");

    /* if business cards can be different length, use the max value length */
    if (!same_len)
        bc_len = val_max;
    mpi_errno = MPIDU_shm_seg_alloc(bc_len * size, (void **) &segment, MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);
    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, local_size, local_rank, local_leader, rank,
                             MPL_MEM_ADDRESS);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

    if (size == 1) {
        memcpy(segment, bc, my_bc_len);
        goto single;
    }

    kvsname = MPL_malloc(name_max, MPL_MEM_ADDRESS);
    MPIR_Assert(kvsname);
    rc = PMI_KVS_Get_my_name(kvsname, name_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_my_name");

    val = MPL_malloc(val_max, MPL_MEM_ADDRESS);
    memset(val, 0, val_max);
    val_p = val;
    rem = val_max;
    rc = MPL_str_add_binary_arg(&val_p, &rem, "mpi", (char *) bc, my_bc_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
    MPIR_Assert(rem >= 0);

    key = MPL_malloc(key_max, MPL_MEM_ADDRESS);
    MPIR_Assert(key);
    if (!roots_only || rank == local_leader) {
        sprintf(key, "bc-%d", rank);
        rc = PMI_KVS_Put(kvsname, key, val);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put");
        rc = PMI_KVS_Commit(kvsname);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit");
    }
    rc = PMI_Barrier();
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_barrier");

    if (!roots_only) {
        start = local_rank * (size / local_size);
        end = start + (size / local_size);
        if (local_rank == local_size - 1)
            end += size % local_size;
        for (i = start; i < end; i++) {
            sprintf(key, "bc-%d", i);
            rc = PMI_KVS_Get(kvsname, key, val, val_max);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
            rc = MPL_str_get_binary_arg(val, "mpi", &segment[i * bc_len], bc_len, &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
        }
    } else {
        int num_nodes, *node_roots;
        MPIR_NODEMAP_get_node_roots(nodemap, size, &node_roots, &num_nodes);

        start = local_rank * (num_nodes / local_size);
        end = start + (num_nodes / local_size);
        if (local_rank == local_size - 1)
            end += num_nodes % local_size;
        for (i = start; i < end; i++) {
            sprintf(key, "bc-%d", node_roots[i]);
            rc = PMI_KVS_Get(kvsname, key, val, val_max);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
            rc = MPL_str_get_binary_arg(val, "mpi", &segment[i * bc_len], bc_len, &out_len);
            MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
        }
        MPL_free(node_roots);
    }
    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  single:
    if (!same_len) {
        indices = MPL_malloc(size * sizeof(size_t), MPL_MEM_ADDRESS);
        MPIR_Assert(indices);
        for (i = 0; i < size; i++)
            indices[i] = bc_len * i;
        *bc_indices = indices;
    }

  fn_exit:
    MPL_free(kvsname);
    MPL_free(key);
    MPL_free(val);
    *bc_table = segment;

    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* PMI1 */
