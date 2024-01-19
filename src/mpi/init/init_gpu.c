/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpi_init.h"

/* Initialize gpu in mpl in order to support shm gpu module initialization
 * inside MPID_Init. This also determines whether GPU support is requested
 * from typerep. */
/* FIXME: we should not be manipulating device-level CVARs from this layer */
int MPII_init_gpu(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ENABLE_GPU) {
        int debug_summary = 0;
        if (MPIR_CVAR_DEBUG_SUMMARY) {
            debug_summary = (MPIR_Process.rank == 0);
        }

        bool specialized_cache =
            (MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE == MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_specialized);

        MPL_gpu_info.specialized_cache = specialized_cache;
        MPL_gpu_info.use_immediate_cmdlist = MPIR_CVAR_GPU_USE_IMMEDIATE_COMMAND_LIST;
        MPL_gpu_info.roundrobin_cmdq = MPIR_CVAR_GPU_ROUND_ROBIN_COMMAND_QUEUES;

        int mpl_errno = MPL_gpu_init(debug_summary);
        MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_init");

        int device_count, max_dev_id, max_subdev_id;
        mpi_errno = MPL_gpu_get_dev_count(&device_count, &max_dev_id, &max_subdev_id);
        MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_init");

        if (device_count <= 0) {
            MPIR_CVAR_ENABLE_GPU = 0;
        } else {
            /* If the MPL backend doesn't support IPC, disable it for the upper layer */
            if (!MPL_gpu_info.enable_ipc) {
                MPIR_CVAR_CH4_IPC_GPU_P2P_THRESHOLD = -1;
            }
            /* If the MPL gpu backend doesn't support specialized cache, fallback to generic. */
            if (specialized_cache && !MPL_gpu_info.specialized_cache) {
                MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE = MPIR_CVAR_CH4_IPC_GPU_HANDLE_CACHE_generic;
            }
        }
    }

  fn_fail:
    return mpi_errno;
}

int MPII_finalize_gpu(void)
{
    int mpi_errno = MPI_SUCCESS;

    if (MPIR_CVAR_ENABLE_GPU) {
        int mpl_errno = MPL_gpu_finalize();
        MPIR_ERR_CHKANDJUMP(mpl_errno != MPL_SUCCESS, mpi_errno, MPI_ERR_OTHER, "**gpu_finalize");
    }

  fn_fail:
    return mpi_errno;
}
