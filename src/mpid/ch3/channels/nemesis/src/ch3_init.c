/* -*- Mode: C; c-basic-offset:4 ; -*- */
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

/* MPIDI_CH3_Init():  Initialize the nemesis channel */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t *pg_p, int pg_rank)
{
    int mpi_errno = MPI_SUCCESS;
    int i;

    MPIU_Assert(sizeof(MPIDI_CH3I_VC) <= sizeof(((MPIDI_VC_t*)0)->channel_private));
    
    /* There are hard-coded copy routines that depend on the size of the mpich2 header
       We only handle the 32- and 40-byte cases.
    */
    MPIU_Assert (sizeof(MPIDI_CH3_Pkt_t) >= 32 && sizeof(MPIDI_CH3_Pkt_t) <= 40);
    
    mpi_errno = MPID_nem_init (pg_rank, pg_p);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    nemesis_initialized = 1;
    
    MPIDI_CH3I_my_rank = pg_rank;
    MPIDI_CH3I_my_pg = pg_p;
    
    /*
     * Initialize Progress Engine 
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno) MPIU_ERR_SETFATALANDJUMP (mpi_errno, MPI_ERR_OTHER, "**init_progress");

    for (i = 0; i < pg_p->size; i++)
    {
	MPIDI_VC_t *vc;
	MPIDI_PG_Get_vc (pg_p, i, &vc);
	mpi_errno = MPIDI_CH3_VC_Init (vc);
        if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    }
    
    mpi_errno = MPID_nem_coll_barrier_init();
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI Port functions */
int MPIDI_CH3_PortFnsInit( MPIDI_PortFns *portFns ) 
{
    MPIU_UNREFERENCED_ARG(portFns);
    return 0;
}

/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ) 
{ 
    return 0;
}

/* Perform the channel-specific vc initialization */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_VC_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_VC_Init( MPIDI_VC_t *vc )
{
    int mpi_errno = MPI_SUCCESS;
    char bc[MPID_NEM_MAX_KEY_VAL_LEN];

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
    
    ((MPIDI_CH3I_VC *)vc->channel_private)->recv_active = NULL;
    vc->state = MPIDI_VC_STATE_ACTIVE;

    mpi_errno = vc->pg->getConnInfo (vc->pg_rank, bc, MPID_NEM_MAX_KEY_VAL_LEN, vc->pg);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_vc_init (vc, bc);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

 fn_exit:
 fn_fail:
    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_VC_Destroy
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_VC_Destroy(MPIDI_VC_t *vc )
{
    return MPID_nem_vc_destroy(vc);
}

/* MPIDI_CH3_Connect_to_root() create a new vc, and connect it to the process listening on port_name */
#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Connect_to_root
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Connect_to_root (const char *port_name, MPIDI_VC_t **new_vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_VC_t * vc;
    MPIU_CHKPMEM_DECL (1);
    
    MPIU_CHKPMEM_MALLOC (vc, MPIDI_VC_t *, sizeof(MPIDI_VC_t), mpi_errno, "vc");
    /* FIXME - where does this vc get freed? */

    *new_vc = vc;

    /* init ch3 portion of vc */
    MPIDI_VC_Init (vc, NULL, 0);

    /* init channel portion of vc */
    MPIU_ERR_CHKANDJUMP (!nemesis_initialized, mpi_errno, MPI_ERR_OTHER, "**intern");
    
    ((MPIDI_CH3I_VC *)vc->channel_private)->recv_active = NULL;
    vc->state = MPIDI_VC_STATE_ACTIVE;

    mpi_errno = MPID_nem_vc_init (vc, port_name);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    mpi_errno = MPID_nem_connect_to_root (port_name, vc);
    if (mpi_errno) MPIU_ERR_POP (mpi_errno);

    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#ifndef MPIDI_CH3_HAS_NO_DYNAMIC_PROCESS
#ifdef USE_DBG_LOGGING
const char * MPIDI_CH3_VC_GetStateString( struct MPIDI_VC *vc )
{
    const char *name = "unknown";
    static char asdigits[20];
    MPIDI_CH3I_VC *vcch = (MPIDI_CH3I_VC *)vc->channel_private;
    int    state = vcch->state;
    
    switch (state) {
    case MPIDI_CH3I_VC_STATE_UNCONNECTED: name = "CH3I_VC_STATE_UNCONNECTED"; break;
    case MPIDI_CH3I_VC_STATE_CONNECTING:  name = "CH3I_VC_STATE_CONNECTING"; break;
    case MPIDI_CH3I_VC_STATE_CONNECTED:   name = "CH3I_VC_STATE_CONNECTED"; break;
    case MPIDI_CH3I_VC_STATE_FAILED:      name = "CH3I_VC_STATE_FAILED"; break;
    default:
	MPIU_Snprintf( asdigits, sizeof(asdigits), "%d", state );
	asdigits[20-1] = 0;
	name = (const char *)asdigits;
    }
    return name;
}
#endif
#endif

/* We don't initialize before calling MPIDI_CH3_VC_Init */
int MPIDI_CH3_PG_Init(MPIDI_PG_t *pg_p)
{
    return MPI_SUCCESS;
}

int MPIDI_CH3_PG_Destroy(MPIDI_PG_t *pg_p)
{
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
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPID_nem_register_initcomp_cb(int (* callback)(void))
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    MPIU_CHKPMEM_DECL(1);

    MPIU_CHKPMEM_MALLOC(ep, initcomp_cb_t *, sizeof(*ep), mpi_errno, "initcomp callback element");

    ep->callback = callback;
    INITCOMP_S_PUSH(ep);
    
    MPIU_CHKPMEM_COMMIT();
 fn_exit:
    return mpi_errno;
 fn_fail:
    MPIU_CHKPMEM_REAP();
    goto fn_exit;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_InitCompleted
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_InitCompleted()
{
    int mpi_errno = MPI_SUCCESS;
    initcomp_cb_t *ep;
    
    ep = INITCOMP_S_TOP();
    while (ep)
    {
        mpi_errno = ep->callback();
        if (mpi_errno) MPIU_ERR_POP(mpi_errno);
        ep = ep->next;
    }
    
 fn_exit:
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}
