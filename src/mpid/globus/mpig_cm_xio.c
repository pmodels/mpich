/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if defined(HAVE_GLOBUS_XIO_MODULE)

#include "globus_dc.h"
#include "globus_xio_tcp_driver.h"


#define FCNAME fcname


/**********************************************************************************************************************************
							BEGIN PARAMETERS
**********************************************************************************************************************************/
/*
 * user tunable parameters
 */
#if !defined(MPIG_CM_XIO_EAGER_MAX_SIZE)
#define MPIG_CM_XIO_EAGER_MAX_SIZE (256*1024+3)
#endif

#if !defined(MPIG_CM_XIO_DATA_DENSITY_THRESHOLD)
#define MPIG_CM_XIO_DATA_DENSITY_THRESHOLD (8*1024)
#endif

#if !defined(MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_SPARSE_BUFFER_SIZE (256*1024-1)
#endif

#if !defined(MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE)
#define MPIG_CM_XIO_DATA_TRUNCATION_BUFFER_SIZE (15)
#endif

#if !defined(MPIG_CM_XIO_MAX_ACCEPT_ERRORS)
#define MPIG_CM_XIO_MAX_ACCEPT_ERRORS 255
#endif

/*
 *internal parameters
 */
#define MPIG_CM_XIO_PROTO_VERSION 1
#define MPIG_CM_XIO_CONNACC_PROTO_VERSION 1
#define MPIG_CM_XIO_PROTO_CONNECT_MAGIC "MPIG-CM-XIO-SHAKE-MAGIC-8-BALL\n"
#define MPIG_CM_XIO_PROTO_ACCEPT_MAGIC "MPIG-CM-XIO-MAGIC-8-BALL-SAYS-YES\n"
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
MPIG_STATIC mpig_mutex_t mpig_cm_xio_mutex;
MPIG_STATIC int mpig_cm_xio_methods_active = 0;

MPIG_STATIC mpig_pe_info_t mpig_cm_xio_pe_info;

MPIG_STATIC int mpig_cm_xio_tcp_buf_size = 0;

static int mpig_cm_xio_module_init(void);

static int mpig_cm_xio_module_finalize(void);

static const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type);


#define mpig_cm_xio_mutex_construct()	mpig_mutex_construct(&mpig_cm_xio_mutex)
#define mpig_cm_xio_mutex_destruct()	mpig_mutex_construct(&mpig_cm_xio_mutex)
#define mpig_cm_xio_mutex_lock()					\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - acquiring mutex"));	\
    mpig_mutex_lock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio local data - mutex acquired"));		\
}
#define mpig_cm_xio_mutex_unlock()					\
{									\
    mpig_mutex_unlock(&mpig_cm_xio_mutex);				\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS,			\
		       "cm_xio global mutex - mutex released"));	\
}
#define mpig_cm_xio_mutex_lock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_lock();}
#define mpig_cm_xio_mutex_unlock_conditional(cond_)	{if (cond_) mpig_cm_xio_mutex_unlock();}
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/
#undef MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_nets.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					 END INCLUSION OF INTERNAL MACROS AND PROTOTYPES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					   BEGIN COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_xio_module_init(void)
 *
 * NOTE: this routine insures that the module data structures are initialized only once even if multiple methods are initialized
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_module_init
static int mpig_cm_xio_module_init(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_module_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_module_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    if(mpig_cm_xio_methods_active == 0)
    {
        mpig_cm_xio_mutex_construct();

	/* initialize the vc tracking list */
	mpig_cm_xio_vc_list_init();
    
	/* attempt to get the user specified TCP buffer size */
	if (mpig_cm_xio_tcp_buf_size == 0)
	{
	    char * env_str;
	    long size;

	     env_str = globus_libc_getenv("MPIG_TCP_BUFFER_SIZE");
	     if (env_str == NULL)
	     {
		 /* if the MPIg TCP buffer size environment variable was not set, then check the MPICH-G2 environment variable to
		    maintain backwards compatability. */
		 env_str = globus_libc_getenv("MPICH_GLOBUS2_TCP_BUFFER_SIZE");
	     }
	     
	     if (env_str != NULL)
	     {
		 size = (int) strtol(env_str, NULL, 0);
		 if (size == 0 && errno == EINVAL)
		 {
		     MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|tcp_buf_size_invalid",
			 "**globus|tcp_buf_size_invalid %s", env_str);
		 }
		 else if (size < 0)
		 {
		     MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|tcp_buf_size_neg",
			 "**globus|tcp_buf_size_neg %s", env_str);
		 }
		 else if (size >= INT_MAX)
		 {
		     MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|tcp_buf_size_overflow",
			 "**globus|tcp_buf_size_overflow %s", env_str);
		 }

		 mpig_cm_xio_tcp_buf_size = (int) size;
	     }
	}

	/* initialize the request completion queue */
        mpi_errno = mpig_cm_xio_rcq_init();
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_init");

	/* initialize the XIO CM progress engine info structure and register the CM with the progress engine */
	mpig_cm_xio_pe_info.active_ops = 0;
