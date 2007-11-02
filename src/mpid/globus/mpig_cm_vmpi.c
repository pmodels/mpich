/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

#if defined(MPIG_VMPI)
/**********************************************************************************************************************************
							BEGIN PARAMETERS
**********************************************************************************************************************************/
#if !defined(MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE)
#define MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE 32
#endif
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
mpig_thread_t mpig_cm_vmpi_main_thread;
MPIG_STATIC mpig_uuid_t mpig_cm_vmpi_job_id;
MPIG_STATIC mpig_vmpi_comm_t mpig_cm_vmpi_bridge_vcomm;
MPIG_STATIC mpig_vc_t * mpig_cm_vmpi_ras_vc = NULL;

#if defined(TYPEOF_VMPI_DATATYPE_IS_BASIC)
#define MPIG_VMPI_DATATYPE_FMT FMT_VMPI_DATATYPE
#define MPIG_VMPI_DATATYPE_CAST(handle_) ((TYPEOF_VMPI_DATATYPE) (handle_))
#else
#define MPIG_VMPI_DATATYPE_FMT "%s"
#define MPIG_VMPI_DATATYPE_CAST(handle_) "N/A"
#endif

#if defined(TYPEOF_VMPI_COMM_IS_BASIC)
#define MPIG_VMPI_COMM_FMT FMT_VMPI_COMM
#define MPIG_VMPI_COMM_CAST(handle_) ((TYPEOF_VMPI_COMM) (handle_))
#else
#define MPIG_VMPI_COMM_FMT "%s"
#define MPIG_VMPI_COMM_CAST(handle_) "N/A"
#endif

#if defined(TYPEOF_VMPI_REQUEST_IS_BASIC)
#define MPIG_VMPI_REQUEST_FMT FMT_VMPI_REQUEST
#define MPIG_VMPI_REQUEST_CAST(handle_) ((TYPEOF_VMPI_REQUEST) (handle_))
#else
#define MPIG_VMPI_REQUEST_FMT "%s"
#define MPIG_VMPI_REQUEST_CAST(handle_) "N/A"
#endif

#if defined(TYPEOF_VMPI_OP_IS_BASIC)
#define MPIG_VMPI_OP_FMT FMT_VMPI_OP
#define MPIG_VMPI_OP_CAST(handle_) ((TYPEOF_VMPI_OP) (handle_))
#else
#define MPIG_VMPI_OP_FMT "%s"
#define MPIG_VMPI_OP_CAST(handle_) "N/A"
#endif
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
#endif /* defined(MPIG_VMPI) */


/**********************************************************************************************************************************
					      BEGIN COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_vmpi_init(mpig_cm_t * cm, int * argc, char *** argv);

static int mpig_cm_vmpi_finalize(mpig_cm_t * cm);

static int mpig_cm_vmpi_add_contact_info(mpig_cm_t * cm, mpig_bc_t * bc);

static int mpig_cm_vmpi_construct_vc_contact_info(mpig_cm_t * cm, mpig_vc_t * vc);

static int mpig_cm_vmpi_select_comm_method(mpig_cm_t * cm, mpig_vc_t * vc, bool_t * selected);

static int mpig_cm_vmpi_get_vc_compatability(mpig_cm_t * cm, const mpig_vc_t * vc1, const mpig_vc_t * vc2,
    unsigned levels_in, unsigned * levels_out);


static mpig_cm_vtable_t mpig_cm_vmpi_vtable =
{
    mpig_cm_vmpi_init,
    mpig_cm_vmpi_finalize,
    mpig_cm_vmpi_add_contact_info,
    mpig_cm_vmpi_construct_vc_contact_info,
    NULL, /*mpig_cm_vmpi_destruct_vc_contact_info */
    mpig_cm_vmpi_select_comm_method,
    mpig_cm_vmpi_get_vc_compatability,
    mpig_cm_vtable_last_entry
};
    
mpig_cm_t mpig_cm_vmpi =
{
    MPIG_CM_TYPE_VMPI,
    "VMPI",
    &mpig_cm_vmpi_vtable
};
/**********************************************************************************************************************************
					       END COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/


#if defined(MPIG_VMPI)
/**********************************************************************************************************************************
						    BEGIN VC CORE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_vmpi_adi3_send(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_rsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_ssend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_vmpi_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

static int mpig_cm_vmpi_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,int ctxoff, MPID_Request ** rreqp);

static int mpig_cm_vmpi_adi3_probe(int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status);

static int mpig_cm_vmpi_adi3_iprobe(int rank, int tag, MPID_Comm * comm, int ctxoff, int * flag_p, MPI_Status * status);

static int mpig_cm_vmpi_adi3_cancel_send(MPID_Request * sreq);

static int mpig_cm_vmpi_adi3_cancel_recv(MPID_Request * rreq);


/* VC communication module function table */
MPIG_STATIC mpig_vc_vtable_t mpig_cm_vmpi_vc_vtable =
{
    mpig_cm_vmpi_adi3_send,
    mpig_cm_vmpi_adi3_isend,
    mpig_cm_vmpi_adi3_rsend,
    mpig_cm_vmpi_adi3_irsend,
    mpig_cm_vmpi_adi3_ssend,
    mpig_cm_vmpi_adi3_issend,
    mpig_cm_vmpi_adi3_recv,
    mpig_cm_vmpi_adi3_irecv,
    mpig_cm_vmpi_adi3_probe,
    mpig_cm_vmpi_adi3_iprobe,
    mpig_cm_vmpi_adi3_cancel_recv,
    mpig_cm_vmpi_adi3_cancel_send,
    NULL, /* mpig_cm_vmpi_vc_recv_any_source */
    NULL, /* vc_inc_ref_count */
    NULL, /* vc_dec_ref_count */
    NULL, /* vc_destruct */
    mpig_vc_vtable_last_entry
};

/* VC communication module function table for the special receive-any-source VC */
MPIG_STATIC mpig_vc_vtable_t mpig_cm_vmpi_ras_vc_vtable =
{
    NULL, /* mpig_cm_vmpi_adi3_send */
    NULL, /* mpig_cm_vmpi_adi3_isend */
    NULL, /* mpig_cm_vmpi_adi3_rsend */
    NULL, /* mpig_cm_vmpi_adi3_irsend */
    NULL, /* mpig_cm_vmpi_adi3_ssend */
    NULL, /* mpig_cm_vmpi_adi3_issend */
    NULL, /* mpig_cm_vmpi_adi3_recv */
    NULL, /* mpig_cm_vmpi_adi3_irecv */
    NULL, /* mpig_cm_vmpi_adi3_probe */
    NULL, /* mpig_cm_vmpi_adi3_iprobe */
    mpig_cm_vmpi_adi3_cancel_recv,
    NULL, /* mpig_cm_vmpi_adi3_cancel_send */
    NULL, /* mpig_cm_vmpi_vc_recv_any_source */
    NULL, /* vc_inc_ref_count */
    NULL, /* vc_dec_ref_count */
    NULL, /* vc_destruct */
    mpig_vc_vtable_last_entry
};
/**********************************************************************************************************************************
						    END VC CORE API VTABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				     BEGIN COMMUNICATION MODULE PROGRESS ENGINE DECLARATIONS
**********************************************************************************************************************************/
MPIG_STATIC mpig_vmpi_request_t * mpig_cm_vmpi_pe_table_vreqs = NULL;
MPIG_STATIC MPID_Request ** mpig_cm_vmpi_pe_table_mreqs = NULL;
MPIG_STATIC int * mpig_cm_vmpi_pe_table_vindices = NULL;
MPIG_STATIC mpig_vmpi_status_t * mpig_cm_vmpi_pe_table_vstatuses = NULL;
MPIG_STATIC int mpig_cm_vmpi_pe_table_size = 0;

MPIG_STATIC mpig_pe_info_t mpig_cm_vmpi_pe_info;

MPIG_STATIC int mpig_cm_vmpi_pe_ras_active_count = 0;

static int mpig_cm_vmpi_pe_init(void);

static int mpig_cm_vmpi_pe_finalize(void);

static int mpig_cm_vmpi_pe_table_inc_size(void);
/**********************************************************************************************************************************
				      END COMMUNICATION MODULE PROGRESS ENGINE DECLARATIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN UTILITY AND ACCESSOR MACROS
**********************************************************************************************************************************/
#define mpig_cm_vmpi_comm_construct(comm_)										\
{															\
    int mpig_cm_vmpi_comm_construct_ctxoff__;										\
															\
    for (mpig_cm_vmpi_comm_construct_ctxoff__ = 0; mpig_cm_vmpi_comm_construct_ctxoff__ < MPIG_COMM_NUM_CONTEXTS;	\
	 mpig_cm_vmpi_comm_construct_ctxoff__++)									\
    {															\
	mpig_cm_vmpi_comm_set_vcomm((comm_), mpig_cm_vmpi_comm_construct_ctxoff__, MPIG_VMPI_COMM_NULL);		\
    }															\
    mpig_cm_vmpi_comm_set_remote_vsize((comm_), 0);									\
    (comm_)->cms.vmpi.remote_ranks_mtov = NULL;										\
    (comm_)->cms.vmpi.remote_ranks_vtom = NULL;										\
}

#define mpig_cm_vmpi_comm_destruct(comm_)											 \
{																 \
    int mpig_cm_vmpi_comm_destruct_ctxoff__;											 \
																 \
    for (mpig_cm_vmpi_comm_destruct_ctxoff__ = 0; mpig_cm_vmpi_comm_destruct_ctxoff__ < MPIG_COMM_NUM_CONTEXTS;			 \
	 mpig_cm_vmpi_comm_destruct_ctxoff__++)											 \
    {																 \
	/* MPIU_Assert(mpig_vmpi_comm_is_null(mpig_cm_vmpi_comm_get_vcomm((comm_), mpig_cm_vmpi_comm_destruct_ctxoff__)));					 \
	 *															 \
	 * the above may not be true if the application did not free or disconnect the communicator before finalize.  since	 \
	 * MPI_Comm_free() is a collective operation, we cannot free the communicators here because the ordering of communicator \
	 * destruction is not known, and unless the communicators are freed in the same order by all processes, deadlock could	 \
	 * occur.  therefore, the best we can do is let the vendor MPI clean up the communicators during finalize.		 \
	 */															 \
	mpig_cm_vmpi_comm_set_vcomm((comm_), mpig_cm_vmpi_comm_destruct_ctxoff__, MPIG_VMPI_COMM_NULL);				 \
    }																 \
    mpig_cm_vmpi_comm_set_remote_vsize((comm_), -1);										 \
    MPIU_Free((comm_)->cms.vmpi.remote_ranks_mtov);										 \
    (comm_)->cms.vmpi.remote_ranks_mtov = MPIG_INVALID_PTR;									 \
    MPIU_Free((comm_)->cms.vmpi.remote_ranks_vtom);										 \
    (comm_)->cms.vmpi.remote_ranks_vtom = MPIG_INVALID_PTR;									 \
}

#define mpig_cm_vmpi_comm_set_remote_vrank(comm_, mrank_, vrank_)	\
{									\
    (comm_)->cms.vmpi.remote_ranks_mtov[mrank_] = (vrank_);		\
}

#define mpig_cm_vmpi_comm_get_remote_vrank(comm_, mrank_) ((comm_)->cms.vmpi.remote_ranks_mtov[mrank_])

#define mpig_cm_vmpi_comm_get_remote_send_vrank(comm_, mrank_)				\
    (((mrank_) >= 0) ? ((comm_)->cms.vmpi.remote_ranks_mtov[mrank_]) :			\
	(((mrank_) == MPI_PROC_NULL) ? MPIG_VMPI_PROC_NULL : MPIG_VMPI_UNDEFINED))

#define mpig_cm_vmpi_comm_get_remote_recv_vrank(comm_, mrank_)					\
    (((mrank_) >= 0) ? ((comm_)->cms.vmpi.remote_ranks_mtov[mrank_]) :				\
	(((mrank_) == MPI_PROC_NULL) ? MPIG_VMPI_PROC_NULL :					\
	    (((mrank_) == MPI_ANY_SOURCE) ? MPIG_VMPI_ANY_SOURCE : MPIG_VMPI_UNDEFINED)))

#define mpig_cm_vmpi_comm_set_remote_mrank(comm_, vrank_, mrank_)	\
{									\
    (comm_)->cms.vmpi.remote_ranks_vtom[vrank_] = (mrank_);		\
}

#define mpig_cm_vmpi_comm_get_remote_mrank(comm_, vrank_) ((comm_)->cms.vmpi.remote_ranks_vtom[vrank_])

#define mpig_cm_vmpi_comm_set_remote_vsize(comm_, size_)	\
{								\
    (comm_)->cms.vmpi.remote_size = (size_);			\
}

#define mpig_cm_vmpi_comm_get_remote_vsize(comm_) ((comm_)->cms.vmpi.remote_size)

#define mpig_cm_vmpi_comm_set_vcomm(comm_, ctxoff_, vcomm_)	\
{								\
    MPIU_Assert((ctxoff_) < MPIG_COMM_NUM_CONTEXTS);		\
    (comm_)->cms.vmpi.comms[ctxoff_] = (vcomm_);		\
}

#define mpig_cm_vmpi_comm_get_vcomm(comm_, ctxoff_) ((comm_)->cms.vmpi.comms[ctxoff_])

#define mpig_cm_vmpi_comm_get_vcomm_ptr(comm_, ctxoff_) (&(comm_)->cms.vmpi.comms[ctxoff_])


#define mpig_cm_vmpi_request_construct(req_)		\
{							\
    (req_)->cms.vmpi.req = MPIG_VMPI_REQUEST_NULL;	\
    (req_)->cms.vmpi.pe_table_index = -1;		\
    (req_)->cms.vmpi.cancelling = FALSE;		\
}

#define mpig_cm_vmpi_request_destruct(req_)		\
{							\
    (req_)->cms.vmpi.req = MPIG_VMPI_REQUEST_NULL;	\
    (req_)->cms.vmpi.pe_table_index = -1;		\
    (req_)->cms.vmpi.cancelling = FALSE;		\
}

#define mpig_cm_vmpi_request_set_vreq(req_, vreq_)	\
{							\
    (req_)->cms.vmpi.req = (vreq_);			\
}

#define mpig_cm_vmpi_request_get_vreq(req_) ((req_)->cms.vmpi.req)

#define mpig_cm_vmpi_request_get_vreq_ptr(req_) (&(req_)->cms.vmpi.req)


#define mpig_cm_vmpi_datatype_set_vdt(dt_, vdt_)			\
{									\
    MPID_Datatype * mpig_cm_vmpi_datatype_set_vdt_dtp__;		\
									\
    MPID_Datatype_get_ptr((dt_), mpig_cm_vmpi_datatype_set_vdt_dtp__);	\
    mpig_cm_vmpi_datatype_set_vdt_dtp__->cms.vmpi.dt = (vdt_);		\
}

#define mpig_cm_vmpi_datatype_get_vdt(dt_, vdt_p_)	\
{							\
    if ((dt_) != MPI_DATATYPE_NULL)			\
    {							\
	MPID_Datatype * mpig_cm_vmpi_datatype_get_vdt_dtp__;				\
	MPID_Datatype_get_ptr((dt_), mpig_cm_vmpi_datatype_get_vdt_dtp__);		\
	*(vdt_p_) = mpig_cm_vmpi_datatype_get_vdt_dtp__->cms.vmpi.dt;			\
    }							\
    else						\
    {							\
	*(vdt_p_) = MPIG_VMPI_DATATYPE_NULL;		\
    }							\
}

#define mpig_cm_vmpi_dtp_get_vdt(dtp_) (dtp_->cms.vmpi.dt)

#define mpig_cm_vmpi_dtp_get_vdt_ptr(dtp_) (&dtp_->cms.vmpi.dt)


#define mpig_cm_vmpi_tag_get_vtag(tag_) \
    (((tag_) >= 0) ? (tag_) : (((tag_) == MPI_ANY_TAG) ? MPIG_VMPI_ANY_TAG : MPIG_VMPI_UNDEFINED))
/**********************************************************************************************************************************
						BEGIN UTILITY AND ACCESSOR MACROS
**********************************************************************************************************************************/
#endif /* defined(MPIG_VMPI) */


/**********************************************************************************************************************************
					    BEGIN COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_vmpi_initialize([IN/OUT] argc, [IN/OUT] argv)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_init
static int mpig_cm_vmpi_init(mpig_cm_t * const cm, int * const argc, char *** const argv)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	int vrc;
	int mpi_errno = MPI_SUCCESS;
	MPIU_CHKPMEM_DECL(1);
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_init);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_init);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

	/* get the id of the thread that calls the vendor MPI_Init() routine */
	mpig_cm_vmpi_main_thread = mpig_thread_self();
	
	/* iniitialize the vendor MPI module.  as part of the initialization process, mpig_vmpi_init() sets the
	   MPIG_VMPI_COMM_WORLD error handler to MPI_ERRORS_RETURN. */
        vrc = mpig_vmpi_init(argc, argv);
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Init", &mpi_errno, {goto error_init_failed;});

	/* get the size of the vendor MPI_COMM_WORLD and the rank of this process within it */
	vrc = mpig_vmpi_comm_size(MPIG_VMPI_COMM_WORLD, &mpig_process.cms.vmpi.cw_size);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_size", &mpi_errno);
	
	vrc = mpig_vmpi_comm_rank(MPIG_VMPI_COMM_WORLD, &mpig_process.cms.vmpi.cw_rank);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_rank", &mpi_errno);

	/*
	 * create a unique id that will allow processes to determine if they are part of the same vendor MPI job (or globus
	 * subjob if you prefer).  process 0 creates the id and broadcasts it to the other processes in the vendor MPI job.
	 */
	if (mpig_process.cms.vmpi.cw_rank == 0)
	{
	    int job_id_len;
	    char job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
	    
	    mpi_errno = mpig_uuid_generate(&mpig_cm_vmpi_job_id);
	    if (mpi_errno == MPI_SUCCESS)
	    {
                mpig_uuid_unparse(&mpig_cm_vmpi_job_id, job_id_str);
		job_id_len = strlen(job_id_str) + 1;
	    }
	    else
	    {
		job_id_len = 0;
	    }

	    vrc = mpig_vmpi_bcast(&job_id_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    MPIU_ERR_CHKANDJUMP((job_id_len == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_generate");
	    
	    vrc = mpig_vmpi_bcast(job_id_str, job_id_len, MPIG_VMPI_CHAR, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	}
	else
	{
	    int job_id_len;
	    char job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
	    
	    vrc = mpig_vmpi_bcast(&job_id_len, 1, MPIG_VMPI_INT, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);
	    MPIU_ERR_CHKANDJUMP((job_id_len == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_create");

	    vrc = mpig_vmpi_bcast(job_id_str, job_id_len, MPIG_VMPI_CHAR, 0, MPIG_VMPI_COMM_WORLD);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Bcast", &mpi_errno);

	    mpi_errno = mpig_uuid_parse(job_id_str, &mpig_cm_vmpi_job_id);
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_parse");
	}

	/*
	 * set up a special VC for handling receive-any-source requests
	 */
	MPIU_CHKPMEM_MALLOC(mpig_cm_vmpi_ras_vc, mpig_vc_t *, sizeof(mpig_vc_t), mpi_errno, "VC for receive-any-source requests");
	mpig_vc_construct(mpig_cm_vmpi_ras_vc);
	mpig_vc_set_cm(mpig_cm_vmpi_ras_vc, &mpig_cm_vmpi);
	mpig_vc_set_vtable(mpig_cm_vmpi_ras_vc, &mpig_cm_vmpi_ras_vc_vtable);
	mpig_uuid_copy(&mpig_cm_vmpi_job_id, &mpig_cm_vmpi_ras_vc->cms.vmpi.job_id);
	mpig_cm_vmpi_ras_vc->cms.vmpi.cw_rank = MPIG_VMPI_ANY_SOURCE;

	/*
	 * create a mapping from the MPICH2 intrinsic datatypes to the vendor MPI intrinsic datatypes
	 */
	/* c basic datatypes */
	mpig_cm_vmpi_datatype_set_vdt(MPI_BYTE, MPIG_VMPI_BYTE);
	mpig_cm_vmpi_datatype_set_vdt(MPI_CHAR, MPIG_VMPI_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_SIGNED_CHAR, MPIG_VMPI_SIGNED_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_CHAR, MPIG_VMPI_UNSIGNED_CHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_WCHAR, MPIG_VMPI_WCHAR);
	mpig_cm_vmpi_datatype_set_vdt(MPI_SHORT, MPIG_VMPI_SHORT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_SHORT, MPIG_VMPI_UNSIGNED_SHORT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_INT, MPIG_VMPI_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED, MPIG_VMPI_UNSIGNED);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG, MPIG_VMPI_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_LONG, MPIG_VMPI_UNSIGNED_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_LONG, MPIG_VMPI_LONG_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_LONG_INT, MPIG_VMPI_LONG_LONG_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UNSIGNED_LONG_LONG, MPIG_VMPI_UNSIGNED_LONG_LONG);
	mpig_cm_vmpi_datatype_set_vdt(MPI_FLOAT, MPIG_VMPI_FLOAT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE, MPIG_VMPI_DOUBLE);
	if (MPI_LONG_DOUBLE != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_DOUBLE, MPIG_VMPI_LONG_DOUBLE);
	}
	/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
	mpig_cm_vmpi_datatype_set_vdt(MPI_SHORT_INT, MPIG_VMPI_SHORT_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_2INT, MPIG_VMPI_2INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_INT, MPIG_VMPI_LONG_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_FLOAT_INT, MPIG_VMPI_FLOAT_INT);
	mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_INT, MPIG_VMPI_DOUBLE_INT);
	if (MPI_LONG_DOUBLE_INT != MPI_DATATYPE_NULL)
	{
	    mpig_cm_vmpi_datatype_set_vdt(MPI_LONG_DOUBLE_INT, MPIG_VMPI_LONG_DOUBLE_INT);
	}
#if	defined(HAVE_FORTRAN_BINDING)
	{
	    /* fortran basic datatypes */
	    mpig_cm_vmpi_datatype_set_vdt(MPI_LOGICAL, MPIG_VMPI_LOGICAL);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_CHARACTER, MPIG_VMPI_CHARACTER);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER, MPIG_VMPI_INTEGER);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_REAL, MPIG_VMPI_REAL);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_PRECISION, MPIG_VMPI_DOUBLE_PRECISION);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX, MPIG_VMPI_COMPLEX);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_DOUBLE_COMPLEX, MPIG_VMPI_DOUBLE_COMPLEX);
	    /* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
	    mpig_cm_vmpi_datatype_set_vdt(MPI_2INTEGER, MPIG_VMPI_2INTEGER);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_2COMPLEX, MPIG_VMPI_2COMPLEX);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_2REAL, MPIG_VMPI_2REAL);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_2DOUBLE_COMPLEX, MPIG_VMPI_2DOUBLE_COMPLEX);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_2DOUBLE_PRECISION, MPIG_VMPI_2DOUBLE_PRECISION);
	    /* fortran size specific datatypes */
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER1, MPIG_VMPI_INTEGER1);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER2, MPIG_VMPI_INTEGER2);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER4, MPIG_VMPI_INTEGER4);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER8, MPIG_VMPI_INTEGER8);
	    if (MPI_INTEGER16 != MPI_DATATYPE_NULL)
	    {
		mpig_cm_vmpi_datatype_set_vdt(MPI_INTEGER16, MPIG_VMPI_INTEGER16);
	    }
	    mpig_cm_vmpi_datatype_set_vdt(MPI_REAL4, MPIG_VMPI_REAL4);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_REAL8, MPIG_VMPI_REAL8);
	    if (MPI_REAL16 != MPI_DATATYPE_NULL)
	    {
		mpig_cm_vmpi_datatype_set_vdt(MPI_REAL16, MPIG_VMPI_REAL16);
	    }
	    mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX8, MPIG_VMPI_COMPLEX8);
	    mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX16, MPIG_VMPI_COMPLEX16);
	    if (MPI_COMPLEX32 != MPI_DATATYPE_NULL)
	    {
		mpig_cm_vmpi_datatype_set_vdt(MPI_COMPLEX32, MPIG_VMPI_COMPLEX32);
	    }
	}
