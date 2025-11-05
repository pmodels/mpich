/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "cma_post.h"
#include "mpidu_init_shm.h"

int MPIDI_CMA_init_local(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    bool cma_happy = false;

    /* check ptrace permission. ref: https://www.kernel.org/doc/Documentation/security/Yama.txt */
    int fd = open("/proc/sys/kernel/yama/ptrace_scope", O_RDONLY);
    char buffer = '0';
    if (fd >= 0) {
        int ret = read(fd, &buffer, 1);
        if (ret >= 0) {
            if (buffer == '0') {
                cma_happy = true;
            }
            close(fd);
        }
    }

    if (!cma_happy) {
        /* strictly we need a collective, which is troublesome during init.
         * For now, let's assume users will have ptrace_scope setting consistently.
         */
        MPIR_CVAR_CH4_CMA_ENABLE = 0;
    }

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CMA_comm_bootstrap(MPIR_Comm * comm)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CMA_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_ENTER;

    MPIR_FUNC_EXIT;
    return mpi_errno;
}