#       if defined(MPIG_THREADED)
	{
	    mpig_cm_xio_pe_info.polling_required = FALSE;
	}
#	else
	{
	    mpig_cm_xio_pe_info.polling_required = TRUE;
	}
#	endif
	mpig_pe_register_cm(&mpig_cm_xio_pe_info);

	/* initialize the data structures used to handle blocking probes */
	mpig_cm_xio_probe_init();
	
	/* activate globus XIO module */
	grc = globus_module_activate(GLOBUS_XIO_MODULE);
	MPIU_ERR_CHKANDJUMP1((grc), mpi_errno, MPI_ERR_OTHER, "**globus|module_activate", "**globus|module_activate %s", "XIO");
    }

    mpig_cm_xio_methods_active += 1;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_module_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}

/*
 * <mpi_errno> mpig_cm_xio_module_finalize(void)
 *
 * NOTE: this routine insures that the module data structures are destroyed only once even if multiple methods are shutdown
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_module_finalize
static int mpig_cm_xio_module_finalize(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    globus_result_t grc;
    int mrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_module_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_module_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpig_cm_xio_methods_active -= 1;

    if(mpig_cm_xio_methods_active == 0)
    {
	/* deactivate the globus XIO module */
	grc = globus_module_deactivate(GLOBUS_XIO_MODULE);
	MPIU_ERR_CHKANDSTMT2((grc), mpi_errno, MPI_ERR_OTHER, {;}, "**globus|module_deactivate",
	    "**globus|module_deactivate %s %s", "XIO", globus_error_print_chain(globus_error_peek(grc)));

	/* clean up the data structures used to handle blocking probes */
	mpig_cm_xio_probe_finalize();
	
	/* inform the progress engine tha the XIO CM is no longer active */
	mpig_pe_unregister_cm(&mpig_cm_xio_pe_info);
	
	/* shutdown the vc tracking list */
	mpig_cm_xio_vc_list_finalize();
    
	/* shutdown the request completion queue */
	mrc = mpig_cm_xio_rcq_finalize();
	MPIU_ERR_CHKANDSTMT((mrc), mrc, MPI_ERR_OTHER, {MPIU_ERR_ADD(mpi_errno, mrc);}, "**globus|cm_xio|rcq_finalize");
    
	mpig_cm_xio_mutex_destruct();

	if (mpi_errno) goto fn_fail;
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_module_finalize);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/**********************************************************************************************************************************
					    END COMMUNICATION MODULE CORE API SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
#define mpig_cm_xio_pe_start_op()		\
{						\
    mpig_cm_xio_pe_info.active_ops += 1;	\
    mpig_pe_start_op();				\
}

#define mpig_cm_xio_pe_end_op(req_)												\
{																\
    /* if the request was posted as a receive any source, then do not decrement the XIO active op count since it was never	\
       incremented */														\
    if (mpig_request_get_rank(req_) != MPI_ANY_SOURCE)										\
    {																\
	mpig_cm_xio_pe_info.active_ops -= 1;											\
	mpig_pe_end_op();													\
    }																\
    else															\
    {																\
	mpig_pe_end_ras_op();													\
    }																\
}

