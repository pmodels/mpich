/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_PROGRESS_MAX_COLLS
      category    : COLLECTIVE
      type        : int
      default     : 0
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of collective operations at a time that the
        progress engine should make progress on

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "gentran_impl.h"

MPII_Coll_queue_t MPII_coll_queue = { NULL };

int MPII_Genutil_progress_hook_id = 0;

int MPII_TSP_init(void)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIR_Progress_hook_register(MPII_Genutil_progress_hook, &MPII_Genutil_progress_hook_id);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


int MPII_TSP_comm_init(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_TSP_comm_cleanup(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


int MPII_TSP_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Progress_hook_deregister(MPII_Genutil_progress_hook_id);

    return mpi_errno;
}


int MPII_TSP_scheds_are_pending(void)
{
    /* this function is only called within a critical section to decide whether
     * yield is necessary. (ref: .../ch3/.../mpid_nem_inline.h)
     * therefore, there is no need for additional lock protection.
     */
    return MPII_coll_queue.head != NULL;
}
