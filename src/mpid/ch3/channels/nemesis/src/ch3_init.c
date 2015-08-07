/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
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

static int nemesis_initialized = 0;

#undef FUNCNAME
#define FUNCNAME split_type
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
static int split_type(MPID_Comm * comm_ptr, int stype, int key,
                      MPID_Info *info_ptr, MPID_Comm ** newcomm_ptr)
{
    MPID_Node_id_t id;
    MPIDI_Rank_t nid;
    int mpi_errno = MPI_SUCCESS;

    if (MPIDI_CH3I_Shm_supported()) {
        mpi_errno = MPID_Get_node_id(comm_ptr, comm_ptr->rank, &id);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }
    else
        id = comm_ptr->rank;

    nid = (stype == MPI_COMM_TYPE_SHARED) ? id : MPI_UNDEFINED;
    mpi_errno = MPIR_Comm_split_impl(comm_ptr, nid, key, newcomm_ptr);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

  fn_exit:
    return mpi_errno;

    /* --BEGIN ERROR HANDLING-- */
  fn_fail:
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

int MPIDI_CH3I_Shm_supported(void)
{
    int mutex_err;
    pthread_mutexattr_t attr;

    /* Test for PTHREAD_PROCESS_SHARED support.  Some platforms do not support
     * this capability even though it is a part of the pthreads core API (e.g.,
     * FreeBSD does not support this as of version 9.1) */
    pthread_mutexattr_init(&attr);
    mutex_err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_destroy(&attr);

    return !mutex_err;
}

static MPID_CommOps comm_fns = {
    split_type
};

/* MPIDI_CH3_Init():  Initialize the nemesis channel */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int i;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_INIT);

    /* Override split_type */
    MPID_Comm_fns = &comm_fns;

    mpi_errno = MPID_nem_init (pg_rank, pg_p, has_parent);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    nemesis_initialized = 1;

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
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_INIT);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* This function simply tells the CH3 device to use the defaults for the
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns )
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PORTFNSINIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PORTFNSINIT);

    MPIU_UNREFERENCED_ARG(portFns);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PORTFNSINIT);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Get_business_card
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Get_business_card(int myRank, char *value, int length)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    MPIDI_FUNC_ENTER(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);

    mpi_errno = MPID_nem_get_business_card(myRank, value, length);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

fn_exit:
    MPIDI_FUNC_EXIT(MPIDI_STATE_MPIDI_CH3_GET_BUSINESS_CARD);
    return mpi_errno;
fn_fail:
    goto fn_exit;
}

/* Perform the channel-specific vc initialization */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_VC_Init
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_VC_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_VC_INIT);

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
    if (!nemesis_initialized)
        goto fn_exit;

    /* no need to initialize vc to self */
    if (vc->pg == MPIDI_CH3I_my_pg && vc->pg_rank == MPIDI_CH3I_my_rank)
        goto fn_exit;

    vc->ch.recv_active = NULL;

    mpi_errno = MPID_nem_vc_init (vc);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

 fn_exit:
 fn_fail:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_VC_INIT);
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_VC_Destroy
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_VC_Destroy(MPIDI_VC_t *vc )
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_VC_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_VC_DESTROY);

    /* no need to destroy vc to self, this corresponds to the optimization above
     * in MPIDI_CH3_VC_Init */
    if (vc->pg == MPIDI_CH3I_my_pg && vc->pg_rank == MPIDI_CH3I_my_rank) {
        MPIU_DBG_MSG_P(NEM_SOCK_DET, VERBOSE, "skipping self vc=%p", vc);
        goto fn_exit;
    }

    mpi_errno = MPID_nem_vc_destroy(vc);

fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_VC_DESTROY);
    return mpi_errno;
}

/* MPIDI_CH3_Connect_to_root() create a new vc, and connect it to the process listening on port_name */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connect_to_root
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_Connect_to_root (const char *port_name, MPIDI_VC_t **new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_CONNECT_TO_ROOT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_CONNECT_TO_ROOT);

    *new_vc = NULL; /* so that the err handling knows to cleanup */

    MPIU_CHKPMEM_MALLOC (vc, MPIDI_VC_t *, sizeof(MPIDI_VC_t), mpi_errno, "vc");
    /* FIXME - where does this vc get freed?
       ANSWER (goodell@) - ch3u_port.c FreeNewVC
                           (but the VC_Destroy is in this file) */

    /* init ch3 portion of vc */
    MPIDI_VC_Init (vc, NULL, 0);

    /* init channel portion of vc */
    MPIR_ERR_CHKINTERNAL(!nemesis_initialized, mpi_errno, "Nemesis not initialized");
    vc->ch.recv_active = NULL;
    MPIDI_CHANGE_VC_STATE(vc, ACTIVE);

    *new_vc = vc; /* we now have a valid, disconnected, temp VC */

    mpi_errno = MPID_nem_connect_to_root (port_name, vc);
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_CONNECT_TO_ROOT);
    return mpi_errno;
 fn_fail:
    /* freeing without giving the lower layer a chance to cleanup can lead to
       leaks on error */
    if (*new_vc)
        MPIDI_CH3_VC_Destroy(*new_vc);
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
#ifdef USE_DBG_LOGGING
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
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PG_INIT);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PG_INIT);
    MPIU_UNREFERENCED_ARG(pg_p);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PG_INIT);
    return MPI_SUCCESS;
}

int MPIDI_CH3_PG_Destroy(MPIDI_PG_t *pg_p)
{
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_PG_DESTROY);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_PG_DESTROY);
    MPIU_UNREFERENCED_ARG(pg_p);

    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_PG_DESTROY);
    return MPI_SUCCESS;
}


typedef struct initcomp_cb
{
    int (* callback)(void);
    struct initcomp_cb *next;
} initcomp_cb_t;

static struct {initcomp_cb_t *top;} initcomp_cb_stack = {0};

#define INITCOMP_S_TOP() GENERIC_S_TOP(initcomp_cb_stack)
#define INITCOMP_S_PUSH(ep) GENERIC_S_PUSH(&initcomp_cb_stack, ep, next)

/* register a function to be called when all initialization is finished */
#undef FUNCNAME
#define FUNCNAME MPID_nem_register_initcomp_cb
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPID_nem_register_initcomp_cb(int (* callback)(void))
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    MPIU_CHKPMEM_DECL(1);
    MPIDI_STATE_DECL(MPID_STATE_MPID_NEM_REGISTER_INITCOMP_CB);

    MPIDI_FUNC_ENTER(MPID_STATE_MPID_NEM_REGISTER_INITCOMP_CB);
    MPIU_CHKPMEM_MALLOC(ep, initcomp_cb_t *, sizeof(*ep), mpi_errno, "initcomp callback element");

    ep->callback = callback;
    INITCOMP_S_PUSH(ep);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPID_NEM_REGISTER_INITCOMP_CB);
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_InitCompleted
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIDI_CH3_InitCompleted(void)
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    initcomp_cb_t *ep_tmp;
    MPIDI_STATE_DECL(MPID_STATE_MPIDI_CH3_INITCOMPLETED);

    MPIDI_FUNC_ENTER(MPID_STATE_MPIDI_CH3_INITCOMPLETED);
    ep = INITCOMP_S_TOP();
    while (ep)
    {
        mpi_errno = ep->callback();
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
        ep_tmp = ep;
        ep = ep->next;
        MPIU_Free(ep_tmp);
    }

 fn_exit:
    MPIDI_FUNC_EXIT(MPID_STATE_MPIDI_CH3_INITCOMPLETED);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
