/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"
#include "mpid_nem_impl.h"
#include "mpid_nem_nets.h"
#include <errno.h>
#include "mpidi_nem_statistics.h"
#include "mpit.h"
#include "mpidu_init_shm.h"

/*
=== BEGIN_MPI_T_CVAR_INFO_BLOCK ===

categories:
    - name        : NEMESIS
      description : cvars that control behavior of the ch3:nemesis channel

cvars:
    - name        : MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ
      category    : NEMESIS
      type        : int
      default     : -1
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar controls the message size at which Nemesis
        switches from eager to rendezvous mode for shared memory.
        If this cvar is set to -1, then Nemesis will choose
        an appropriate value.

    - name        : MPIR_CVAR_NEMESIS_SHM_READY_EAGER_MAX_SZ
      category    : NEMESIS
      type        : int
      default     : -2
      class       : none
      verbosity   : MPI_T_VERBOSITY_USER_BASIC
      scope       : MPI_T_SCOPE_ALL_EQ
      description : >-
        This cvar controls the message size at which Nemesis
        switches from eager to rendezvous mode for ready-send
        messages.  If this cvar is set to -1, then ready messages
        will always be sent eagerly.  If this cvar is set to -2,
        then Nemesis will choose an appropriate value.

=== END_MPI_T_CVAR_INFO_BLOCK ===
*/

/* constants for configure time selection of local LMT implementations */
#define MPID_NEM_LOCAL_LMT_NONE 0
#define MPID_NEM_LOCAL_LMT_SHM_COPY 1
#define MPID_NEM_LOCAL_LMT_DMA 2
#define MPID_NEM_LOCAL_LMT_VMSPLICE 3

#ifdef MEM_REGION_IN_HEAP
MPID_nem_mem_region_t *MPID_nem_mem_region_ptr = 0;
#else /* MEM_REGION_IN_HEAP */
MPID_nem_mem_region_t MPID_nem_mem_region;
#endif /* MEM_REGION_IN_HEAP */

char MPID_nem_hostname[MAX_HOSTNAME_LEN] = "UNKNOWN";

static int get_local_procs(MPIDI_PG_t *pg, int our_pg_rank, int *num_local_p,
                           int **local_procs_p, int *local_rank_p);

char *MPID_nem_asymm_base_addr = 0;

/* used by mpid_nem_inline.h and mpid_nem_finalize.c */
unsigned long long *MPID_nem_fbox_fall_back_to_queue_count = NULL;

static int MPID_nem_init_stats(int n_local_ranks)
{
    int mpi_errno = MPI_SUCCESS;

    if (ENABLE_PVAR_NEM) {
        MPID_nem_fbox_fall_back_to_queue_count = MPL_calloc(n_local_ranks, sizeof(unsigned long long), MPL_MEM_MPIT);
    }

    MPIR_T_PVAR_COUNTER_REGISTER_DYNAMIC(
        NEM,
        MPI_UNSIGNED_LONG_LONG,
        nem_fbox_fall_back_to_queue_count, /* name */
        MPID_nem_fbox_fall_back_to_queue_count, /* address */
        n_local_ranks, /* count, known at pvar registration time */
        MPI_T_VERBOSITY_USER_DETAIL,
        MPI_T_BIND_NO_OBJECT,
        MPIR_T_PVAR_FLAG_CONTINUOUS, /* flags */
        NULL, /* get_value */
        NULL, /* get_count */
        "NEMESIS", /* category */
        "Array counting how many times nemesis had to fall back to the regular queue when sending messages between pairs of local processes");

    return mpi_errno;
}

