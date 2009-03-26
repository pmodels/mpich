/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "newmad_impl.h"


#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_finalize
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_finalize()
{
    int mpi_errno = MPI_SUCCESS;

   fn_exit:
     return mpi_errno;
   fn_fail:  ATTRIBUTE((unused))
     goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPID_nem_newmad_ckpt_shutdown
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int
MPID_nem_newmad_ckpt_shutdown ()
{
   int mpi_errno = MPI_SUCCESS;
   fn_exit:
      return mpi_errno;
   fn_fail:  ATTRIBUTE((unused))
      goto fn_exit;
}