/*
 * <mpi_errno> mpig_cm_xio_pe_wait([IN/OUT] state)
 *
 * FIXME: this should be rolled together with the rcq_wait and rcq_test so that multiple requests can be completed in a single
 * call without having to reacquire the RCQ mutex for each request completion.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_wait
int mpig_cm_xio_pe_wait(struct MPID_Progress_state * state)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t blocking;
    MPID_Request * req;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

     if (mpig_pe_cm_has_active_ops(&mpig_cm_xio_pe_info))
     {
	blocking = (mpig_pe_cm_owns_all_active_ops(&mpig_cm_xio_pe_info) && mpig_pe_op_has_completed(state) == FALSE);
	if (blocking)
	{
	    mpi_errno = mpig_cm_xio_rcq_deq_wait(&req);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_wait");
	}
	else
	{
	    mpi_errno = mpig_cm_xio_rcq_deq_test(&req);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");
	}

	while (req != NULL)
	{
	    mpig_cm_xio_pe_end_op(req);
	    mpig_request_complete(req);

	    mpi_errno = mpig_cm_xio_rcq_deq_test(&req);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");
	}
     }
     
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_wait);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_wait() */


/*
 * <mpi_errno> mpig_cm_xio_pe_test(void)
 *
 * FIXME: this should be rolled together with the rcq_test so that multiple requests can be completed in a single call without
 * having to reacquire the RCQ mutex for each request completion.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_pe_test
int mpig_cm_xio_pe_test(void)
{
    static const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * req;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_xio_pe_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_xio_pe_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    globus_poll_nonblocking();
    
    mpi_errno = mpig_cm_xio_rcq_deq_test(&req);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");

    while (req != NULL)
    {
	mpig_cm_xio_pe_end_op(req);
	mpig_request_complete(req);
	
	mpi_errno = mpig_cm_xio_rcq_deq_test(&req);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_xio|rcq_deq_test");
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_xio_pe_test);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    {
	goto fn_return;
    }
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_xio_pe_test() */
/**********************************************************************************************************************************
				     END COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					 BEGIN INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/
#define MPIG_CM_XIO_INCLUDE_DEFINE_FUNCTIONS
#include "mpig_cm_xio_req.i"
#include "mpig_cm_xio_vc.i"
#include "mpig_cm_xio_nets.i"
#include "mpig_cm_xio_conn.i"
#include "mpig_cm_xio_data.i"
#include "mpig_cm_xio_comm.i"
/**********************************************************************************************************************************
					  END INCLUSION OF INTERNAL FUNCTION DEFINTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN MISCELLANEOUS FUNCTIONS
**********************************************************************************************************************************/
/*
 * char * mpig_cm_xio_msg_type_get_string([IN/MOD] req)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_xio_msg_type_get_string
static const char * mpig_cm_xio_msg_type_get_string(mpig_cm_xio_msg_type_t msg_type)
{
    const char * str;
    
    switch(msg_type)
    {
	case MPIG_CM_XIO_MSG_TYPE_FIRST:
	    str ="MPIG_CM_XIO_MSG_TYPE_FIRST";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_UNDEFINED:
	    str ="MPIG_CM_XIO_MSG_TYPE_UNDEFINED";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_EAGER_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_EAGER_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_RTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_RTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_CTS:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_CTS";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_RNDV_DATA:
	    str ="MPIG_CM_XIO_MSG_TYPE_RNDV_DATA";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_SSEND_ACK:
	    str ="MPIG_CM_XIO_MSG_TYPE_SSEND_ACK";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_CANCEL_SEND_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_REQ:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_PROC_REQ";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_PORT_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_OPEN_ERROR_RESP:
	    str ="MPIG_CM_XIO_MSG_TYPE_OPEN_RESP";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_CLOSE_PROC:
	    str ="MPIG_CM_XIO_MSG_TYPE_CLOSE_PROC";
	    break;
	case MPIG_CM_XIO_MSG_TYPE_LAST:
	    str ="MPIG_CM_XIO_MSG_TYPE_LAST";
	    break;
	default:
	    str = "(unrecognized message type)";
	    break;
    }

    return str;
}
/* mpig_cm_xio_msg_type_get_string() */
/**********************************************************************************************************************************
					       END MISCELLANEOUS HELPER FUNCTIONS
**********************************************************************************************************************************/

#endif /* defined(HAVE_GLOBUS_XIO_MODULE) */