int
MPID_nem_init(int pg_rank, MPIDI_PG_t *pg_p, int has_parent ATTRIBUTE((unused)))
{
    int    mpi_errno       = MPI_SUCCESS;
    int    tmp_mpi_errno;
    int    num_procs       = pg_p->size;
    int    ret;
    int    num_local       = -1;
    int   *local_procs     = NULL;
    int    local_rank      = -1;
    int    idx;
    int    i;
    char  *publish_bc_orig = NULL;
    char  *bc_val          = NULL;
    int    val_max_remaining;
    int    grank;
    size_t len;
    void *fastboxes_p = NULL;
    void *cells_p = NULL;
    MPID_nem_queue_t *recv_queues_p = NULL;
    MPID_nem_queue_t *free_queues_p = NULL;
    char strerrbuf[MPIR_STRERROR_BUF_SIZE];

    MPIR_CHKPMEM_DECL(8);

    /* TODO add compile-time asserts (rather than run-time) and convert most of these */

    /* Make sure the nemesis packet is no larger than the generic
       packet.  This is needed because we no longer include channel
       packet types in the CH3 packet types to allow dynamic channel
       loading. */
    MPIR_Assert(sizeof(MPIDI_CH3_nem_pkt_t) <= sizeof(MPIDI_CH3_Pkt_t));

    /* The MPID_nem_cell_rel_ptr_t defined in mpid_nem_datatypes.h
       should only contain a MPL_atomic_ptr_t.  This is to check that
       absolute pointers are exactly the same size as relative
       pointers. */
    MPIR_Assert(sizeof(MPID_nem_cell_rel_ptr_t) == sizeof(MPL_atomic_ptr_t));

    /* Make sure payload is aligned on 8-byte boundary */
    MPIR_Assert(MPID_NEM_ALIGNED(&((MPID_nem_cell_t*)0)->payload[0], 8));
    /* Make sure the padding to cacheline size in MPID_nem_queue_t works */
    MPIR_Assert(MPID_NEM_CACHE_LINE_LEN > 2 * sizeof(MPID_nem_cell_rel_ptr_t));

    /* Initialize the business card */
    mpi_errno = MPIDI_CH3I_BCInit( &bc_val, &val_max_remaining );
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    publish_bc_orig = bc_val;

    ret = gethostname (MPID_nem_hostname, MAX_HOSTNAME_LEN);
    MPIR_ERR_CHKANDJUMP2 (ret == -1, mpi_errno, MPI_ERR_OTHER, "**sock_gethost", "**sock_gethost %s %d",
                          MPIR_Strerror(errno, strerrbuf, MPIR_STRERROR_BUF_SIZE), errno);

    MPID_nem_hostname[MAX_HOSTNAME_LEN-1] = '\0';

    mpi_errno = get_local_procs(pg_p, pg_rank, &num_local, &local_procs, &local_rank);
    MPIR_ERR_CHECK(mpi_errno);

#ifdef MEM_REGION_IN_HEAP
    MPIR_CHKPMEM_MALLOC (MPID_nem_mem_region_ptr, MPID_nem_mem_region_t *, sizeof(MPID_nem_mem_region_t), mpi_errno, "mem_region", MPL_MEM_SHM);
#endif /* MEM_REGION_IN_HEAP */

    MPID_nem_mem_region.rank           = pg_rank;
    MPID_nem_mem_region.num_local      = num_local;
    MPID_nem_mem_region.num_procs      = num_procs;
    MPID_nem_mem_region.local_procs    = local_procs;
    MPID_nem_mem_region.local_rank     = local_rank;
    MPIR_CHKPMEM_MALLOC (MPID_nem_mem_region.local_ranks, int *, num_procs * sizeof(int), mpi_errno, "mem_region local ranks", MPL_MEM_SHM);
    MPID_nem_mem_region.ext_procs      = num_procs - num_local ;
    if (MPID_nem_mem_region.ext_procs > 0)
        MPIR_CHKPMEM_MALLOC (MPID_nem_mem_region.ext_ranks, int *, MPID_nem_mem_region.ext_procs * sizeof(int), mpi_errno, "mem_region ext ranks", MPL_MEM_SHM);
    MPID_nem_mem_region.next           = NULL;

    for (idx = 0 ; idx < num_procs; idx++)
    {
	MPID_nem_mem_region.local_ranks[idx] = MPID_NEM_NON_LOCAL;
    }
    for (idx = 0; idx < num_local; idx++)
    {
	grank = local_procs[idx];
	MPID_nem_mem_region.local_ranks[grank] = idx;
    }

    idx = 0;
    for(grank = 0 ; grank < num_procs ; grank++)
    {
	if(!MPID_NEM_IS_LOCAL(grank))
	{
	    MPID_nem_mem_region.ext_ranks[idx++] = grank;
	}
    }

#ifdef FORCE_ASYM
    {
        /* this is used for debugging
           each process allocates a different sized piece of shared
           memory so that when the shared memory segment used for
           communication is allocated it will probably be mapped at a
           different location for each process
        */
        MPL_shm_hnd_t handle;
	int size = (local_rank * 65536) + 65536;
	char *base_addr;

        mpi_errno = MPL_shm_hnd_init(&handle);
        if(mpi_errno != MPI_SUCCESS) { MPIR_ERR_POP(mpi_errno); }

        mpi_errno = MPL_shm_seg_create_and_attach(handle, size, &base_addr, 0);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            MPL_shm_seg_remove(handle);
            MPL_shm_hnd_finalize(&handle);
            MPIR_ERR_POP (mpi_errno);
        }
        /* --END ERROR HANDLING-- */

        mpi_errno = MPL_shm_seg_remove(handle);
        /* --BEGIN ERROR HANDLING-- */
        if (mpi_errno)
        {
            MPL_shm_hnd_finalize(&handle);
            MPIR_ERR_POP (mpi_errno);
        }
        /* --END ERROR HANDLING-- */

        MPL_shm_hnd_finalize(&handle);
    }
    /*fprintf(stderr,"[%i] -- address shift ok \n",pg_rank); */
