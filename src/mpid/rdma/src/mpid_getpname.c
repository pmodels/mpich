/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPID_Get_processor_name()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Send
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_Get_processor_name(char * name, int * resultlen)
{
    if (MPIDI_Process.processor_name != NULL)
    {
	MPIU_Strncpy(name, MPIDI_Process.processor_name, 
		     MPI_MAX_PROCESSOR_NAME );
	*resultlen = (int)strlen(MPIDI_Process.processor_name);
    }
    else
    {
	return MPI_ERR_UNKNOWN;
    }

    return MPI_SUCCESS;
}
