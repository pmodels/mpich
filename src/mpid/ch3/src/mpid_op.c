/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2019 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */

#include "mpidimpl.h"

#undef FUNCNAME
#define FUNCNAME MPID_Op_commit_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Op_commit_hook(MPIR_Op * op)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_CREATE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_CREATE_HOOK);

    return 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_CREATE_HOOK);
}

#undef FUNCNAME
#define FUNCNAME MPID_Op_free_hook
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_Op_free_hook(MPIR_Op * op)
{
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OP_FREE_HOOK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OP_FREE_HOOK);

    return 0;

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OP_FREE_HOOK);
}