#endif  /*FORCE_ASYM */

    /* Initialize core shared memory segment */
    mpi_errno = MPIDU_Init_shm_init();
    MPIR_ERR_CHECK(mpi_errno);

    /* Request fastboxes region */
    size_t fbox_len = MPL_MAX((num_local*((num_local-1) * MPID_NEM_FBOX_LEN)),
                              MPID_NEM_ASYMM_NULL_VAL);

    /* Request data cells region */
    size_t cells_len = num_local * MPID_NEM_NUM_CELLS * MPID_NEM_CELL_LEN;

    /* Request free q region */
    size_t freeQ_len = num_local * sizeof(MPID_nem_queue_t);

    /* Request recv q region */
    size_t recvQ_len = num_local * sizeof(MPID_nem_queue_t);

    len = fbox_len + cells_len + freeQ_len + recvQ_len;

    /* Actually allocate the segment and assign regions to the pointers */
    mpi_errno = MPIDU_Init_shm_alloc(len, &MPID_nem_mem_region.shm_ptr);
    /* check_alloc steps */
    if (MPIDU_Init_shm_is_symm(MPID_nem_mem_region.shm_ptr) == 1) {
        MPID_nem_asymm_base_addr = NULL;
    } else {
        MPID_nem_asymm_base_addr = MPID_nem_mem_region.shm_ptr;
#ifdef MPID_NEM_SYMMETRIC_QUEUES
        MPIR_ERR_INTERNALANDJUMP(mpi_errno, "queues are not symmetrically allocated as expected");
#endif
    }

    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    fastboxes_p = (void *) MPID_nem_mem_region.shm_ptr;
    cells_p = (char *) fastboxes_p + fbox_len;
    free_queues_p = (MPID_nem_queue_t *)((char *) cells_p + cells_len);
    recv_queues_p = (MPID_nem_queue_t *)((char *) free_queues_p + freeQ_len);

    /* local procs barrier */
    mpi_errno = MPIDU_Init_shm_barrier();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);

    /* find our cell region */
    MPID_nem_mem_region.Elements = (void *) ((char *) cells_p + local_rank * MPID_NEM_NUM_CELLS * MPID_NEM_CELL_LEN);

    /* Tables of pointers to shared memory Qs */
    MPIR_CHKPMEM_MALLOC(MPID_nem_mem_region.FreeQ, MPID_nem_queue_ptr_t *, num_procs * sizeof(MPID_nem_queue_ptr_t), mpi_errno, "FreeQ", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPID_nem_mem_region.RecvQ, MPID_nem_queue_ptr_t *, num_procs * sizeof(MPID_nem_queue_ptr_t), mpi_errno, "RecvQ", MPL_MEM_SHM);

    /* Init table entry for our Qs */
    MPID_nem_mem_region.FreeQ[pg_rank] = &free_queues_p[local_rank];
    MPID_nem_mem_region.RecvQ[pg_rank] = &recv_queues_p[local_rank];

    /* Init our queues */
    MPID_nem_queue_init(MPID_nem_mem_region.RecvQ[pg_rank]);
    MPID_nem_queue_init(MPID_nem_mem_region.FreeQ[pg_rank]);
    
    /* Init and enqueue our free cells */
    for (idx = 0; idx < MPID_NEM_NUM_CELLS; ++idx)
    {
        MPID_nem_cell_t *cell = (void *) ((char *) MPID_nem_mem_region.Elements + idx * MPID_NEM_CELL_LEN);
        MPID_nem_cell_init(cell);
        MPID_nem_queue_enqueue(MPID_nem_mem_region.FreeQ[pg_rank], cell);
    }

    mpi_errno = MPID_nem_coll_init();
    if (mpi_errno) MPIR_ERR_POP (mpi_errno);
    
    /* This must be done before initializing the netmod so that the nemesis
       communicator creation hooks get registered (and therefore called) before
       the netmod hooks, giving the netmod an opportunity to override the
       nemesis collective function table. */
    mpi_errno = MPIDI_CH3U_Comm_register_create_hook(MPIDI_CH3I_comm_create, NULL);
    MPIR_ERR_CHECK(mpi_errno);

    /* network init */
    if (MPID_nem_num_netmods)
    {
        mpi_errno = MPID_nem_choose_netmod();
        MPIR_ERR_CHECK(mpi_errno);
	mpi_errno = MPID_nem_netmod_func->init(pg_p, pg_rank, &bc_val, &val_max_remaining);
        MPIR_ERR_CHECK(mpi_errno);
    }

    /* Register destroy hooks after netmod init so the netmod hooks get called
       before nemesis hooks. */
    mpi_errno = MPIDI_CH3U_Comm_register_destroy_hook(MPIDI_CH3I_comm_destroy, NULL);
    MPIR_ERR_CHECK(mpi_errno);
    
    /* set default route for external processes through network */
    for (idx = 0 ; idx < MPID_nem_mem_region.ext_procs ; idx++)
    {
	grank = MPID_nem_mem_region.ext_ranks[idx];
	MPID_nem_mem_region.FreeQ[grank] = NULL;
	MPID_nem_mem_region.RecvQ[grank] = NULL;
    }


    /* set route for local procs through shmem */
    for (idx = 0; idx < num_local; idx++)
    {
	grank = local_procs[idx];
	MPID_nem_mem_region.FreeQ[grank] = &free_queues_p[idx];
	MPID_nem_mem_region.RecvQ[grank] = &recv_queues_p[idx];

	MPIR_Assert(MPID_NEM_ALIGNED(MPID_nem_mem_region.FreeQ[grank], MPID_NEM_CACHE_LINE_LEN));
	MPIR_Assert(MPID_NEM_ALIGNED(MPID_nem_mem_region.RecvQ[grank], MPID_NEM_CACHE_LINE_LEN));
    }

    /* make pointers to our queues global so we don't have to dereference the array */
    MPID_nem_mem_region.my_freeQ = MPID_nem_mem_region.FreeQ[pg_rank];
    MPID_nem_mem_region.my_recvQ = MPID_nem_mem_region.RecvQ[pg_rank];

    
    /* local barrier */
    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);

    
    /* Allocate table of pointers to fastboxes */
    MPIR_CHKPMEM_MALLOC(MPID_nem_mem_region.mailboxes.in,  MPID_nem_fastbox_t **, num_local * sizeof(MPID_nem_fastbox_t *), mpi_errno, "fastboxes", MPL_MEM_SHM);
    MPIR_CHKPMEM_MALLOC(MPID_nem_mem_region.mailboxes.out, MPID_nem_fastbox_t **, num_local * sizeof(MPID_nem_fastbox_t *), mpi_errno, "fastboxes", MPL_MEM_SHM);

    MPIR_Assert(num_local > 0);

