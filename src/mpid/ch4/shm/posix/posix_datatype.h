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
#ifndef SHM_DATATYPE_H_INCLUDED
#define SHM_DATATYPE_H_INCLUDED

#include "posix_impl.h"

static inline int MPIDI_SHM_type_free_hook(MPIR_Datatype * datatype_p)
{
    return 0;
}

static inline int MPIDI_SHM_type_create_hook(MPIR_Datatype * datatype_p)
{
    return 0;
}

static inline void MPIDI_SHM_type_dup_hook(MPIR_Datatype * old_datatype_p,
                                           MPIR_Datatype * new_datatype_p)
{
    return;
}
#endif