#	endif
	/* type representing a packed application buffer */
	mpig_cm_vmpi_datatype_set_vdt(MPI_PACKED, MPIG_VMPI_PACKED);
	/* pseudo datatypes used to manipulate the extent */
	mpig_cm_vmpi_datatype_set_vdt(MPI_LB, MPIG_VMPI_LB);
	mpig_cm_vmpi_datatype_set_vdt(MPI_UB, MPIG_VMPI_UB);

	/*
	 * set MPICH2's upper bound to be no larger than the upper bound reported by the vendor MPI
	 */
	{
	    int * tag_ub;
	    int exists;
	    
	    vrc = mpig_vmpi_comm_get_attr(MPIG_VMPI_COMM_WORLD, MPIG_VMPI_TAG_UB, &tag_ub, &exists);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_get_attr", &mpi_errno);
	    
	    if (exists && *tag_ub < MPIR_Process.attrs.tag_ub)
	    {
		MPIR_Process.attrs.tag_ub = *tag_ub;
	    }
	}
	
	/*
	 * create a bridge communicator to be used when new intercommunicators are created
	 */
	vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, &mpig_cm_vmpi_bridge_vcomm);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	
	/*
	 * intialize the VMPI communication module progress engine
	 */
	mpi_errno = mpig_cm_vmpi_pe_init();
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_pe_init");
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_init);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIU_CHKPMEM_REAP();

	    vrc = mpig_vmpi_finalize();
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Finalize", &mpi_errno);

	  error_init_failed:
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#   endif
}
/* mpig_cm_vmpi_init() */


/*
 * <mpi_errno> mpig_cm_vmpi_finalize(void)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_finalize
static int mpig_cm_vmpi_finalize(mpig_cm_t * const cm)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	int vrc;
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_finalize);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_finalize);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

	/* XXX: wait for outstanding request to complete?  free them? */

	/*
	 * shutdown the VMPI communication module progress engine
	 */
	mpi_errno = mpig_cm_vmpi_pe_finalize();
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_pe_finalize");

	/* the MPICH2 predefined communicators are destroyed prior to the calling of the CM finalize routines.  the free and
	 * destruct hooks take care of releasing any resources used by this module that are associated with those communicators,
	 * so it is unneccessary to do anything at this time.
	 */
	
	/*
	 * release the bridge communicator used when new intercommunicators are created
	 */
	vrc = mpig_vmpi_comm_free(&mpig_cm_vmpi_bridge_vcomm);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_free", &mpi_errno);

	/*
	 * shutdown the vendor MPI module
	 */
	vrc = mpig_vmpi_finalize();
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Finalize", &mpi_errno);

      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_finalize);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#endif
}
/* mpig_cm_vmpi_finalize() */


/*
 * <mpi_errno> mpig_cm_vmpi_add_contact_info([IN/MOD] bc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_add_contact_info
static int mpig_cm_vmpi_add_contact_info(mpig_cm_t * const cm, mpig_bc_t * const bc)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	char uint_str[10];
        char job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_add_contact_info);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_add_contact_info);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

        mpig_uuid_unparse(&mpig_cm_vmpi_job_id, job_id_str);
	mpi_errno = mpig_bc_add_contact(bc, "CM_VMPI_UUID", job_id_str);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_VMPI_UUID");
	
	MPIU_Snprintf(uint_str, 10, "%u", (unsigned) mpig_process.cms.vmpi.cw_rank);
	mpi_errno = mpig_bc_add_contact(bc, "CM_VMPI_CW_RANK", uint_str);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	    "**globus|bc_add_contact %s", "CM_VMPI_CW_RANK");
	
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_add_contact_info);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	/* ...nothing to do... */
	return MPI_SUCCESS;
    }
#   endif
}
/* mpig_cm_vmpi_add_contact_info() */


/*
 * <mpi_errno> mpig_cm_vmpi_construct_vc_contact_info([IN/MOD] vc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_construct_vc_contact_info
static int mpig_cm_vmpi_construct_vc_contact_info(mpig_cm_t * cm, mpig_vc_t * const vc)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	mpig_bc_t * bc;
	char * uuid_str = NULL;
	char * cw_rank_str = NULL;
	int cw_rank;
	bool_t found;
	static bool_t predefined_comms_initialized = FALSE;
	int rc;
	int mpi_errno = MPI_SUCCESS;
	MPIU_CHKPMEM_DECL(4);
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_construct_vc_contact_info);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_construct_vc_contact_info);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

	/* initialize the VMPI data in the VC.  the job id is given a temporary id that will never match a real id */
	vc->cms.vmpi.cw_rank = -1;
	mpig_uuid_nullify(&vc->cms.vmpi.job_id);

	bc = mpig_vc_get_bc(vc);

	/* extract the subjob id from the business card */
	mpi_errno = mpig_bc_get_contact(bc, "CM_VMPI_UUID", &uuid_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_VMPI_UUID");
	if (!found) goto fn_return;
    
	/* extract the rank of the process in _its_ MPI_COMM_WORLD from the business card */
	mpi_errno = mpig_bc_get_contact(bc, "CM_VMPI_CW_RANK", &cw_rank_str, &found);
	MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	    "**globus|bc_get_contact %s", "CM_VMPI_CW_RANK");
	if (!found) goto fn_return;
	
	rc = sscanf(cw_rank_str, "%d", &cw_rank);
	MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

	/* if all when well, copy the extracted contact information into the VC */
	mpi_errno = mpig_uuid_parse(uuid_str, &vc->cms.vmpi.job_id);
	MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|uuid_import");
	vc->cms.vmpi.cw_rank = cw_rank;

	/*
	 * set the topology information
	 *
         * NOTE: this may seem a bit wacky since the VMPI level is set even if the VMPI module is not responsible for the VC;
         * however, the topology information is defined such that a level set if it is _possible_ for the module to perform the
         * communication regardless of whether it does so or not.
	 */
	vc->cms.topology_levels |= MPIG_TOPOLOGY_LEVEL_VMPI_MASK;
	if (vc->cms.topology_num_levels <= MPIG_TOPOLOGY_LEVEL_VMPI)
	{
	    vc->cms.topology_num_levels = MPIG_TOPOLOGY_LEVEL_VMPI + 1;
	}

	/*
	 * initialize the VMPI specific fields that are part of the MPI_COMM_WORLD and MPI_COMM_SELF objects
	 *
	 * NOTE: the initialization of the predefined communicators is done here rather than in mpig_cm_vmpi_init() because the
	 * size of the enitre process group is not known until this point.
	 */
	if (!predefined_comms_initialized)
	{
	    MPID_Comm * comm;
	    int rank;
	    int ctxoff;
	    int vrc;
	    
	    /* initialize the vendor MPI specific fields in the MPI_COMM_WORLD object.  then create a new vendor communicator for
	       each context in the MPICH2 communicator.  finally, allocate and initialize the mapping arrays for converting
	       MPICH2 ranks to vendor ranks and vice versa.  these mappings will be defined during the module selection process.
	       see mpig_cm_vmpi_select_comm_method() for details.  the exception is the mappings for local process, which are set
	       here, because the local process will not be selected by this module but the mappings are needed for the
	       communicator construction hooks to work properly.  NOTE: mpig_process.my_pg_size and mpig_process.my_pg_size are
	       used in place of comm->remote_size and comm->rank because those fields in the communicator aren't defined until
	       the end of MPID_Init(). */
	    comm = MPIR_Process.comm_world;
	    mpig_cm_vmpi_comm_construct(comm);
	    mpig_cm_vmpi_comm_set_remote_vsize(comm, mpig_process.cms.vmpi.cw_size);

	    for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	    {
		vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_WORLD, mpig_cm_vmpi_comm_get_vcomm_ptr(comm, ctxoff));
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	    }

	    MPIU_CHKPMEM_MALLOC(comm->cms.vmpi.remote_ranks_mtov, int *, mpig_process.my_pg_size * sizeof(int), mpi_errno,
		"MPICH2 to VMPI rank translation table for VMPI_COMM_WORLD");
	    MPIU_CHKPMEM_MALLOC(comm->cms.vmpi.remote_ranks_vtom, int *, mpig_process.cms.vmpi.cw_size * sizeof(int), mpi_errno,
		"VMPI to MPICH2 rank translation table for VMPI_COMM_WORLD");

	    for (rank = 0; rank < comm->remote_size; rank++)
	    {
		mpig_cm_vmpi_comm_set_remote_vrank(comm, rank, MPIG_VMPI_UNDEFINED);
	    }

	    for (rank = 0; rank <  mpig_process.cms.vmpi.cw_size; rank++)
	    {
		mpig_cm_vmpi_comm_set_remote_mrank(comm, rank, MPI_UNDEFINED);
	    }

	    mpig_cm_vmpi_comm_set_remote_mrank(comm, mpig_process.cms.vmpi.cw_rank, mpig_process.my_pg_rank);
	    mpig_cm_vmpi_comm_set_remote_vrank(comm, mpig_process.my_pg_rank, mpig_process.cms.vmpi.cw_rank);

	    /* initialize the vendor MPI specific fields in the MPI_COMM_SELF object.  while intraprocess communication is
	       handled by the "self" communication modules, the communicator construction hooks require MPIG_VMPI_COMM_SELF (or a
	       duplicate) if the application uses MPI_COMM_SELF as the original communicator. */
	    comm = MPIR_Process.comm_self;
	    mpig_cm_vmpi_comm_construct(comm);
	    mpig_cm_vmpi_comm_set_remote_vsize(comm, 1);

	    /* create a new vendor communicator for each context in the MPICH2 communicator */
	    for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	    {
		vrc = mpig_vmpi_comm_dup(MPIG_VMPI_COMM_SELF, mpig_cm_vmpi_comm_get_vcomm_ptr(comm, ctxoff));
		MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Comm_dup", &mpi_errno);
	    }
	    
	    MPIU_CHKPMEM_MALLOC(comm->cms.vmpi.remote_ranks_mtov, int *, sizeof(int), mpi_errno,
		"MPICH2 to VMPI rank translation table for VMPI_COMM_SELF");
	    MPIU_CHKPMEM_MALLOC(comm->cms.vmpi.remote_ranks_vtom, int *, sizeof(int), mpi_errno,
		"VMPI to MPICH2 rank translation table for VMPI_COMM_SELF");

	    mpig_cm_vmpi_comm_set_remote_mrank(comm, 0, 0);
	    mpig_cm_vmpi_comm_set_remote_vrank(comm, 0, 0);

	    /* MPIU_CHKPMEM_COMMIT() is implicit */
	    
	    predefined_comms_initialized = TRUE;
	}
	
      fn_return:
	/* free the contact strings returned from mpig_bc_get_contact() */
	if (uuid_str != NULL) mpig_bc_free_contact(uuid_str);
	if (cw_rank_str != NULL) mpig_bc_free_contact(cw_rank_str);
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc),
	    mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_construct_vc_contact_info);
	return mpi_errno;

      fn_fail:
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIU_CHKPMEM_REAP();
	    goto fn_return;
	}   /* --END ERROR HANDLING-- */
    }
#   else
    {
	vc->cms.vmpi.cw_rank = -1;
	return MPI_SUCCESS;
    }
#   endif
}
/* int mpig_cm_vmpi_construct_vc_contact_info() */


/*
 * <mpi_errno> mpig_cm_vmpi_select_comm_method([IN/MOD] vc, [OUT] selected)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_select_comm_method
static int mpig_cm_vmpi_select_comm_method(mpig_cm_t * cm, mpig_vc_t * const vc, bool_t * const selected)
{
#   if defined(MPIG_VMPI)
    {
	const char fcname[] = MPIG_QUOTE(FUNCNAME);
	int mpi_errno = MPI_SUCCESS;
	MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_select_comm_method);

	MPIG_UNUSED_VAR(fcname);
	
	MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_select_comm_method);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

	*selected = FALSE;

	/* if the VC is not in the same subjob, or has already been selected by another module, then return */
	if (mpig_vc_get_cm(vc) != NULL) goto fn_return;
	if (mpig_uuid_compare(&vc->cms.vmpi.job_id, &mpig_cm_vmpi_job_id) == FALSE) goto fn_return;
    
	/* initialize the CM VMPI fields in the VC object */
	mpig_vc_set_cm(vc, cm);
	mpig_vc_set_vtable(vc, &mpig_cm_vmpi_vc_vtable);

	/* set the rank translations in the MPI_COMM_WORLD communicator for the local process */
	mpig_cm_vmpi_comm_set_remote_vrank(MPIR_Process.comm_world, mpig_vc_get_pg_rank(vc), vc->cms.vmpi.cw_rank);
	mpig_cm_vmpi_comm_set_remote_mrank(MPIR_Process.comm_world, vc->cms.vmpi.cw_rank, mpig_vc_get_pg_rank(vc));

	/* set the selected flag to indicate that the "vmpi" communication module has accepted responsibility for the VC */
	*selected = TRUE;
    
      fn_return:
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", selected=%s, mpi_errno=" MPIG_ERRNO_FMT,
	    MPIG_PTR_CAST(vc), MPIG_BOOL_STR(*selected), mpi_errno));
	MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_select_comm_method);
	return mpi_errno;
    }
#   else
    {
	*selected = FALSE;
	return MPI_SUCCESS;
    }
#   endif
}
/* int mpig_cm_vmpi_select_comm_method() */


/*
 * <mpi_errno> mpig_cm_vmpi_get_vc_compatability([IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_get_vc_compatability
static int mpig_cm_vmpi_get_vc_compatability(mpig_cm_t * cm, const mpig_vc_t * const vc1, const mpig_vc_t * const vc2,
    const unsigned levels_in, unsigned * const levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_in=0x%08x",
	MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), levels_in));

    *levels_out = 0;

    if (levels_in & MPIG_TOPOLOGY_LEVEL_VMPI_MASK)
    {
	if (mpig_uuid_compare(&vc1->cms.vmpi.job_id, &vc2->cms.vmpi.job_id))
	{
	    *levels_out |= levels_in & MPIG_TOPOLOGY_LEVEL_VMPI_MASK;
	}
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
	"mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_get_vc_compatability);
    return mpi_errno;
}
/* int mpig_cm_vmpi_get_vc_compatability() */
/**********************************************************************************************************************************
					     END COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/


#if defined(MPIG_VMPI)
/**********************************************************************************************************************************
				    BEGIN COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)
#define mpig_cm_vmpi_pe_table_init_entry(entry_)			\
{									\
    mpig_cm_vmpi_pe_table_vreqs[entry_] = MPIG_VMPI_REQUEST_NULL;	\
    mpig_cm_vmpi_pe_table_mreqs[entry_] = NULL;				\
}
#else
#define mpig_cm_vmpi_pe_table_init_entry(entry_)
#endif

/*
 * <mpi_errno> mpig_cm_vmpi_pe_init(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_init
static int mpig_cm_vmpi_pe_init(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_init);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    
    /* preallocate a set of entries in the progress engine tables for tracking outstanding requests */
    mpi_errno = mpig_cm_vmpi_pe_table_inc_size();
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|vmpi_pe_table_inc_size");

    /* initialize the VMPI CM progress engine info structure and register the CM with the progress engine core */
    mpig_cm_vmpi_pe_info.active_ops = 0;
    mpig_cm_vmpi_pe_info.polling_required = TRUE;
    mpig_pe_register_cm(&mpig_cm_vmpi_pe_info);
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_pe_init() */


/*
 * <mpi_errno> mpig_cm_vmpi_pe_finalize(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_finalize
static int mpig_cm_vmpi_pe_finalize(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));
    
    /* inform the progress engine core tha the VMPI CM is no longer active */
    mpig_pe_unregister_cm(&mpig_cm_vmpi_pe_info);
	
    /* free the memory used by the progress engine tables */
    MPIU_Free(mpig_cm_vmpi_pe_table_vreqs);
    MPIU_Free(mpig_cm_vmpi_pe_table_mreqs);
    MPIU_Free(mpig_cm_vmpi_pe_table_vindices);
    MPIU_Free(mpig_cm_vmpi_pe_table_vstatuses);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_finalize);
    return mpi_errno;
}
/* mpig_cm_vmpi_pe_finalize() */


/*
 * <mpi_errno> mpig_cm_vmpi_pe_table_inc_size(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_table_inc_size
static int mpig_cm_vmpi_pe_table_inc_size(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    void * ptr;
    int entry;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_table_inc_size);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_table_inc_size);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    ptr = MPIU_Realloc(mpig_cm_vmpi_pe_table_vreqs, (mpig_cm_vmpi_pe_table_size + MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE) *
	sizeof(mpig_vmpi_request_t));
    if (ptr == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s", "table of active vendor MPI requests");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    mpig_cm_vmpi_pe_table_vreqs = (mpig_vmpi_request_t *) ptr;
    
    ptr = MPIU_Realloc(mpig_cm_vmpi_pe_table_mreqs, (mpig_cm_vmpi_pe_table_size + MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE) *
	sizeof(MPID_Request *));
    if (ptr == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s",
	    "table of MPICH2 requests associated with the active vendor MPI requests");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    mpig_cm_vmpi_pe_table_mreqs = (MPID_Request **) ptr;
    
    ptr = MPIU_Realloc(mpig_cm_vmpi_pe_table_vindices, (mpig_cm_vmpi_pe_table_size + MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE) *
	sizeof(int));
    if (ptr == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s",
	    "table of indices pointing at completed requests");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    mpig_cm_vmpi_pe_table_vindices = (int *) ptr;
    
    ptr = MPIU_Realloc(mpig_cm_vmpi_pe_table_vstatuses, (mpig_cm_vmpi_pe_table_size + MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE) *
	sizeof(mpig_vmpi_status_t));
    if (ptr == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_ERR_SETFATALANDSTMT1(mpi_errno, MPI_ERR_OTHER, {;}, "**nomem", "**nomem %s",
	    "table of vendor MPI status structures for completed requests");
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    mpig_cm_vmpi_pe_table_vstatuses = (mpig_vmpi_status_t *) ptr;
    
    for (entry = mpig_cm_vmpi_pe_table_size; entry < mpig_cm_vmpi_pe_table_size + MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE; entry++)
    {
	mpig_cm_vmpi_pe_table_init_entry(entry);
    }
    
    mpig_cm_vmpi_pe_table_size += MPIG_CM_VMPI_PE_TABLE_ALLOC_SIZE;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_table_inc_size);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_pe_table_inc_size() */