#define MAILBOX_INDEX(sender, receiver) ( ((sender) > (receiver)) ? ((num_local-1) * (sender) + (receiver)) :		\
                                          (((sender) < (receiver)) ? ((num_local-1) * (sender) + ((receiver)-1)) : 0) )

    /* fill in tables */
    for (i = 0; i < num_local; ++i)
    {
	if (i == local_rank)
	{
            /* No fastboxs to myself */
	    MPID_nem_mem_region.mailboxes.in [i] = NULL ;
	    MPID_nem_mem_region.mailboxes.out[i] = NULL ;
	}
	else
	{
	    MPID_nem_mem_region.mailboxes.in [i] = (void *) ((char *) fastboxes_p + (MAILBOX_INDEX(i, local_rank)) * MPID_NEM_FBOX_LEN);
	    MPID_nem_mem_region.mailboxes.out[i] = (void *) ((char *) fastboxes_p + (MAILBOX_INDEX(local_rank, i)) * MPID_NEM_FBOX_LEN);
	    MPL_atomic_relaxed_store_int(&MPID_nem_mem_region.mailboxes.in [i]->flag, 0);
	    MPL_atomic_relaxed_store_int(&MPID_nem_mem_region.mailboxes.out[i]->flag, 0);
	}
    }
#undef MAILBOX_INDEX

    /* setup local LMT */
