/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"
#include "mpidu_init_shm.h"
#include "pip_pre.h"

#ifdef MPIDI_CH4_SHM_ENABLE_PIP

#include <numa.h>
#include <sched.h>

int MPIDI_PIP_mpi_init_task_queue(MPIDI_PIP_task_queue_t * task_queue)
{
    int err;
    int mpi_errno = MPI_SUCCESS;
    task_queue->head = task_queue->tail = NULL;
    MPID_Thread_mutex_create(&task_queue->lock, &err);
    if (err) {
        fprintf(stderr, "Init queue lock error\n");
        mpi_errno = MPI_ERR_OTHER;
    }
    task_queue->task_num = 0;
    return mpi_errno;
}

int MPIDI_PIP_mpi_init_hook(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_PIP_INIT_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_PIP_INIT_HOOK);
    MPIR_CHKPMEM_DECL(4);

#ifdef MPL_USE_DBG_LOGGING
    extern MPL_dbg_class MPIDI_CH4_SHM_PIP_GENERAL;
    MPIDI_CH4_SHM_PIP_GENERAL = MPL_dbg_class_alloc("SHM_PIP", "shm_pip");
#endif /* MPL_USE_DBG_LOGGING */

    int num_local = MPIR_Process.local_size;
    MPIDI_PIP_global.num_local = num_local;
    MPIDI_PIP_global.local_rank = MPIR_Process.local_rank;
    MPIDI_PIP_global.rank = rank;

    /* NUMA info */
    int cpu = sched_getcpu();
    int local_numa_id = numa_node_of_cpu(cpu);
    int num_numa_node = numa_num_task_nodes();
    MPIDI_PIP_global.num_numa_node = num_numa_node;
    MPIDI_PIP_global.local_numa_id = local_numa_id;

    /* Allocate task queue */
    MPIR_CHKPMEM_MALLOC(MPIDI_PIP_global.task_queue, MPIDI_PIP_task_queue_t *,
                        sizeof(MPIDI_PIP_task_queue_t), mpi_errno, "pip task queue", MPL_MEM_SHM);
    mpi_errno = MPIDI_PIP_mpi_init_task_queue(MPIDI_PIP_global.task_queue);
    MPIR_ERR_CHECK(mpi_errno);

    /* Init local completion queue */
    MPIR_CHKPMEM_MALLOC(MPIDI_PIP_global.compl_queue, MPIDI_PIP_task_queue_t *,
                        sizeof(MPIDI_PIP_task_queue_t), mpi_errno, "pip compl queue", MPL_MEM_SHM);
    mpi_errno = MPIDI_PIP_mpi_init_task_queue(MPIDI_PIP_global.compl_queue);
    MPIR_ERR_CHECK(mpi_errno);

    /* Get task queue array */
    MPIDU_Init_shm_put(&MPIDI_PIP_global.task_queue, sizeof(MPIDI_PIP_task_queue_t *));
    MPIDU_Init_shm_barrier();
    MPIR_CHKPMEM_MALLOC(MPIDI_PIP_global.task_queue_array, MPIDI_PIP_task_queue_t **,
                        sizeof(MPIDI_PIP_task_queue_t *) * num_local,
                        mpi_errno, "pip task queue array", MPL_MEM_SHM);
    for (i = 0; i < num_local; i++)
        MPIDU_Init_shm_get(i, sizeof(MPIDI_PIP_task_queue_t *),
                           &MPIDI_PIP_global.task_queue_array[i]);
    MPIDU_Init_shm_barrier();

    /* Share MPIDI_PIP_global for future information inquiry purpose */
    MPIDU_Init_shm_put(&MPIDI_PIP_global, sizeof(MPIDI_PIP_global_t *));
    MPIDU_Init_shm_barrier();
    MPIR_CHKPMEM_MALLOC(MPIDI_PIP_global.pip_global_array, MPIDI_PIP_global_t **,
                        sizeof(MPIDI_PIP_task_queue_t *) * num_local,
                        mpi_errno, "pip global array", MPL_MEM_SHM);
    for (i = 0; i < num_local; i++)
        MPIDU_Init_shm_get(i, sizeof(MPIDI_PIP_global_t *), &MPIDI_PIP_global.pip_global_array[i]);
    MPIDU_Init_shm_barrier();

    /* For stealing rand seeds */
    srand(time(NULL) + MPIDI_PIP_global.local_rank * MPIDI_PIP_global.local_rank);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_PIP_INIT_HOOK);
    return mpi_errno;
  fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_PIP_mpi_finalize_hook(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i, ret = 0;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);


    MPIR_Assert(MPIDI_PIP_global.task_queue->task_num == 0);
    MPL_free(MPIDI_PIP_global.task_queue);

    MPIR_Assert(MPIDI_PIP_global.compl_queue->task_num == 0);
    MPL_free(MPIDI_PIP_global.compl_queue);

    MPL_free(MPIDI_PIP_global.task_queue_array);
    MPL_free(MPIDI_PIP_global.pip_global_array);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_XPMEM_FINALIZE_HOOK);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif
