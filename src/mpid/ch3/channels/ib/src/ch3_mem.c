/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidi_ch3_impl.h"

static MPIDI_CH3I_Alloc_mem_list_t *MPIDI_CH3I_Alloc_mem_list_head = NULL;

/*
 * MPIDI_CH3_alloc_mem()
 */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Alloc_mem
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
void *MPIDI_CH3_Alloc_mem(size_t size, MPID_Info *info_ptr)
{
    int mpi_errno = MPI_SUCCESS;
    void *ap = NULL;
    ibu_mem_t mem;
    MPIDI_CH3I_Alloc_mem_list_t *new_ptr;
    
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ALLOC_MEM);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ALLOC_MEM);

    ap = MPIU_Malloc(size);
    if (ap != NULL)
    {
	mpi_errno = ibu_nocache_register_memory(ap, size, &mem);
	if (mpi_errno == MPI_SUCCESS)
	{
	    new_ptr = (MPIDI_CH3I_Alloc_mem_list_t *) MPIU_Malloc(sizeof(MPIDI_CH3I_Alloc_mem_list_t));
	    /* --BEGIN ERROR HANDLING-- */
	    if (new_ptr == NULL)
	    {
		mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_RECOVERABLE, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
		goto fn_exit;
	    }
	    /* --END ERROR HANDLING-- */

	    new_ptr->ptr = ap;
	    new_ptr->length = size;
	    new_ptr->mem = mem;
	    new_ptr->next = MPIDI_CH3I_Alloc_mem_list_head;
	    MPIDI_CH3I_Alloc_mem_list_head = new_ptr;
	}
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ALLOC_MEM);
    return ap;
}

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

void MPIDI_CH3_Cleanup_mem()
{
    MPIDI_CH3I_Alloc_mem_list_t *cur_ptr;
    cur_ptr = MPIDI_CH3I_Get_mem_list_head();
    while (cur_ptr != NULL)
    {
	MPIDI_CH3_Free_mem(cur_ptr->ptr);
	cur_ptr = MPIDI_CH3I_Get_mem_list_head();
    }
}

/* in ib/include/mpidi_ch3_impl.h at the moment but will
 * need to move to mpidpost.h if used elsewhere (and the
 * name changed to CH3)
 */
MPIDI_CH3I_Alloc_mem_list_t * MPIDI_CH3I_Get_mem_list_head()
{
    return MPIDI_CH3I_Alloc_mem_list_head;
}
