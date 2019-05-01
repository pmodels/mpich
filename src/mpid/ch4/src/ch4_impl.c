/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"
#include "ch4_impl.h"

int MPIDIG_get_context_index(uint64_t context_id)
{
    int raw_prefix, idx, bitpos, gen_id;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GET_CONTEXT_INDEX);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GET_CONTEXT_INDEX);

    raw_prefix = MPIR_CONTEXT_READ_FIELD(PREFIX, context_id);
    idx = raw_prefix / MPIR_CONTEXT_INT_BITS;
    bitpos = raw_prefix % MPIR_CONTEXT_INT_BITS;
    gen_id = (idx * MPIR_CONTEXT_INT_BITS) + (31 - bitpos);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GET_CONTEXT_INDEX);
    return gen_id;
}

uint64_t MPIDIG_generate_win_id(MPIR_Comm * comm_ptr)
{
    uint64_t ret;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_GENERATE_WIN_ID);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_GENERATE_WIN_ID);

    /* context id lower bits, window instance upper bits */
    ret = 1 + (((uint64_t) comm_ptr->context_id) |
               ((uint64_t) ((MPIDIG_COMM(comm_ptr, window_instance))++) << 32));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_GENERATE_WIN_ID);
    return ret;
}

/* Collectively allocate shared memory region.
 * MPL_shm routines and MPI collectives are internally used.
 * The parameter *mapfail_flag_ptr is set to true and MPI_SUCCESS is returned
 * if mapping fails (e.g., no memory resource, or opened too many files), thus the
 * caller can choose fallback if exists. If communication fails it returns an MPI error.*/
int MPIDIU_allocate_shm_segment(MPIR_Comm * shm_comm_ptr, MPI_Aint shm_segment_len,
                                MPL_shm_hnd_t * shm_segment_hdl_ptr, void **base_ptr,
                                bool * mapfail_flag_ptr)
{
    MPIR_Errflag_t errflag = MPIR_ERR_NONE;
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;
    bool any_shm_fail_flag = false, shm_fail_flag = false, mapped_flag = false;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIU_ALLOCATE_SHM_SEGMENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIU_ALLOCATE_SHM_SEGMENT);

    mpl_err = MPL_shm_hnd_init(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**alloc_shar_mem");

    *mapfail_flag_ptr = false;
    *base_ptr = NULL;
    if (shm_comm_ptr->rank == 0) {
        char *serialized_hnd_ptr = NULL;
        char mpl_err_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* create shared memory region for all processes in win and map */
        mpl_err = MPL_shm_seg_create_and_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
        if (mpl_err != MPL_SHM_SUCCESS) {
            shm_fail_flag = true;
            goto hnd_sync;
        } else
            mapped_flag = true;

        /* serialize handle and broadcast it to the other processes in win */
        mpl_err = MPL_shm_hnd_get_serialized_by_ref(*shm_segment_hdl_ptr, &serialized_hnd_ptr);
        if (mpl_err != MPL_SHM_SUCCESS)
            shm_fail_flag = true;

      hnd_sync:
        /* bcast empty hnd as error reporting */
        if (shm_fail_flag)
            serialized_hnd_ptr = &mpl_err_hnd[0];
        mpi_errno = MPIR_Bcast_impl(serialized_hnd_ptr, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                                    shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        if (shm_fail_flag)
            goto map_fail;

        /* ensure all other processes have mapped successfully */
        mpi_errno = MPIR_Allreduce(&shm_fail_flag, &any_shm_fail_flag, 1, MPI_C_BOOL,
                                   MPI_LOR, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* unlink shared memory region so it gets deleted when all processes exit */
        mpl_err = MPL_shm_seg_remove(*shm_segment_hdl_ptr);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");

        if (any_shm_fail_flag)
            goto map_fail;

    } else {
        char serialized_hnd[MPL_SHM_GHND_SZ] = { 0 };

        /* get serialized handle from rank 0 and deserialize it */
        mpi_errno = MPIR_Bcast_impl(serialized_hnd, MPL_SHM_GHND_SZ, MPI_CHAR, 0,
                                    shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        /* empty handler means root fails */
        if (strlen(serialized_hnd) == 0)
            goto map_fail;

        mpl_err = MPL_shm_hnd_deserialize(*shm_segment_hdl_ptr, serialized_hnd,
                                          strlen(serialized_hnd));
        if (mpl_err != MPL_SHM_SUCCESS) {
            shm_fail_flag = true;
            goto result_sync;
        }

        /* attach to shared memory region created by rank 0 */
        mpl_err = MPL_shm_seg_attach(*shm_segment_hdl_ptr, shm_segment_len, base_ptr, 0);
        if (mpl_err != MPL_SHM_SUCCESS) {
            shm_fail_flag = true;
            goto result_sync;
        } else
            mapped_flag = true;

      result_sync:
        mpi_errno = MPIR_Allreduce(&shm_fail_flag, &any_shm_fail_flag, 1, MPI_C_BOOL,
                                   MPI_LOR, shm_comm_ptr, &errflag);
        MPIR_ERR_CHKANDJUMP(errflag, mpi_errno, MPI_ERR_OTHER, "**coll_fail");

        if (any_shm_fail_flag)
            goto map_fail;
    }

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIU_ALLOCATE_SHM_SEGMENT);
    return mpi_errno;
  map_fail:
    /* clean up shm mapping resource */
    if (mapped_flag) {
        mpl_err = MPL_shm_seg_detach(*shm_segment_hdl_ptr, base_ptr, shm_segment_len);
        MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");
        *base_ptr = NULL;
    }
    mpl_err = MPL_shm_hnd_finalize(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");
    *mapfail_flag_ptr = true;
    goto fn_exit;
  fn_fail:
    goto fn_exit;
}

/* Destroy shared memory region on the local process.
 * MPL_shm routines are internally used. */
int MPIDIU_destroy_shm_segment(MPI_Aint shm_segment_len, MPL_shm_hnd_t * shm_segment_hdl_ptr,
                               void **base_ptr)
{
    int mpi_errno = MPI_SUCCESS, mpl_err = 0;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDIG_DESTROY_SHM_SEGMENT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDIG_DESTROY_SHM_SEGMENT);

    mpl_err = MPL_shm_seg_detach(*shm_segment_hdl_ptr, base_ptr, shm_segment_len);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**detach_shar_mem");

    mpl_err = MPL_shm_hnd_finalize(shm_segment_hdl_ptr);
    MPIR_ERR_CHKANDJUMP(mpl_err, mpi_errno, MPI_ERR_OTHER, "**remove_shar_mem");

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDIG_DESTROY_SHM_SEGMENT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
