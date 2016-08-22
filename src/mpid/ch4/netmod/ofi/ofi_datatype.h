/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2012 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef NETMOD_OFI_DATATYPE_H_INCLUDED
#define NETMOD_OFI_DATATYPE_H_INCLUDED

#include "ofi_impl.h"

static inline void MPIDI_NM_datatype_destroy(MPIR_Datatype * datatype_p)
{
    return;
}

static inline void MPIDI_NM_datatype_commit(MPIR_Datatype * datatype_p)
{
    return;
}

static inline void MPIDI_NM_datatype_dup(MPIR_Datatype * old_datatype_p,
                                         MPIR_Datatype * new_datatype_p)
{
    return;
}
#endif
