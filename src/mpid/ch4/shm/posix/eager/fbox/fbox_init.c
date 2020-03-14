/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpidimpl.h"
#include "fbox_noinline.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE
      category    : CH4
      type        : int
      default     : 3
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        The size of the array to store expected receives to speed up fastbox polling.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#define FBOX_INDEX(sender, receiver)  ((MPIDI_POSIX_global.num_local) * (sender) + (receiver))
#define FBOX_OFFSET(sender, receiver) (MPIDI_POSIX_FBOX_SIZE * FBOX_INDEX(sender, receiver))

int MPIDI_POSIX_fbox_init(int rank, int size)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_INIT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_INIT);

#ifdef MPL_USE_DBG_LOGGING
    MPIDI_CH4_SHM_POSIX_FBOX_GENERAL = MPL_dbg_class_alloc("SHM_POSIX_FBOX", "shm_posix_fbox");
#endif /* MPL_USE_DBG_LOGGING */

    MPIR_CHKPMEM_DECL(3);

    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks,
                        int16_t *,
                        sizeof(*MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks) *
                        (MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE + 1), mpi_errno,
                        "first_poll_local_ranks", MPL_MEM_SHM);

    /* -1 means we aren't looking for anything in particular. */
    for (i = 0; i < MPIR_CVAR_CH4_POSIX_EAGER_FBOX_POLL_CACHE_SIZE; i++) {
        MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks[i] = -1;
    }

    /* The final entry in the cache array is for tracking the fallback place to start looking for
     * messages if all other entries in the cache are empty. */
    MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks[i] = 0;

    /* Create region with one fastbox for every pair of local processes. */
    size_t len =
        MPIDI_POSIX_global.num_local * MPIDI_POSIX_global.num_local * MPIDI_POSIX_FBOX_SIZE;

    /* Actually allocate the segment and assign regions to the pointers */
    mpi_errno = MPIDU_Init_shm_alloc(len, &MPIDI_POSIX_eager_fbox_control_global.shm_ptr);
    MPIR_ERR_CHECK(mpi_errno);

    /* Allocate table of pointers to fastboxes */
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in,
                        MPIDI_POSIX_fastbox_t **,
                        MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_fastbox_t *), mpi_errno,
                        "fastboxes", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out,
                        MPIDI_POSIX_fastbox_t **,
                        MPIDI_POSIX_global.num_local * sizeof(MPIDI_POSIX_fastbox_t *), mpi_errno,
                        "fastboxes", MPL_MEM_SHM);

    /* Fill in fbox tables */
    char *fastboxes_p = (char *) MPIDI_POSIX_eager_fbox_control_global.shm_ptr;
    for (i = 0; i < MPIDI_POSIX_global.num_local; i++) {
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i] =
            (void *) (fastboxes_p + FBOX_OFFSET(i, MPIDI_POSIX_global.my_local_rank));
        MPIDI_POSIX_eager_fbox_control_global.mailboxes.out[i] =
            (void *) (fastboxes_p + FBOX_OFFSET(MPIDI_POSIX_global.my_local_rank, i));

        memset(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in[i], 0, MPIDI_POSIX_FBOX_SIZE);
    }

    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    MPIR_CHKPMEM_COMMIT();

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_INIT);
    return mpi_errno;
  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIDI_POSIX_fbox_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_POSIX_FBOX_FINALIZE);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_POSIX_FBOX_FINALIZE);

    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.in);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.mailboxes.out);
    MPL_free(MPIDI_POSIX_eager_fbox_control_global.first_poll_local_ranks);

    mpi_errno = MPIDU_Init_shm_free(MPIDI_POSIX_eager_fbox_control_global.shm_ptr);

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_POSIX_FBOX_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
