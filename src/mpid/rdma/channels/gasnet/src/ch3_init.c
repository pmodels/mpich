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

int MPIDI_CH3_packet_len;
MPIDI_VC *MPIDI_CH3_vc_table;

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

MPIDI_CH3I_Process_t MPIDI_CH3I_Process;

/* XXX - all calls to assert() need to be turned into real error checking and
   return meaningful errors */

#undef FUNCNAME
#define FUNCNAME MPIDI_CH3_Init
#undef FCNAME
#define FCNAME MPIDI_QUOTE(FUNCNAME)
int MPIDI_CH3_Init(int * argc, char *** argv, int * has_args, int * has_env,
		   int * has_parent)
{
    int gn_errno;
    int mpi_errno;
    MPIDI_CH3I_Process_group_t * pg;
    
    int pg_rank;
    int pg_size;
    MPIDI_VC * vc_table;
    MPID_Comm * comm, *commworld, *intercomm;
    int p;
    int name_sz;
#ifdef FOO
    int i;
#endif

    MPIDI_CH3I_inside_handler = 0;
	
    gn_errno = gasnet_init (argc, argv);
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
                                          "gasnet_attach failed %d", gn_errno);
        return mpi_errno;
    }

    MPIDI_CH3_packet_len = gasnet_AMMaxMedium ();
#warning DARIUS
    MPIDI_CH3_packet_len = 16384;

    pg_rank = gasnet_mynode ();
    pg_size = gasnet_nodes ();

    MPIDI_CH3I_my_rank = pg_rank;

    /* Allocate process group data structure and populate */
    pg = MPIU_Malloc(sizeof(MPIDI_CH3I_Process_group_t));
    if (pg == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->size = pg_size;
    mpi_errno = PMI_KVS_Get_name_length_max(&name_sz);
    if (mpi_errno != PMI_SUCCESS)
    {
	MPID_Abort (NULL, MPI_SUCCESS, -1);
    }
    pg->kvs_name = MPIU_Malloc(name_sz + 1);
    if (pg->kvs_name == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count = 1;
    MPIDI_CH3I_Process.pg = pg;
    
    /* Allocate and initialize the VC table associated with this
     * process group (and thus COMM_WORLD) */
    vc_table = MPIU_Malloc(sizeof(MPIDI_VC) * pg_size);
    if (vc_table == NULL)
    {
	mpi_errno = MPIR_Err_create_code(MPI_SUCCESS, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**nomem", 0);
	return mpi_errno;
    }
    pg->ref_count += pg_size;
    for (p = 0; p < pg_size; p++)
    {
	MPIDI_CH3U_VC_init(&vc_table[p], p);
	vc_table[p].gasnet.pg = pg;
	vc_table[p].gasnet.pg_rank = p;
	vc_table[p].gasnet.recv_active = NULL;
    }
    pg->vc_table = vc_table;
    MPIDI_CH3_vc_table = vc_table;
    
    /*
     * Initialize Progress Engine 
     */
    mpi_errno = MPIDI_CH3I_Progress_init();
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**init_progress", 0);
	return mpi_errno;
    }
    
    /* Initialize MPI_COMM_WORLD object */
    comm = MPIR_Process.comm_world;
    comm->rank = pg_rank;
    comm->remote_size = comm->local_size = pg_size;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_vcrt",
					 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_getptr",
					 0);
	return mpi_errno;
    }
    for (p = 0; p < pg_size; p++)
    {
	MPID_VCR_Dup(&vc_table[p], &comm->vcr[p]);
    }
    
    /* Initialize MPI_COMM_SELF object */
    comm = MPIR_Process.comm_self;
    comm->rank = 0;
    comm->remote_size = comm->local_size = 1;
    mpi_errno = MPID_VCRT_Create(comm->remote_size, &comm->vcrt);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_vcrt",
					 0);
	return mpi_errno;
    }
    mpi_errno = MPID_VCRT_Get_ptr(comm->vcrt, &comm->vcr);
    if (mpi_errno != MPI_SUCCESS)
    {
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER, "**init_getptr",
					 0);
	return mpi_errno;
    }
    MPID_VCR_Dup(&vc_table[pg_rank], &comm->vcr[0]);    
    
    /* XXX - has_args and has_env need to come from PMI eventually... */
    *has_args = TRUE;
    *has_env = TRUE;

    *has_parent = 0;
    
    if (*has_parent) 
    {
	/* gasnet can't spawn --Darius */
	mpi_errno = MPIR_Err_create_code(mpi_errno, MPIR_ERR_FATAL, FCNAME,
					 __LINE__, MPI_ERR_OTHER,
					 "**notimpl", 0);
	return mpi_errno;
    }

    return MPI_SUCCESS;
}
