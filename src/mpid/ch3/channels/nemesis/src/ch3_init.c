/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include "mpid_nem_impl.h"

void *MPIDI_CH3_packet_buffer = NULL;
int MPIDI_CH3I_my_rank = -1;
MPIDI_PG_t *MPIDI_CH3I_my_pg = NULL;

int MPIDI_nemesis_initialized = 0;

int MPIDI_CH3I_Shm_supported(void)
{
    return MPL_proc_mutex_enabled();
}

/* MPIDI_CH3_Init():  Initialize the nemesis channel */
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIR_FUNC_ENTER;

    mpi_errno = MPID_nem_init (pg_rank, pg_p, has_parent);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    MPIDI_nemesis_initialized = 1;

    MPIDI_CH3I_my_rank = pg_rank;
    MPIDI_CH3I_my_pg = pg_p;

    /*
     * Initialize Progress Engine
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno) MPIR_ERR_SETFATALANDJUMP (mpi_errno, MPI_ERR_OTHER, "**init_progress");

    for (i = 0; i < pg_p->size; i++)
    {
	mpi_errno = MPIDI_CH3_VC_Init(&pg_p->vct[i]);
        MPIR_ERR_CHECK(mpi_errno);
    }

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* This function simply tells the CH3 device to use the defaults for the
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns )
{

    MPIR_FUNC_ENTER;

    MPL_UNREFERENCED_ARG(portFns);

    MPIR_FUNC_EXIT;
    return 0;
}

int MPIDI_CH3_Get_business_card(int myRank, char *value, int length)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    mpi_errno = MPID_nem_get_business_card(myRank, value, length);
    MPIR_ERR_CHECK(mpi_errno);

fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Perform the channel-specific vc initialization */
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* FIXME: Circular dependency.  Before calling MPIDI_CH3_Init,
       MPID_Init calls InitPG which calls MPIDI_PG_Create which calls
       MPIDI_CH3_VC_Init.  But MPIDI_CH3_VC_Init needs nemesis to be
       initialized.  We can't call MPIDI_CH3_Init before initializing
       the PG, because it needs the PG.

        There is a hook called MPIDI_CH3_PG_Init which is called from
        MPIDI_PG_Create before MPIDI_CH3_VC_Init, but we don't have
        the pg_rank in that function.

        Maybe this shouldn't really be a FIXME, since I believe that
        this issue will be moot once nemesis is a device.

	So what we do now, is do nothing if MPIDI_CH3_VC_Init is
	called before MPIDI_CH3_Init, and call MPIDI_CH3_VC_Init from
	inside MPIDI_CH3_Init after initializing nemesis
    */
    if (!MPIDI_nemesis_initialized)
        goto fn_exit;

    /* no need to initialize vc to self */
    if (vc->pg == MPIDI_CH3I_my_pg && vc->pg_rank == MPIDI_CH3I_my_rank)
        goto fn_exit;

    vc->ch.recv_active = NULL;

    mpi_errno = MPID_nem_vc_init (vc);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

 fn_exit:
 fn_fail:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

int MPIDI_CH3_VC_Destroy(MPIDI_VC_t *vc )
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_FUNC_ENTER;

    /* no need to destroy vc to self, this corresponds to the optimization above
     * in MPIDI_CH3_VC_Init */
    if (vc->pg == MPIDI_CH3I_my_pg && vc->pg_rank == MPIDI_CH3I_my_rank) {
        MPL_DBG_MSG_P(MPIDI_CH3_DBG_VC, VERBOSE, "skipping self vc=%p", vc);
        goto fn_exit;
    }

    mpi_errno = MPID_nem_vc_destroy(vc);

fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
}

/* MPIDI_CH3_Connect_to_root() create a new vc, and connect it to the process listening on port_name */
int MPIDI_CH3_Connect_to_root (const char *port_name, MPIDI_VC_t **new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;

    *new_vc = NULL; /* so that the err handling knows to cleanup */

    MPIR_CHKPMEM_MALLOC(vc, sizeof(MPIDI_VC_t), MPL_MEM_ADDRESS);
    /* FIXME - where does this vc get freed?
       ANSWER (goodell@) - ch3u_port.c FreeNewVC
                           (but the VC_Destroy is in this file) */

    /* init ch3 portion of vc */
    MPIDI_VC_Init (vc, NULL, 0);

    /* init channel portion of vc */
    MPIR_ERR_CHKINTERNAL(!MPIDI_nemesis_initialized, mpi_errno, "Nemesis not initialized");
    vc->ch.recv_active = NULL;
    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    *new_vc = vc; /* we now have a valid, disconnected, temp VC */

    mpi_errno = MPID_nem_connect_to_root (port_name, vc);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    /* freeing without giving the lower layer a chance to cleanup can lead to
       leaks on error */
    if (*new_vc)
        MPIDI_CH3_VC_Destroy(*new_vc);
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
#ifdef MPL_USE_DBG_LOGGING
const char * MPIDI_CH3_VC_GetStateString( struct MPIDI_VC *vc )
{
    /* Nemesis doesn't have connection state associated with the VC */
    return "N/A";
}
#endif
#endif

/* We don't initialize before calling MPIDI_CH3_VC_Init */
int MPIDI_CH3_PG_Init(MPIDI_PG_t *pg_p)
{

    MPIR_FUNC_ENTER;
    MPL_UNREFERENCED_ARG(pg_p);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}

int MPIDI_CH3_PG_Destroy(MPIDI_PG_t *pg_p)
{

    MPIR_FUNC_ENTER;
    MPL_UNREFERENCED_ARG(pg_p);

    MPIR_FUNC_EXIT;
    return MPI_SUCCESS;
}


typedef struct initcomp_cb
{
    int (* callback)(void);
    struct initcomp_cb *next;
} initcomp_cb_t;

static struct {initcomp_cb_t *top;} initcomp_cb_stack;

#define INITCOMP_S_TOP() GENERIC_S_TOP(initcomp_cb_stack)
#define INITCOMP_S_PUSH(ep) GENERIC_S_PUSH(&initcomp_cb_stack, ep, next)

/* register a function to be called when all initialization is finished */
int MPID_nem_register_initcomp_cb(int (* callback)(void))
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    MPIR_CHKPMEM_DECL();

    MPIR_FUNC_ENTER;
    MPIR_CHKPMEM_MALLOC(ep, sizeof(*ep), MPL_MEM_OTHER);

    ep->callback = callback;
    INITCOMP_S_PUSH(ep);

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int MPIDI_CH3_InitCompleted(void)
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    initcomp_cb_t *ep_tmp;

    MPIR_FUNC_ENTER;
    ep = INITCOMP_S_TOP();
    while (ep)
    {
        mpi_errno = ep->callback();
        MPIR_ERR_CHECK(mpi_errno);
        ep_tmp = ep;
        ep = ep->next;
        MPL_free(ep_tmp);
    }

 fn_exit:
    MPIR_FUNC_EXIT;
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