#if MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_SHM_COPY
        MPID_nem_local_lmt_progress = MPID_nem_lmt_shm_progress;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_DMA
        MPID_nem_local_lmt_progress = MPID_nem_lmt_dma_progress;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_VMSPLICE
        MPID_nem_local_lmt_progress = MPID_nem_lmt_vmsplice_progress;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_NONE
        MPID_nem_local_lmt_progress = NULL;
#else
#  error Must select a valid local LMT implementation!
#endif

    /* publish business card */
    mpi_errno = MPIDI_PG_SetConnInfo(pg_rank, (const char *)publish_bc_orig);
    MPIR_ERR_CHECK(mpi_errno);
    MPL_free(publish_bc_orig);


    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPID_nem_mpich_init();
    MPIR_ERR_CHECK(mpi_errno);
    mpi_errno = MPIDU_Init_shm_barrier();
    MPIR_ERR_CHECK(mpi_errno);
#ifdef ENABLE_CHECKPOINTING
    mpi_errno = MPIDI_nem_ckpt_init();
    MPIR_ERR_CHECK(mpi_errno);
#endif

#ifdef PAPI_MONITOR
    my_papi_start( pg_rank );
#endif /*PAPI_MONITOR   */

    MPID_nem_init_stats(num_local);

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    /* we do not want to lose a potential failed errno */
    tmp_mpi_errno = MPIDU_Init_shm_finalize();
    MPIR_ERR_ADD(mpi_errno, tmp_mpi_errno);
    return mpi_errno;
 fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */

}

/* MPID_nem_vc_init initialize nemesis' part of the vc */
int
MPID_nem_vc_init (MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPIR_CHKPMEM_DECL(1);
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_VC_INIT);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_VC_INIT);
    
    vc_ch->pkt_handler = NULL;
    vc_ch->num_pkt_handlers = 0;
    
    vc_ch->send_seqno         = 0;