/* NOTE: THIS IS VENDOR MPI DEPENDENT.  DO NOT USE WITHOUT FIRST CHECKING THE TYPE OF A VENDOR MPI REQUEST. */
#define MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE(label_)										\
{																\
    int i__;															\
    for (i__ = 0; i__ < mpig_cm_vmpi_pe_info.active_ops; i__++)									\
    {																\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS, "VMPI PE table: label=%s, size=%d, entry=%d, mreq=" MPIG_HANDLE_FMT	\
	    ", vreq=" MPIG_VMPI_REQUEST_FMT, (label_), mpig_cm_vmpi_pe_info.active_ops, i__,					\
	    MPIG_HANDLE_VAL(mpig_cm_vmpi_pe_table_mreqs[i__]), MPIG_VMPI_REQUEST_CAST(mpig_cm_vmpi_pe_table_vreqs[i__])))	\
    }																\
}

#define mpig_cm_vmpi_pe_table_add_req(mreq_, mpi_errno_p_)									\
{																\
    *(mpi_errno_p_) = MPI_SUCCESS;												\
																\
    /* if the progress engine tables for tracking outstanding requests are not big enough to add a new request, then increase	\
       their size */														\
    if (mpig_cm_vmpi_pe_info.active_ops == mpig_cm_vmpi_pe_table_size)								\
    {																\
	*(mpi_errno_p_) = mpig_cm_vmpi_pe_table_inc_size();									\
	if (*(mpi_errno_p_)) goto end_macro;											\
    }																\
																\
    /* add the new request to the last end of the table */									\
    MPIU_Assert(mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(mreq_)) == FALSE);					\
    mpig_cm_vmpi_pe_table_vreqs[mpig_cm_vmpi_pe_info.active_ops] = mpig_cm_vmpi_request_get_vreq(mreq_);			\
    mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_info.active_ops] = (mreq_);							\
    (mreq_)->cms.vmpi.pe_table_index = mpig_cm_vmpi_pe_info.active_ops;								\
																\
    mpig_cm_vmpi_pe_info.active_ops += 1;											\
																\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, " request added to the VMPI PE table: index=%d"	\
	", mreq=" MPIG_HANDLE_FMT ", mreqp=" MPIG_PTR_FMT ", vreq=" MPIG_VMPI_REQUEST_FMT ", active_vmpi_ops=%d",		\
	(mreq_)->cms.vmpi.pe_table_index, (mreq_)->handle, MPIG_PTR_CAST(mreq_),						\
	MPIG_VMPI_REQUEST_CAST(mpig_cm_vmpi_request_get_vreq(mreq_)), mpig_cm_vmpi_pe_info.active_ops));			\
																\
    /* notify the progress engine core that a new operation has been started */							\
    mpig_pe_start_op();														\
  end_macro: ;															\
}

#define mpig_cm_vmpi_pe_table_remove_req(mreq_)											\
{																\
    int mpig_cm_vmpi_pe_table_remove_req_table_index__ = (mreq_)->cms.vmpi.pe_table_index;					\
																\
    (mreq_)->cms.vmpi.pe_table_index = -1;											\
																\
    /* move the last request in the table to the location of the request that completed */					\
    mpig_cm_vmpi_pe_info.active_ops -= 1;											\
    if (mpig_cm_vmpi_pe_table_remove_req_table_index__ != mpig_cm_vmpi_pe_info.active_ops)					\
    {																\
	mpig_cm_vmpi_pe_table_vreqs[mpig_cm_vmpi_pe_table_remove_req_table_index__] =						\
	    mpig_cm_vmpi_pe_table_vreqs[mpig_cm_vmpi_pe_info.active_ops];							\
	mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_table_remove_req_table_index__] =						\
	    mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_info.active_ops];							\
	if (mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_table_remove_req_table_index__])					\
	{															\
	    mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_table_remove_req_table_index__]->cms.vmpi.pe_table_index =		\
		mpig_cm_vmpi_pe_table_remove_req_table_index__;									\
	}															\
    }																\
    mpig_cm_vmpi_pe_table_init_entry(mpig_cm_vmpi_pe_info.active_ops);								\
																\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, " request added to the VMPI PE table: index=%d"	\
	", mreq=" MPIG_HANDLE_FMT ", mreqp=" MPIG_PTR_FMT ", vreq=" MPIG_VMPI_REQUEST_FMT ", active_vmpi_ops=%d",		\
	mpig_cm_vmpi_pe_table_remove_req_table_index__, (mreq_)->handle, MPIG_PTR_CAST(mreq_),					\
	MPIG_VMPI_REQUEST_CAST(mpig_cm_vmpi_request_get_vreq(mreq_)), mpig_cm_vmpi_pe_info.active_ops));			\
																\
    /* notify the progress engine core that an operation has completed */							\
    mpig_pe_end_op();														\
}

#define mpig_cm_vmpi_pe_table_remove_entry(pe_table_index_)									\
{																\
    MPIU_Assert(mpig_vmpi_request_is_null(mpig_cm_vmpi_pe_table_vreqs[pe_table_index_]));					\
																\
    /* move the last request in the table to the location of the request that completed */					\
    mpig_cm_vmpi_pe_info.active_ops -= 1;											\
    																\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, " entry removed from the VMPI PE table: index=%d"	\
	", mreq=" MPIG_HANDLE_FMT ", vreq=" MPIG_VMPI_REQUEST_FMT ", active_vmpi_ops=%d", pe_table_index_,			\
	mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_info.active_ops],								\
	MPIG_VMPI_REQUEST_CAST(mpig_cm_vmpi_pe_table_vreqs[mpig_cm_vmpi_pe_info.active_ops]),					\
	mpig_cm_vmpi_pe_info.active_ops));											\
																\
    if (pe_table_index_ != mpig_cm_vmpi_pe_info.active_ops)									\
    {																\
	mpig_cm_vmpi_pe_table_vreqs[pe_table_index_] = mpig_cm_vmpi_pe_table_vreqs[mpig_cm_vmpi_pe_info.active_ops];		\
	mpig_cm_vmpi_pe_table_mreqs[pe_table_index_] = mpig_cm_vmpi_pe_table_mreqs[mpig_cm_vmpi_pe_info.active_ops];		\
	if (mpig_cm_vmpi_pe_table_mreqs[pe_table_index_])									\
	{															\
	    mpig_cm_vmpi_pe_table_mreqs[pe_table_index_]->cms.vmpi.pe_table_index = pe_table_index_;				\
	}															\
    }																\
    mpig_cm_vmpi_pe_table_init_entry(mpig_cm_vmpi_pe_info.active_ops);								\
																\
    /* notify the progress engine core that an operation has completed */							\
    mpig_pe_end_op();														\
}


/* #define mpig_cm_vmpi_request_status_vtom(mreq_, vstatus_, verror_) */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_request_status_vtom
void mpig_cm_vmpi_request_status_vtom(MPID_Request * const mreq, mpig_vmpi_status_t * const vstatus, const int verror);
void mpig_cm_vmpi_request_status_vtom(MPID_Request * const mreq, mpig_vmpi_status_t * const vstatus, const int verror)
{
    char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_request_status_vtom);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_request_status_vtom);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering: mreq=" MPIG_PTR_FMT ", vstatus=" MPIG_PTR_FMT
	", verror=%d", MPIG_PTR_CAST(mreq), MPIG_PTR_CAST(vstatus), verror));

    /* if the operation succeeded then populate the MPICH2 request status structure with information from the vendor status
       structure; otherwise, set the error code in the status structure of the MPICH2 request object */
    if (verror == MPIG_VMPI_SUCCESS)
    {
	if (mreq->cms.vmpi.cancelling)
	{
	    vrc = mpig_vmpi_test_cancelled(vstatus, &mreq->status.cancelled);
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Test_cancelled", &mreq->status.MPI_ERROR);
	    mreq->status.count = 0;
	    /* mreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	}
    }
    else
    {
	int verror_class;
	int verror_str_len;
	char verror_str[MPIG_VMPI_MAX_ERROR_STRING + 1];

	vrc = mpig_vmpi_error_class(verror, &verror_class);
	MPIU_Assert(vrc == MPI_SUCCESS && "ERROR: vendor MPI_Error_class failed");

	vrc = mpig_vmpi_error_string(verror, verror_str, &verror_str_len);
	MPIU_Assert(vrc == MPI_SUCCESS && "ERROR: vendor MPI_Error_string failed");

	MPIU_ERR_SET1(mreq->status.MPI_ERROR, mpig_cm_vmpi_error_class_vtom(verror_class),
	    "**globus|vmpi_req_failed", "**globus|vmpi_req_failed %s", verror_str);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request failed: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", msg=%s",
	    mreq->handle, MPIG_PTR_CAST(mreq), verror_str));
    }
    
    /* if this is a receive request, then set the source, tag and number of received bytes in the status structure of the
       MPICH2 request object */
    if (mreq->status.cancelled == FALSE && mreq->kind == MPID_REQUEST_RECV)
    {
	int vstatus_source = mpig_vmpi_status_get_source(vstatus);
	int mrank = (vstatus_source >= 0) ? mpig_cm_vmpi_comm_get_remote_mrank(mreq->comm, vstatus_source) : MPI_UNDEFINED;
	int vstatus_tag = mpig_vmpi_status_get_tag(vstatus);
	int mtag = (vstatus_tag >= 0) ? vstatus_tag : MPI_UNDEFINED;
	int count;
	
	MPIG_Assert(vstatus_source < mpig_cm_vmpi_comm_get_remote_vsize(mreq->comm));
	
	vrc = mpig_vmpi_get_count(vstatus, MPIG_VMPI_BYTE, &count);
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Get_count", &mreq->status.MPI_ERROR);
	
	mreq->status.MPI_SOURCE = mrank;
	mreq->status.MPI_TAG = mtag;
	mreq->status.count = count;
	/* mreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "nonblocking recv complete: vrank=%d, mrank=%d"
	    ", vtag=%d, mtag=%d, count=%d", vstatus_source, mrank, vstatus_tag, mtag, count));
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mreq=" MPIG_PTR_FMT, MPIG_PTR_CAST(mreq)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_request_status_vtom);
    return;
}


#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_complete_reqs
static void mpig_cm_vmpi_pe_complete_reqs(const int num_reqs_completed, const bool_t errors);
static void mpig_cm_vmpi_pe_complete_reqs(const int num_reqs_completed, const bool_t errors)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int n;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_complete_reqs);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_complete_reqs);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering: num_reqs=%d, errors=%s", num_reqs_completed,
	MPIG_BOOL_STR(errors)));

#   if defined(MPIG_DEBUG)
    {
	for (n = 0; n < num_reqs_completed; n++)
	{
	    const int i = mpig_cm_vmpi_pe_table_vindices[n];
	    MPID_Request * mreq = (i >= 0 && i < mpig_cm_vmpi_pe_info.active_ops) ? mpig_cm_vmpi_pe_table_mreqs[i] : NULL;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS, "VMPI COMPLETION TABLE: entry=%d, index=%d, mreq=" MPIG_HANDLE_FMT
		", mreqp=" MPIG_PTR_FMT, n, mpig_cm_vmpi_pe_table_vindices[n], MPIG_HANDLE_VAL(mreq), MPIG_PTR_CAST(mreq)));
	}
    }
#   endif
    
    for (n = 0; n < num_reqs_completed; n++)
    {
	const int i = mpig_cm_vmpi_pe_table_vindices[n];
	MPID_Request * mreq = mpig_cm_vmpi_pe_table_mreqs[i];
	mpig_vmpi_status_t * const vstatus = &mpig_cm_vmpi_pe_table_vstatuses[n];
	const int verror = errors ? mpig_vmpi_status_get_error(vstatus) : MPIG_VMPI_SUCCESS;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_REQ, "vendor request has completed: req=" MPIG_HANDLE_FMT
	    ", reqp=" MPIG_PTR_FMT, mreq->handle, MPIG_PTR_CAST(mreq)));
	    
	MPIU_Assert(mreq->cms.vmpi.pe_table_index == i);

	/* *sigh* the following should be not be needed as all requests should be reset to MPIG_VMPI_REQUEST_NULL when they are
	   completed, but some implementations of MPI fail to reset the request under some circumstances, like when the request
	   was successfully cancelled.  since many of the routines in this module use the request being set to
	   MPIG_VMPI_REQUEST_NULL to detect if the vendor request is complete, we perform the reset here. */
	mpig_cm_vmpi_pe_table_vreqs[i] = MPIG_VMPI_REQUEST_NULL;
	
	mpig_cm_vmpi_request_set_vreq(mreq, MPIG_VMPI_REQUEST_NULL);
	mreq->cms.vmpi.pe_table_index = -1;

	/* if the request was a receive any source, and the request could have been satisfied by a source other than vendor MPI,
	   then attempt to remove the request from the global receive queue.  if the request was found and removed, then release
	   the reference acquired in mpig_cm_vmpi_unregister_recv_any_source().  if request is not found, then another thread is
	   simultaneously attempting to unregister the receive any source request while this thread is completing it.  the
	   unregister process will release the reference count once that process has completed. */
	if (mpig_request_get_rank(mreq) == MPI_ANY_SOURCE)
	{
	    int vcancelled = FALSE;
	    int vrc;
		
	    if (mreq->cms.vmpi.cancelling && verror == MPIG_VMPI_SUCCESS)
	    {
		vrc = mpig_vmpi_test_cancelled(vstatus, &vcancelled);
		MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Test_cancelled", &mreq->status.MPI_ERROR);
	    }

	    /* if the dequeue routine returns true, then the receive-any-source request has completed.  if it returns false, then
	       the receive-any-source request was given to another module.  in the latter case, the request object may have
	       completed and thus be invalid.  as such, all references to it should be eliminated. */
	    if (mpig_recvq_deq_posted_ras_req(mreq, (bool_t) vcancelled))
	    {
		if (vcancelled == FALSE)
		{
		    const int vrank = mpig_vmpi_status_get_source(vstatus);
		    const int mrank = mpig_cm_vmpi_comm_get_remote_mrank(mreq->comm, vrank);
		    mpig_vc_t * const vc = mpig_comm_get_remote_vc(mreq->comm, mrank);
		    
		    mpig_request_set_vc(mreq, vc);
		}
		
		mpig_pe_end_ras_op();
	    }
	    else
	    {
		mreq= NULL;
		mpig_cm_vmpi_pe_table_mreqs[i] = NULL;
	    }
	    
	    mpig_cm_vmpi_pe_ras_active_count -= 1;
	}

	/* if the MPICH2 request is still valid, then extract the status from the vendor request and complete MPICH2 request */
	if (mreq)
	{
	    mpig_cm_vmpi_request_status_vtom(mreq, vstatus, verror);
	    mpig_request_complete(mreq);
	}
    }
    /* end for (i = 0; i < reqs_completed; i++) */

    if (num_reqs_completed == 1)
    {
	/* if only one vendor request completed, then remove the entry from the PE table the simple way */
	const int i = mpig_cm_vmpi_pe_table_vindices[0];
	
	mpig_cm_vmpi_pe_table_remove_entry(i);
    }
    else
    {
	/* if multiple vendor requests completed, then removing the entries from the PE table is not completely straight forward.
	   the extra complexity is caused by the entry removal process moving entries in the PE tables to keep the tables dense.
	   this movement of entries essentially invalidates the indices array.  to insure that all entries are properly removed,
	   the following code verifies that another completed vendor request has not been moved into the location of the entry
	   just removed.  if it has, then the entry is removed again.  any entries pointed to by the indices array that have
	   moved can be detected and ignored since their index would then be greater than the number of valid entries in the PE
	   tables.

	   an alternative approach would be to run through all entries in the vreq table looking for requests that have completed
	   (are equal to VMPI_REQUEST_NULL).  however, that approach is inefficient for a large table of active requests and a
	   small number of completions.  also, the algorithm below is only slightly more complex since the alternative scanning
	   approach would still have to handle the case where a completed vendor request was moved into the current entry. */
	n = 0;
	do
	{
	    const int i = mpig_cm_vmpi_pe_table_vindices[n];

	    while (i < mpig_cm_vmpi_pe_info.active_ops && mpig_vmpi_request_is_null(mpig_cm_vmpi_pe_table_vreqs[i]))
	    {
		mpig_cm_vmpi_pe_table_remove_entry(i);
	    }

	    n += 1;
	}
	while (n < num_reqs_completed);
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting"));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_complete_reqs);
    return;
}
/* mpig_cm_vmpi_pe_complete_reqs() */


#define MPIG_CM_VMPI_ERR_TESTWAIT_CHKANDJUMP(vrc_tw_, vfcname_, req_errors_p_)							\
{																\
    if ((vrc_tw_) == MPIG_VMPI_SUCCESS)												\
    {																\
	*(req_errors_p_) = FALSE;												\
    }																\
    else															\
    {																\
	int mpig_cm_vmpi_err_testwait_chkandjump_vrc_tw_class__;								\
	int mpig_cm_vmpi_err_testwait_chkandjump_vrc__;										\
																\
	mpig_cm_vmpi_err_testwait_chkandjump_vrc__ = mpig_vmpi_error_class((vrc_tw_),						\
	    &mpig_cm_vmpi_err_testwait_chkandjump_vrc_tw_class__);								\
	MPIU_Assert(mpig_cm_vmpi_err_testwait_chkandjump_vrc__ == MPI_SUCCESS && "vendor MPI_Error_class failed");		\
	if (mpig_cm_vmpi_err_testwait_chkandjump_vrc_tw_class__ != MPIG_VMPI_ERR_IN_STATUS)					\
	{															\
	    MPIG_ERR_VMPI_SET((vrc_tw_), vfcname_, &mpi_errno);									\
	    goto fn_fail;													\
	}															\
																\
	*(req_errors_p_) = TRUE;												\
    }																\
}

#define MPIG_CM_VMPI_USE_WAITANY 1

/*
 * <mpi_errno> mpig_cm_vmpi_pe_wait([IN/OUT] state)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_wait
int mpig_cm_vmpi_pe_wait(struct MPID_Progress_state * state)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_wait);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_wait);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    if (mpig_cm_vmpi_pe_ras_active_count > 0)
    {
	mpi_errno = mpig_recvq_unregister_ras_vreqs();
	if (mpi_errno != MPI_SUCCESS)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PROGRESS, "ERROR: unregistering queued vendor "
		"requests failed"));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|recvq_unreg_ras");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
	    
    if (mpig_cm_vmpi_pe_info.active_ops > 0)
    {
	const bool_t blocking = (mpig_pe_cm_owns_all_active_ops(&mpig_cm_vmpi_pe_info) &&
	    mpig_pe_op_has_completed(state) == FALSE) ? TRUE : FALSE;
	const char * tw_fcname = NULL;
	int num_reqs_completed = 0;
	bool_t errors_in_status;
	int vrc_tw;

	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("wait-A");

#       if defined(MPIG_CM_VMPI_USE_WAITSOME)
	{
	    if (blocking)
	    {
		tw_fcname = "MPI_Waitsome";
		vrc_tw = mpig_vmpi_waitsome(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		    &num_reqs_completed, mpig_cm_vmpi_pe_table_vindices, mpig_cm_vmpi_pe_table_vstatuses);
	    }
	    else
	    {
		tw_fcname = "MPI_Testsome";
		vrc_tw = mpig_vmpi_testsome(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		    &num_reqs_completed, mpig_cm_vmpi_pe_table_vindices, mpig_cm_vmpi_pe_table_vstatuses);
	    }
	}
#       elif defined(MPIG_CM_VMPI_USE_WAITANY)
	{
	    /* some MPI implementations incorrectly return errors related to the communication operation via the error field in
	       the status object.  the MPI standard says that that implementation shoudl only do so for MPI_Test/Waitsome and
	       MPI_Test/Waitall routines, and then only if MPI_ERR_IN_STATUS is return by those routines.  to detect the error in
	       one of these erroneous implementations, we must reset the error value in the status structure; otherwise, we might
	       falsely detect an error when using a correct implementation whose MPI_Test/Waitany routines do not modify the
	       error field of the status object. */
	    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], MPIG_VMPI_SUCCESS);
	    
	    if (blocking)
	    {
		tw_fcname = "MPI_Waitany";
		vrc_tw = mpig_vmpi_waitany(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		    &mpig_cm_vmpi_pe_table_vindices[0], &mpig_cm_vmpi_pe_table_vstatuses[0]);

		if (mpig_cm_vmpi_pe_table_vindices[0] != MPI_UNDEFINED)
		{
		    num_reqs_completed = 1;
		}
	    }
	    else
	    {
		int vcompleted;
		
		tw_fcname = "MPI_Testany";
		vrc_tw = mpig_vmpi_testany(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		    &mpig_cm_vmpi_pe_table_vindices[0], &vcompleted, &mpig_cm_vmpi_pe_table_vstatuses[0]);

		if (vcompleted || mpig_cm_vmpi_pe_table_vindices[0] != MPIG_VMPI_UNDEFINED)
		{
		    num_reqs_completed = 1;
		}
	    }

	    if (num_reqs_completed > 0)
	    {
		if (vrc_tw != MPIG_VMPI_SUCCESS)
		{
		    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], vrc_tw);
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
		else if (mpig_vmpi_status_get_error(&mpig_cm_vmpi_pe_table_vstatuses[0]) != MPIG_VMPI_SUCCESS)
		{
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
	    }
	}
