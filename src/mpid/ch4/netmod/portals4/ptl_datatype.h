/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2016 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 */
#ifndef NETMOD_PTL_DATATYPE_H_INCLUDED
#define NETMOD_PTL_DATATYPE_H_INCLUDED

#include "ptl_impl.h"

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

#endif /* NETMOD_PTL_DATATYPE_H_INCLUDED */
