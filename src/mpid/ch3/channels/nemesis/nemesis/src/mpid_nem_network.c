/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpid_nem_impl.h"
#ifdef HAVE_STRINGS_H
    #include "strings.h"
#endif

/* initialize to prevent the compiler from generating common symbols */
MPID_nem_netmod_funcs_t *MPID_nem_netmod_func = NULL;
int MPID_nem_netmod_id = -1;

MPID_nem_net_module_vc_dbg_print_sendq_t  MPID_nem_net_module_vc_dbg_print_sendq = 0;

#undef FUNCNAME
#define FUNCNAME MPID_nem_choose_netmod
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_choose_netmod(void)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_CHOOSE_NETMOD);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_CHOOSE_NETMOD);

    if (strcmp(MPIR_PARAM_NEMESIS_NETMOD, "") == 0)
    {
        /* netmod not specified, using the default */
        MPID_nem_netmod_func = MPID_nem_netmod_funcs[0];
        MPID_nem_netmod_id = 0;
#ifdef ENABLE_COMM_OVERRIDES
        MPIDI_Anysource_iprobe_fn = MPID_nem_netmod_func->anysource_iprobe;
#endif
        goto fn_exit;
    }

    for (i = 0; i < MPID_nem_num_netmods; ++i)
    {
        if (!MPIU_Strncasecmp(MPIR_PARAM_NEMESIS_NETMOD, MPID_nem_netmod_strings[i], MPID_NEM_MAX_NETMOD_STRING_LEN))
        {
            MPID_nem_netmod_func = MPID_nem_netmod_funcs[i];
            MPID_nem_netmod_id = i;
#ifdef ENABLE_COMM_OVERRIDES
            MPIDI_Anysource_iprobe_fn = MPID_nem_netmod_func->anysource_iprobe;
#endif
            goto fn_exit;
        }
    }

    MPIU_ERR_SETANDJUMP1(mpi_errno, MPI_ERR_OTHER, "**invalid_netmod", "**invalid_netmod %s", MPIR_PARAM_NEMESIS_NETMOD);

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_CHOOSE_NETMOD);
    return mpi_errno;
 fn_fail:

    goto fn_exit;
}