#       elif defined(MPIG_CM_VMPI_USE_WAIT)
	{
	    int idx = 0;

	    if (blocking && mpig_cm_vmpi_pe_info.active_ops == 1)
	    {
		tw_fcname = "MPI_Wait";
		vrc_tw = mpig_vmpi_wait(&mpig_cm_vmpi_pe_table_vreqs[0], &mpig_cm_vmpi_pe_table_vstatuses[0]);
	    }
	    else
	    {
		int vcompleted = 0;

		vrc_tw = MPIG_VMPI_SUCCESS;
		while(idx < mpig_cm_vmpi_pe_info.active_ops || blocking)
		{
		    idx %= mpig_cm_vmpi_pe_info.active_ops;
			
		    tw_fcname = "MPI_Test";
		    vrc_tw = mpig_vmpi_test(&mpig_cm_vmpi_pe_table_vreqs[idx], &vcompleted, &mpig_cm_vmpi_pe_table_vstatuses[0]);
		    if (vcompleted || vrc_tw != MPIG_VMPI_SUCCESS)
		    {
			break;
		    }

		    idx += 1;
		}
	    }
		
	    if (idx < mpig_cm_vmpi_pe_info.active_ops)
	    {
		num_reqs_completed = 1;
		mpig_cm_vmpi_pe_table_vindices[0] = idx;

		if (vrc_tw != MPIG_VMPI_SUCCESS)
		{
		    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], vrc_tw);
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
	    }
	    
	    MPIG_CM_VMPI_ERR_TESTWAIT_CHKANDJUMP(vrc_tw,
		(blocking && mpig_cm_vmpi_pe_info.active_ops == 1) ? "MPI_Wait" : "MPI_Test", &errors_in_status);
	}
#	else
#	    error "INTERNAL ERROR: a method for polling the vendor MPI has not been selected."
#       endif
	
	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("wait-B");

	MPIG_CM_VMPI_ERR_TESTWAIT_CHKANDJUMP(vrc_tw, tw_fcname, &errors_in_status);
	
	if (num_reqs_completed > 0)
	{
	    mpig_cm_vmpi_pe_complete_reqs(num_reqs_completed, errors_in_status);
	}
	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("wait-C");
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_wait);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_pe_wait() */


/*
 * <mpi_errno> mpig_cm_vmpi_pe_test(void)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_pe_test
int mpig_cm_vmpi_pe_test(void)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_pe_test);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_pe_test);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "entering"));

    if (mpig_cm_vmpi_pe_ras_active_count > 0)
    {
	mpi_errno = mpig_recvq_unregister_ras_vreqs();
	if (mpi_errno != MPI_SUCCESS)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PROGRESS, "ERROR: unregistering queued vendor "
		"requests failed"));
	    MPIU_ERR_SET(mpi_errno, MPI_ERR_OTHER, "**globus|recvq_unreg_ras");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }

    if (mpig_cm_vmpi_pe_info.active_ops > 0)
    {
	const char * tw_fcname = NULL;
	int num_reqs_completed = 0;
	bool_t errors_in_status;
	int vrc_tw;
	
	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("test-A");
	
#       if defined(MPIG_CM_VMPI_USE_WAITSOME)
	{
	    tw_fcname = "MPI_Testsome";
	    vrc_tw = mpig_vmpi_testsome(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		&num_reqs_completed, mpig_cm_vmpi_pe_table_vindices, mpig_cm_vmpi_pe_table_vstatuses);
	}	
#       elif defined(MPIG_CM_VMPI_USE_WAITANY)
	{
	    int vcompleted;
	    
	    /* some MPI implementations incorrectly return errors related to the communication operation via the error field in
	       the status object.  the MPI standard says that that implementation shoudl only do so for MPI_Test/Waitsome and
	       MPI_Test/Waitall routines, and then only if MPI_ERR_IN_STATUS is return by those routines.  to detect the error in
	       one of these erroneous implementations, we must reset the error value in the status structure; otherwise, we might
	       falsely detect an error when using a correct implementation whose MPI_Test/Waitany routines do not modify the
	       error field of the status object. */
	    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], MPIG_VMPI_SUCCESS);
	    
	    tw_fcname = "MPI_Testany";
	    vrc_tw = mpig_vmpi_testany(mpig_cm_vmpi_pe_info.active_ops, mpig_cm_vmpi_pe_table_vreqs,
		&mpig_cm_vmpi_pe_table_vindices[0], &vcompleted, &mpig_cm_vmpi_pe_table_vstatuses[0]);
	    
	    if (vcompleted || mpig_cm_vmpi_pe_table_vindices[0] != MPIG_VMPI_UNDEFINED)
	    {
		num_reqs_completed = 1;

		if (vrc_tw != MPIG_VMPI_SUCCESS)
		{
		    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], vrc_tw);
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
		else if (mpig_vmpi_status_get_error(&mpig_cm_vmpi_pe_table_vstatuses[0]) != MPIG_VMPI_SUCCESS)
		{
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
	    }
	}
#       elif defined(MPIG_CM_VMPI_USE_WAIT)
	{
	    int idx = 0;
	    int vcompleted = 0;

	    vrc_tw = MPIG_VMPI_SUCCESS;
	    while(idx < mpig_cm_vmpi_pe_info.active_ops)
	    {
		vrc_tw = mpig_vmpi_test(&mpig_cm_vmpi_pe_table_vreqs[idx], &vcompleted, &mpig_cm_vmpi_pe_table_vstatuses[0]);
		if (vcompleted || vrc_tw != MPIG_VMPI_SUCCESS)
		{
		    break;
		}

		idx += 1;
	    }
		
	    if (idx < mpig_cm_vmpi_pe_info.active_ops)
	    {
		num_reqs_completed = 1;
		mpig_cm_vmpi_pe_table_vindices[0] = idx;

		if (vrc_tw != MPIG_VMPI_SUCCESS)
		{
		    mpig_vmpi_status_set_error(&mpig_cm_vmpi_pe_table_vstatuses[0], vrc_tw);
		    vrc_tw = MPIG_VMPI_ERR_IN_STATUS;
		}
	    }
	    
	    MPIG_CM_VMPI_ERR_TESTWAIT_CHKANDJUMP(vrc_tw, "MPI_Test", &errors_in_status);
	}
#       else
#	    error "INTERNAL ERROR: a method for polling the vendor MPI has not been selected."
#       endif
	
	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("test-B");
	
	MPIG_CM_VMPI_ERR_TESTWAIT_CHKANDJUMP(vrc_tw, tw_fcname, &errors_in_status);
	
	if (num_reqs_completed > 0)
	{
	    mpig_cm_vmpi_pe_complete_reqs(num_reqs_completed, errors_in_status);
	}
	
	MPIG_CM_VMPI_DEBUG_PRINT_PE_TABLE("test-C");
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PROGRESS, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_pe_test);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_pe_test() */

/**********************************************************************************************************************************
				     END COMMUNICATION MODULE PROGRESS ENGINE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN VC CORE API FUNCTIONS
**********************************************************************************************************************************/
#define mpig_cm_vmpi_send_macro(send_op_, send_op_name_, isend_op_name_, buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_,		 \
    sreqp_, mpi_errno_p_)													 \
{																 \
    mpig_vmpi_datatype_t mpig_cm_vmpi_send_vdt__;										 \
    const int mpig_cm_vmpi_send_vrank__ = mpig_cm_vmpi_comm_get_remote_vrank((comm_), (rank_));					 \
    const int mpig_cm_vmpi_send_vtag__ = mpig_cm_vmpi_tag_get_vtag((tag_));							 \
    const mpig_vmpi_comm_t mpig_cm_vmpi_send_vcomm__ = mpig_cm_vmpi_comm_get_vcomm((comm_), (ctxoff_));				 \
    int mpig_cm_vmpi_send_vrc__;												 \
    int mpig_cm_vmpi_send_dt_size__ = 0;											 \
																 \
    mpig_cm_vmpi_datatype_get_vdt((dt_), &mpig_cm_vmpi_send_vdt__);								 \
																 \
    /* NOTE: a blocking send may only be used when no other communication module requires polling to make progress, or no other	 \
       requests have outstanding requests. */											 \
    if (mpig_pe_cm_owns_all_active_ops(&mpig_cm_vmpi_pe_info))									 \
    {																 \
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "performing a blocking %s operation: vdt=" MPIG_VMPI_DATATYPE_FMT		 \
	    ", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, #send_op_, MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_send_vdt__),	 \
	    mpig_cm_vmpi_send_vrank__, mpig_cm_vmpi_send_vtag__, MPIG_VMPI_COMM_CAST(mpig_cm_vmpi_send_vcomm__)));		 \
	mpig_cm_vmpi_send_vrc__ = mpig_vmpi_ ## send_op_((buf_), (cnt_), (mpig_cm_vmpi_send_vdt__),				 \
	    (mpig_cm_vmpi_send_vrank__), (mpig_cm_vmpi_send_vtag__), (mpig_cm_vmpi_send_vcomm__));				 \
	MPIG_ERR_VMPI_CHKANDJUMP(mpig_cm_vmpi_send_vrc__, (send_op_name_), (mpi_errno_p_));					 \
																 \
	/* the send request is set to NULL in order to inform the upper layer that the requested operation has completed */	 \
	*(sreqp_) = NULL;													 \
    }																 \
    else															 \
    {																 \
	mpig_vc_t * const mpig_cm_vmpi_send_vc__ = mpig_comm_get_remote_vc((comm_), (rank_));					 \
	MPID_Request * mpig_cm_vmpi_send_sreq__;										 \
																 \
	/* create a new send request */												 \
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "creating a new send request"));						 \
	/* NOTE: mpig_request_create_isreq() jumps to fn_fail if failure occurs */						 \
	mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 2, 1, (void *) (buf_), (cnt_), (dt_), (rank_), (tag_),		 \
	    (comm_)->context_id + (ctxoff_), (comm_), mpig_cm_vmpi_send_vc__, &mpig_cm_vmpi_send_sreq__);			 \
	mpig_cm_vmpi_request_construct(mpig_cm_vmpi_send_sreq__);								 \
																 \
	/* start the nonblocking send operation */										 \
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "starting a nonblocking %s operation: vdt=" MPIG_VMPI_DATATYPE_FMT		 \
	    ", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, #send_op_, MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_send_vdt__),	 \
	    mpig_cm_vmpi_send_vrank__,mpig_cm_vmpi_send_vtag__, MPIG_VMPI_COMM_CAST(mpig_cm_vmpi_send_vcomm__)));		 \
	mpig_cm_vmpi_send_vrc__ = mpig_vmpi_i ## send_op_((buf_), (cnt_), (mpig_cm_vmpi_send_vdt__),				 \
	    (mpig_cm_vmpi_send_vrank__), (mpig_cm_vmpi_send_vtag__), (mpig_cm_vmpi_send_vcomm__),				 \
	    mpig_cm_vmpi_request_get_vreq_ptr(mpig_cm_vmpi_send_sreq__));							 \
	MPIG_ERR_VMPI_CHKANDJUMP((mpig_cm_vmpi_send_vrc__), (isend_op_name_), (mpi_errno_p_));					 \
																 \
	mpig_cm_vmpi_pe_table_add_req(mpig_cm_vmpi_send_sreq__, (mpi_errno_p_));						 \
	if (*(mpi_errno_p_))													 \
	{   /* --BEGIN ERROR HANDLING-- */											 \
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: attempt to register a request with the " \
		"VMPI process engine failed: sreq" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, mpig_cm_vmpi_send_sreq__->handle,	 \
		MPIG_PTR_CAST(mpig_cm_vmpi_send_sreq__)));									 \
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|pe_table_add_req",					 \
		"**globus|cm_vmpi|pe_table_add_req %R %p", mpig_cm_vmpi_send_sreq__->handle, mpig_cm_vmpi_send_sreq__);		 \
	    goto fn_fail;													 \
	}															 \
																 \
	*(sreqp_) = mpig_cm_vmpi_send_sreq__;											 \
    }																 \
																 \
    /* add to usage bytes sent counter */											 \
    mpig_datatype_get_size((dt_), &mpig_cm_vmpi_send_dt_size__);								 \
    MPIG_USAGE_INC_VMPI_NBYTES_SENT(mpig_cm_vmpi_send_dt_size__ * (cnt_));							 \
}


#define mpig_cm_vmpi_isend_macro(send_op_, isend_op_name_, buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, sreqp_, mpi_errno_p_)	\
{																\
    mpig_vmpi_datatype_t mpig_cm_vmpi_isend_vdt__;										\
    const int mpig_cm_vmpi_isend_vrank__ = mpig_cm_vmpi_comm_get_remote_vrank((comm_), (rank_));				\
    const int mpig_cm_vmpi_isend_vtag__ = mpig_cm_vmpi_tag_get_vtag((tag_));							\
    const mpig_vmpi_comm_t mpig_cm_vmpi_isend_vcomm__ = mpig_cm_vmpi_comm_get_vcomm((comm_), (ctxoff_));			\
    mpig_vc_t * const mpig_cm_vmpi_isend_vc__ = mpig_comm_get_remote_vc((comm_), (rank_));					\
    MPID_Request * mpig_cm_vmpi_isend_sreq__;											\
    int mpig_cm_vmpi_isend_vrc__;												\
    int mpig_cm_vmpi_isend_dt_size__ = 0;											\
																\
    /* create a new send request */												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "creating a new send request"));							\
    /* NOTE: mpig_request_create_isreq() jumps to fn_fail if failure occurs */							\
    mpig_request_create_isreq(MPIG_REQUEST_TYPE_SEND, 2, 1, (void *) (buf_), (cnt_), (dt_), (rank_), (tag_),			\
	(comm_)->context_id + (ctxoff_), (comm_), mpig_cm_vmpi_isend_vc__, &mpig_cm_vmpi_isend_sreq__);				\
    mpig_cm_vmpi_request_construct(mpig_cm_vmpi_isend_sreq__);									\
																\
    /* start the nonblocking send operation */											\
    mpig_cm_vmpi_datatype_get_vdt((dt_), &mpig_cm_vmpi_isend_vdt__);								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "starting a nonblocking %s operation: vdt=" MPIG_VMPI_DATATYPE_FMT		\
	", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, #send_op_, MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_isend_vdt__),		\
	mpig_cm_vmpi_isend_vrank__, mpig_cm_vmpi_isend_vtag__, MPIG_VMPI_COMM_CAST(mpig_cm_vmpi_isend_vcomm__)));		\
    mpig_cm_vmpi_isend_vrc__ = mpig_vmpi_i ## send_op_((buf_), (cnt_), (mpig_cm_vmpi_isend_vdt__),				\
	(mpig_cm_vmpi_isend_vrank__), (mpig_cm_vmpi_isend_vtag__), (mpig_cm_vmpi_isend_vcomm__),				\
	mpig_cm_vmpi_request_get_vreq_ptr(mpig_cm_vmpi_isend_sreq__));								\
    MPIG_ERR_VMPI_CHKANDJUMP((mpig_cm_vmpi_isend_vrc__), (isend_op_name_), (mpi_errno_p_));					\
																\
    mpig_cm_vmpi_pe_table_add_req(mpig_cm_vmpi_isend_sreq__, (mpi_errno_p_));							\
    if (*(mpi_errno_p_))													\
    {   /* --BEGIN ERROR HANDLING-- */												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: attempt to register a request with the "	\
	    "VMPI process engine failed: sreq" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, mpig_cm_vmpi_isend_sreq__->handle,	\
	    MPIG_PTR_CAST(mpig_cm_vmpi_isend_sreq__)));									\
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|pe_table_add_req",						\
	    "**globus|cm_vmpi|pe_table_add_req %R %p", mpig_cm_vmpi_isend_sreq__->handle, mpig_cm_vmpi_isend_sreq__);		\
	goto fn_fail;														\
    }																\
																\
    *(sreqp_) = mpig_cm_vmpi_isend_sreq__;											\
																\
    /* add to usage bytes sent counter */											\
    mpig_datatype_get_size((dt_), &mpig_cm_vmpi_isend_dt_size__);								\
    MPIG_USAGE_INC_VMPI_NBYTES_SENT(mpig_cm_vmpi_isend_dt_size__ * (cnt_));							\
}


/*
 * int mpig_cm_vmpi_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_send
static int mpig_cm_vmpi_adi3_send(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,"entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_send_macro(send, "MPI_Send", "MPI_Isend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_send(...) */


/*
 * int mpig_cm_vmpi_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_isend
static int mpig_cm_vmpi_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_isend_macro(send, "MPI_Isend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_isend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_isend() */


/*
 * int mpig_cm_vmpi_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_rsend
static int mpig_cm_vmpi_adi3_rsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_rsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_rsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_send_macro(rsend, "MPI_Rsend", "MPI_Irsend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_rsend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_rsend() */


/*
 * int mpig_cm_vmpi_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_irsend
static int mpig_cm_vmpi_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_isend_macro(rsend, "MPI_Irsend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_irsend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_irsend() */


/*
 * int mpig_cm_vmpi_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_ssend
static int mpig_cm_vmpi_adi3_ssend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_ssend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_ssend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_send_macro(ssend, "MPI_Ssend", "MPI_Issend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_ssend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_ssend() */


/*
 * int mpig_cm_vmpi_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_issend
static int mpig_cm_vmpi_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_issend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpig_cm_vmpi_isend_macro(ssend, "MPI_Issend", buf, cnt, dt, rank, tag, comm, ctxoff, sreqp, &mpi_errno);

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_issend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_issend() */


