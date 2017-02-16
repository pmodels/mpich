/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2017 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiu_addr.h"
#include "mpidimpl.h"
#include "mpidu_shm.h"
#include "pmi.h"
#include "mpl.h"

static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;

/* FIXME: assumes all addresses are of equal size */
#undef FUNCNAME
#define FUNCNAME MPIU_addr_table_create
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIU_addr_table_create(int size, int rank, int local_size, int local_rank,
                           int local_leader, void *addr, int addr_len,
                           void **addr_table)
{
    int rc, mpi_errno = MPI_SUCCESS;
    int start, end, i;
    int key_max, val_max, name_max, out_len, rem;
    char *kvsname = NULL, *key = NULL, *val = NULL, *val_p;
    char *segment = NULL;

    rc = PMI_KVS_Get_name_length_max(&name_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_name_length_max");
    rc = PMI_KVS_Get_key_length_max(&key_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_key_length_max");
    rc = PMI_KVS_Get_value_length_max(&val_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_value_length_max");

    kvsname = MPL_malloc(name_max);
    MPIR_Assert(kvsname);
    rc = PMI_KVS_Get_my_name(kvsname, name_max);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get_my_name");

    val = MPL_malloc(val_max);
    memset(val, 0, val_max);
    val_p = val;
    rem = val_max;
    rc = MPL_str_add_binary_arg(&val_p, &rem, "mpi", (char *)addr, addr_len);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**buscard");
    MPIR_Assert(rem >= 0);

    key = MPL_malloc(key_max);
    MPIR_Assert(key);
    sprintf(key, "bc-%d", rank);
    rc = PMI_KVS_Put(kvsname, key, val);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_put");
    rc = PMI_KVS_Commit(kvsname);
    MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_commit");

    mpi_errno = MPIDU_shm_seg_alloc(addr_len * size, (void **)&segment);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIDU_shm_seg_commit(&memory, &barrier, local_size, local_rank, local_leader, rank);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    start = local_rank * (size / local_size);
    end = start + (size / local_size);
    if (local_rank == local_size - 1)
        end += size % local_size;
    for (i = start; i < end; i++) {
        sprintf(key, "bc-%d", i);
        rc = PMI_KVS_Get(kvsname, key, val, val_max);
	MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**pmi_kvs_get");
        rc = MPL_str_get_binary_arg(val, "mpi", &segment[i * addr_len], addr_len, &out_len);
        MPIR_Assert(out_len == addr_len);
        MPIR_ERR_CHKANDJUMP(rc, mpi_errno, MPI_ERR_OTHER, "**argstr_missinghost");
    }
    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    MPL_free(kvsname);
    MPL_free(key);
    MPL_free(val);
    *addr_table = segment;

    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIU_addr_table_destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIU_addr_table_destroy(void *addr_table, int local_size)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    mpi_errno = MPIDU_shm_seg_destroy(&memory, local_size);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
