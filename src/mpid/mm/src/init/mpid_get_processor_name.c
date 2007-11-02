/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* gethostname */
#endif

/*@
   MPID_Get_processor_name - get processor name

   Input Arguments:
+  char * name - name
-  int * resultlen - name buffer length

   Output Arguments:
.  int * resultlen - resulting name length

   Notes:

.N Fortran

.N Errors
.N MPI_SUCCESS
@*/
int MPID_Get_processor_name( char *name, int *resultlen )
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_GET_PROCESSOR_NAME);

    gethostname(name, *resultlen);
    *resultlen = strlen(name);

    MPIDI_FUNC_EXIT(MPID_STATE_MPID_GET_PROCESSOR_NAME);
    return MPI_SUCCESS;
}
