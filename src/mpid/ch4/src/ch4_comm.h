/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef CH4_COMM_H_INCLUDED
#define CH4_COMM_H_INCLUDED

#include "ch4_impl.h"

int MPIDI_Comm_split_type(MPIR_Comm * user_comm_ptr, int split_type, int key, MPIR_Info * info_ptr,
                          MPIR_Comm ** newcomm_ptr);

MPL_STATIC_INLINE_PREFIX int MPID_Comm_AS_enabled(MPIR_Comm * comm)
{



    MPIR_Assert(0);


    return MPI_SUCCESS;
}

#endif /* CH4_COMM_H_INCLUDED */