/*
 * int mpig_cm_vmpi_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_recv
static int mpig_cm_vmpi_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t vdt;
    const int vrank = mpig_cm_vmpi_comm_get_remote_vrank(comm, rank);
    const int vtag = mpig_cm_vmpi_tag_get_vtag(tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff);
    const int ctx = comm->context_id + ctxoff;
    MPID_Request * rreq = NULL;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_cm_vmpi_datatype_get_vdt(dt, &vdt);

    /* NOTE: the blocking receive routine may only be used when no other communication method requires polling to make progress,
       or no other requests have outstanding requests. */
    if (mpig_pe_cm_owns_all_active_ops(&mpig_cm_vmpi_pe_info))
    {
	mpig_vmpi_status_t vstatus;
	
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "performing a blocking recv operation: vdt=" MPIG_VMPI_DATATYPE_FMT
	    ", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, MPIG_VMPI_DATATYPE_CAST(vdt), vrank, vtag,
	    MPIG_VMPI_COMM_CAST(vcomm)));
	vrc = mpig_vmpi_recv(buf, cnt, vdt, vrank, vtag, vcomm, &vstatus);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Rrecv", &mpi_errno);

	/* if the operation completed successfully, and the application supplied a status structure, then fill in the status */
	if (status != MPI_STATUS_IGNORE)
	{
	    int count;
	    int vstatus_source = mpig_vmpi_status_get_source(&vstatus);
	    int mrank = (vstatus_source >= 0) ? mpig_cm_vmpi_comm_get_remote_mrank(comm, vstatus_source) : MPI_UNDEFINED;
	    int vstatus_tag = mpig_vmpi_status_get_tag(&vstatus);
	    int mtag = (vstatus_tag >= 0) ? vstatus_tag : MPI_UNDEFINED;
	    
	    MPIG_Assert(vstatus_source < mpig_cm_vmpi_comm_get_remote_vsize(comm));
	    
	    vrc = mpig_vmpi_get_count(&vstatus, MPIG_VMPI_BYTE, &count);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);

	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "blocking recv complete: vrank=%d, mrank=%d"
		", vtag=%d, mtag=%d, count=%d", vstatus_source, mrank, vstatus_tag, mtag, count));

	    status->MPI_SOURCE = mrank;
	    status->MPI_TAG = mtag;
	    /* NOTE: MPI_ERROR must not be set except by the MPI_Test/Wait routines when MPI_ERR_IN_STATUS is to be returned. */
	    status->count = count;
	    status->cancelled = FALSE;
	    /* status->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	}
    
	/* the receive request is set to NULL in order to inform the upper layer that the requested operation has completed */
	*rreqp = NULL;
    }
    else
    {
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(comm, rank);
	
	/* create a new receive request */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "creating a new receive request"));
	/* NOTE: mpig_request_create_irreq() jumps to fn_fail if failure occurs */
	mpig_request_create_irreq(2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc, &rreq);
	mpig_cm_vmpi_request_construct(rreq);

	/* start the nonblocking receive */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "starting a nonblocking recv operation: vdt=" MPIG_VMPI_DATATYPE_FMT
	    ", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, MPIG_VMPI_DATATYPE_CAST(vdt), vrank, vtag,
	    MPIG_VMPI_COMM_CAST(vcomm)));
	vrc = mpig_vmpi_irecv(buf, cnt, vdt, vrank, vtag, vcomm, mpig_cm_vmpi_request_get_vreq_ptr(rreq));
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Irecv", &mpi_errno);

	/* add the outstanding request to the progress engine request tracking data structures */
	mpig_cm_vmpi_pe_table_add_req(rreq, &mpi_errno);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: attempt to register a request with the "
		"VMPI process engine failed: rreq" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|pe_table_add_req",
		"**globus|cm_vmpi|pe_table_add_req %R %p", rreq->handle, rreq);
	    goto fn_fail;
	}

	*rreqp = rreq;
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpig_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), MPIG_PTR_CAST(*rreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_recv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (rreq != NULL)
	{
	    mpig_request_destroy(rreq);
	}
	if (status != MPI_STATUS_IGNORE)
	{
	    status->MPI_ERROR = mpi_errno;
	}
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* int mpig_cm_vmpi_adi3_recv() */


/*
 * int mpig_cm_vmpi_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_irecv
static int mpig_cm_vmpi_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    mpig_vmpi_datatype_t vdt;
    const int vrank = mpig_cm_vmpi_comm_get_remote_vrank(comm, rank);
    const int vtag = mpig_cm_vmpi_tag_get_vtag(tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff);
    mpig_vc_t * const vc = mpig_comm_get_remote_vc(comm, rank);
    MPID_Request * rreq = NULL;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: buf=" MPIG_PTR_FMT
	", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d", MPIG_PTR_CAST(buf), cnt, dt,
	rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpig_cm_vmpi_datatype_get_vdt(dt, &vdt);
    
    /* create a new receive request */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "creating a new receive request"));
    /* NOTE: mpig_request_create_irreq() jumps to fn_fail if failure occurs */
    mpig_request_create_irreq(2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc, &rreq);
    mpig_cm_vmpi_request_construct(rreq);

    /* start the nonblocking receive */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "starting a nonblocking recv operation: vdt=" MPIG_VMPI_DATATYPE_FMT
	", vrank=%d, vtag=%d, vcomm=" MPIG_VMPI_COMM_FMT, MPIG_VMPI_DATATYPE_CAST(vdt), vrank, vtag,
	MPIG_VMPI_COMM_CAST(vcomm)));
    vrc = mpig_vmpi_irecv(buf, cnt, vdt, vrank, vtag, vcomm, mpig_cm_vmpi_request_get_vreq_ptr(rreq));
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Irecv", &mpi_errno);

    /* add the outstanding request to the progress engine request tracking data structures */
    mpig_cm_vmpi_pe_table_add_req(rreq, &mpi_errno);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: attempt to register a request with the "
	    "VMPI process engine failed: rreq" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));
	MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|pe_table_add_req",
	    "**globus|cm_vmpi|pe_table_add_req %R %p", rreq->handle, rreq);
	goto fn_fail;
    }

    *rreqp = rreq;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpig_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), MPIG_PTR_CAST(*rreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_irecv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (rreq != NULL)
	{
	    mpig_request_set_cc(rreq, 0);
	    mpig_request_set_ref_count(rreq, 0);
	    mpig_request_destroy(rreq);
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_irecv() */


/*
 * int mpig_cm_vmpi_adi3_probe(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_probe
static int mpig_cm_vmpi_adi3_probe(
    const int rank, const int tag, MPID_Comm * const comm, const int ctxoff, MPI_Status * const status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int vrank = mpig_cm_vmpi_comm_get_remote_recv_vrank(comm, rank);
    const int vtag = mpig_cm_vmpi_tag_get_vtag(tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff);
    mpig_vmpi_status_t vstatus;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_probe);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_probe);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, status=" MPIG_PTR_FMT, rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff,
	MPIG_PTR_CAST(status)));

    if (mpig_pe_cm_owns_all_active_ops(&mpig_cm_vmpi_pe_info) || !mpig_pe_polling_required(&mpig_cm_vmpi_pe_info) == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "performing a blocking probe operation: vrank=%d, vtag=%d, vcomm="
	    MPIG_VMPI_COMM_FMT, vrank, vtag, MPIG_VMPI_COMM_CAST(vcomm)));
	vrc = mpig_vmpi_probe(vrank, vtag, vcomm, &vstatus);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Probe", &mpi_errno);
    }
    else
    {
	int vfound;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "performing a nonblocking probe operation: vrank=%d, vtag=%d, vcomm="
	    MPIG_VMPI_COMM_FMT, vrank, vtag, MPIG_VMPI_COMM_CAST(vcomm)));
	vrc = mpig_vmpi_iprobe(vrank, vtag, vcomm, &vfound, &vstatus);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Iprobe", &mpi_errno);
	if (vfound) goto blk_exit;

	while (TRUE)
	{
	    mpi_errno = MPID_Progress_poke();
	    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|pe_poke");
	    
	    vrc = mpig_vmpi_iprobe(vrank, vtag, vcomm, &vfound, &vstatus);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Iprobe", &mpi_errno);
	    if (vfound) break;

	    mpig_thread_yield();
	}
	
      blk_exit: ;
    }

    if (status != MPI_STATUS_IGNORE)
    {
	int count;
	int vstatus_source = mpig_vmpi_status_get_source(&vstatus);
	int vstatus_tag = mpig_vmpi_status_get_tag(&vstatus);

	MPIG_Assert(vstatus_source < mpig_cm_vmpi_comm_get_remote_vsize(comm));
	    
	vrc = mpig_vmpi_get_count(&vstatus, MPIG_VMPI_BYTE, &count);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);
	    
	status->MPI_SOURCE = (vstatus_source >= 0) ? mpig_cm_vmpi_comm_get_remote_mrank(comm, vstatus_source) : MPI_UNDEFINED;
	status->MPI_TAG = (vstatus_tag >= 0) ? vstatus_tag : MPI_UNDEFINED;
	/* NOTE: MPI_ERROR must not be set except by the MPI_Test/Wait routines when MPI_ERR_IN_STATUS is to be returned. */
	status->count = count;
	status->cancelled = FALSE;
	/* status->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, mpi_errno=" MPIG_ERRNO_FMT, rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff,
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_probe);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_probe(...) */


/*
 * int mpig_cm_vmpi_adi3_iprobe(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_iprobe
static int mpig_cm_vmpi_adi3_iprobe(
    const int rank, const int tag, MPID_Comm * const comm, const int ctxoff, int * const found_p, MPI_Status * const status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int vrank = mpig_cm_vmpi_comm_get_remote_recv_vrank(comm, rank);
    const int vtag = mpig_cm_vmpi_tag_get_vtag(tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff);
    mpig_vmpi_status_t vstatus;
    int vfound = FALSE;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, status=" MPIG_PTR_FMT, rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff,
	MPIG_PTR_CAST(status)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "performing a nonblocking probe operation: vrank=%d, vtag=%d, vcomm="
	MPIG_VMPI_COMM_FMT, vrank, vtag, MPIG_VMPI_COMM_CAST(vcomm)));
    vrc = mpig_vmpi_iprobe(vrank, vtag, vcomm, &vfound, &vstatus);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Iprobe", &mpi_errno);

    if (vfound && status != MPI_STATUS_IGNORE)
    {
	int count;
	int vstatus_source = mpig_vmpi_status_get_source(&vstatus);
	int vstatus_tag = mpig_vmpi_status_get_tag(&vstatus);

	MPIG_Assert(vstatus_source < mpig_cm_vmpi_comm_get_remote_vsize(comm));
	    
	vrc = mpig_vmpi_get_count(&vstatus, MPIG_VMPI_BYTE, &count);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);
	    
	status->MPI_SOURCE = (vstatus_source >= 0) ? mpig_cm_vmpi_comm_get_remote_mrank(comm, vstatus_source) : MPI_UNDEFINED;
	status->MPI_TAG = (vstatus_tag >= 0) ? vstatus_tag : MPI_UNDEFINED;
	/* NOTE: MPI_ERROR must not be set except by the MPI_Test/Wait routines when MPI_ERR_IN_STATUS is to be returned. */
	status->count = count;
	status->cancelled = FALSE;
	/* status->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
    }
    
    *found_p = vfound;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, found=%s, mpi_errno=" MPIG_ERRNO_FMT, rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff,
	MPIG_BOOL_STR(vfound), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_iprobe);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_adi3_iprobe(...) */


/*
 * int mpig_cm_vmpi_adi3_cancel_send([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_cancel_send
static int mpig_cm_vmpi_adi3_cancel_send(MPID_Request * const sreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, sreq->handle, MPIG_PTR_CAST(sreq)));

    if (mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(sreq)) == FALSE && sreq->cms.vmpi.cancelling == FALSE)
    {
	vrc = mpig_vmpi_cancel(mpig_cm_vmpi_request_get_vreq_ptr(sreq));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Cancel", &mpi_errno);

	sreq->cms.vmpi.cancelling = TRUE;
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT ", mpig_errno=" MPIG_ERRNO_FMT,
		       sreq->handle, MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_cancel_send);
    return mpi_errno;
}
/* mpig_cm_vmpi_adi3_cancel_send() */


/*
 * int mpig_cm_vmpi_adi3_cancel_recv([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_adi3_cancel_recv

static int mpig_cm_vmpi_adi3_cancel_recv(MPID_Request * const rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    int vrc;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq)));

    if (mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(rreq)) == FALSE && rreq->cms.vmpi.cancelling == FALSE)
    {
	vrc = mpig_vmpi_cancel(mpig_cm_vmpi_request_get_vreq_ptr(rreq));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Cancel", &mpi_errno);
    
	rreq->cms.vmpi.cancelling = TRUE;
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
		       "exiting: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT "mpig_errno=" MPIG_ERRNO_FMT,
		       rreq->handle, MPIG_PTR_CAST(rreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_adi3_cancel_recv);
    return mpi_errno;
}
/* mpig_cm_vmpi_adi3_cancel_recv() */
/**********************************************************************************************************************************
						    END VC CORE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATOR MANAGEMENT HOOKS
**********************************************************************************************************************************/
typedef int (*mpig_cm_vmpi_comm_create_fn_t)(MPID_Comm * orig_comm, MPID_Comm * new_comm);

static int mpig_cm_vmpi_comm_construct_localcomm(MPID_Comm * orig_comm, MPID_Comm * new_comm,
    mpig_cm_vmpi_comm_create_fn_t comm_create_func);


/*
 * <mpi_errno> mpig_cm_vmpi_comm_dup_hook([IN] orig_comm, [IN/MOD] new_comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_comm_dup_hook
int mpig_cm_vmpi_comm_dup_hook(MPID_Comm * const orig_comm, MPID_Comm * const new_comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int ctxoff = 0;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_comm_dup_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_comm_dup_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, orig_comm->handle, MPIG_PTR_CAST(orig_comm),
	MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm)));

    mpig_cm_vmpi_comm_construct(new_comm);
    
    if (mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) > 0)
    {
	if (orig_comm->comm_kind == MPID_INTRACOMM)
	{
	    mpig_cm_vmpi_comm_set_remote_vsize(new_comm, mpig_cm_vmpi_comm_get_remote_vsize(orig_comm));
	
	    /*
	     * duplicate the MPICH2 to Vendor MPI remote rank mapping table
	     */
	    new_comm->cms.vmpi.remote_ranks_mtov = (int *) MPIU_Malloc(orig_comm->remote_size * sizeof(int));
	    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of MPICH2 to VMPI rank "
		    "translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
		    MPIG_PTR_CAST(new_comm)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPICH2 to VMPI rank translation table");
		goto error_malloc_mtov;
	    }   /* --END ERROR HANDLING-- */

	    memcpy(new_comm->cms.vmpi.remote_ranks_mtov, orig_comm->cms.vmpi.remote_ranks_mtov, orig_comm->remote_size * sizeof(int));

	    /*
	     * duplicate the VMPI to MPICH2 MPI remote rank mapping table
	     */
	    new_comm->cms.vmpi.remote_ranks_vtom = (int *) MPIU_Malloc(mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) * sizeof(int));
	    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of VMPI to MPICH2 rank "
		    "translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
		    MPIG_PTR_CAST(new_comm)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "VMPI to MPICH2 rank translation table");
		goto error_malloc_vtom;
	    }   /* --END ERROR HANDLING-- */

	    memcpy(new_comm->cms.vmpi.remote_ranks_vtom, orig_comm->cms.vmpi.remote_ranks_vtom,
		mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) * sizeof(int));

	    /*
	     * create a new vendor communicator for each context in the MPICH2 communicator
	     */
	    for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	    {
		const mpig_vmpi_comm_t orig_vcomm = mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0);
		mpig_vmpi_comm_t * const new_vcomm_p = mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, ctxoff);
	
		vrc = mpig_vmpi_comm_dup(orig_vcomm, new_vcomm_p);
		MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", &mpi_errno, {goto error_comm_dup;});
	    }
	    /* NOTE: ctxoff is used by the error handling code to determine how many communicators need to be freed.  therefore, it
	       is important that it ctxoff not be altered after this point. */
	}
	else
	{
	    mpi_errno = mpig_cm_vmpi_intercomm_create_hook(orig_comm->local_comm, new_comm);
	    if (mpi_errno != MPI_SUCCESS) goto error_intercomm_dup;
	}
    }

#   if 0
    {
	/*
	 * if the new communicator is an intercommunicator, then construct a local communicator for the collective operations
	 *
	 * NOTE: this is only needed if the vendor MPI supports the duplication of intercommunicators; otherwise, teh creation o
	 * fhte local communicator for collective operations in handled by the mpig_cm_vmpi_intercomm_create_hook().
	 */
	if (orig_comm->comm_kind == MPID_INTERCOMM)
	{
	    mpi_errno = mpig_cm_vmpi_comm_construct_localcomm(orig_comm->local_comm, new_comm, mpig_cm_vmpi_comm_dup_hook);
	    if (mpi_errno) goto error_construct_localcomm;
	}
    }
