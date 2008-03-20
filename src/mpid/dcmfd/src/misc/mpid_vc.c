/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_vc.c
 * \brief Maintain the virtual connection reference table
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

/**
 * \brief Virtual connection reference table
 */
typedef struct MPIDI_VCRT
{
  int handle;              /**< This element is not used, but exists so that we may use the MPIU_Object routines for reference counting */
  volatile int ref_count;  /**< Number of references to this table */
  int size;                /**< Number of entries inthe table */
  MPIDI_VC * vcr_table[1]; /**< array of virtual connection references */
}
MPIDI_VCRT;


int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_DUP);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_DUP);
    MPIU_Object_add_ref(orig_vcr);
    *new_vcr = orig_vcr;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_DUP);
    return MPI_SUCCESS;
}

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

int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCR_GET_LPID);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCR_GET_LPID);
    *lpid_ptr = vcr->lpid;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCR_GET_LPID);
    return MPI_SUCCESS;
}


int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    MPIDI_VCRT * vcrt;
    int result;
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_CREATE);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_CREATE);

    vcrt = MPIU_Malloc(sizeof(MPIDI_VCRT) + (size - 1) * sizeof(MPIDI_VC));
    if (vcrt != NULL)
    {
        MPIU_Object_set_ref(vcrt, 1);
        vcrt->size = size;
        *vcrt_ptr = vcrt;
        result = MPI_SUCCESS;
    }
    else
    {
        result = MPIR_ERR_MEMALLOCFAILED;
    }
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_CREATE);
    return result;
}

int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_ADD_REF);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_ADD_REF);
    MPIU_Object_add_ref(vcrt);
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_ADD_REF);
    return MPI_SUCCESS;
}

int MPID_VCRT_Release(MPID_VCRT vcrt, int isDisconnect)
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

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    MPIDI_STATE_DECL(MPID_STATE_MPID_VCRT_GET_PTR);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_VCRT_GET_PTR);
    *vc_pptr = vcrt->vcr_table;
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_VCRT_GET_PTR);
    return MPI_SUCCESS;
}
