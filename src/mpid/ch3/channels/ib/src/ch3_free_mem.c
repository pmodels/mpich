/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

/*
 * MPIDI_CH3_Free_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Free_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Free_mem(void *addr)
{
    int mpi_errno = MPI_SUCCESS, found;
    MPIDI_CH3I_Alloc_mem_list_t *prev_ptr, *cur_ptr;

    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_FREE_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_FREE_MEM);

    /* check whether it was registered when allocated */

    prev_ptr = cur_ptr = MPIDI_CH3I_Alloc_mem_list_head;
    found = 0;
    while (cur_ptr != NULL)
    {
        if (addr == cur_ptr->ptr)
	{
            found = 1;

	    mpi_errno = ibu_nocache_deregister_memory(addr, cur_ptr->length, &cur_ptr->mem);
	    /* --BEGIN ERROR HANDLING-- */
            if (mpi_errno != MPI_SUCCESS)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**fail", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    /* remove this entry from the list */
	    if (cur_ptr == MPIDI_CH3I_Alloc_mem_list_head)
                MPIDI_CH3I_Alloc_mem_list_head = cur_ptr->next;
            else 
                prev_ptr->next = cur_ptr->next;
            MPIU_Free(cur_ptr);

            break;
        }
        prev_ptr = cur_ptr;
        cur_ptr = cur_ptr->next;
    }

    if (!found)
    {
	MPIU_Free(addr);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_FREE_MEM);
    return mpi_errno;
}
