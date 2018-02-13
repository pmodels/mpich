/*
    src/mpi/coll/transports/gentran/tsp_gentran_types.h \
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2017 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

cvars:
    - name        : MPIR_CVAR_PROGRESS_MAX_COLLS
      category    : COLLECTIVE
      type        : int
      default     : 8
      class       : device
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        Maximum number of collective operations at a time that the
        progress engine should make progress on

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

#include "mpiimpl.h"
#include "tsp_gentran_types.h"
#include "tsp_gentran.h"
#include "gentran_utils.h"

MPII_Coll_queue_t coll_queue = { NULL };

int MPII_Genutil_progress_hook_id = 0;

#undef FUNCNAME
#define FUNCNAME MPII_Gentran_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Gentran_init()
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPID_Progress_register_hook(MPII_Genutil_progress_hook, &MPII_Genutil_progress_hook_id);
    if (mpi_errno)
        MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

  fn_fail:
    goto fn_exit;
}


#undef FUNCNAME
#define FUNCNAME MPII_Gentran_comm_init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Gentran_comm_init(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Gentran_comm_cleanup
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Gentran_comm_cleanup(MPIR_Comm * comm_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Gentran_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Gentran_finalize()
{
    int mpi_errno = MPI_SUCCESS;

    MPID_Progress_deregister_hook(MPII_Genutil_progress_hook_id);

    return mpi_errno;
}


#undef FUNCNAME
#define FUNCNAME MPII_Gentran_scheds_are_pending
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPII_Gentran_scheds_are_pending()
{
    return coll_queue.head != NULL;
}
