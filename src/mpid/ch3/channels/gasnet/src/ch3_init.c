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

#include "mpidi_ch3_impl.h"
#include "pmi.h"

static int MPIDI_CH3I_PG_Compare_ids(void * id1, void * id2)
{
    return TRUE;
}

static int MPIDI_CH3I_PG_Destroy(MPIDI_PG_t * pg)
{
    return MPI_SUCCESS;
}

int MPIDI_CH3_packet_len;
void *MPIDI_CH3_packet_buffer;

int MPIDI_CH3I_my_rank;

extern void MPIDI_CH3_start_packet_handler (gasnet_token_t token, void* buf,
					    size_t data_sz);
extern void MPIDI_CH3_continue_packet_handler (gasnet_token_t token, void* buf,
					       size_t data_sz);
extern void MPIDI_CH3_CTS_packet_handler (gasnet_token_t token, void* buf,
					  size_t buf_sz, MPI_Request sreq_id,
					  MPI_Request rreq_id, int remote_buf_sz,
					  int n_iov);
extern void MPIDI_CH3_reload_IOV_or_done_handler (gasnet_token_t token,
						  int rreq_id);
extern void MPIDI_CH3_reload_IOV_reply_handler (gasnet_token_t token, void *buf,
						int buf_sz, int sreq_id,
						int n_iov);

gasnet_handlerentry_t MPIDI_CH3_gasnet_handler_table[] =
{
    { MPIDI_CH3_start_packet_handler_id, MPIDI_CH3_start_packet_handler },
    { MPIDI_CH3_continue_packet_handler_id, MPIDI_CH3_continue_packet_handler },
    { MPIDI_CH3_CTS_packet_handler_id, MPIDI_CH3_CTS_packet_handler },
    { MPIDI_CH3_reload_IOV_or_done_handler_id,
      MPIDI_CH3_reload_IOV_or_done_handler },
    { MPIDI_CH3_reload_IOV_reply_handler_id, MPIDI_CH3_reload_IOV_reply_handler }
};

//MPIDI_CH3I_Process_t MPIDI_CH3I_Process = { 0 };

/* XXX - all calls to assert() need to be turned into real error checking and
   return meaningful errors */

static int called_pre_init = 0;

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Pre_init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Pre_init (int *setvals, int *has_parent, int *rank, int *size)
{
    int gn_errno;
    int mpi_errno = MPI_SUCCESS;
    int null_argc = 0;
    char **null_argv = 0;


#ifdef HYBRID_ENABLED_GASNET
    if (gasnet_isInit ())
    {
	/* gasnet has already been initialized (eg in a hybrid UPC/MPI
	 * environment), so all we have to do is register additional
	 * handlers (and hope the handler ids don't collide)
	 */
	gn_errno = gasnet_registerHandlers (MPIDI_CH3_gasnet_handler_table,
					    sizeof (MPIDI_CH3_gasnet_handler_table) /
					    sizeof (gasnet_handlerentry_t));
	if (gn_errno != GASNET_OK)
	{
	    mpi_errno = MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					      __LINE__, MPI_ERR_OTHER, "**init",
					      "gasnet_registerHandlers failed %d",
					      gn_errno);
	    return mpi_errno;
	}
    }
    else
#endif
    {
	/* gasnet has not been initialized, so we initialize and attach */
	
	/* Pass in null args */
	gn_errno = gasnet_init (&null_argc, &null_argv);
	if (gn_errno != GASNET_OK)
	{
	    mpi_errno = MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					      __LINE__, MPI_ERR_OTHER, "**init",
					      "gasnet_init failed %d", gn_errno);
	    return mpi_errno;
	}


	/* by specifying a 0 segsize, we're disabling remote memory
	 * access.  For now, we're just using medium AM messages */
	gn_errno = gasnet_attach (MPIDI_CH3_gasnet_handler_table,
				  sizeof (MPIDI_CH3_gasnet_handler_table) /
				  sizeof (gasnet_handlerentry_t),
				  0, (uintptr_t)-1);
	if (gn_errno != GASNET_OK)
	{
	    mpi_errno = MPIR_Err_create_code (MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					      __LINE__, MPI_ERR_OTHER, "**init",
					      "gasnet_attach failed %d",
					      gn_errno);
	    return mpi_errno;
	}
    }
    
    MPIDI_CH3I_my_rank = gasnet_mynode ();
    *rank = MPIDI_CH3I_my_rank;
    *size = gasnet_nodes ();
    *has_parent = 0;
    *setvals = 1;
    
    called_pre_init = 1;

    return mpi_errno;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int has_parent, MPIDI_PG_t * pg_p, int pg_rank)
{
    int mpi_errno;    
    int size;
    MPID_Comm * comm, *commworld, *intercomm;
    int p;
    int name_sz;
#ifdef FOO
    int i;
#endif

    if (!called_pre_init)
    {
	mpi_errno =  MPIR_Err_create_code( MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_INTERN, "**intern", 0 );
	return mpi_errno;
    }

    MPIDI_CH3I_inside_handler = 0;

    /*MPIDI_CH3_packet_len = gasnet_AMMaxMedium ();*/
    /* 16K gives better performance than 64K */
    /* FIXME:  This should probably be a #define or something */
    MPIDI_CH3_packet_len = 16384;

    MPIDI_CH3_packet_buffer = MPIU_Malloc (MPIDI_CH3_packet_len);
    if (MPIDI_CH3_packet_buffer == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }    

    /*
     * Initialize Progress Engine 
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**init_progress", 0);
	return mpi_errno;
    }
    
    /* initialize VCs */
    for (p = 0; p < pg_p->size; p++)
    {
	struct MPIDI_VC *vcp;

	MPIDI_PG_Get_vcr (pg_p, p, &vcp);
	
	vcp->gasnet.pg_rank = p;
	vcp->gasnet.data_sz = 0;
	vcp->gasnet.data = NULL;
	vcp->gasnet.recv_active = NULL;
	vcp->state = MPIDI_VC_STATE_ACTIVE;
    }

    if (has_parent) 
    {
	/* gasnet can't spawn --Darius */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME, __LINE__, MPI_ERR_OTHER, "**notimpl", 0);
	return mpi_errno;
    }

    return MPI_SUCCESS;
}


/* This function simply tells the CH3 device to use the defaults for the 
   MPI-2 RMA functions */
int MPIDI_CH3_RMAFnsInit( MPIDI_RMAFns *a ) 
{ 
    return 0;
}

