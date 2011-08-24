/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 * Copyright © 2006-2011 Guillaume Mercier, Institut Polytechnique de
 * Bordeaux. All rights reserved. Permission is hereby granted to use,
 * reproduce, prepare derivative works, and to redistribute to others.
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

    while(mpid_nem_newmad_pending_send_req > 0)
     MPID_nem_newmad_poll(FALSE);   
   
    nm_session_destroy(mpid_nem_newmad_session);
    common_exit(NULL);

    MPID_nem_newmad_internal_req_queue_destroy();
   
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