#   endif

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, orig_comm->handle,
	MPIG_PTR_CAST(orig_comm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), mpi_errno));

    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_comm_dup_hook);
    return mpi_errno;
    
    {   /* --BEGIN ERROR HANDLING-- */
	int c;
	/* error_construct_localcomm: */
      error_comm_dup:
	for (c = 0; c < ctxoff; c++)
	{
	    /* FIXME: check error codes returned by vendor MPI routines */
	    mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, c));
	}

	MPIU_Free(new_comm->cms.vmpi.remote_ranks_vtom);
	new_comm->cms.vmpi.remote_ranks_vtom = NULL;
	
      error_malloc_vtom:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
	new_comm->cms.vmpi.remote_ranks_mtov = NULL;
	
      error_malloc_mtov:
	mpig_cm_vmpi_comm_set_remote_vsize(new_comm, 0);

      error_intercomm_dup:
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_comm_dup_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_comm_split_hook([IN] orig_comm, [IN/MOD] new_comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_comm_split_hook
int mpig_cm_vmpi_comm_split_hook(MPID_Comm * const orig_comm, MPID_Comm * const new_comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int p;
    int color = MPIG_VMPI_UNDEFINED;
    int key = MPIG_VMPI_UNDEFINED;
    int ctxoff = 0;
    const mpig_vc_t * const my_vc = mpig_comm_get_my_vc(orig_comm);
    int vcnt = 0;
    int total_vcnt;
    mpig_vmpi_comm_t tmp_vcomm;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_comm_split_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_comm_split_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, orig_comm->handle, MPIG_PTR_CAST(orig_comm),
	MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm)));

    if (mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) > 0)
    {
	if (new_comm != NULL)
	{
	    mpig_cm_vmpi_comm_construct(new_comm);
	    
	    /*
	     * construct the MPICH2 to VMPI remote rank mapping.  also, attempt to determine the color and key to be passed to
	     * MPIG_VMPI_Comm_split() by the local process.
	     */
	    new_comm->cms.vmpi.remote_ranks_mtov = (int *) MPIU_Malloc(new_comm->remote_size * sizeof(int));
	    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of MPICH2 to VMPI rank "
		    "translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
		    MPIG_PTR_CAST(new_comm)));
		MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPICH2 to VMPI rank translation table");
		goto error_malloc_mtov;
	    }   /* --END ERROR HANDLING-- */

	    for (p = 0; p < new_comm->remote_size; p++)
	    {
		const mpig_vc_t * const vc = mpig_comm_get_remote_vc(new_comm, p);
		bool_t same_vmpi = mpig_uuid_compare(&vc->cms.vmpi.job_id, &mpig_cm_vmpi_job_id);

                MPIG_DEBUG_STMT(MPIG_DEBUG_LEVEL_COMM, {
                    char my_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
                    char vc_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];

                    mpig_uuid_unparse(&mpig_cm_vmpi_job_id, my_job_id_str);
                    mpig_uuid_unparse(&vc->cms.vmpi.job_id, vc_job_id_str);
                    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "comparing job ids: p=%d, vc=" MPIG_PTR_FMT ", my_vmpi_job_id=%s"
                        ", vc_vmpi_job_id=%s, same_vmpi=%s", p, MPIG_PTR_CAST(vc), my_job_id_str, vc_job_id_str,
                        MPIG_BOOL_STR(same_vmpi)));
                });
                
		if (same_vmpi)
		{
		    if (color == MPIG_VMPI_UNDEFINED)
		    {
			color = vc->cms.vmpi.cw_rank;
		    }

		    if (vc == my_vc)
		    {
			key = vcnt;
		    }
		    
		    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, vcnt);
		    vcnt += 1;
		}
		else
		{
		    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, MPIG_VMPI_UNDEFINED);
		}
	    }
	    /* end for (p = 0; p < new_comm->remote_size; p++) */

	    if (vcnt > 0)
	    {
		mpig_cm_vmpi_comm_set_remote_vsize(new_comm, vcnt);

		vrc = mpig_vmpi_allreduce(&vcnt, &total_vcnt, 1, MPIG_VMPI_INT, MPIG_VMPI_SUM,
		    mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0));
		MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Allreduce", &mpi_errno, {goto error_vcnt_allreduce;});
	    
		/* the key will still be undefined if the communicator is an intercommunicator.  scan the local process table to
		   determine the key to be used by the local process. */
		if (orig_comm->comm_kind == MPID_INTERCOMM)
		{
		    for (p = 0; p < new_comm->local_size; p++)
		    {
			if (mpig_comm_get_local_vc(new_comm, p) == my_vc)
			{
			    key = p;
			}
		    }
		}
	    
		MPIU_Assert(color != MPIG_VMPI_UNDEFINED);
		MPIU_Assert(key != MPIG_VMPI_UNDEFINED);

		/*
		 * construct the VMPI to MPICH2 remote rank mapping table
		 */
		new_comm->cms.vmpi.remote_ranks_vtom = (int *) MPIU_Malloc(mpig_cm_vmpi_comm_get_remote_vsize(new_comm) *
		    sizeof(int));
		if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
		{   /* --BEGIN ERROR HANDLING-- */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of VMPI to MPICH2 rank "
			"translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
			MPIG_PTR_CAST(new_comm)));
		    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "VMPI to MPICH2 rank translation table");
		    goto error_malloc_vtom;
		}   /* --END ERROR HANDLING-- */

		for (p = 0; p < new_comm->remote_size; p++)
		{
		    int vrank = mpig_cm_vmpi_comm_get_remote_vrank(new_comm, p);
		    if (vrank != MPIG_VMPI_UNDEFINED)
		    {
			mpig_cm_vmpi_comm_set_remote_mrank(new_comm, vrank, p);
		    }
		}

		/*
		 * perform the split operation, creating a new vendor MPI communicator that represents the first context of the
		 * new communicator
		 */
		if ((MPIG_VMPI_VERSION) > 1 || orig_comm->comm_kind == MPID_INTRACOMM)
		{
		    MPIU_Assert(key != MPIG_VMPI_UNDEFINED);
	    
		    /* all processes must perform the VMPI_Comm_split (see new_comm == NULL case below) */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "intra/mpi-2 - calling VMPI_Comm_split: orig_comm=" MPIG_HANDLE_FMT
			", orig_comm_ctx=%d, remote_vsize=%d, color=%d, key=%d, new_comm=" MPIG_HANDLE_FMT ", new_comm_ctx=%d",
			orig_comm->handle, orig_comm->context_id, vcnt, color, key, new_comm->handle, new_comm->context_id));

		    vrc = mpig_vmpi_comm_split(mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0), color, key,
			mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, 0));
		    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_split", &mpi_errno, {goto error_comm_split;});
		}
		else /* if ((MPIG_VMPI_VERSION) <= 1 && orig_comm->comm_kind == MPID_INTERCOMM) */
		{
		    /* NOTE: the communicator split operation cannot be implemented directly using VMPI_Comm_split since the
		       underlying vendor MPI may not be MPI-2 compliant.  instead, the split operation is applied to the local
		       intracommunicator asscociated with orig_comm.  VMPI_Intercomm_create is then used to combine the resulting
		       local and remote groups of processes into a new intercommunicator. */
		    int remote_leader;

		    /* all processes must perform the VMPI_Comm_split (see new_comm == NULL case below) */
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "inter - calling VMPI_Comm_split: orig_comm=" MPIG_HANDLE_FMT
			", orig_comm_ctx=%d, remote_vsize=%d, color=%d, key=%d, new_comm=" MPIG_HANDLE_FMT ", new_comm_ctx=%d",
			orig_comm->handle, orig_comm->context_id, vcnt, color, key, new_comm->handle, new_comm->context_id));

		    vrc = mpig_vmpi_comm_split(mpig_cm_vmpi_comm_get_vcomm(orig_comm->local_comm, 0), color, key, &tmp_vcomm);
		    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_split", &mpi_errno, {goto error_comm_split;});
		    MPIU_Assert(!mpig_vmpi_comm_is_null(tmp_vcomm));
	    
		    /* however, only the processes remain that are part of the new communicator need to participate in creating
		       the new intercommunicator */
		    remote_leader = mpig_comm_get_remote_vc(new_comm,
			mpig_cm_vmpi_comm_get_remote_mrank(new_comm, 0))->cms.vmpi.cw_rank;
	    
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "inter - calling VMPI_Intercomm_create: orig_comm=" MPIG_HANDLE_FMT
			", orig_comm_ctx=%d, local_leader=0, remote_leader=%d, tag=%d, new_comm=" MPIG_HANDLE_FMT
			", new_comm_ctx=%d", orig_comm->handle, orig_comm->context_id, remote_leader, new_comm->context_id,
			new_comm->handle, new_comm->context_id));
    
		    vrc = mpig_vmpi_intercomm_create(tmp_vcomm, 0, mpig_cm_vmpi_bridge_vcomm, remote_leader, new_comm->context_id,
			mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, 0));
		    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Intercomm_create", &mpi_errno);
	    
		    vrc = mpig_vmpi_comm_free(&tmp_vcomm);
		    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Comm_free", &mpi_errno);
	    
		    if (mpi_errno) goto error_comm_split;
		}
		/* end if/else ((MPIG_VMPI_VERSION) > 1 || orig_comm->comm_kind == MPID_INTRACOMM) */

		/*
		 * construct communicators for the remaining contexts using VMPI_Comm_dup().  only the processes that are part of
		 * the new communicator need to participate in the duplication process.
		 */ 
		for (ctxoff = 1; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
		{
		    const mpig_vmpi_comm_t new_vcomm0 = mpig_cm_vmpi_comm_get_vcomm(new_comm, 0);
		    mpig_vmpi_comm_t * const new_vcomm_p = mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, ctxoff);

		    vrc = mpig_vmpi_comm_dup(new_vcomm0, new_vcomm_p);
		    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", &mpi_errno, {goto error_comm_dup;});
		}
		/* NOTE: ctxoff is used by the error handling code to determine how many communicators need to be freed.
		   therefore, it is important that it ctxoff not be altered after this point. */
	    }
	    else /* if (vcnt == 0) */
	    {
		MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
		new_comm->cms.vmpi.remote_ranks_mtov = NULL;
	    }
	    /* end if/else (vcnt > 0) */
	}
	/* end if (new_comm != NULL) */

	if (vcnt == 0 /* || new_comm == NULL) */)
	{
	    vrc = mpig_vmpi_allreduce(&vcnt, &total_vcnt, 1, MPIG_VMPI_INT, MPIG_VMPI_SUM,
		mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0));
	    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Allreduce", &mpi_errno, {goto error_vcnt_allreduce_nop;});

	    if (total_vcnt > 0)
	    {
		/* all processes must perform the VMPI_Comm_split() even if they aren't in the new communicator */
		if ((MPIG_VMPI_VERSION) > 1 || orig_comm->comm_kind == MPID_INTRACOMM)
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "intra/mpi-2 - calling VMPI_Comm_split: orig_comm=" MPIG_HANDLE_FMT
			", orig_comm_ctx=%d, remote_vsize=0, color=MPIG_VMPI_UNDEFINED, key=MPIG_VMPI_UNDEFINED"
			", new_comm=MPI_COMM_NULL", orig_comm->handle, orig_comm->context_id));
		    vrc = mpig_vmpi_comm_split(mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0), MPIG_VMPI_UNDEFINED, MPIG_VMPI_UNDEFINED,
			&tmp_vcomm);
		    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_split", &mpi_errno, {goto error_comm_split_nop;});
		}
		else
		{
		    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "inter - calling VMPI_Comm_split: orig_comm=" MPIG_HANDLE_FMT
			", orig_comm_ctx=%d, remote_vsize=0, color=MPIG_VMPI_UNDEFINED, key=MPIG_VMPI_UNDEFINED"
			", new_comm=MPI_COMM_NULL", orig_comm->handle, orig_comm->context_id));
		    vrc = mpig_vmpi_comm_split(mpig_cm_vmpi_comm_get_vcomm(orig_comm->local_comm, 0), MPIG_VMPI_UNDEFINED,
			MPIG_VMPI_UNDEFINED, &tmp_vcomm);
		    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Comm_split", &mpi_errno);

		    vrc = mpig_vmpi_comm_free(&tmp_vcomm);
		    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Comm_free", &mpi_errno);

		    if (mpi_errno) goto error_comm_split_nop;
		}
	    }
	}
	/* end if (vcnt == 0) */
    }
    else /* if (mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) == 0) */
    {
	if (new_comm != NULL)
	{
	    mpig_cm_vmpi_comm_construct(new_comm);
	}    
    }
    /* end if/else (mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) > 0) */
    
    /*
     * if the new communicator is an intercommunicator, then construct a local communicator for the collective operations
     */
    if (orig_comm->comm_kind == MPID_INTERCOMM)
    {
	mpi_errno = mpig_cm_vmpi_comm_construct_localcomm(orig_comm->local_comm, new_comm, mpig_cm_vmpi_comm_split_hook);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    if (new_comm != NULL)
	    {
		goto error_construct_localcomm;
	    }
	    else
	    {
		goto error_construct_localcomm_nop;
	    }
	}   /* --END ERROR HANDLING-- */
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, orig_comm->handle,
	MPIG_PTR_CAST(orig_comm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_comm_split_hook);
    return mpi_errno;
    
    {   /* --BEGIN ERROR HANDLING-- */
	int c;

      error_construct_localcomm:
      error_comm_dup:
	for (c = 0; c < ctxoff; c++)
	{
	    /* FIXME: check error codes returned by vendor MPI routines */
	    mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, c));
	}
	
      error_comm_split:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_vtom);
	new_comm->cms.vmpi.remote_ranks_vtom = NULL;
	
      error_malloc_vtom:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
	new_comm->cms.vmpi.remote_ranks_mtov = NULL;

      error_vcnt_allreduce:
      error_malloc_mtov:
	mpig_cm_vmpi_comm_set_remote_vsize(new_comm, 0);
	goto fn_return;

      error_construct_localcomm_nop:
      error_comm_split_nop:
      error_vcnt_allreduce_nop:
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_comm_split_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_intercomm_create_hook([IN] orig_comm, [IN/MOD] new_comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_intercomm_create_hook
int mpig_cm_vmpi_intercomm_create_hook(MPID_Comm * const orig_comm, MPID_Comm * const new_comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int p;
    int vcnt;
    int remote_leader;
    int ctxoff = 0;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_intercomm_create_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_intercomm_create_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ",orig_size=%d, orig_rank=%d, new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT "new_local_size=%d"
	", new_remote_size=%d, new_rank=%d", orig_comm->handle, MPIG_PTR_CAST(orig_comm), orig_comm->remote_size, orig_comm->rank,
	MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), new_comm->local_size, new_comm->remote_size, new_comm->rank));

    mpig_cm_vmpi_comm_construct(new_comm);

    /*
     * construct the MPICH2 to VMPI rank mapping
     */
    new_comm->cms.vmpi.remote_ranks_mtov = (int *) MPIU_Malloc(new_comm->remote_size * sizeof(int));
    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of MPICH2 to VMPI rank "
	    "translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
	    MPIG_PTR_CAST(new_comm)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPICH2 to VMPI rank translation table");
	goto error_malloc_mtov;
    }   /* --END ERROR HANDLING-- */

    vcnt = 0;
    for (p = 0; p < new_comm->remote_size; p++)
    {
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(new_comm, p);
	bool_t same_vmpi = mpig_uuid_compare(&vc->cms.vmpi.job_id, &mpig_cm_vmpi_job_id);

        MPIG_DEBUG_STMT(MPIG_DEBUG_LEVEL_COMM, {
            char my_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
            char vc_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
            
            mpig_uuid_unparse(&mpig_cm_vmpi_job_id, my_job_id_str);
            mpig_uuid_unparse(&vc->cms.vmpi.job_id, vc_job_id_str);
            
            MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "comparing job ids: p=%d, vc=" MPIG_PTR_FMT ", my_vmpi_job_id=%s"
                ", vc_vmpi_job_id=%s, same_vmpi=%s", p, MPIG_PTR_CAST(vc), my_job_id_str, vc_job_id_str,
                MPIG_BOOL_STR(same_vmpi)));
        });
	
	if (same_vmpi)
	{
	    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, vcnt);
	    vcnt += 1;
	}
	else
	{
	    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, MPIG_VMPI_UNDEFINED);
	}
    }

    mpig_cm_vmpi_comm_set_remote_vsize(new_comm, vcnt);

    if (vcnt > 0)
    {
	/*
	 * if one or more of the remote processes can be communicated with using the vendor MPI, then construct the VMPI to
	 * MPICH2 remote rank mapping table
	 */
	new_comm->cms.vmpi.remote_ranks_vtom = (int *) MPIU_Malloc(mpig_cm_vmpi_comm_get_remote_vsize(new_comm) * sizeof(int));
	if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of VMPI to MPICH2 rank "
		"translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
		MPIG_PTR_CAST(new_comm)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "VMPI to MPICH2 rank translation table");
	    goto error_malloc_vtom;
	}   /* --END ERROR HANDLING-- */

	for (p = 0; p < new_comm->remote_size; p++)
	{
	    const int vrank = mpig_cm_vmpi_comm_get_remote_vrank(new_comm, p);
	
	    if (vrank != MPIG_VMPI_UNDEFINED)
	    {
		mpig_cm_vmpi_comm_set_remote_mrank(new_comm, vrank, p);
	    }
	}

	/*
	 * construct intercommunicators for all MPICH2 contexts.  since some vendor MPI implementations, like MPICH-GM, do not
	 * support duplicatiing intercommunicators, VMPI_Intercomm_create() is used to generate each context rather using it to
	 * create the first context and VMPI_Comm_dup () to create the remainder.
	 *
	 * NOTE: the bridge communicator is a duplicate of the vendor MPI_COMM_WORLD.  since only processes that are part of the
	 * same vendor MPI job will call this routine, we can guarantee that one process from the local group and one process
	 * from the remote group will be part of bridge communicator.
	 */ 
	remote_leader = mpig_comm_get_remote_vc(new_comm, mpig_cm_vmpi_comm_get_remote_mrank(new_comm, 0))->cms.vmpi.cw_rank;
	    
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "calling VMPI_Intercomm_create: orig_comm=" MPIG_HANDLE_FMT ", orig_comm_ctx=%d"
	    ", local_leader=0, remote_leader=%d, tag=%d, new_comm=" MPIG_HANDLE_FMT ", new_comm_ctx=%d", orig_comm->handle,
	    orig_comm->context_id, remote_leader, new_comm->context_id, new_comm->handle, new_comm->context_id));
    
	for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	{
	    const mpig_vmpi_comm_t orig_vcomm0 = mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0);
	    mpig_vmpi_comm_t * const new_vcomm_p = mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, ctxoff);
	    
	    vrc = mpig_vmpi_intercomm_create(orig_vcomm0, 0, mpig_cm_vmpi_bridge_vcomm, remote_leader, new_comm->context_id,
		new_vcomm_p);
	    MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Intercomm_create", &mpi_errno, {goto error_intercomm_create;});
	}
	/* NOTE: ctxoff is used by the error handling code to determine how many communicators need to be freed.  therefore, it
	   is important that it ctxoff not be altered after this point. */
    }
    else
    {
	/* if none of the remote processes can be communicated with using vendor MPI, then free the array containing the mapping
	   of MPICH2 to VMPI ranks. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "cannot communicate with any processes in remote group using vendor MPI"));

	MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
	new_comm->cms.vmpi.remote_ranks_mtov = NULL;
    }

    /*
     * construct a local communicator for use by the collective operations.  this is just a dup of orig_comm.
     */
    mpi_errno = mpig_cm_vmpi_comm_construct_localcomm(orig_comm, new_comm, mpig_cm_vmpi_comm_dup_hook);
    if (mpi_errno) goto error_construct_localcomm;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, orig_comm->handle,
	MPIG_PTR_CAST(orig_comm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_intercomm_create_hook);
    return mpi_errno;
    
    {   /* --BEGIN ERROR HANDLING-- */
	int c;
	
      error_construct_localcomm:
      error_intercomm_create:
	for (c = 0; c < ctxoff; c++)
	{
	    /* FIXME: check error codes returned by vendor MPI routines */
	    mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, c));
	}
	
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_vtom);
	new_comm->cms.vmpi.remote_ranks_vtom = NULL;
	
      error_malloc_vtom:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
	new_comm->cms.vmpi.remote_ranks_mtov = NULL;
	
      error_malloc_mtov:
	mpig_cm_vmpi_comm_set_remote_vsize(new_comm, 0);
	goto fn_return;
    }   /* --BEGIN ERROR HANDLING-- */
}
/* mpig_cm_vmpi_intercomm_create_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_intercomm_merge_hook([IN] orig_comm, [IN/MOD] new_comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_intercomm_merge_hook
int mpig_cm_vmpi_intercomm_merge_hook(MPID_Comm * const orig_comm, MPID_Comm * const new_comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int high = (mpig_comm_get_local_vc(orig_comm, 0) != mpig_comm_get_remote_vc(new_comm, 0));
    int p;
    int vcnt;
    int ctxoff = 0;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_intercomm_merge_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_intercomm_merge_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", new_comm_ctx=%d", orig_comm->handle,
	MPIG_PTR_CAST(orig_comm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), new_comm->context_id));
    /* NOTE: the context id will change later, and is printed here only for debugging purposes.  for more details, see the note
       in src/mpi/intercomm_merge.c above the call to the MPID_DEV_COMM_FUNC_HOOK. */

    mpig_cm_vmpi_comm_construct(new_comm);

    /*
     * construct the MPICH2 to VMPI rank mapping
     */
    new_comm->cms.vmpi.remote_ranks_mtov = (int *) MPIU_Malloc(new_comm->remote_size * sizeof(int));
    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of MPICH2 to VMPI rank "
	    "translation table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle,
	    MPIG_PTR_CAST(new_comm)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "MPICH2 to VMPI rank translation table");
	goto error_malloc_mtov;
    }   /* --END ERROR HANDLING-- */

    vcnt = 0;
    for (p = 0; p < new_comm->remote_size; p++)
    {
	mpig_vc_t * vc = mpig_comm_get_remote_vc(new_comm, p);
	bool_t same_vmpi = mpig_uuid_compare(&vc->cms.vmpi.job_id, &mpig_cm_vmpi_job_id);

        MPIG_DEBUG_STMT(MPIG_DEBUG_LEVEL_COMM, {
            char my_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
            char vc_job_id_str[MPIG_UUID_MAX_STR_LEN + 1];
            
            mpig_uuid_unparse(&mpig_cm_vmpi_job_id, my_job_id_str);
            mpig_uuid_unparse(&vc->cms.vmpi.job_id, vc_job_id_str);
            
            MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COMM, "comparing job ids: p=%d, vc=" MPIG_PTR_FMT ", my_vmpi_job_id=%s"
                ", vc_vmpi_job_id=%s, same_vmpi=%s", p, MPIG_PTR_CAST(vc), my_job_id_str, vc_job_id_str,
                MPIG_BOOL_STR(same_vmpi)));
        });
	
	if (same_vmpi)
	{
	    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, vcnt);
	    vcnt += 1;
	}
	else
	{
	    mpig_cm_vmpi_comm_set_remote_vrank(new_comm, p, MPIG_VMPI_UNDEFINED);
	}
    }

    MPIU_Assert(vcnt > 0);
    mpig_cm_vmpi_comm_set_remote_vsize(new_comm, vcnt);

    /*
     * construct the VMPI to MPICH2 remote rank mapping table
     */
    new_comm->cms.vmpi.remote_ranks_vtom = (int *) MPIU_Malloc(mpig_cm_vmpi_comm_get_remote_vsize(new_comm) * sizeof(int));
    if (new_comm->cms.vmpi.remote_ranks_mtov == NULL)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: allocation of VMPI to MPICH2 rank translation "
	    "table failed: new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle, MPIG_PTR_CAST(new_comm)));
	MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "VMPI to MPICH2 rank translation table");
	goto error_malloc_vtom;
    }   /* --END ERROR HANDLING-- */

    for (p = 0; p < new_comm->remote_size; p++)
    {
	const int vrank = mpig_cm_vmpi_comm_get_remote_vrank(new_comm, p);
	
	if (vrank != MPIG_VMPI_UNDEFINED)
	{
	    mpig_cm_vmpi_comm_set_remote_mrank(new_comm, vrank, p);
	}
    }

    /*
     * if the intercommunicator contains remote processes with which current process can communicate with using vendor MPI, then
     * perform the vendor intercomm merge operation to create a communicator to represent the first context of the new MPICH2
     * communicator.  However, if the intercommunicator does not contain remote processes that the current process can
     * communicate with using vendor MPI, then the vendor intercommunicators will be set to MPIG_VMPI_COMM_NULL.  in this case,
     * the vendor communicator represent the first context of the MPICH2 communicator is created by duplicating one of the vendor
     * communicators from the original communicators localcomm.
     */
    if (mpig_cm_vmpi_comm_get_remote_vsize(orig_comm) > 0)
    {
	vrc = mpig_vmpi_intercomm_merge(mpig_cm_vmpi_comm_get_vcomm(orig_comm, 0), high,
	    mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, 0));
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_merge", &mpi_errno, {goto error_intercomm_merge;});
    }
    else
    {
	MPIU_Assert(mpig_cm_vmpi_comm_get_remote_vsize(orig_comm->local_comm) > 0);
	
	vrc = mpig_vmpi_comm_dup(mpig_cm_vmpi_comm_get_vcomm(orig_comm->local_comm, 0),
	    mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, 0));
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", &mpi_errno, {goto error_localcomm_dup;});
    }

    /*
     * construct communicators for the remaining contexts using VMPI_Comm_dup().  only the processes that are part of the new
     * communicator need to participate in the duplication process.
     */ 
    for (ctxoff = 1; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
    {
	const mpig_vmpi_comm_t new_vcomm0 = mpig_cm_vmpi_comm_get_vcomm(new_comm, 0);
	mpig_vmpi_comm_t * const new_vcomm_p = mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, ctxoff);

	vrc = mpig_vmpi_comm_dup(new_vcomm0, new_vcomm_p);
	MPIG_ERR_VMPI_CHKANDSTMT(vrc, "MPI_Comm_dup", &mpi_errno, {goto error_comm_dup;});
    }
    /* NOTE: ctxoff is used by the error handling code to determine how many communicators need to be freed.  therefore, it
       is important that it ctxoff not be altered after this point. */
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_comm=" MPIG_HANDLE_FMT ", orig_commp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, orig_comm->handle,
	MPIG_PTR_CAST(orig_comm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_intercomm_merge_hook);
    return mpi_errno;
    
    {   /* --BEGIN ERROR HANDLING-- */
	int c;
	
      error_comm_dup:
	for (c = 0; c < ctxoff; c++)
	{
	    /* FIXME: check error codes returned by vendor MPI routines */
	    mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(new_comm, c));
	}

      error_localcomm_dup:
      error_intercomm_merge:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_vtom);
	new_comm->cms.vmpi.remote_ranks_vtom = NULL;
	
      error_malloc_vtom:
	MPIU_Free(new_comm->cms.vmpi.remote_ranks_mtov);
	new_comm->cms.vmpi.remote_ranks_mtov = NULL;
	
      error_malloc_mtov:
	goto fn_return;

    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_intercomm_merge_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_comm_free_hook([IN] comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_comm_free_hook
int mpig_cm_vmpi_comm_free_hook(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int ctxoff;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_comm_free_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_comm_free_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM,
	"entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, comm->handle, MPIG_PTR_CAST(comm)));

    if (mpig_cm_vmpi_comm_get_remote_vsize(comm) > 0)
    {
	/* free all of the vendor MPI communicators that represent the various MPICH2 contexts.  this must be done when the
	   application calls MPI_Comm_free() rather than when the communicator object is destroyed by MPIR_Comm_release().
	   VMPI_Comm_free() is a collective operation.  the call to MPI_Comm_free() is guaranteed to be called collectively by
	   all processes within the communicator.  the same cannot be said of MPIR_Comm_release(). */
	for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	{
	    vrc = mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(comm, ctxoff));
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Comm_free", &mpi_errno);
	}
    }

    if (comm->comm_kind == MPID_INTERCOMM && mpig_cm_vmpi_comm_get_remote_vsize(comm->local_comm) > 0)
    {
	for (ctxoff = 0; ctxoff < MPIG_COMM_NUM_CONTEXTS; ctxoff++)
	{
	    vrc = mpig_vmpi_comm_free(mpig_cm_vmpi_comm_get_vcomm_ptr(comm->local_comm, ctxoff));
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Comm_free", &mpi_errno);
	}
    }

    /* NOTE: VMPI CM resources contained within the communicator object are not released here since active requests may still be
       associated with the communicator.  those resources may be needed to complete those outstanding requests.  for exsample,
       the vtom rank mapping table may be needed to set the source field in the status structure returned by a receive operation.
       these resources will be released by mpig_cm_vmpi_comm_destruct(), which is called once all outstanding operations have
       completed. */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: mpig_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_comm_free_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_comm_free_hook() */


/*
 * void mpig_cm_vmpi_comm_destruct_hook([IN/MOD] comm)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_comm_destruct_hook
void mpig_cm_vmpi_comm_destruct_hook(MPID_Comm * const comm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_comm_destruct_hook);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_comm_destruct_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT,
	comm->handle, MPIG_PTR_CAST(comm)));

    if (comm->comm_kind == MPID_INTERCOMM)
    {
	mpig_cm_vmpi_comm_destruct(comm->local_comm);
    }
    mpig_cm_vmpi_comm_destruct(comm);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT,
	comm->handle, MPIG_PTR_CAST(comm)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_comm_destruct_hook);
    return;
}
/* mpig_cm_vmpi_comm_destruct_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_comm_construct_localcomm([IN] orig_local_comm, [IN/MOD] new_comm)
 *
 * because the current implementation of MPICH2 intercommunicators use a secondary intracommunicator (local_comm), two additional
 * steps are necessary.  first, MPIR_Setup_intercomm_localcomm(new_comm) must be called to create the secondary communicator
 * object.  second, the communicators in orig_local_comm->cms.vmpi.comms[] must duplicated.  the latter step is accomplished by
 * calling this routine again, passing it the original and new localcomm objects.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_comm_construct_localcomm
static int mpig_cm_vmpi_comm_construct_localcomm(MPID_Comm * const orig_lcomm, MPID_Comm * const new_comm,
    const mpig_cm_vmpi_comm_create_fn_t comm_create_func)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_comm_construct_localcomm);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_comm_construct_localcomm);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "entering: orig_lcomm=" MPIG_HANDLE_FMT ", orig_lcommp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, orig_lcomm->handle, MPIG_PTR_CAST(orig_lcomm),
	MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm)));

    if (new_comm != NULL)
    {
	mpi_errno = MPIR_Setup_intercomm_localcomm(new_comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: construction of localcomm failed: "
		"new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle, MPIG_PTR_CAST(new_comm)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|localcomm", "**globus|cm_vmpi|localcomm %C %p",
		new_comm->handle, new_comm);
	    goto error_localcomm_create;
	}   /* --END ERROR HANDLING-- */

	mpig_cm_vmpi_comm_construct(new_comm->local_comm);

	mpi_errno = comm_create_func(orig_lcomm, new_comm->local_comm);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: construction of localcomm failed: "
		"new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle, MPIG_PTR_CAST(new_comm)));
	    goto error_localcomm_vdup;
	}   /* --END ERROR HANDLING-- */
    }
    else
    {
	mpi_errno = comm_create_func(orig_lcomm, NULL);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_COMM, "ERROR: construction of localcomm failed: "
		"new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT, new_comm->handle, MPIG_PTR_CAST(new_comm)));
	    goto error_nop_localcomm_vdup;
	}   /* --END ERROR HANDLING-- */
    }
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_COMM, "exiting: orig_lcomm=" MPIG_HANDLE_FMT ", orig_lcommp="
	MPIG_PTR_FMT ", new_comm=" MPIG_HANDLE_FMT ", new_commp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, orig_lcomm->handle,
	MPIG_PTR_CAST(orig_lcomm), MPIG_HANDLE_VAL(new_comm), MPIG_PTR_CAST(new_comm), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_comm_construct_localcomm);
    return mpi_errno;
    
    {   /* --BEGIN ERROR HANDLING-- */
      error_localcomm_vdup:
	MPIR_Comm_release(new_comm->local_comm);
	new_comm->local_comm = NULL;
	
      error_localcomm_create:
	goto fn_return;

      error_nop_localcomm_vdup:
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_comm_construct_localcomm() */
/**********************************************************************************************************************************
						END COMMUNICATOR MANAGEMENT HOOKS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN DATATYPE HOOKS
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_vmpi_type_contiguous_hook([IN] cnt, [IN] old_dt, [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_contiguous_hook
int mpig_cm_vmpi_type_contiguous_hook(const int cnt, const MPI_Datatype old_dt, MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t old_vdt;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_contiguous_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_contiguous_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: cnt=%d, old_dt=" MPIG_HANDLE_FMT ", new_dt="
	MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, cnt, old_dt, new_dtp->handle, MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_contigous: cnt=%d, old_vdt=" MPIG_VMPI_DATATYPE_FMT,
	cnt, MPIG_VMPI_DATATYPE_CAST(old_vdt)));
    vrc = mpig_vmpi_type_contiguous(cnt, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_contiguous", &mpi_errno);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_contigous returned: new_vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(new_dtp))));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_contiguous_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_contiguous_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_vector_hook([IN] cnt, [IN] blklen, [IN] stride, [IN] stride_in_bytes, [IN] old_dt, [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_vector_hook
int mpig_cm_vmpi_type_vector_hook(const int cnt, const int blklen, const mpig_vmpi_aint_t stride, const int stride_in_bytes,
    const MPI_Datatype old_dt, MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t old_vdt;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_vector_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_vector_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: cnt=%d, blklen=%d, stride=" MPIG_AINT_FMT
	", stride_in_bytes=%s, old_dt=" MPIG_HANDLE_FMT ", new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, cnt, blklen,
	MPIG_PTR_CAST(stride), MPIG_BOOL_STR(stride_in_bytes), old_dt, new_dtp->handle, MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);
    
    if (stride_in_bytes == FALSE)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_vector: cnt=%d, blklen=%d, stride=" MPIG_AINT_FMT
	    ", old_vdt=" MPIG_VMPI_DATATYPE_FMT, cnt, blklen, stride, MPIG_VMPI_DATATYPE_CAST(old_vdt)));
	vrc = mpig_vmpi_type_vector(cnt, blklen, (int) stride, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_vector", &mpi_errno);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_vector returned: new_vdt=" MPIG_VMPI_DATATYPE_FMT,
	    MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(new_dtp))));
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_create_hvector: cnt=%d, blklen=%d, stride=" MPIG_AINT_FMT
	    ", old_vdt=" MPIG_VMPI_DATATYPE_FMT, cnt, blklen, stride, MPIG_VMPI_DATATYPE_CAST(old_vdt)));
	vrc = mpig_vmpi_type_create_hvector(cnt, blklen, stride, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_create_hvector", &mpi_errno);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_create_hvector returned: new_vdt=" MPIG_VMPI_DATATYPE_FMT,
	    MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(new_dtp))));
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_vector_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_vector_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_indexed_hook([IN] cnt, [IN] blklens, [IN] displs, [IN] displs_in_bytes, [IN] old_dt,
 *     [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_indexed_hook
int mpig_cm_vmpi_type_indexed_hook(const int cnt, int * const blklens, void * const displs, const int displs_in_bytes,
    const MPI_Datatype old_dt, MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t old_vdt;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_indexed_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_indexed_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: cnt=%d, blklens=" MPIG_PTR_FMT ", displs="
	MPIG_PTR_FMT ", displs_in_bytes=%s, old_dt=" MPIG_HANDLE_FMT ", new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, cnt,
	MPIG_PTR_CAST(blklens), MPIG_PTR_CAST(displs), MPIG_BOOL_STR(displs_in_bytes), old_dt, new_dtp->handle,
	MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);
    
    if (displs_in_bytes == FALSE)
    {
	vrc = mpig_vmpi_type_indexed(cnt, blklens, (int *) displs, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_indexed", &mpi_errno);
    }
    else
    {
	vrc = mpig_vmpi_type_create_hindexed(cnt, blklens, (mpig_vmpi_aint_t *) displs, old_vdt,
	    mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_create_hindexed", &mpi_errno);
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_indexed_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_indexed_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_blockindexed_hook([IN] cnt, [IN] blklen, [IN] displs, [IN] displs_in_bytes, [IN] old_dt,
 *     [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_blockindexed_hook
int mpig_cm_vmpi_type_blockindexed_hook(const int cnt, const int blklen, void * const displs, const int displs_in_bytes,
    const MPI_Datatype old_dt, MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int * blklens;
    mpig_vmpi_datatype_t old_vdt;
    int n;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(1);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_blockindexed_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_blockindexed_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: cnt=%d, blklen=%d, displs=" MPIG_PTR_FMT
	", displs_in_bytes=%s, old_dt=" MPIG_HANDLE_FMT ", new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, cnt, blklen,
	MPIG_PTR_CAST(displs), MPIG_BOOL_STR(displs_in_bytes), old_dt, new_dtp->handle, MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);

#   if defined(HAVE_VMPI_TYPE_CREATE_INDEXED_BLOCK)
    {
	if (displs_in_bytes == FALSE)
	{
	    vrc = mpig_vmpi_type_create_indexed_block(cnt, blklen, (int *) displs, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_indexed", &mpi_errno);

	    goto fn_return;
	}
    }
#   endif
    
    MPIU_CHKLMEM_MALLOC(blklens, int *, sizeof(int) * cnt, mpi_errno, "block length array");
    for (n = 0; n < cnt; n++)
    {
	blklens[n] =  blklen;
    }
    
    if (displs_in_bytes == FALSE)
    {
	vrc = mpig_vmpi_type_indexed(cnt, blklens, (int *) displs, old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_indexed", &mpi_errno);
    }
    else
    {
	vrc = mpig_vmpi_type_create_hindexed(cnt, blklens, (mpig_vmpi_aint_t *) displs, old_vdt,
	    mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_create_hindexed", &mpi_errno);
    }
    
  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_blockindexed_hook);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_type_blockindexed_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_struct_hook([IN] cnt, [IN] blklens, [IN] displs, [IN] displs_in_bytes, [IN] old_dt,
 *     [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_struct_hook
int mpig_cm_vmpi_type_struct_hook(const int cnt, int * const blklens, mpig_vmpi_aint_t * const displs, MPI_Datatype * old_dts,
    MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int * vblklens;
    mpig_vmpi_aint_t * vdispls;
    mpig_vmpi_datatype_t * old_vdts;
    bool_t have_ub = FALSE;
    int n;
    int vrc;
    int vcnt = cnt;
    int mpi_errno = MPI_SUCCESS;
    MPIU_CHKLMEM_DECL(3);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_struct_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_struct_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: cnt=%d, blklens=" MPIG_PTR_FMT ", displs="
	MPIG_PTR_FMT ", old_dts=" MPIG_PTR_FMT ", new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, cnt, MPIG_PTR_CAST(blklens),
	MPIG_PTR_CAST(displs), MPIG_PTR_CAST(old_dts), new_dtp->handle, MPIG_PTR_CAST(new_dtp)));

    MPIU_CHKLMEM_MALLOC(vblklens, int *, sizeof(int) * (cnt + 1), mpi_errno, "array of vendor blklens");
    MPIU_CHKLMEM_MALLOC(vdispls, mpig_vmpi_aint_t *, sizeof(mpig_vmpi_aint_t) * (cnt + 1), mpi_errno,
	"array of vendor displacements");
    MPIU_CHKLMEM_MALLOC(old_vdts, mpig_vmpi_datatype_t *, sizeof(mpig_vmpi_datatype_t) * (cnt + 1), mpi_errno,
	"array of vendor datatypes");

    for (n = 0; n < cnt; n++)
    {
	if (old_dts[n] == MPI_UB)
	{
	    have_ub = TRUE;
	}

	vblklens[n] = blklens[n];
	vdispls[n] = displs[n];
	mpig_cm_vmpi_datatype_get_vdt(old_dts[n], &old_vdts[n]);

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "vblklens[%d]=%d, vdispls[%d]=" MPIG_PTR_FMT ", old_dt[%d]=" MPIG_HANDLE_FMT
	    ", old_vdt[%d]=" MPIG_VMPI_DATATYPE_FMT, n, vblklens[n], n, MPIG_PTR_CAST(vdispls[n]), n, old_dts[n], n,
	    MPIG_VMPI_DATATYPE_CAST(old_vdts[n])));
    }

    /* XXX: we need to create two new datatypes: one with the explicit UB, and one without.  the one with the explicit UB will be
       used for communication.  the one without will be used when creating new datatypes that encapsulate this one. */
    
    if (have_ub == FALSE)
    {
	vblklens[cnt] = 1;
	vdispls[cnt] = new_dtp->ub;
	old_vdts[cnt] = MPIG_VMPI_UB;
	vcnt = cnt + 1;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "vblklens[%d]=%d, vdispls[%d]=" MPIG_PTR_FMT ", old_dt[%d]=" MPIG_HANDLE_FMT
	    ", old_vdt[%d]=" MPIG_VMPI_DATATYPE_FMT, cnt, vblklens[cnt], cnt, MPIG_PTR_CAST(displs[cnt]), cnt, MPI_UB, cnt,
	    MPIG_VMPI_DATATYPE_CAST(old_vdts[cnt])));
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_create_struct: vcnt=%d, vblklens=" MPIG_PTR_FMT ", vdispls="
    MPIG_PTR_FMT ", old_vdt=" MPIG_PTR_FMT, vcnt, MPIG_PTR_CAST(vblklens), MPIG_PTR_CAST(vdispls), MPIG_PTR_CAST(old_vdts)));
    
    vrc = mpig_vmpi_type_create_struct(vcnt, vblklens, vdispls, old_vdts, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_create_struct", &mpi_errno);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_create_struct returned: new_vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(new_dtp))));

  fn_return:
    MPIU_CHKLMEM_FREEALL();
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_struct_hook);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_type_struct_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_create_resized_hook([IN] cnt, [IN] old_dt, [OUT] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_create_resized_hook
int mpig_cm_vmpi_type_create_resized_hook(const MPI_Datatype old_dt, const MPI_Aint lb, const MPI_Aint extent,
    MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t old_vdt;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_create_resized_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_create_resized_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: old_dt=" MPIG_HANDLE_FMT ", lb=" MPIG_AINT_FMT
	", extent=" MPIG_AINT_FMT ",new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, old_dt, lb, extent, new_dtp->handle,
	MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);
    
#   if defined(HAVE_VMPI_TYPE_CREATE_RESIZED)
    {
	vrc = mpig_vmpi_type_create_resized(old_vdt, lb, extent, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Datatype_create_resized", &mpi_errno);
    }
#   else
    {
	mpig_vmpi_aint_t old_lb;
	mpig_vmpi_aint_t old_ub;

	vrc = mpig_vmpi_type_lb(old_vdt, &old_lb);
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_lb", &mpi_errno);
	vrc = mpig_vmpi_type_ub(old_vdt, &old_ub);
	MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_ub", &mpi_errno);

	if (lb <= old_lb && lb + extent >= old_ub)
	{
	    int blklens[3];
	    mpig_vmpi_aint_t displs[3];
	    mpig_vmpi_datatype_t dts[3];

	    blklens[0] = 1;
	    displs[0] = lb;
	    dts[0] = MPIG_VMPI_LB;
	    
	    blklens[1] = 1;
	    displs[1] = lb + extent;
	    dts[1] = MPIG_VMPI_UB;
	    
	    blklens[2] = 1;
	    displs[2] = 0;
	    dts[2] = old_vdt;

	    vrc = mpig_vmpi_type_create_struct(3, blklens, displs, dts, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
	    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_create_struct", &mpi_errno);
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DT, "ERROR: vendor MPI does not support "
		"MPI_Type_create_resized, which is required for the requested adjustment: old_dt=" MPIG_HANDLE_FMT ", old_lb="
		MPIG_AINT_FMT ", old_ub=" MPIG_AINT_FMT ", new_lb=" MPIG_AINT_FMT ", new_extent=" MPIG_AINT_FMT ", new_dt="
		MPIG_HANDLE_FMT ", new_dtp=", old_dt, old_lb, old_ub, lb, extent, new_dtp->handle, MPIG_PTR_CAST(new_dtp)));
	    MPIU_ERR_SET1(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|dt_resize_bounds",
		"**globus|cm_vmpi|dt_resize_bounds %D", old_dt);
	}
    }
#   endif
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Datatype_create_resized", &mpi_errno);
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_create_resized_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_create_resized_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_dup_hook([IN] old_dt, [IN/MOD] new_dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_dup_hook
int mpig_cm_vmpi_type_dup_hook(const MPI_Datatype old_dt, MPID_Datatype * const new_dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t old_vdt;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_dup_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_dup_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: old_dt=" MPIG_HANDLE_FMT ", new_dt="
	MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT, old_dt, new_dtp->handle, MPIG_PTR_CAST(new_dtp)));

    mpig_cm_vmpi_datatype_get_vdt(old_dt, &old_vdt);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_dup: old_vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(old_vdt)));
    vrc = mpig_vmpi_type_dup(old_vdt, mpig_cm_vmpi_dtp_get_vdt_ptr(new_dtp));
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_dup", &mpi_errno);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_dup returned: new_vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(new_dtp))));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: new_dt=" MPIG_HANDLE_FMT ", new_dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, new_dtp->handle, MPIG_PTR_CAST(new_dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_dup_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_dup_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_commit_hook([IN/MOD] dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_commit_hook
int mpig_cm_vmpi_type_commit_hook(MPID_Datatype * const dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_commit_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_commit_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: dt=" MPIG_HANDLE_FMT ", dtp=" MPIG_PTR_FMT,
	dtp->handle, MPIG_PTR_CAST(dtp)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_commit: vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(dtp))));

    vrc = mpig_vmpi_type_commit(mpig_cm_vmpi_dtp_get_vdt_ptr(dtp));
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_commit", &mpi_errno);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_commit returned: vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(dtp))));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting: dt=" MPIG_HANDLE_FMT ", dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, dtp->handle, MPIG_PTR_CAST(dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_commit_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_commit_hook() */


