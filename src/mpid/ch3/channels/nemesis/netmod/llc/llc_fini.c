/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/* vim: set ts=8 sts=4 sw=4 noexpandtab : */
/*
 *
 */



#include "mpid_nem_impl.h"
#include "llc_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_llc_finalize
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_llc_finalize(void)
{
    int mpi_errno = MPI_SUCCESS;
    int rc;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_LLC_FINALIZE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_LLC_FINALIZE);

    rc = LLC_finalize();
    MPIR_ERR_CHKANDJUMP(rc != 0, mpi_errno, MPI_ERR_OTHER, "**fail");

  fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_LLC_FINALIZE);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}