#ifdef ENABLE_CHECKPOINTING
    vc_ch->ckpt_msg_len       = 0;
    vc_ch->ckpt_msg_buf       = NULL;
    vc_ch->ckpt_pause_send_vc = NULL;
    vc_ch->ckpt_continue_vc   = NULL;
    vc_ch->ckpt_restart_vc    = NULL;
#endif
    vc_ch->pending_pkt_len    = 0;
    MPIR_CHKPMEM_MALLOC (vc_ch->pending_pkt, MPIDI_CH3_Pkt_t *, sizeof (MPIDI_CH3_Pkt_t), mpi_errno, "pending_pkt", MPL_MEM_BUFFER);

    /* We do different things for vcs in the COMM_WORLD pg vs other pgs
       COMM_WORLD vcs may use shared memory, and already have queues allocated
    */
    if (vc->lpid < MPID_nem_mem_region.num_procs)
    {
	/* This vc is in COMM_WORLD */
	vc_ch->is_local = MPID_NEM_IS_LOCAL (vc->lpid);
	vc_ch->free_queue = MPID_nem_mem_region.FreeQ[vc->lpid]; /* networks and local procs have free queues */
    }
    else
    {
	/* this vc is the result of a connect */
	vc_ch->is_local = 0;
	vc_ch->free_queue = NULL;
    }

    /* MT we acquire the LMT CS here, b/c there is at least a theoretical race
     * on some fields, such as lmt_copy_buf.  In practice it's not an issue, but
     * this will keep DRD happy. */
    MPID_THREAD_CS_ENTER(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* override rendezvous functions */
    vc->rndvSend_fn = MPID_nem_lmt_RndvSend;
    vc->rndvRecv_fn = MPID_nem_lmt_RndvRecv;

    if (vc_ch->is_local)
    {
        MPIDI_CHANGE_VC_STATE(vc, ACTIVE);
        
	vc_ch->fbox_out = MPID_nem_mem_region.mailboxes.out[MPID_nem_mem_region.local_ranks[vc->lpid]];
	vc_ch->fbox_in = MPID_nem_mem_region.mailboxes.in[MPID_nem_mem_region.local_ranks[vc->lpid]];
	vc_ch->recv_queue = MPID_nem_mem_region.RecvQ[vc->lpid];

        /* override nocontig send function */
        vc->sendNoncontig_fn = MPIDI_CH3I_SendNoncontig;

        /* local processes use the default method */
        vc_ch->iStartContigMsg = NULL;
        vc_ch->iSendContig     = NULL;
        vc_ch->iSendIov     = NULL;

#if MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_SHM_COPY
        vc_ch->lmt_initiate_lmt  = MPID_nem_lmt_shm_initiate_lmt;
        vc_ch->lmt_start_recv    = MPID_nem_lmt_shm_start_recv;
        vc_ch->lmt_start_send    = MPID_nem_lmt_shm_start_send;
        vc_ch->lmt_handle_cookie = MPID_nem_lmt_shm_handle_cookie;
        vc_ch->lmt_done_send     = MPID_nem_lmt_shm_done_send;
        vc_ch->lmt_done_recv     = MPID_nem_lmt_shm_done_recv;
        vc_ch->lmt_vc_terminated = MPID_nem_lmt_shm_vc_terminated;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_DMA
        vc_ch->lmt_initiate_lmt  = MPID_nem_lmt_dma_initiate_lmt;
        vc_ch->lmt_start_recv    = MPID_nem_lmt_dma_start_recv;
        vc_ch->lmt_start_send    = MPID_nem_lmt_dma_start_send;
        vc_ch->lmt_handle_cookie = MPID_nem_lmt_dma_handle_cookie;
        vc_ch->lmt_done_send     = MPID_nem_lmt_dma_done_send;
        vc_ch->lmt_done_recv     = MPID_nem_lmt_dma_done_recv;
        vc_ch->lmt_vc_terminated = MPID_nem_lmt_dma_vc_terminated;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_VMSPLICE
        vc_ch->lmt_initiate_lmt  = MPID_nem_lmt_vmsplice_initiate_lmt;
        vc_ch->lmt_start_recv    = MPID_nem_lmt_vmsplice_start_recv;
        vc_ch->lmt_start_send    = MPID_nem_lmt_vmsplice_start_send;
        vc_ch->lmt_handle_cookie = MPID_nem_lmt_vmsplice_handle_cookie;
        vc_ch->lmt_done_send     = MPID_nem_lmt_vmsplice_done_send;
        vc_ch->lmt_done_recv     = MPID_nem_lmt_vmsplice_done_recv;
        vc_ch->lmt_vc_terminated = MPID_nem_lmt_vmsplice_vc_terminated;
#elif MPID_NEM_LOCAL_LMT_IMPL == MPID_NEM_LOCAL_LMT_NONE
        vc_ch->lmt_initiate_lmt  = NULL;
        vc_ch->lmt_start_recv    = NULL;
        vc_ch->lmt_start_send    = NULL;
        vc_ch->lmt_handle_cookie = NULL;
        vc_ch->lmt_done_send     = NULL;
        vc_ch->lmt_done_recv     = NULL;
        vc_ch->lmt_vc_terminated = NULL;
#else
#  error Must select a valid local LMT implementation!
#endif

        vc_ch->lmt_copy_buf        = NULL;
        mpi_errno = MPL_shm_hnd_init(&(vc_ch->lmt_copy_buf_handle));
        if(mpi_errno != MPI_SUCCESS) { MPIR_ERR_POP(mpi_errno); }
        mpi_errno = MPL_shm_hnd_init(&(vc_ch->lmt_recv_copy_buf_handle));
        if(mpi_errno != MPI_SUCCESS) { MPIR_ERR_POP(mpi_errno); }
        vc_ch->lmt_queue.head      = NULL;
        vc_ch->lmt_queue.tail      = NULL;
        vc_ch->lmt_active_lmt      = NULL;
        vc_ch->lmt_enqueued        = FALSE;
        vc_ch->lmt_rts_queue.head  = NULL;
        vc_ch->lmt_rts_queue.tail  = NULL;

        if (MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ == -1)
            vc->eager_max_msg_sz = MPID_NEM_MPICH_DATA_LEN - sizeof(MPIDI_CH3_Pkt_t);
        else
            vc->eager_max_msg_sz = MPIR_CVAR_NEMESIS_SHM_EAGER_MAX_SZ;

        if (MPIR_CVAR_NEMESIS_SHM_READY_EAGER_MAX_SZ == -2)
            vc->ready_eager_max_msg_sz = vc->eager_max_msg_sz; /* force local ready sends to use LMT */
        else
            vc->ready_eager_max_msg_sz = MPIR_CVAR_NEMESIS_SHM_READY_EAGER_MAX_SZ;

        MPL_DBG_MSG(MPIDI_CH3_DBG_VC, VERBOSE, "vc using shared memory");
    }
    else
    {
	vc_ch->fbox_out   = NULL;
	vc_ch->fbox_in    = NULL;
	vc_ch->recv_queue = NULL;

        vc_ch->lmt_initiate_lmt  = NULL;
        vc_ch->lmt_start_recv    = NULL;
        vc_ch->lmt_start_send    = NULL;
        vc_ch->lmt_handle_cookie = NULL;
        vc_ch->lmt_done_send     = NULL;
        vc_ch->lmt_done_recv     = NULL;
        vc_ch->lmt_vc_terminated = NULL;

        /* FIXME: DARIUS set these to default for now */
        vc_ch->iStartContigMsg = NULL;
        vc_ch->iSendContig     = NULL;

        MPL_DBG_MSG_FMT(MPIDI_CH3_DBG_VC, VERBOSE, (MPL_DBG_FDEST, "vc using %s netmod for rank %d pg %s",
                                       MPID_nem_netmod_strings[MPID_nem_netmod_id], vc->pg_rank,
                                       ((vc->pg == MPIDI_Process.my_pg) 
                                        ? "my_pg" 
                                        :   ((vc->pg)
                                             ? ((char *)vc->pg->id)
                                             : "unknown"
                                            )
                                           )
                             ));
        
        mpi_errno = MPID_nem_netmod_func->vc_init(vc);
	MPIR_ERR_CHECK(mpi_errno);

/* FIXME: DARIUS -- enable this assert once these functions are implemented */
/*         /\* iStartContigMsg iSendContig and sendNoncontig_fn must */
/*            be set for nonlocal processes.  Default functions only */
/*            support shared-memory communication. *\/ */
/*         MPIR_Assert(vc_ch->iStartContigMsg && vc_ch->iSendContig && vc->sendNoncontig_fn); */

    }

    MPID_THREAD_CS_EXIT(POBJ, MPIR_THREAD_GLOBAL_ALLFUNC_MUTEX);

    /* FIXME: ch3 assumes there is a field called sendq_head in the ch
       portion of the vc.  This is unused in nemesis and should be set
       to NULL */
    vc_ch->sendq_head = NULL;

    MPIR_CHKPMEM_COMMIT();
 fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_VC_INIT);
    return mpi_errno;
 fn_fail:
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
}

