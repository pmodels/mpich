/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"


typedef struct mpig_port_vc_exchange_contextid_info_t
{
    mpig_vc_t * port_vc;
    bool_t acceptor;
}
mpig_port_vc_exchange_contextid_info_t;


static int mpig_port_vc_exchange_contextid(int mask_size, const unsigned * local_mask, unsigned * remote_mask, void * xchg_info);

/*
 * MPID_Open_port
 */
#undef FUNCNAME
#define FUNCNAME MPID_Open_port
int MPID_Open_port(MPID_Info * info, char * port_name)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;

    MPIG_STATE_DECL(MPID_STATE_MPID_OPEN_PORT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_OPEN_PORT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering"));

    mpi_errno = mpig_port_open(info, port_name);
    MPIU_ERR_CHKANDJUMP2((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|open_port",
	"**globus|connacc|open_port %p %p", info, port_name);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting: port_name=%s",
		       port_name));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_OPEN_PORT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Open_port */


/*
 * MPID_Comm_Close_port()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Close_port
int MPID_Close_port(const char * port_name)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno=MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_CLOSE_PORT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_CLOSE_PORT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s",
		       port_name));

    mpi_errno = mpig_port_close(port_name);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|connacc|close_port",
	"**globus|connacc|close_port %s", port_name);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_CLOSE_PORT);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* MPID_Comm_Close_port() */


