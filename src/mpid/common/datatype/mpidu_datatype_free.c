/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */

/*
 *  (C) 2002 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <mpiimpl.h>
#include <mpidu_dataloop.h>
#include <stdlib.h>
#include <limits.h>

/* #define MPID_TYPE_ALLOC_DEBUG */

/*@

  MPIDU_Datatype_free

Input Parameters:
. MPIDU_Datatype ptr - pointer to MPID datatype structure that is no longer
  referenced

Output Parameters:
  none

  Return Value:
  none

  This function handles freeing dynamically allocated memory associated with
  the datatype.  In the process MPIDU_Datatype_free_contents() is also called,
  which handles decrementing reference counts to constituent types (in
  addition to freeing the space used for contents information).
  MPIDU_Datatype_free_contents() will call MPIDU_Datatype_free() on constituent
  types that are no longer referenced as well.

  @*/
void MPIDU_Datatype_free(MPIDU_Datatype *ptr)
{
    MPL_DBG_MSG_P(MPIR_DBG_DATATYPE,VERBOSE,"type %x freed.", ptr->handle);

#ifdef MPID_Dev_datatype_destroy_hook
       MPID_Dev_datatype_destroy_hook(ptr);
#endif /* MPID_Dev_datatype_destroy_hook */

    /* before freeing the contents, check whether the pointer is not
       null because it is null in the case of a datatype shipped to the target
       for RMA ops */  
    if (ptr->contents) {
        MPIDU_Datatype_free_contents(ptr);
    }
    if (ptr->dataloop) {
	MPIDU_Dataloop_free(&(ptr->dataloop));
    }
#if defined(MPID_HAS_HETERO) || 1
    if (ptr->hetero_dloop) {
	MPIDU_Dataloop_free(&(ptr->hetero_dloop));
    }
#endif /* MPID_HAS_HETERO */
    MPIR_Handle_obj_free(&MPIDU_Datatype_mem, ptr);
}
