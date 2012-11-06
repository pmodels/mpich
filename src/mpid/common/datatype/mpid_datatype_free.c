/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpiimpl.h>
#include <mpid_dataloop.h>
#include <stdlib.h>
#include <limits.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@

  MPID_Datatype_free

Input Parameters:
. MPID_Datatype ptr - pointer to MPID datatype structure that is no longer
  referenced

Output Parameters:
  none

  Return Value:
  none

  This function handles freeing dynamically allocated memory associated with
  the datatype.  In the process MPID_Datatype_free_contents() is also called,
  which handles decrementing reference counts to constituent types (in
  addition to freeing the space used for contents information).
  MPID_Datatype_free_contents() will call MPID_Datatype_free() on constituent
  types that are no longer referenced as well.

  @*/
void MPID_Datatype_free(MPID_Datatype *ptr)
{
    MPIU_DBG_MSG_P(DATATYPE,VERBOSE,"type %x freed.", ptr->handle);

#ifdef MPID_Dev_datatype_destroy_hook
       MPID_Dev_datatype_destroy_hook(ptr);
#endif /* MPID_Dev_datatype_destroy_hook */

    /* before freeing the contents, check whether the pointer is not
       null because it is null in the case of a datatype shipped to the target
       for RMA ops */  
    if (ptr->contents) {
        MPID_Datatype_free_contents(ptr);
    }
    if (ptr->dataloop) {
	MPID_Dataloop_free(&(ptr->dataloop));
    }
#if defined(MPID_HAS_HETERO) || 1
    if (ptr->hetero_dloop) {
	MPID_Dataloop_free(&(ptr->hetero_dloop));
    }
#endif /* MPID_HAS_HETERO */
    MPIU_Handle_obj_free(&MPID_Datatype_mem, ptr);
}
