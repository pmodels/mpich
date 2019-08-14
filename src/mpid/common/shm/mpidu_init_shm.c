/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpidimpl.h>
#include "mpidu_init_shm.h"
#include "build_nodemap.h"

/******************************************************/
/* NOTE: Proof of concept (I don't quite understand the need for current complexity */

#define INIT_SHM_NAME "/dev/shm/mpich-init-shm"

struct MPIDU_init_block {
    char bc[MPIDU_MAX_BC_SIZE];
    /* each feature can add their field here */
};

struct MPIDU_init_shm {
    MPIDU_shm_barrier barrier;
    struct MPIDU_init_block node_block[0]; /* FIXME: use C99 flexible array member with empty brackets */
};

static int init_shm_fd;
static struct MPIDU_init_shm *init_shm;
static int init_shm_size;

int MPIDU_init_shm_init(void)
{
    int mpi_errno = MPI_SUCCESS;
    /* NOTE: potentially we still need create unique handle and use PMI to exchange it */
    /* It'd be nice if PMI provide us a unique id */
    init_shm_fd = shm_open(INIT_SHM_NAME, O_CREAT | O_EXCL | ORDWR, 0600);
    MPIR_ERR_CHKANDJUMP(init_shm_fd < 0, mpi_errno, MPI_ERR_OTHER, "**init_shm_init");

    init_shm_size = sizeof(struct MPIDU_init_shm) +
                    sizeof(struct MPIDU_Init_shm_block) * MPIR_Process.local_size;
    ftruncate(init_shm_fd, init_shm_size);
    init_shm = (struct MPIDU_Init_shm_block *) mmap(0, total_size, PROT_READ | PROT_WRITE,
                                                           MAP_SHARED, init_shm_fd, 0);
    mpi_errno = MPIDU_init_shm_barrier_init(init_shm->barrier);
    MPIR_ERR_CHECK(mpi_errno);

    MPIDU_init_shm_barrier();

  fn_exit:  
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_init_shm_finish(void)
{
    munmap(init_shm, init_shm_size);
    close(init_shm_fd);
    return MPI_SUCCESS;
}

int MPIDU_init_shm_barrier(void)
{
    /* to be implemented */
    return MPI_SUCCESS;
}

/******************************************************/
static int local_size;
static int my_local_rank;
static MPIDU_shm_seg_t memory;
static MPIDU_shm_barrier_t *barrier;
static void *baseaddr;

int MPIDU_Init_shm_init(int rank, int size, int *nodemap)
{
    int mpi_errno = MPI_SUCCESS;
    int local_leader;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_SEG_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_SEG_INIT);

    MPIR_NODEMAP_get_local_info(rank, size, nodemap, &local_size, &my_local_rank, &local_leader);

    mpi_errno =
        MPIDU_shm_seg_alloc(local_size * sizeof(MPIDU_Init_shm_block_t), (void **) &baseaddr,
                            MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno =
        MPIDU_shm_seg_commit(&memory, &barrier, local_size, my_local_rank, local_leader, rank,
                             MPL_MEM_SHM);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_SEG_INIT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_SEG_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_SEG_FINALIZE);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);
    MPIR_ERR_CHECK(mpi_errno);

    mpi_errno = MPIDU_shm_seg_destroy(&memory, local_size);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_SEG_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIDU_Init_shm_barrier(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_SEG_BARRIER);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_SEG_BARRIER);

    mpi_errno = MPIDU_shm_barrier(barrier, local_size);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_SEG_BARRIER);

    return mpi_errno;
}

int MPIDU_Init_shm_put(void *orig, size_t len)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_SEG_PUT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_SEG_PUT);

    MPIR_Assert(len <= sizeof(MPIDU_Init_shm_block_t));
    MPIR_Memcpy(baseaddr + my_local_rank * sizeof(MPIDU_Init_shm_block_t), orig, len);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_SEG_PUT);

    return mpi_errno;
}

int MPIDU_Init_shm_get(int local_rank, void **target)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDU_INIT_SHM_SEG_GET);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDU_INIT_SHM_SEG_GET);

    MPIR_Assert(local_rank < local_size);
    *target = baseaddr + local_rank * sizeof(MPIDU_Init_shm_block_t);

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDU_INIT_SHM_SEG_GET);

    return mpi_errno;
}