/*
 * <mpi_errno> mpig_cm_vmpi_type_free_hook([IN/MOD] dtp)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_type_free_hook
int mpig_cm_vmpi_type_free_hook(MPID_Datatype * const dtp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_type_free_hook);

    MPIG_UNUSED_VAR(fcname);
    
    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_type_free_hook);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "entering: dt=" MPIG_HANDLE_FMT ", dtp=" MPIG_PTR_FMT,
	dtp->handle, MPIG_PTR_CAST(dtp)));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "calling MPIG_Type_free: vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(dtp))));

    vrc = mpig_vmpi_type_free(mpig_cm_vmpi_dtp_get_vdt_ptr(dtp));
    MPIG_ERR_VMPI_CHKANDSET(vrc, "MPI_Type_free", &mpi_errno);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "VMPI_Type_free returned: vdt=" MPIG_VMPI_DATATYPE_FMT,
	MPIG_VMPI_DATATYPE_CAST(mpig_cm_vmpi_dtp_get_vdt(dtp))));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DT, "exiting: dt=" MPIG_HANDLE_FMT ", dtp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, dtp->handle, MPIG_PTR_CAST(dtp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_type_free_hook);
    return mpi_errno;
}
/* mpig_cm_vmpi_type_free_hook() */
/**********************************************************************************************************************************
						       END DATATYPE HOOKS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					  BEGIN MISCELLANEOUS PUBLICLY VISIBLE ROUTINES
**********************************************************************************************************************************/
/*
 * <mpi_errno> int mpig_cm_vmpi_iprobe_any_source(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_iprobe_any_source
int mpig_cm_vmpi_iprobe_any_source(int tag, MPID_Comm * comm, int ctxoff, bool_t * found_p, MPI_Status * status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int vtag = mpig_cm_vmpi_tag_get_vtag(tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(comm, ctxoff);
    mpig_vmpi_status_t vstatus;
    int vfound;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_iprobe_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_iprobe_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: tag=%d, comm=" MPIG_HANDLE_FMT ", commp="
	MPIG_PTR_FMT ", ctx=%d", tag, comm->handle, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    MPIU_Assert(mpig_cm_vmpi_comm_has_remote_vprocs(comm));
    
    vrc = mpig_vmpi_iprobe(MPIG_VMPI_ANY_SOURCE, vtag, vcomm, &vfound, &vstatus);
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Iprobe", &mpi_errno);

    if (vfound && status != MPI_STATUS_IGNORE)
    {
	int count;
	int vstatus_source = mpig_vmpi_status_get_source(&vstatus);
	int vstatus_tag = mpig_vmpi_status_get_tag(&vstatus);

	MPIG_Assert(vstatus_source < mpig_cm_vmpi_comm_get_remote_vsize(comm));
	    
	vrc = mpig_vmpi_get_count(&vstatus, MPIG_VMPI_BYTE, &count);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Get_count", &mpi_errno);
	    
	status->MPI_SOURCE = (vstatus_source >= 0) ? mpig_cm_vmpi_comm_get_remote_mrank(comm, vstatus_source) : MPI_UNDEFINED;
	status->MPI_TAG = (vstatus_tag >= 0) ? vstatus_tag : MPI_UNDEFINED;
	/* NOTE: MPI_ERROR must not be set except by the MPI_Test/Wait routines when MPI_ERR_IN_STATUS is to be returned. */
	status->count = count;
	status->cancelled = FALSE;
	/* status->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
    }

    *found_p = vfound;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: tag=%d, comm=" MPIG_HANDLE_FMT ", commp="
	MPIG_PTR_FMT ", ctx=%d, found=%s, mpi_errno=" MPIG_ERRNO_FMT, tag, comm->handle, MPIG_PTR_CAST(comm), comm->context_id +
	ctxoff, MPIG_BOOL_STR(vfound), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_iprobe_any_source);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_iprobe_any_source() */


