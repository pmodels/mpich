/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpir_pset.h"

#define MPIR_PSET_NUM_DEFAULT_PSETS 2

/* Add all MPI default psets to the session */
int MPIR_Session_add_default_psets(MPIR_Session * session_ptr)
{
    int mpi_errno = MPI_SUCCESS;

    session_ptr->num_psets = MPIR_PSET_NUM_DEFAULT_PSETS;
    session_ptr->psets = MPL_malloc(session_ptr->num_psets * sizeof(struct MPIR_Pset),
                                    MPL_MEM_GROUP);
    MPIR_ERR_CHKANDJUMP(!session_ptr->psets, mpi_errno, MPI_ERR_OTHER, "**nomem");

    session_ptr->psets[0].name = MPL_strdup(MPIR_PSET_WORLD_NAME);
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_WORLD_PTR, session_ptr, &session_ptr->psets[0].group);
    MPIR_ERR_CHECK(mpi_errno);

    session_ptr->psets[1].name = MPL_strdup(MPIR_PSET_SELF_NAME);
    mpi_errno = MPIR_Group_dup(MPIR_GROUP_SELF_PTR, session_ptr, &session_ptr->psets[1].group);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
