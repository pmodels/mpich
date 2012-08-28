/* begin_generated_IBM_copyright_prolog                             */
/*                                                                  */
/* This is an automatically generated copyright prolog.             */
/* After initializing,  DO NOT MODIFY OR MOVE                       */
/*  --------------------------------------------------------------- */
/* Licensed Materials - Property of IBM                             */
/* Blue Gene/Q 5765-PER 5765-PRP                                    */
/*                                                                  */
/* (C) Copyright IBM Corp. 2011, 2012 All Rights Reserved           */
/* US Government Users Restricted Rights -                          */
/* Use, duplication, or disclosure restricted                       */
/* by GSA ADP Schedule Contract with IBM Corp.                      */
/*                                                                  */
/*  --------------------------------------------------------------- */
/*                                                                  */
/* end_generated_IBM_copyright_prolog                               */
/*  (C)Copyright IBM Corp.  2007, 2011  */
/**
 * \file src/mpid_vc.c
 * \brief Maintain the virtual connection reference table
 */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <mpidimpl.h>

/**
 * \brief Virtual connection reference table
 */
struct MPIDI_VCRT
{
  MPIU_OBJECT_HEADER;
  unsigned size;          /**< Number of entries in the table */
  MPID_VCR vcr_table[0];  /**< Array of virtual connection references */
};


int MPID_VCR_Dup(MPID_VCR orig_vcr, MPID_VCR * new_vcr)
{
    *new_vcr = orig_vcr;
    return MPI_SUCCESS;
}

int MPID_VCR_Get_lpid(MPID_VCR vcr, int * lpid_ptr)
{
    *lpid_ptr = (int)vcr;
    return MPI_SUCCESS;
}

int MPID_VCRT_Create(int size, MPID_VCRT *vcrt_ptr)
{
    struct MPIDI_VCRT * vcrt;
    int result;

    vcrt = MPIU_Malloc(sizeof(struct MPIDI_VCRT) + size*sizeof(MPID_VCR));
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
    return result;
}

int MPID_VCRT_Add_ref(MPID_VCRT vcrt)
{
    MPIU_Object_add_ref(vcrt);
    return MPI_SUCCESS;
}

int MPID_VCRT_Release(MPID_VCRT vcrt, int isDisconnect)
{
    int count;

    MPIU_Object_release_ref(vcrt, &count);
    if (count == 0)
      MPIU_Free(vcrt);
    return MPI_SUCCESS;
}

int MPID_VCRT_Get_ptr(MPID_VCRT vcrt, MPID_VCR **vc_pptr)
{
    *vc_pptr = vcrt->vcr_table;
    return MPI_SUCCESS;
}
