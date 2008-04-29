/*  (C)Copyright IBM Corp.  2007, 2008  */
/**
 * \file src/misc/mpid_alloc_mem.c
 * \brief ???
 */
/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpidimpl.h"

void *MPID_Alloc_mem( size_t size, MPID_Info *info_ptr )
{
    void *ap;
    ap = MPIU_Malloc(size);
    return ap;
}