int
MPID_nem_vc_destroy(MPIDI_VC_t *vc)
{
    int mpi_errno = MPI_SUCCESS;
    MPIDI_CH3I_VC *vc_ch = &vc->ch;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPID_NEM_VC_DESTROY);

    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPID_NEM_VC_DESTROY);

    MPL_free(vc_ch->pending_pkt);

    mpi_errno = MPID_nem_netmod_func->vc_destroy(vc);
    MPIR_ERR_CHECK(mpi_errno);

    fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPID_NEM_VC_DESTROY);
    return mpi_errno;
 fn_fail:
    goto fn_exit;
}

int
MPID_nem_get_business_card (int my_rank, char *value, int length)
{
    return MPID_nem_netmod_func->get_business_card (my_rank, &value, &length);
}

int MPID_nem_connect_to_root (const char *business_card, MPIDI_VC_t *new_vc)
{
    return MPID_nem_netmod_func->connect_to_root (business_card, new_vc);
}

/* get_local_procs() determines which processes are local and
   should use shared memory
 
   If an output variable pointer is NULL, it won't be set.

   Caller should NOT free any returned buffers.

   Note that this is really only a temporary solution as it only
   calculates these values for processes MPI_COMM_WORLD, i.e., not for
   spawned or attached processes.
*/
static int get_local_procs(MPIDI_PG_t *pg, int our_pg_rank, int *num_local_p,
                           int **local_procs_p, int *local_rank_p)
{
    int mpi_errno = MPI_SUCCESS;
    int *procs;
    int i;
    int num_local = 0;
    int our_node_id;
    MPIR_CHKPMEM_DECL(1);

    MPIR_Assert(our_pg_rank < pg->size);
    our_node_id = pg->vct[our_pg_rank].node_id;

    MPIR_CHKPMEM_MALLOC(procs, int *, pg->size * sizeof(int), mpi_errno, "local process index array", MPL_MEM_ADDRESS);

    for (i = 0; i < pg->size; ++i) {
        if (our_node_id == pg->vct[i].node_id) {
            if (i == our_pg_rank && local_rank_p != NULL) {
                *local_rank_p = num_local;
            }
            procs[num_local] = i;
            ++num_local;
        }
    }

    MPIR_CHKPMEM_COMMIT();

    if (num_local_p != NULL)
        *num_local_p = num_local;
    if (local_procs_p != NULL)
        *local_procs_p = procs;
fn_exit:
    return mpi_errno;
fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    MPIR_CHKPMEM_REAP();
    goto fn_exit;
    /* --END ERROR HANDLING-- */
}