/*
 * <mpi_errno> int mpig_cm_vmpi_cancel_recv_any_source(...)
 *
 * MPI-2-FIXME: this routine is not thread safe.  if the underlying vendor MPI is not thread safe, and multiple application
 * threads are allowed, then this routine will have to queue cancel requests so that they can be processed later by the progress
 * engine.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_cancel_recv_any_source
int mpig_cm_vmpi_cancel_recv_any_source(MPID_Request * const ras_req, bool_t * cancelled_p)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t cancelled = FALSE;
    int vcompleted;
    int vcancelled;
    mpig_vmpi_status_t vstatus;
    int vrc;
    int vrc_test;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_cancel_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_cancel_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: ras_req=" MPIG_HANDLE_FMT ", ras_reqp="
	MPIG_PTR_FMT, ras_req->handle, MPIG_PTR_CAST(ras_req)));

    MPIU_Assert(mpig_cm_vmpi_comm_has_remote_vprocs(ras_req->comm));
    MPIU_Assert(mpig_cm_vmpi_thread_is_main());
    
    if (mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE && ras_req->cms.vmpi.cancelling == FALSE)
    {
	vrc = mpig_vmpi_cancel(mpig_cm_vmpi_request_get_vreq_ptr(ras_req));
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Cancel", &mpi_errno);

	mpig_request_set_vc(ras_req, mpig_cm_vmpi_ras_vc);
	ras_req->cms.vmpi.cancelling = TRUE;

	/* do a quick check to see if the request has already completed */
	vrc_test = mpig_vmpi_test(mpig_cm_vmpi_request_get_vreq_ptr(ras_req), &vcompleted, &vstatus);
	if (vrc_test && mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE)
	{   /* --BEGIN ERROR HANDLING-- */
	    /* if an error occurred and the request was not completed, then report the error immediately.  if the request was
	       complete, then it will be reported below. */
	    MPIG_ERR_VMPI_SET(vrc, "MPI_Test", &mpi_errno);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    
	if (vcompleted)
	{
	    mpig_cm_vmpi_pe_table_remove_req(ras_req);
	    mpig_cm_vmpi_pe_ras_active_count -= 1;
	    
	    vrc = mpig_vmpi_test_cancelled(&vstatus, &vcancelled);
	    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Test_cancelled", &mpi_errno);
	    
	    if (!vcancelled)
	    {
		mpig_cm_vmpi_request_status_vtom(ras_req, &vstatus, vrc_test);
		mpig_request_complete(ras_req);
		/* mpig_pe_end_ras_op() should only be called when the request wasn't cancelled.  in the case where it is
		   cancelled, mpig_adi3_cancel_recv() will make the call, and it must do so since it doesn't know that vendor MPI
		   exists. */
		mpig_pe_end_ras_op();
	    }
	    else
	    {
		cancelled = TRUE;
	    }
	}
    }
    
    *cancelled_p = cancelled;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: ras_req=" MPIG_HANDLE_FMT ", ras_reqp="
	MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, ras_req->handle, MPIG_PTR_CAST(ras_req), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_cancel_recv_any_source);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_cancel_recv_any_source() */


/*
 * <mpi_errno> int mpig_cm_vmpi_register_recv_any_source(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_register_recv_any_source
int mpig_cm_vmpi_register_recv_any_source(const mpig_msg_op_params_t * const ras_params, bool_t * const found_p,
    MPID_Request * const ras_req)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_vmpi_datatype_t vdt;
    const int vtag = mpig_cm_vmpi_tag_get_vtag(ras_params->tag);
    const mpig_vmpi_comm_t vcomm = mpig_cm_vmpi_comm_get_vcomm(ras_params->comm, ras_params->ctxoff);
    bool_t found = FALSE;
    int vfound;
    mpig_vmpi_status_t vstatus;
    int vrc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_register_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_register_recv_any_source)
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: tag=%d, comm=" MPIG_HANDLE_FMT ", commp="
	MPIG_PTR_FMT ", ctx=%d", ras_params->tag, ras_params->comm->handle, MPIG_PTR_CAST(ras_params->comm),
	ras_params->comm->context_id + ras_params->ctxoff));

    MPIU_Assert(mpig_cm_vmpi_comm_has_remote_vprocs(ras_params->comm));
    MPIU_Assert(mpig_cm_vmpi_thread_is_main());
    
    mpig_cm_vmpi_datatype_get_vdt(ras_params->dt, &vdt);
    
    /* initialize the vendor MPI structure within the supplied request */
    mpig_cm_vmpi_request_construct(ras_req);

    /* start the nonblocking receive */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "starting a nonblocking recv"));
    vrc = mpig_vmpi_irecv(ras_params->buf, ras_params->cnt, vdt, MPIG_VMPI_ANY_SOURCE, vtag, vcomm,
	mpig_cm_vmpi_request_get_vreq_ptr(ras_req));
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Irecv", &mpi_errno);

    /* do a quick check to see if the request has already completed */
    vrc = mpig_vmpi_test(mpig_cm_vmpi_request_get_vreq_ptr(ras_req), &vfound, &vstatus);
    if (vrc && mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE)
    {
	/* if an error occurred and the request was not completed, then report the error immediately.  if the request was
	   complete, then it will be reported below. */
	MPIG_ERR_VMPI_SET(vrc, "MPI_Test", &mpi_errno);
	goto fn_fail;
    }
    else if (vfound)
    {
	/* if the envelope matched a message already received via vendor MPI, then finish constructing the request and tell the
	   calling routine that a matching request was found. */
	const int vrank = mpig_vmpi_status_get_source(&vstatus);
	const int mrank = mpig_cm_vmpi_comm_get_remote_mrank(ras_params->comm, vrank);
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(ras_params->comm, mrank);
	
	mpig_request_construct_irreq(ras_req, 1, 0, ras_params->buf, ras_params->cnt, ras_params->dt,
	    ras_params->rank, ras_params->tag, ras_params->comm->context_id + ras_params->ctxoff, ras_params->comm, vc);
	mpig_cm_vmpi_request_status_vtom(ras_req, &vstatus, vrc);
	/* mpig_request_complete(ras_req); -- completion counter set to zero during request construction above */
	found = TRUE;
    }
    else
    {
	/* if a vendor MPI receive operation was posted but not matched, then add the outstanding request to the progress engine
	   request tracking data structures. */
	mpig_cm_vmpi_pe_table_add_req(ras_req, &mpi_errno);
	if (mpi_errno)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: attempt to register a request with the "
		"VMPI process engine failed: ras_req" MPIG_HANDLE_FMT ", ras_reqp=" MPIG_PTR_FMT, ras_req->handle,
		MPIG_PTR_CAST(ras_req)));
	    MPIU_ERR_SET2(mpi_errno, MPI_ERR_OTHER, "**globus|cm_vmpi|pe_table_add_req",
		"**globus|cm_vmpi|pe_table_add_req %R %p", ras_req->handle, ras_req);
	    goto fn_fail;
	}

	mpig_cm_vmpi_pe_ras_active_count += 1;
    }

    *found_p = found;
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: tag=%d, comm=" MPIG_HANDLE_FMT ", commp="
	MPIG_PTR_FMT ", ctx=%d, found=%s, req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, ras_params->tag, ras_params->comm->handle,
	MPIG_PTR_CAST(ras_params->comm), ras_params->comm->context_id + ras_params->ctxoff, MPIG_BOOL_STR(found),
	MPIG_HANDLE_VAL(ras_req), MPIG_PTR_CAST(ras_req)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_register_recv_any_source);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_register_recv_any_source() */


/*
 * <mpi_errno> int mpig_cm_vmpi_unregister_recv_any_source(...)
 *
 * NOTE: if the request completed during the unregister process, and the application has released its reference to the request,
 * then the request object wil be destroyed.  as a result, the calling routine should assume the request object is invalid if the
 * completed flag in the cancel RAS structure is set to true.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_unregister_recv_any_source
int mpig_cm_vmpi_unregister_recv_any_source(mpig_recvq_ras_op_t * const ras_op)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * ras_req = mpig_recvq_ras_op_get_rreq(ras_op);
    int vcompleted;
    int vcancelled;
    mpig_vmpi_status_t vstatus;
    int vrc;
    int vrc_test;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_unregister_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_unregister_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: ras_op=" MPIG_PTR_FMT ", ras_req="
	MPIG_HANDLE_FMT ", ras_reqp=" MPIG_PTR_FMT, MPIG_PTR_CAST(ras_op), ras_req->handle, MPIG_PTR_CAST(ras_req)));

    MPIU_Assert(mpig_cm_vmpi_comm_has_remote_vprocs(ras_req->comm));
    MPIU_Assert(mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE);
    MPIU_Assert(mpig_cm_vmpi_thread_is_main());

    vrc = mpig_vmpi_cancel(mpig_cm_vmpi_request_get_vreq_ptr(ras_req));
    MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Cancel", &mpi_errno);

    ras_req->cms.vmpi.cancelling = TRUE;

    /* XXX: this is a major hack to insure the vendor request completes before returning to
       mpig_recvq_deq_posted_or_enq_unexp(). this will only work for multithreaded builds since XIO will make progress
       independent of the main thread.  the right way to solve this is to use a single threaded callback space in which the
       communication modules register callbacks to make progress.  the callback space would be associated with the condition
       variable if the cond_wait was to occur in the main thread, thus insuring progress would occur for all communication
       modules with outstanding requests eventhough the main thread was being blocked.  unfortunately, I don't have time to deal
       with that right now, so instead a VMPI_Wait is performed if no condition variable is associated with the ras_op.  right
       now, that condition only occurs if this routine is called from the main thread. */
    if (ras_op->cond_in_use)
    {
	/* do a quick check to see if the request has already completed */
	vrc_test = mpig_vmpi_test(mpig_cm_vmpi_request_get_vreq_ptr(ras_req), &vcompleted, &vstatus);
	if (vrc_test && mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE)
	{   /* --BEGIN ERROR HANDLING-- */
	    /* if an error occurred and the request was not completed, then report the error immediately.  if the request was
	       complete, then it will be reported below. */
	    MPIG_ERR_VMPI_SET(vrc, "MPI_Test", &mpi_errno);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */
    }
    else
    {
	vrc_test = mpig_vmpi_wait(mpig_cm_vmpi_request_get_vreq_ptr(ras_req), &vstatus);
	if (vrc_test && mpig_vmpi_request_is_null(mpig_cm_vmpi_request_get_vreq(ras_req)) == FALSE)
	{   /* --BEGIN ERROR HANDLING-- */
	    /* if an error occurred and the request was not completed, then report the error immediately.  if the request was
	       complete, then it will be reported below. */
	    MPIG_ERR_VMPI_SET(vrc, "MPI_Wait", &mpi_errno);
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	vcompleted = TRUE;
    }
    
    if (vcompleted)
    {
	mpig_cm_vmpi_pe_table_remove_req(ras_req);
	mpig_cm_vmpi_pe_ras_active_count -= 1;
	    
	vrc = mpig_vmpi_test_cancelled(&vstatus, &vcancelled);
	MPIG_ERR_VMPI_CHKANDJUMP(vrc, "MPI_Test_cancelled", &mpi_errno);
	
	if (!vcancelled)
	{
	    const int vrank = mpig_vmpi_status_get_source(&vstatus);
	    const int mrank = mpig_cm_vmpi_comm_get_remote_mrank(ras_req->comm, vrank);
	    mpig_vc_t * const vc = mpig_comm_get_remote_vc(ras_req->comm, mrank);

	    mpig_request_set_vc(ras_req, vc);
	    mpig_cm_vmpi_request_status_vtom(ras_req, &vstatus, vrc_test);
	    mpig_pe_end_ras_op();
	    mpig_request_complete(ras_req);
	    ras_req = NULL;
	}
	else
	{
	    /* another communication module will take responsibility for this request.  in that case, we do not want to
	       decrement the request's completion counter or the reference count. */
	    mpig_recvq_ras_op_set_cancelled_state(ras_op, TRUE);
	}
	
	mpig_recvq_ras_op_signal_complete(ras_op);
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: ras_op=" MPIG_PTR_FMT ",ras_req=" MPIG_HANDLE_FMT
	", ras_reqp=" MPIG_PTR_FMT ", completed=%s, cancelled=%s", MPIG_PTR_CAST(ras_op), MPIG_HANDLE_VAL(ras_req),
	MPIG_PTR_CAST(ras_req), MPIG_BOOL_STR(ras_op->complete), MPIG_BOOL_STR(ras_op->cancelled)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_unregister_recv_any_source);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_vmpi_unregister_recv_any_source() */


/*
 * int mpig_cm_vmpi_error_class_vtom([IN] vendor_class)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_vmpi_error_class_vtom
int mpig_cm_vmpi_error_class_vtom(int vendor_class)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpich_class;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_vmpi_error_class_vtom);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_vmpi_error_class_vtom);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vendor_class=%d", vendor_class));

    if (vendor_class == MPIG_VMPI_SUCCESS)
    {
	mpich_class = MPI_SUCCESS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BUFFER)
    {
	mpich_class = MPI_ERR_BUFFER;
    }
    else if (vendor_class == MPIG_VMPI_ERR_COUNT)
    {
	mpich_class = MPI_ERR_COUNT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TYPE)
    {
	mpich_class = MPI_ERR_TYPE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TAG)
    {
	mpich_class = MPI_ERR_TAG;
    }
    else if (vendor_class == MPIG_VMPI_ERR_COMM)
    {
	mpich_class = MPI_ERR_COMM;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RANK)
    {
	mpich_class = MPI_ERR_RANK;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ROOT)
    {
	mpich_class = MPI_ERR_ROOT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TRUNCATE)
    {
	mpich_class = MPI_ERR_TRUNCATE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_GROUP)
    {
	mpich_class = MPI_ERR_GROUP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_OP)
    {
	mpich_class = MPI_ERR_OP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_REQUEST)
    {
	mpich_class = MPI_ERR_REQUEST;
    }
    else if (vendor_class == MPIG_VMPI_ERR_TOPOLOGY)
    {
	mpich_class = MPI_ERR_TOPOLOGY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DIMS)
    {
	mpich_class = MPI_ERR_DIMS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ARG)
    {
	mpich_class = MPI_ERR_ARG;
    }
    else if (vendor_class == MPIG_VMPI_ERR_OTHER)
    {
	mpich_class = MPI_ERR_OTHER;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNKNOWN)
    {
	mpich_class = MPI_ERR_UNKNOWN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INTERN)
    {
	mpich_class = MPI_ERR_INTERN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_IN_STATUS)
    {
	mpich_class = MPI_ERR_IN_STATUS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_PENDING)
    {
	mpich_class = MPI_ERR_PENDING;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE)
    {
	mpich_class = MPI_ERR_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ACCESS)
    {
	mpich_class = MPI_ERR_ACCESS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_AMODE)
    {
	mpich_class = MPI_ERR_AMODE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BAD_FILE)
    {
	mpich_class = MPI_ERR_BAD_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE_EXISTS)
    {
	mpich_class = MPI_ERR_FILE_EXISTS;
    }
    else if (vendor_class == MPIG_VMPI_ERR_FILE_IN_USE)
    {
	mpich_class = MPI_ERR_FILE_IN_USE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_SPACE)
    {
	mpich_class = MPI_ERR_NO_SPACE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_SUCH_FILE)
    {
	mpich_class = MPI_ERR_NO_SUCH_FILE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_IO)
    {
	mpich_class = MPI_ERR_IO;
    }
    else if (vendor_class == MPIG_VMPI_ERR_READ_ONLY)
    {
	mpich_class = MPI_ERR_READ_ONLY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_CONVERSION)
    {
	mpich_class = MPI_ERR_CONVERSION;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DUP_DATAREP)
    {
	mpich_class = MPI_ERR_DUP_DATAREP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNSUPPORTED_DATAREP)
    {
	mpich_class = MPI_ERR_UNSUPPORTED_DATAREP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO)
    {
	mpich_class = MPI_ERR_INFO;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_KEY)
    {
	mpich_class = MPI_ERR_INFO_KEY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_VALUE)
    {
	mpich_class = MPI_ERR_INFO_VALUE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_INFO_NOKEY)
    {
	mpich_class = MPI_ERR_INFO_NOKEY;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NAME)
    {
	mpich_class = MPI_ERR_NAME;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NO_MEM)
    {
	mpich_class = MPI_ERR_NO_MEM;
    }
    else if (vendor_class == MPIG_VMPI_ERR_NOT_SAME)
    {
	mpich_class = MPI_ERR_NOT_SAME;
    }
    else if (vendor_class == MPIG_VMPI_ERR_PORT)
    {
	mpich_class = MPI_ERR_PORT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_QUOTA)
    {
	mpich_class = MPI_ERR_QUOTA;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SERVICE)
    {
	mpich_class = MPI_ERR_SERVICE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SPAWN)
    {
	mpich_class = MPI_ERR_SPAWN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_UNSUPPORTED_OPERATION)
    {
	mpich_class = MPI_ERR_UNSUPPORTED_OPERATION;
    }
    else if (vendor_class == MPIG_VMPI_ERR_WIN)
    {
	mpich_class = MPI_ERR_WIN;
    }
    else if (vendor_class == MPIG_VMPI_ERR_BASE)
    {
	mpich_class = MPI_ERR_BASE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_LOCKTYPE)
    {
	mpich_class = MPI_ERR_LOCKTYPE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_KEYVAL)
    {
	mpich_class = MPI_ERR_KEYVAL;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RMA_CONFLICT)
    {
	mpich_class = MPI_ERR_RMA_CONFLICT;
    }
    else if (vendor_class == MPIG_VMPI_ERR_RMA_SYNC)
    {
	mpich_class = MPI_ERR_RMA_SYNC;
    }
    else if (vendor_class == MPIG_VMPI_ERR_SIZE)
    {
	mpich_class = MPI_ERR_SIZE;
    }
    else if (vendor_class == MPIG_VMPI_ERR_DISP)
    {
	mpich_class = MPI_ERR_DISP;
    }
    else if (vendor_class == MPIG_VMPI_ERR_ASSERT)
    {
	mpich_class = MPI_ERR_ASSERT;
    }
    else
    {
	mpich_class = MPI_ERR_UNKNOWN;
    }
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpich_class=%d", mpich_class));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_vmpi_error_class_vtom);
    return mpich_class;
}
/* mpig_cm_vmpi_error_class_vtom() */
/**********************************************************************************************************************************
					   END MISCELLANEOUS PUBLICLY VISIBLE ROUTINES
**********************************************************************************************************************************/


#endif /* defined(MPIG_VMPI) */
