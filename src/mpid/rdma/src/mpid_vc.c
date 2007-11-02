/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/*
 * MPIDI_VCRT - virtual connection reference table
 *
 * handle - this element is not used, but exists so that we may use the MPIU_Object routines for reference counting
 *
 * ref_count - number of references to this table
 *
 * vcr_table - array of virtual connection references
 */
typedef struct MPIDI_VCRT
{
    int handle;
    volatile int ref_count;
    int size;
    MPIDI_VC * vcr_table[1];
}
MPIDI_VCRT;


#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Create
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT * vcrt;
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);
    
    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT) + (size - 1) * sizeof(MPIDI_VC));
    if (vcrt != NULL)
    {
	MPIU_Object_set_ref(vcrt, 1);
	vcrt->size = size;
	*vcrt_ptr = vcrt;
    }
    else
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Add_ref
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Release
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Release(MPID_VCRT vcrt)
{
    int count;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_RELEASE);

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
    {
	int i;

	for (i = 0; i < vcrt->size; i++)
	{
	    MPID_VCR_Release(vcrt->vcr_table[i]);
	}
	
	MPIU_Free(vcrt);
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_RELEASE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCRT_Get_ptr
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCR_Dup
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIU_Object_add_ref(orig_vcr);
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCR_Release
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Release(MPID_VCR vcr)
{
    int count;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_RELEASE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_RELEASE);
    MPIU_Object_release_ref(vcr, &count);
    /* FIXME: if necessary, update number of active VCs in the VC table */
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_RELEASE);
    return MPI_SUCCESS;
}

#undef FUNCNAME
#define FUNCNAME MPID_VCR_Get_lpid
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}


