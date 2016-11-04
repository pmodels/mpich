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
#ifndef MPIDPOST_H_INCLUDED
#define MPIDPOST_H_INCLUDED

#include "mpidu_datatype.h"
#include "mpidch4.h"

MPL_STATIC_INLINE_PREFIX void MPID_Request_create_hook(MPIR_Request * req)
{
    MPIDI_CH4U_REQUEST(req, req) = NULL;
#ifdef MPIDI_BUILD_CH4_SHM
    MPIDI_CH4I_REQUEST_ANYSOURCE_PARTNER(req) = NULL;
#endif
}

MPL_STATIC_INLINE_PREFIX void MPID_Request_destroy_hook(MPIR_Request * req)
{
    return;
}

#endif /* MPIDPOST_H_INCLUDED */