/*
 * MPID_Comm_accept()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_accept
int MPID_Comm_accept(char * const port_name, MPID_Info * const info, const int root, MPID_Comm * const comm,
    MPID_Comm ** const newcomm_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Comm * newcomm = NULL;
    mpig_vc_t * port_vc = NULL;
    char * local_vct_str = NULL;
    unsigned local_vct_len = 0;
    char * remote_vct_str = NULL;
    unsigned remote_vct_len = 0;
    int newctx = 0;
    mpig_vcrt_t * vcrt = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_ACCEPT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_ACCEPT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, root, comm->handle, MPIG_PTR_CAST(comm)));

    /* allocate a new communicator structure */
    mpi_errno = MPIR_Comm_create(&newcomm);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate an intercommunicator:"
	    "port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "intercommunicator");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    

    if (comm->rank == root)
    {
	/* create an array of process containing the process group id, process group rank, and serialized business card for each
	   VC in the virtual connection reference table (VCRT) attached to the supplied communicator */
	mpi_errno = mpig_vcrt_serialize_object(comm->vcrt, &local_vct_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
		"port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|vcrt_serialize",
		"**globus|connacc|vcrt_serialize %s %C %p", port_name, comm->handle, comm->vcrt);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	local_vct_len = strlen(local_vct_str) + 1;

	/* accept a new connection for the port */
	mpi_errno = mpig_port_accept(port_name, &port_vc);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: accepting a new connection failed:"
		"port_name=%s", port_name));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_accept",
		"**globus|connacc|port_accept %s %i %C", port_name, root, comm->handle);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "port accept succeeded: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	
	/* exchange information about the length of the VCT string */
	{
	    char buf[sizeof(u_int32_t)];
	    u_int32_t tmp_uint;

	    tmp_uint = local_vct_len;
	    mpig_dc_put_uint32(buf, tmp_uint);
	    
	    mpi_errno = mpig_port_vc_send(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string length "
		    "failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		    "**globus|connacc|port_vc_send %p %p %s", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpi_errno = mpig_port_vc_recv(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string length "
		    "failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		    "**globus|connacc|port_vc_recv %p %p %s", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpig_dc_get_uint32(mpig_port_vc_get_endian(port_vc), buf, &tmp_uint);
	    remote_vct_len = tmp_uint;

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "remote nprocs and VCT string length received: port_vc=" MPIG_PTR_FMT
		", remote_vct_len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	}
	
	/* send the local VCT string to the connecting process */
	mpi_errno = mpig_port_vc_send(port_vc, local_vct_str, local_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ",str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), MPIG_PTR_CAST(local_vct_str),
		local_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p %p %d", port_vc, local_vct_str, (int) local_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	mpig_vcrt_free_serialized_object(local_vct_str);

	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold the "
		"remote VCT string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VCT string");
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string from the connecting process */
	mpi_errno = mpig_port_vc_recv(port_vc, remote_vct_str, remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc),
		MPIG_PTR_CAST(remote_vct_str), remote_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		"**globus|connacc|port_vc_recv %p %p %d", port_vc, remote_vct_str, (int) remote_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
      endblk:
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    int mrc;
	    
	    remote_vct_len = 0;
	    mrc = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	    if (mrc)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of an error condition "
		    "to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT1(mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);},
		    "**globus|connacc|bcast_vct_error_status", "**globus|connacc|bcast_vct_error_status %p", port_vc);
		goto fn_fail;
	    }
	    
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* send remote VC string to the other processes in the local communicator */
	mpi_errno = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string length "
		"to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len",
		"**globus|connacc|bcast_vct_len %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpi_errno = MPIR_Bcast_impl(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string to "
		"processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	{
	    mpig_port_vc_exchange_contextid_info_t xchg_info;

	    /* allocate a new context id for the communicator.  if an error occurs while allocating a context id (and the
	       MPIR_Get_contextid routine returns), all processes should be aware of the error and thus it should be safe for all
	       processes to exit the routine without any further communication to verify the error. */
	    xchg_info.port_vc = port_vc;
	    xchg_info.acceptor = TRUE;
	
	    newctx = MPIR_Get_contextid(comm, mpig_port_vc_exchange_contextid, root, &xchg_info);
	    if (newctx == 0)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context "
		    "id: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
    }
    else /* not the root */
    {
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of remote VCT string length "
		"from the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len", "**globus|connacc|bcast_vct_len %p",
		port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	if (remote_vct_len == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: communication between the two root "
		"processes failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_error_status_recvd",
		"**globus|connacc|bcast_vct_error_status_recvd %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (remote_vct_str == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold "
		"the remote string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_str = 0;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = MPIR_Bcast_impl(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of  remote VCT string from "
		"the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* allocate a new context id for the communicator.  as with the root, all processes should be aware of any error that
	   occurs and thus it should be safe to exit the routine when an error is detected. */
	newctx = MPIR_Get_contextid(comm, mpig_port_vc_exchange_contextid, root, NULL);
	if (newctx == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context id: "
		"port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	    
    }
    /* to be the root or not to be the root, that is the question */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "new context id allocated for the intercommunicator, port_vc=" MPIG_PTR_FMT
	",ctx=%d", MPIG_PTR_CAST(port_vc), newctx));

    /* deserialize the remote VC string, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */
    mpi_errno = mpig_vcrt_deserialize_object(remote_vct_str, &vcrt);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
	    "port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|vcrt_deserialize");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* FT-FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

    /* complete the contruction of the intercommunicator */
    newcomm->comm_kind = MPID_INTERCOMM;
    newcomm->context_id = newctx;

    newcomm->local_size = comm->remote_size;
    newcomm->rank = comm->rank;
    MPID_VCRT_Add_ref(comm->vcrt);
    newcomm->local_vcrt = comm->vcrt;
    newcomm->local_vcr = comm->vcr;

    newcomm->remote_size = mpig_vcrt_size(vcrt);
    newcomm->vcrt = vcrt;
    MPID_VCRT_Get_ptr(newcomm->vcrt, &newcomm->vcr);

    newcomm->is_low_group = TRUE;
    newcomm->local_comm = NULL;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook(newcomm);

    /* return the new communicator to the calling routine */
    *newcomm_p = newcomm;

  fn_return:
    if (port_vc)
    {
	mpig_port_vc_close(port_vc);
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", newcomm=" MPIG_PTR_FMT, port_name, root, comm->handle,
	MPIG_PTR_CAST(comm), MPIG_PTR_CAST(newcomm)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_ACCEPT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (newcomm->vcrt != NULL)
	{
	    MPID_VCRT_Release(newcomm->vcrt);
	    MPID_VCRT_Release(newcomm->local_vcrt);
	}
	
	if (newctx != 0)
	{
	    MPIR_Free_contextid(newctx);
	}
	
	if (newcomm != NULL)
	{
	    MPIU_Handle_obj_free(&MPID_Comm_mem, newcomm);
	    newcomm = NULL;
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Comm_accept() */


/*
 * mpid_comm_connect()
 */
#undef FUNCNAME
#define FUNCNAME MPID_Comm_connect
int MPID_Comm_connect(const char * const port_name, MPID_Info * const info, const int root, MPID_Comm * const comm,
    MPID_Comm ** const newcomm_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Comm * newcomm = NULL;
    mpig_vc_t * port_vc = NULL;
    char * local_vct_str = NULL;
    unsigned local_vct_len = 0;
    char * remote_vct_str = NULL;
    unsigned remote_vct_len = 0;
    int newctx = 0;
    mpig_vcrt_t * vcrt = NULL;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_MPID_COMM_CONNECT);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPID_COMM_CONNECT);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, root, comm->handle, MPIG_PTR_CAST(comm)));

    /* allocate a new communicator structure */
    mpi_errno = MPIR_Comm_create(&newcomm);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate an intercommunicator:"
	    "port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "intercommunicator");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    

    if (comm->rank == root)
    {
	/* create an array of process containing the process group id, process group rank, and serialized business card for each
	   VC in the virtual connection reference table (VCRT) attached to the supplied communicator */
	mpi_errno = mpig_vcrt_serialize_object(comm->vcrt, &local_vct_str);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
		"port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|vcrt_serialize",
		"**globus|connacc|vcrt_serialize %s %C %p", port_name, comm->handle, comm->vcrt);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	local_vct_len = strlen(local_vct_str) + 1;

	/* connect to the provided port */
	mpi_errno = mpig_port_connect(port_name, &port_vc);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: connecting to the port failed:"
		"port_name=%s", port_name));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_connect",
		"**globus|connacc|port_connect %s %i %C", port_name, root, comm->handle);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "port connect succeeded: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	
	/* exchange information about the length of the VCT string */
	{
	    char buf[sizeof(u_int32_t)];
	    u_int32_t tmp_uint;

	    mpi_errno = mpig_port_vc_recv(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string length "
		    "failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		    "**globus|connacc|port_vc_recv %p %p %s", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    mpig_dc_get_uint32(mpig_port_vc_get_endian(port_vc), buf, &tmp_uint);
	    remote_vct_len = tmp_uint;

	    tmp_uint = local_vct_len;
	    mpig_dc_put_uint32(buf, tmp_uint);
	    
	    mpi_errno = mpig_port_vc_send(port_vc, buf, sizeof(buf));
	    if (mpi_errno)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string length "
		    "failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		    "**globus|connacc|port_vc_send %p %p %s", port_vc, buf, sizeof(buf));
		goto endblk;
	    }   /* --END ERROR HANDLING-- */

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "remote nprocs and VCT string length received: port_vc=" MPIG_PTR_FMT
		", remote_vct_len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	}
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold the "
		"remote VCT string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VCT string");
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string from the accepting process */
	mpi_errno = mpig_port_vc_recv(port_vc, remote_vct_str, remote_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ", str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc),
		MPIG_PTR_CAST(remote_vct_str), remote_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_recv",
		"**globus|connacc|port_vc_recv %p %p %d", port_vc, remote_vct_str, (int) remote_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */
	
	/* send the local VCT string to the accepting process */
	mpi_errno = mpig_port_vc_send(port_vc, local_vct_str, local_vct_len);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local VCT string failed: "
		"port_vc=" MPIG_PTR_FMT ",str=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), MPIG_PTR_CAST(local_vct_str),
		local_vct_len));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p %p %d", port_vc, local_vct_str, (int) local_vct_len);
	    goto endblk;
	}   /* --END ERROR HANDLING-- */

	mpig_vcrt_free_serialized_object(local_vct_str);
	
      endblk:
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    int mrc;
	    
	    remote_vct_len = 0;
	    mrc = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	    if (mrc)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of an error condition "
		    "to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SETANDSTMT1(mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);},
		    "**globus|connacc|bcast_vct_error_status", "**globus|connacc|bcast_vct_error_status %p", port_vc);
		goto fn_fail;
	    }
	    
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* send remote VC string to the other processes in the local communicator */
	mpi_errno = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string length "
		"to processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len",
		"**globus|connacc|bcast_vct_len %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	mpi_errno = MPIR_Bcast_impl(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: broadcast of remote VCT string to "
		"processes in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	{
	    mpig_port_vc_exchange_contextid_info_t xchg_info;

	    /* allocate a new context id for the communicator.  if an error occurs while allocating a context id (and the
	       MPIR_Get_contextid routine returns), all processes should be aware of the error and thus it should be safe for all
	       processes to exit the routine without any further communication to verify the error. */
	    xchg_info.port_vc = port_vc;
	    xchg_info.acceptor = FALSE;
	
	    newctx = MPIR_Get_contextid(comm, mpig_port_vc_exchange_contextid, root, &xchg_info);
	    if (newctx == 0)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context "
		    "id: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */
	}
    }
    else /* not the root */
    {
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = MPIR_Bcast_impl(&remote_vct_len, 1, MPI_UNSIGNED, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of remote VCT string length "
		"from the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_len", "**globus|connacc|bcast_vct_len %p",
		port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	if (remote_vct_len == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: communication between the two root "
		"processes failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|bcast_vct_error_status_recvd",
		"**globus|connacc|bcast_vct_error_status_recvd %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	/* allocate a buffer for the remote VCT string */
	remote_vct_str = (char *) MPIU_Malloc(remote_vct_len);
	if (remote_vct_str == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: unable to allocate a buffer to hold "
		"the remote string: port_vc=" MPIG_PTR_FMT ", len=%u", MPIG_PTR_CAST(port_vc), remote_vct_len));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem",  "**nomem %s", "receive buffer for remote VC objects");
	    remote_vct_str = 0;
	}   /* --END ERROR HANDLING-- */
	
	/* receive the remote VCT string length from the root of the local communicator */
	mpi_errno = MPIR_Bcast_impl(remote_vct_str, (int) remote_vct_len, MPI_BYTE, root, comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: reception of  remote VCT string from "
		"the root process in the local communicator failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|bcast_vct_str",
		"**globus|connacc|bcast_vct_str %p %p %d", port_vc, remote_vct_str, remote_vct_len);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* allocate a new context id for the communicator.  as with the root, all processes should be aware of any error that
	   occurs and thus it should be safe to exit the routine when an error is detected. */
	newctx = MPIR_Get_contextid(comm, mpig_port_vc_exchange_contextid, root, NULL);
	if (newctx == 0)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: failed to allocate a new context id: "
		"port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(port_vc)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|ctxalloc", "**globus|connacc|ctxalloc %p", port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	    
    }
    /* to be the root or not to be the root, that is the question */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DYNAMIC, "new context id allocated for the intercommunicator, port_vc=" MPIG_PTR_FMT
	", ctx=%d", MPIG_PTR_CAST(port_vc), newctx));

    /* deserialize the remote VC string, creating additional process groups as necessary.  during this process, construct a remote
       VCRT for new intercommunicator */
    mpi_errno = mpig_vcrt_deserialize_object(remote_vct_str, &vcrt);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: serializing the VCRT failed: "
	    "port_name=%s, comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, port_name, comm->handle, MPIG_PTR_CAST(comm)));
	MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|connacc|vcrt_deserialize");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    /* FT-FIXME: gather status from local comm, exchange status with remote root, and broadcast status to local comm so that all
       processes know the creation of the intercommunciator was successful */

    /* complete the contruction of the intercommunicator */
    newcomm->comm_kind = MPID_INTERCOMM;
    newcomm->context_id = newctx;

    newcomm->local_size = comm->remote_size;
    newcomm->rank = comm->rank;
    MPID_VCRT_Add_ref(comm->vcrt);
    newcomm->local_vcrt = comm->vcrt;
    newcomm->local_vcr = comm->vcr;

    newcomm->remote_size = mpig_vcrt_size(vcrt);
    newcomm->vcrt = vcrt;
    MPID_VCRT_Get_ptr(newcomm->vcrt, &newcomm->vcr);

    newcomm->is_low_group = FALSE;
    newcomm->local_comm = NULL;

    /* Notify the device of this new communicator */
    MPID_Dev_comm_create_hook(newcomm);

    /* return the new communicator to the calling routine */
    *newcomm_p = newcomm;

  fn_return:
    if (port_vc)
    {
	mpig_port_vc_close(port_vc);
    }

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_name=%s, root=%d"
	", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT ", newcomm=" MPIG_PTR_FMT, port_name, root, comm->handle,
	MPIG_PTR_CAST(comm), MPIG_PTR_CAST(newcomm)));
    MPIG_FUNC_EXIT(MPID_STATE_MPID_COMM_CONNECT);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (newcomm->vcrt != NULL)
	{
	    MPID_VCRT_Release(newcomm->vcrt);
	    MPID_VCRT_Release(newcomm->local_vcrt);
	}
	
	if (newctx != 0)
	{
	    MPIR_Free_contextid(newctx);
	}
	
	if (newcomm != NULL)
	{
	    MPIU_Handle_obj_free(&MPID_Comm_mem, newcomm);
	    newcomm = NULL;
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* MPID_Comm_connect() */


/*
 * mpig_port_vc_exchange_contextid()
 */
#undef FUNCNAME
#define FUNCNAME mpig_port_vc_exchange_contextid
static int mpig_port_vc_exchange_contextid(const int mask_size, const unsigned * const local_mask, unsigned * const remote_mask,
    void * const arg)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_port_vc_exchange_contextid_info_t * xchg_info = (mpig_port_vc_exchange_contextid_info_t *) arg;
    mpig_endian_t endian;
    char * buf;
    char * bufp;
    u_int32_t tmp_uint;
    int n;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_MPIG_PORT_VC_EXCHANGE_CONTEXTID);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_MPIG_PORT_VC_EXCHANGE_CONTEXTID);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: mask_size=%d, "
	", port_vc=" MPIG_PTR_FMT ", acceptor=%s", mask_size, MPIG_PTR_CAST(xchg_info->port_vc),
	MPIG_BOOL_STR(xchg_info->acceptor)));

    MPIU_CHKLMEM_MALLOC(buf, char *, mask_size * sizeof(u_int32_t), mpi_errno, "buffer for exchanging context id masks");

    endian = mpig_port_vc_get_endian(xchg_info->port_vc);
    
    if (xchg_info->acceptor)
    {
	/* pack up the local mask and send it to the connector */
	bufp = buf;
	for (n = 0; n < mask_size; n++)
	{
	    tmp_uint = local_mask[n];
	    mpig_dc_put_uint32(bufp, tmp_uint);
	    bufp += sizeof(u_int32_t);
	}

	mpi_errno = mpig_port_vc_send(xchg_info->port_vc, buf, mask_size * sizeof(u_int32_t));
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local context id mask to the "
		"connector failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(xchg_info->port_vc)));
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p", xchg_info->port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	/* receive the remote mask from the connector and unpack it */
	mpi_errno = mpig_port_vc_recv(xchg_info->port_vc, buf, mask_size * sizeof(u_int32_t));
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote context id mask from "
		"the connector failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(xchg_info->port_vc)));
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p", xchg_info->port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	bufp = buf;
	for (n = 0; n < mask_size; n++)
	{
	    mpig_dc_get_uint32(endian, bufp, &tmp_uint);
	    remote_mask[n] = tmp_uint;
	    bufp += sizeof(u_int32_t);
	}
    }
    else
    {
	/* receive the remote mask from the acceptor and unpack it */
	mpi_errno = mpig_port_vc_recv(xchg_info->port_vc, buf, mask_size * sizeof(u_int32_t));
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: receiving remote context id mask from "
		"the acceptor failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(xchg_info->port_vc)));
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p", xchg_info->port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
	
	bufp = buf;
	for (n = 0; n < mask_size; n++)
	{
	    mpig_dc_get_uint32(endian, bufp, &tmp_uint);
	    remote_mask[n] = tmp_uint;
	    bufp += sizeof(u_int32_t);
	}

	/* pack up the local mask and send it to the acceptor */
	bufp = buf;
	for (n = 0; n < mask_size; n++)
	{
	    tmp_uint = local_mask[n];
	    mpig_dc_put_uint32(bufp, tmp_uint);
	    bufp += sizeof(u_int32_t);
	}

	mpi_errno = mpig_port_vc_send(xchg_info->port_vc, buf, mask_size * sizeof(u_int32_t));
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DYNAMIC, "ERROR: sending local context id mask to the "
		"acceptor failed: port_vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(xchg_info->port_vc)));
	    MPIU_ERR_SETANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|connacc|port_vc_send",
		"**globus|connacc|port_vc_send %p", xchg_info->port_vc);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_DYNAMIC, "entering: port_vc=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(xchg_info->port_vc)));
    MPIG_FUNC_EXIT(MPID_STATE_MPIG_PORT_VC_EXCHANGE_CONTEXTID);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	/* set the masks to all zeros to indicate that no context id is available */
	for (n = 0; n < mask_size; n++)
	{
	    remote_mask[n] = 0;
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_port_vc_exchange_contextid() */
