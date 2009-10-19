/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"

#ifdef USE_PMI2_API
#include "pmi2.h"
#else
#include "pmi.h"
#endif

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Abort
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Abort(int exit_code, char *error_msg)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_ABORT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_ABORT);

#ifdef USE_PMI2_API
    PMI2_Abort(TRUE, error_msg);
#else
    PMI_Abort(exit_code, error_msg);
#endif
    /* if abort returns for some reason, exit here */

    MPIU_Error_printf("%s", error_msg);
    fflush(stderr);

    MPIU_Exit(exit_code);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_ABORT);
    return MPI_ERR_INTERN;
}
