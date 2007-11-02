/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#include "mpidimpl.h"

/**********************************************************************************************************************************
							BEGIN PARAMETERS
**********************************************************************************************************************************/
#if !defined(MPIG_COPY_BUFFER_SIZE)
#define MPIG_COPY_BUFFER_SIZE (256*1024)
#endif
/**********************************************************************************************************************************
							 END PARAMETERS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
				      BEGIN MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/
static mpig_vc_t * mpig_cm_self_vc = NULL;

static int mpig_cm_self_send(
    mpig_request_type_t type, const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, int ctx, MPID_Comm * comm,
    MPID_Request ** sreqp);

static void mpig_cm_self_buffer_copy(
    const void * const sbuf, int scnt, MPI_Datatype sdt, int * smpi_errno,
    void * const rbuf, int rcnt, MPI_Datatype rdt, MPIU_Size_t * rsz, int * rmpi_errno);
/**********************************************************************************************************************************
				       END MISCELLANEOUS MACROS, PROTOTYPES, AND VARIABLES
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					      BEGIN COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_self_init(mpig_cm_t * cm, int * argc, char *** argv);

static int mpig_cm_self_finalize(mpig_cm_t * cm);

static int mpig_cm_self_add_contact_info(mpig_cm_t * cm, mpig_bc_t * bc);

static int mpig_cm_self_construct_vc_contact_info(mpig_cm_t * cm, mpig_vc_t * vc);

static void mpig_cm_self_destruct_vc_contact_info(mpig_cm_t * cm, mpig_vc_t * vc);

static int mpig_cm_self_select_comm_method(mpig_cm_t * cm, mpig_vc_t * vc, bool_t * selected);

#if FALSE
static int mpig_cm_self_get_vc_compatability(mpig_cm_t * cm, const mpig_vc_t * vc1, const mpig_vc_t * vc2,
    unsigned levels_in, unsigned * levels_out);
#endif

static mpig_cm_vtable_t mpig_cm_self_vtable =
{
    mpig_cm_self_init,
    mpig_cm_self_finalize,
    mpig_cm_self_add_contact_info,
    mpig_cm_self_construct_vc_contact_info,
    mpig_cm_self_destruct_vc_contact_info,
    mpig_cm_self_select_comm_method,
    NULL, /*mpig_cm_self_get_vc_compatability */
    mpig_cm_vtable_last_entry
};

mpig_cm_t mpig_cm_self =
{
    MPIG_CM_TYPE_SELF,
    "self",
    &mpig_cm_self_vtable
};
/**********************************************************************************************************************************
					       END COMMUNICATION MODULE API VTABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN VC CORE API VTABLE
**********************************************************************************************************************************/
static int mpig_cm_self_adi3_send(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_isend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_rsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_irsend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_ssend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_issend(
    const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPID_Request ** sreqp);

static int mpig_cm_self_adi3_recv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status,
    MPID_Request ** rreqp);

static int mpig_cm_self_adi3_irecv(
    void * buf, int cnt, MPI_Datatype dt, int rank, int tag, MPID_Comm * comm,int ctxoff, MPID_Request ** rreqp);

static int mpig_cm_self_adi3_probe(int rank, int tag, MPID_Comm * comm, int ctxoff, MPI_Status * status);

static int mpig_cm_self_adi3_iprobe(int rank, int tag, MPID_Comm * comm, int ctxoff, int * flag_p, MPI_Status * status);

static int mpig_cm_self_adi3_cancel_send(MPID_Request * sreq);

static int mpig_cm_self_vc_recv_any_source(mpig_vc_t * vc, MPID_Request * rreq);


MPIG_STATIC mpig_vc_vtable_t mpig_cm_self_vc_vtable =
{
    mpig_cm_self_adi3_send,
    mpig_cm_self_adi3_isend,
    mpig_cm_self_adi3_rsend,
    mpig_cm_self_adi3_irsend,
    mpig_cm_self_adi3_ssend,
    mpig_cm_self_adi3_issend,
    mpig_cm_self_adi3_recv,
    mpig_cm_self_adi3_irecv,
    mpig_cm_self_adi3_probe,
    mpig_cm_self_adi3_iprobe,
    mpig_adi3_cancel_recv,
    mpig_cm_self_adi3_cancel_send,
    mpig_cm_self_vc_recv_any_source,
    NULL, /* vc_inc_ref_count */
    NULL, /* vc_dec_ref_count */
    NULL, /* vc_destruct */
    mpig_vc_vtable_last_entry
};
/**********************************************************************************************************************************
						     END VC CORE API VTABLE
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					    BEGIN COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * <mpi_errno> mpig_cm_self_initialize([IN/OUT] argc, [IN/OUT] argv)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_init
static int mpig_cm_self_init(mpig_cm_t * const cm, int * const argc, char *** const argv)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_init);
    
    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_init);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    MPIU_ERR_CHKANDJUMP((strlen(mpig_process.my_hostname) == 0), mpi_errno, MPI_ERR_OTHER, "**globus|cm_self|hostname");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_init);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_init() */


/*
 * <mpi_errno> mpig_cm_self_finalize(void)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_finalize
static int mpig_cm_self_finalize(mpig_cm_t * const cm)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_finalize);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_finalize);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    /* release the PG reference held by the VC */
    if (mpig_cm_self_vc != NULL)
    {
	mpig_vc_release_ref(mpig_cm_self_vc);
	mpig_cm_self_vc = MPIG_INVALID_PTR;
    }

    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_finalize);
    return mpi_errno;
}
/* mpig_cm_self_finalize() */


/*
 * <mpi_errno> mpig_cm_self_add_contact_info([IN/MOD] bc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_add_contact_info
static int mpig_cm_self_add_contact_info(mpig_cm_t * const cm, mpig_bc_t * const bc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    char pid[64];
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_add_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_add_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));

    mpi_errno = mpig_bc_add_contact(bc, "CM_SELF_HOSTNAME", mpig_process.my_hostname);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	"**globus|bc_add_contact %s", "CM_SELF_HOSTNAME");

    MPIU_Snprintf(pid, (size_t) 64, "%lu", (unsigned long) mpig_process.my_pid);
    mpi_errno = mpig_bc_add_contact(bc, "CM_SELF_PID", pid);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_add_contact",
	"**globus|bc_add_contact %s", "CM_SELF_PID");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: mpi_errno=" MPIG_ERRNO_FMT, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_add_contact_info);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_add_contact_info() */


/*
 * <mpi_errno> mpig_cm_self_construct_vc_contact_info([IN/MOD] vc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_construct_vc_contact_info
static int mpig_cm_self_construct_vc_contact_info(mpig_cm_t * const cm, mpig_vc_t * const vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    mpig_bc_t * bc;
    bool_t found;
    char * hostname_str = NULL;
    char * pid_str = NULL;
    unsigned long pid;
    int rc;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_construct_vc_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_construct_vc_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    vc->cms.self.hostname = NULL;
    vc->cms.self.pid = 0;

    bc = mpig_vc_get_bc(vc);

    /* extract the hostname from the business card */
    mpi_errno = mpig_bc_get_contact(bc, "CM_SELF_HOSTNAME", &hostname_str, &found);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact", "**globus|bc_get_contact %s",
	"CM_SELF_HOSTNAME");
    if (!found) goto fn_return;

    /* extract the process id from the business card */
    mpi_errno = mpig_bc_get_contact(bc, "CM_SELF_PID", &pid_str, &found);
    MPIU_ERR_CHKANDJUMP1((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|bc_get_contact",
	"**globus|bc_get_contact %s", "CM_SELF_PID");
    if (!found) goto fn_return;

    rc = sscanf(pid_str, "%lu", &pid);
    MPIU_ERR_CHKANDJUMP((rc != 1), mpi_errno, MPI_ERR_INTERN, "**keyval");

    /* if all when well, copy the extracted contact information into the VC */
    vc->cms.self.hostname = MPIU_Strdup(hostname_str);
    MPIU_ERR_CHKANDJUMP1((vc->cms.self.hostname == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s",
	"name of remote host");
    vc->cms.self.pid = pid;

#if FALSE
    /* set the topology information.  NOTE: this may seem a bit wacky since the PROC level is set even if the SELF module is
       not responsible for the VC; however, the topology information is defined such that a level set if it is _possible_ for
       the module to perform the communication regardless of whether it does so or not. */
    vc->cms.topology_levels |= MPIG_TOPOLOGY_LEVEL_PROC_MASK;
    if (vc->cms.topology_num_levels <= MPIG_TOPOLOGY_LEVEL_PROC)
    {
	vc->cms.topology_num_levels = MPIG_TOPOLOGY_LEVEL_PROC + 1;
    }
#endif
    
  fn_return:
    if (hostname_str != NULL) mpig_bc_free_contact(hostname_str);
    if (pid_str != NULL) mpig_bc_free_contact(pid_str);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc),
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_construct_vc_contact_info);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	MPIU_Free(vc->cms.self.hostname);
	vc->cms.self.pid = 0;
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_construct_vc_contact_info() */


/*
 * <mpi_errno> mpig_cm_self_destruct_vc_contact_info([IN/MOD] vc)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_destruct_vc_contact_info
static void mpig_cm_self_destruct_vc_contact_info(mpig_cm_t * const cm, mpig_vc_t * const vc)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_destruct_vc_contact_info);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_destruct_vc_contact_info);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    MPIU_Free(vc->cms.self.hostname);
    vc->cms.self.hostname = MPIG_INVALID_PTR;
    vc->cms.self.pid = 0;
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_destruct_vc_contact_info);
    return;
}
/* mpig_cm_self_destruct_vc_contact_info() */


/*
 * <mpi_errno> mpig_cm_self_select_comm_method([IN/MOD] vc, [OUT] selected)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_select_comm_method
static int mpig_cm_self_select_comm_method(mpig_cm_t * const cm, mpig_vc_t * const vc, bool_t * const selected)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t vc_was_inuse;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_select_comm_method);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_select_comm_method);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc)));

    *selected = FALSE;

    /* determine if communication is possible */
    if (mpig_vc_get_cm(vc) != NULL) goto fn_return;
    if (vc->cms.self.hostname == NULL || strcmp(vc->cms.self.hostname, mpig_process.my_hostname) != 0) goto fn_return;
    if (vc->cms.self.pid != (unsigned long) mpig_process.my_pid) goto fn_return;

    /* initialize the CM self fields in the VC object */
    mpig_vc_set_cm(vc, cm);
    mpig_vc_set_vtable(vc, &mpig_cm_self_vc_vtable);

    /* set the selected flag to indicate that the "self" communication module has accepted responsibility for the VC */
    *selected = TRUE;

    /* store a copy of the VC, adjusting the VC and PG reference counts as necessary */
    mpig_cm_self_vc = vc;
    mpig_vc_inc_ref_count(vc, &vc_was_inuse);
    if (vc_was_inuse == FALSE)
    {
	mpig_pg_inc_ref_count(mpig_vc_get_pg(vc));
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc=" MPIG_PTR_FMT ", selected=%s, mmpi_errno=" MPIG_ERRNO_FMT,
	MPIG_PTR_CAST(vc), MPIG_BOOL_STR(*selected), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_select_comm_method);
    return mpi_errno;
}
/* mpig_cm_self_select_comm_method() */


#if FALSE
/*
 * <mpi_errno> mpig_cm_self_get_vc_compatability([IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out)
 *
 * see documentation in mpidpre.h.
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_get_vc_compatability
static int mpig_cm_self_get_vc_compatability(mpig_cm_t * const cm, const mpig_vc_t * const vc1,
    const mpig_vc_t * const vc2, unsigned levels_in, unsigned * const levels_out)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_get_vc_compatability);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_get_vc_compatability);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_in=0x%08x",
	MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), levels_in));

    *levels_out = 0;

    if (levels_in & MPIG_TOPOLOGY_LEVEL_PROC_MASK)
    {
	if (vc1->ci.self.hostname != NULL && vc2->ci.self.hostname != NULL &&
	    strcmp(vc1->ci.self.hostname, vc2->ci.self.hostname) == 0 && vc1->ci.self.pid == vc2->ci.self.pid)
	{
	    *levels_out |= MPIG_TOPOLOGY_LEVEL_PROC_MASK;
	}
    }
    
  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting: vc1=" MPIG_PTR_FMT ", vc2=" MPIG_PTR_FMT ", levels_out=0x%08x, "
	"mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc1), MPIG_PTR_CAST(vc2), *levels_out, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_get_vc_compatability);
    return;
}
/* mpig_cm_self_get_vc_compatability() */
#endif /* FALSE */
/**********************************************************************************************************************************
					     END COMMUNICATION MODULE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN VC CORE API FUNCTIONS
**********************************************************************************************************************************/
/*
 * int mpig_cm_self_adi3_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_send
static int mpig_cm_self_adi3_send(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_send(...) */


/*
 * int mpig_cm_self_adi3_isend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_isend
static int mpig_cm_self_adi3_isend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_isend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_isend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_isend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_isend() */


/*
 * int mpig_cm_self_adi3_rsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_rsend
static int mpig_cm_self_adi3_rsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_rsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_rsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_RSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_rsend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_rsend() */


/*
 * int mpig_cm_self_adi3_irsend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_irsend
static int mpig_cm_self_adi3_irsend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_irsend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_irsend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_RSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_irsend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_irsend() */


/*
 * int mpig_cm_self_adi3_ssend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_ssend
static int mpig_cm_self_adi3_ssend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_ssend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_ssend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);
#   if (MPICH_THREAD_LEVEL < MPI_THREAD_MULTIPLE)
    {
	MPIU_ERR_CHKANDJUMP(((*sreqp)->cc != 0), mpi_errno, MPI_ERR_OTHER, "**dev|selfsenddeadlock");
    }
#   endif

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_ssend);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_ssend() */


/*
 * int mpig_cm_self_adi3_issend(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_issend
static int mpig_cm_self_adi3_issend(
    const void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_issend);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_issend);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    mpi_errno = mpig_cm_self_send(MPIG_REQUEST_TYPE_SSEND, buf, cnt, dt, rank, tag, ctx, comm, sreqp);

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_issend);
    return mpi_errno;
}
/* mpig_cm_self_adi3_issend() */


/*
 * int mpig_cm_self_adi3_recv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_recv
static int mpig_cm_self_adi3_recv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPI_Status * const status, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_recv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_recv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), comm->context_id + ctxoff));

    mpi_errno = mpig_cm_self_adi3_irecv(buf, cnt, dt, rank, tag, comm, ctxoff, rreqp);
    /* the status will be extracted by MPI_Recv() once the request is complete */
    
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp), MPIG_PTR_CAST(*rreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_recv);
    return mpi_errno;
}
/* int mpig_cm_self_adi3_recv() */


/*
 * int mpig_cm_self_adi3_irecv(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_irecv
static int mpig_cm_self_adi3_irecv(
    void * const buf, const int cnt, const MPI_Datatype dt, const int rank, const int tag, MPID_Comm * const comm,
    const int ctxoff, MPID_Request ** const rreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    MPID_Request * rreq;
    const int ctx = comm->context_id + ctxoff;
    int rreq_found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_irecv);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_irecv);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm=" MPIG_PTR_FMT ", ctx=%d",
	MPIG_PTR_CAST(buf), cnt, dt, rank, tag, MPIG_PTR_CAST(comm), ctx));

    MPIU_Assert(rank == comm->rank);
    
    mpi_errno = mpig_recvq_deq_unexp_or_enq_posted(rank, tag, ctx, NULL, &rreq_found, &rreq);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: receive queue operation failed; dequeue "
	    "unexpected or enqueue posted: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
	MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|recvq_deq_unexp_or_enq_posted",
	    "**globus|recvq_deq_unexp_or_enq_posted %d %d %d", rank, tag, ctx);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    if (rreq_found)
    {
	void * sreq_buf;
	int sreq_cnt;
	MPI_Datatype sreq_dt;
	MPIU_Size_t data_size;

	/* finish filling in the request fields */
	mpig_request_set_buffer(rreq, buf, cnt, dt);
        mpig_request_add_comm_ref(rreq, comm); /* used by MPI layer and ADI3 selection mechanism (in mpidpost.h), most notably
						  by the receive cancel routine */
	
	sreq = rreq->partner_request;
	MPIU_Assertp(sreq != NULL);

	if (rreq->status.MPI_ERROR == MPI_SUCCESS)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request found in unexpected queue; copying data: rreq=" MPIG_HANDLE_FMT
		", rreqp=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq),
		sreq->handle, MPIG_PTR_CAST(sreq)));

	    mpig_request_get_buffer(sreq, &sreq_buf, &sreq_cnt, &sreq_dt);
	    mpig_cm_self_buffer_copy(sreq_buf, sreq_cnt, sreq_dt, &sreq->status.MPI_ERROR,
				     buf, cnt, dt, &data_size, &rreq->status.MPI_ERROR);
	
	    rreq->status.count = (int) data_size;
	    /* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	}
	else
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request found in unexpected queue; rreq has error set; "
		"skipping data copy: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp="
		MPIG_PTR_FMT, rreq->handle, MPIG_PTR_CAST(rreq), sreq->handle, MPIG_PTR_CAST(sreq)));
	
	    rreq->status.count = (int) 0;
	    /* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	}
    
	/* neither the application nor any other part of MPICH has a handle to the receive request, so it is safe to just reset
	   the reference count and completion counter */
	mpig_request_set_ref_count(rreq, 1);
	mpig_request_set_cc(rreq, 0);
	
	mpig_request_complete(sreq);
    }
    else
    {
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(comm, rank);
	
	/* the message has yet to arrived.  the request has been placed on the posted receive queue. */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request allocated in posted queue: req=" MPIG_HANDLE_FMT ", ptr=" MPIG_PTR_FMT,
	    rreq->handle, MPIG_PTR_CAST(rreq)));

	mpig_request_construct_irreq(rreq, 2, 1, buf, cnt, dt, rank, tag, ctx, comm, vc);
    }

    *rreqp = rreq;
    
  fn_return:
    /* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
    mpig_request_mutex_unlock_conditional(rreq, (rreq != NULL));

    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*rreqp),	MPIG_PTR_CAST(*rreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_irecv);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_irecv() */


/*
 * int mpig_cm_self_adi3_probe(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_probe
static int mpig_cm_self_adi3_probe(
    const int rank, const int tag, MPID_Comm * const comm, const int ctxoff, MPI_Status * const status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    bool_t found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_probe);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_probe);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, status=" MPIG_PTR_FMT, rank, tag, MPIG_PTR_CAST(comm), ctx, MPIG_PTR_CAST(status)));

    mpi_errno = mpig_recvq_find_unexp_and_extract_status(rank, tag, comm, ctx, &found, status);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|recvq_fuaes");
    MPIU_ERR_CHKANDJUMP((!found), mpi_errno, MPI_ERR_OTHER, "**globus|cm_self|probedeadlock");

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, mpi_errno=" MPIG_ERRNO_FMT, rank, tag, MPIG_PTR_CAST(comm), ctx, mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_probe);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_probe(...) */


/*
 * int mpig_cm_self_adi3_iprobe(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_iprobe
static int mpig_cm_self_adi3_iprobe(
    const int rank, const int tag, MPID_Comm * const comm, const int ctxoff, int * const found_p, MPI_Status * const status)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    const int ctx = comm->context_id + ctxoff;
    bool_t found = FALSE;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_iprobe);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_iprobe);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "entering: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, status=" MPIG_PTR_FMT, rank, tag, MPIG_PTR_CAST(comm), ctx, MPIG_PTR_CAST(status)));

    mpi_errno = mpig_recvq_find_unexp_and_extract_status(rank, tag, comm, ctx, &found, status);
    MPIU_ERR_CHKANDJUMP((mpi_errno), mpi_errno, MPI_ERR_OTHER, "**globus|recvq_fuaes");

    *found_p = found;

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: rank=%d, tag=%d, comm="
	MPIG_PTR_FMT ", ctx=%d, found=%s, mpi_errno=" MPIG_ERRNO_FMT, rank, tag, MPIG_PTR_CAST(comm), ctx, MPIG_BOOL_STR(found),
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_iprobe);
    return mpi_errno;

  fn_fail:
    /* --BEGIN ERROR HANDLING-- */
    goto fn_return;
    /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_adi3_iprobe(...) */


/*
 * int mpig_cm_self_adi3_cancel_send([IN/MOD] sreq)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_adi3_cancel_send
static int mpig_cm_self_adi3_cancel_send(MPID_Request * const sreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    int mpi_errno = MPI_SUCCESS;
    MPID_Request * const rreq = sreq->partner_request;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_adi3_cancel_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_adi3_cancel_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT,
	"entering: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, sreq->handle, MPIG_PTR_CAST(sreq)));

    if (sreq->cc != 0)
    {
	bool_t rreq_found;

	MPIU_Assert(rreq != NULL);
	
	rreq_found = mpig_recvq_deq_unexp_rreq(rreq);

	if (rreq_found)
	{
	    sreq->status.cancelled = TRUE;
	    mpig_request_set_cc(rreq, 0);
	    mpig_request_set_ref_count(rreq, 0);
	    mpig_request_destroy(rreq);
	}
    }
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_ADI3 | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT
	", sreqp=" MPIG_PTR_FMT "mpi_errno=" MPIG_ERRNO_FMT, sreq->handle, MPIG_PTR_CAST(sreq), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_adi3_cancel_send);
    return mpi_errno;
}
/* mpig_cm_self_adi3_cancel_send() */


/*
 * mpig_cm_self_vc_recv_any_source()
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_vc_recv_any_source
static int mpig_cm_self_vc_recv_any_source(mpig_vc_t * const vc, MPID_Request * const rreq)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq;
    void * sreq_buf;
    int sreq_cnt;
    MPI_Datatype rreq_dt;
    void * rreq_buf;
    int rreq_cnt;
    MPI_Datatype sreq_dt;
    MPIU_Size_t data_size;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_vc_recv_any_source);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_vc_recv_any_source);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "entering: vc=" MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", comm=" MPIG_HANDLE_FMT ", commp=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc), rreq->handle,
	MPIG_PTR_CAST(rreq), rreq->comm->handle, MPIG_PTR_CAST(rreq->comm)));
	
    sreq = rreq->partner_request;
    MPIU_Assertp(sreq != NULL);

    if (rreq->status.MPI_ERROR == MPI_SUCCESS)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request matched by an MPI_ANY_SOURCE receive; copying data: rreq="
	    MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT, rreq->handle,
	    MPIG_PTR_CAST(rreq), sreq->handle, MPIG_PTR_CAST(sreq)));

	mpig_request_get_buffer(sreq, &sreq_buf, &sreq_cnt, &sreq_dt);
	mpig_request_get_buffer(rreq, &rreq_buf, &rreq_cnt, &rreq_dt);
	mpig_cm_self_buffer_copy(sreq_buf, sreq_cnt, sreq_dt, &sreq->status.MPI_ERROR,
				 rreq_buf, rreq_cnt, rreq_dt, &data_size, &rreq->status.MPI_ERROR);
	
	rreq->status.count = (int) data_size;
	/* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
    }
    else
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT, "request matched by an MPI_ANY_SOURCE receive; rreq has error set; "
	    "skipping data copy: rreq=" MPIG_HANDLE_FMT ", rreqp=" MPIG_PTR_FMT ", sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT,
	    rreq->handle, MPIG_PTR_CAST(rreq), sreq->handle, MPIG_PTR_CAST(sreq)));
	
	rreq->status.count = (int) 0;
	/* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
    }
    
    /* neither the application nor any other part of MPICH has a handle to the receive request, so it is safe to just reset the
       reference count and completion counter */
    mpig_request_set_ref_count(rreq, 1);
    mpig_request_set_cc(rreq, 0);
    
    mpig_request_complete(sreq);
    
    /* fn_return: */
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: vc= " MPIG_PTR_FMT ", rreq=" MPIG_HANDLE_FMT
	", rreqp=" MPIG_PTR_FMT ", mpi_errno=" MPIG_ERRNO_FMT, MPIG_PTR_CAST(vc),  rreq->handle, MPIG_PTR_CAST(rreq),
	mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_vc_recv_any_source);
    return mpi_errno;
}
/* mpig_cm_self_vc_recv_any_source() */
/**********************************************************************************************************************************
						    END VC CORE API FUNCTIONS
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					     BEGIN MISCELLANEOUS INTERNAL FUNCTIONS
**********************************************************************************************************************************/
/*
 * mpig_cm_self_send(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_send
static int mpig_cm_self_send(
    const mpig_request_type_t type, const void * const buf, const int cnt, MPI_Datatype dt, const int rank,
    const int tag, const int ctx, MPID_Comm * const comm, MPID_Request ** const sreqp)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    MPID_Request * sreq = NULL;
    MPID_Request * rreq = NULL;
    bool_t rreq_found;
    int mpi_errno = MPI_SUCCESS;
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_send);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_send);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT,
		       "entering: type=%s, buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT ", rank=%d, tag=%d, comm="
		       MPIG_PTR_FMT ", ctx=%d", mpig_request_type_get_string(type), MPIG_PTR_CAST(buf), cnt, dt, rank, tag,
		       MPIG_PTR_CAST(comm), ctx));
	
    MPIU_Assert(rank == comm->rank);
    
    mpig_request_create_isreq(type, 2, 1, (void *) buf, cnt, dt, rank, tag, ctx, comm, mpig_cm_self_vc, &sreq);

    mpi_errno = mpig_recvq_deq_posted_or_enq_unexp(NULL, rank, tag, ctx, &rreq_found, &rreq);
    if (mpi_errno)
    {   /* --BEGIN ERROR HANDLING-- */
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_PT2PT, "ERROR: receive queue operation failed; dequeue "
	    "posted or enqueue unexpected: rank=%d, tag=%d, ctx=%d", rank, tag, ctx));
	MPIU_ERR_SETANDSTMT3(mpi_errno, MPI_ERR_OTHER, {;}, "**globus|recvq_deq_posted_or_enq_unexp",
	    "**globus|recvq_deq_posted_or_enq_unexp %d %d %d", rank, tag, ctx);
	goto fn_fail;
    }   /* --END ERROR HANDLING-- */
    
    if (rreq_found)
    {
	MPIU_Size_t data_size;

	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			   "found posted receive request; copying data"));
	mpig_cm_self_buffer_copy(buf, cnt, dt, &rreq->status.MPI_ERROR, rreq->dev.buf, rreq->dev.cnt, rreq->dev.dt,
				 &data_size, &rreq->status.MPI_ERROR);

	/* if the request was posted as a receive any source, then inform the PE that the op has completed */
	if (mpig_request_get_rank(rreq) == MPI_ANY_SOURCE)
	{
	    mpig_pe_end_ras_op();
	}

	/* mpig_request_set_envelope(rreq, rank, tag, ctx); */
	rreq->status.MPI_SOURCE = rank;
	rreq->status.MPI_TAG = tag;
	rreq->status.count = (int) data_size;
	/* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	/* mpig_request_complete(rreq); -- performed below to avoid destroying the request before it is unlocked */

	/* neither the application nor any other part of MPICH has a handle to the send request, so it is safe to just reset the
	   reference count and completion counter */
	mpig_request_set_ref_count(sreq, 1);
	mpig_request_set_cc(sreq, 0);
    }
    else
    {
	MPIU_Size_t dt_size;
	mpig_vc_t * const vc = mpig_comm_get_remote_vc(comm, rank);

	rreq->partner_request = sreq;
	sreq->partner_request = rreq;
	
	if (type != MPIG_REQUEST_TYPE_RSEND)
	{
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			       "adding receive request to unexpected queue; attaching send request"));
	    mpig_request_construct_irreq(rreq, 2, 1, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, comm, vc);
	    rreq->status.MPI_SOURCE = rank;
	    rreq->status.MPI_TAG = tag;
	    mpig_request_set_remote_req_id(rreq, sreq->handle);

	    /* the count (in bytes) must be set for MPI_Probe() and MPI_Iprobe() to operate properly */
	    MPID_Datatype_get_size_macro(dt, dt_size);
	    rreq->status.count = cnt * dt_size;
	    /* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */
	}
	else
	{
	    int err = MPI_SUCCESS;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PT2PT,
			       "ready send unable to find matching recv req"));
	    MPIU_ERR_SETANDSTMT2(err, MPI_ERR_OTHER, {;}, "**rsendnomatch", "**rsendnomatch %d %d", rank, tag);

	    mpig_request_construct_irreq(rreq, 1, 0, NULL, 0, MPI_DATATYPE_NULL, rank, tag, ctx, comm, vc);
	    rreq->status.MPI_SOURCE = rank;
	    rreq->status.MPI_TAG = tag;
	    rreq->status.MPI_ERROR = err;
	    rreq->status.count = 0;
	    /* rreq->status.mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL; */

	    /* neither the application nor any other part of MPICH has a handle to the send request, so it is safe to just reset
	       the reference count and completion counter */
	    sreq->status.MPI_ERROR = err;
	    mpig_request_set_ref_count(sreq, 1);
	    mpig_request_set_cc(sreq, 0);
	}
    }

    *sreqp = sreq;
    
  fn_return:
    if (rreq != NULL)
    { 
	/* the receive request is locked by the recvq routine to insure atomicity.  it must be unlocked before returning. */
	mpig_request_mutex_unlock(rreq);

	if (rreq_found)
	{
	    /* if a matching receive was found in the unexpected queue, then the send will have completed the receive.  the
	       request is completed here to avoid destroying the request while the request mutex is still locked. */
	    mpig_request_complete(rreq);
	}
	else
	{
	    /* MT-FIXME: a new request has been added to unexpected queue.  if another application thread is blocking in a
	       MPID_Probe(), then the progress engine must be woken up to give MPID_Probe() a chance to inspect the unexpected
	       queue again. */
	}
    }
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_PT2PT, "exiting: sreq=" MPIG_HANDLE_FMT ", sreqp=" MPIG_PTR_FMT
	", mpi_errno=" MPIG_ERRNO_FMT, MPIG_HANDLE_VAL(*sreqp), MPIG_PTR_CAST(*sreqp), mpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_send);
    return mpi_errno;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	if (sreq != NULL)
	{
	    mpig_request_set_cc(sreq->partner_request, 0);
	    mpig_request_set_ref_count(sreq->partner_request, 0);
	    mpig_request_destroy(sreq);
	}
	
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_send() */


/*
 * mpig_cm_self_buffer_copy(...)
 */
#undef FUNCNAME
#define FUNCNAME mpig_cm_self_buffer_copy
static void mpig_cm_self_buffer_copy(
    const void * const sbuf, const int scnt, const MPI_Datatype sdt, int * const smpi_errno,
    void * const rbuf, const int rcnt, const MPI_Datatype rdt, MPIU_Size_t * const rsz, int * const rmpi_errno)
{
    const char fcname[] = MPIG_QUOTE(FUNCNAME);
    bool_t sdt_contig;
    bool_t rdt_contig;
    MPIU_Size_t sdt_size;
    MPIU_Size_t rdt_size;
    MPIU_Size_t sdt_nblks;
    MPIU_Size_t rdt_nblks;
    MPI_Aint sdt_true_lb;
    MPI_Aint rdt_true_lb;
    MPIU_Size_t sdata_size;
    MPIU_Size_t rdata_size;
    MPIG_STATE_DECL(MPID_STATE_memcpy);

    MPIG_UNUSED_VAR(fcname);
    MPIG_STATE_DECL(MPID_STATE_mpig_cm_self_buffer_copy);

    MPIG_UNUSED_VAR(fcname);

    MPIG_FUNC_ENTER(MPID_STATE_mpig_cm_self_buffer_copy);
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA, "entering: sbuf=" MPIG_PTR_FMT ", scnt=%d, sdt="
	MPIG_HANDLE_FMT ", rbuf="  MPIG_PTR_FMT ", rcnt=%d, rdt=" MPIG_HANDLE_FMT , MPIG_PTR_CAST(sbuf), scnt, sdt,
	MPIG_PTR_CAST(rbuf), rcnt, rdt));
    *smpi_errno = MPI_SUCCESS;
    *rmpi_errno = MPI_SUCCESS;

    mpig_datatype_get_info(sdt, &sdt_contig, &sdt_size, &sdt_nblks, &sdt_true_lb);
    mpig_datatype_get_info(rdt, &rdt_contig, &rdt_size, &rdt_nblks, &rdt_true_lb);
    sdata_size = sdt_size * scnt;
    rdata_size = rdt_size * rcnt;

    if (sdata_size > rdata_size)
    {
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATA | MPIG_DEBUG_LEVEL_PT2PT,
	    "message truncated, sdata_size=" MPIG_SIZE_FMT " rdata_size=" MPIG_SIZE_FMT, sdata_size, rdata_size));
	sdata_size = rdata_size;
	MPIU_ERR_SETANDSTMT2(*rmpi_errno, MPI_ERR_TRUNCATE, {;}, "**truncate", "**truncate %d %d", sdata_size, rdata_size);
    }
    
    if (sdata_size == 0)
    {
	*rsz = 0;
	goto fn_return;
    }
    
    if (sdt_contig && rdt_contig)
    {
	MPIG_FUNC_ENTER(MPID_STATE_memcpy);
	memcpy((char *)rbuf + rdt_true_lb, (const char *) sbuf + sdt_true_lb, (size_t) sdata_size);
	MPIG_FUNC_EXIT(MPID_STATE_memcpy);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "copied " MPIG_SIZE_FMT
	    " bytes directly from send buffer to receive buffer", sdata_size));
	*rsz = sdata_size;
    }
    else if (sdt_contig)
    {
	MPID_Segment seg;
	MPIU_Size_t last;

	MPID_Segment_init(rbuf, rcnt, rdt, &seg, 0);
	last = sdata_size;
	MPID_Segment_unpack(&seg, (MPI_Aint) 0, (MPI_Aint *) &last, (const char *) sbuf + sdt_true_lb);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "unpacked data in receive buffer: nbytes_moved=" MPIG_SIZE_FMT, last));
	if (last != sdata_size)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATA, "data type mismatch: send_nbytes=" MPIG_SIZE_FMT
		", nbytes_moved=" MPIG_SIZE_FMT, sdata_size, last));
	    MPIU_ERR_SET(*rmpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	*rsz = last;
    }
    else if (rdt_contig)
    {
	MPID_Segment seg;
	MPIU_Size_t last;

	MPID_Segment_init(sbuf, scnt, sdt, &seg, 0);
	last = sdata_size;
	MPID_Segment_pack(&seg, (MPI_Aint) 0, (MPI_Aint *) &last, (char *) rbuf + rdt_true_lb);
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "packed data in receive buffer: nbytes_moved=" MPIG_SIZE_FMT, last));
	if (last != sdata_size)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATA, "data type mismatch: send_nbytes=" MPIG_SIZE_FMT
		", nbytes_moved=" MPIG_SIZE_FMT, sdata_size, last));
	    MPIU_ERR_SET(*rmpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	*rsz = last;
    }
    else
    {
	char * buf;
	MPIU_Size_t buf_off;
	MPID_Segment sseg;
	MPIU_Size_t sfirst;
	MPID_Segment rseg;
	MPIU_Size_t rfirst;

	buf = MPIU_Malloc((size_t) MPIG_COPY_BUFFER_SIZE);
	if (buf == NULL)
	{   /* --BEGIN ERROR HANDLING-- */
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATA,
		"ERROR: copy (pack/unpack) buffer allocation failure"));
	    MPIU_ERR_SET1(*smpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "copy (pack/unpack) buffer");
	    *rmpi_errno = *smpi_errno;
	    *rsz = 0;
	    goto fn_fail;
	}   /* --END ERROR HANDLING-- */

	MPID_Segment_init(sbuf, scnt, sdt, &sseg, 0);
	MPID_Segment_init(rbuf, rcnt, rdt, &rseg, 0);

	sfirst = 0;
	rfirst = 0;
	buf_off = 0;
	
	for(;;)
	{
	    MPIU_Size_t last;
	    char * buf_end;

	    if (sdata_size - sfirst > MPIG_COPY_BUFFER_SIZE - buf_off)
	    {
		last = sfirst + (MPIG_COPY_BUFFER_SIZE - buf_off);
	    }
	    else
	    {
		last = sdata_size;
	    }
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "pre-pack: first=" MPIG_SIZE_FMT ", last=" MPIG_SIZE_FMT, sfirst, last));
	    MPID_Segment_pack(&sseg,  *(MPI_Aint *) &sfirst, (MPI_Aint *) &last, buf + buf_off);
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "post-pack: first=" MPIG_SIZE_FMT ", last=" MPIG_SIZE_FMT, sfirst, last));
	    MPIU_Assert(last > sfirst);
	    
	    buf_end = buf + buf_off + (last - sfirst);
	    sfirst = last;
	    
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "pre-unpack: first=" MPIG_SIZE_FMT ", last=" MPIG_SIZE_FMT, rfirst, last));
	    MPID_Segment_unpack(&rseg, *(MPI_Aint *) &rfirst, (MPI_Aint *) &last, buf);
	    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "post-unpack: first=" MPIG_SIZE_FMT ", last=" MPIG_SIZE_FMT, rfirst, last));
	    MPIU_Assert(last > rfirst);

	    rfirst = last;
	    if (rfirst == sdata_size) break;  /* successful completion */

	    if (sfirst == sdata_size)
	    {   /* --BEGIN ERROR HANDLING-- */
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR | MPIG_DEBUG_LEVEL_DATA, "data type mismatch: send_nbytes="
		    MPIG_SIZE_FMT ", nbytes_moved=" MPIG_SIZE_FMT, sdata_size, last));
		MPIU_ERR_SET(*smpi_errno, MPI_ERR_TYPE, "**dtypemismatch");
		*rmpi_errno = *smpi_errno;
		break;
		goto fn_fail;
	    }   /* --END ERROR HANDLING-- */

	    buf_off = sfirst - rfirst;
	    if (buf_off > 0)
	    {
		MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATA, "moving " MPIG_SIZE_FMT
		    " bytes to the beginning of the intermediate buffer", buf_off));
		memmove(buf, buf_end - buf_off, (size_t) buf_off);
	    }
	}

	*rsz = rfirst;
	MPIU_Free(buf);
    }

  fn_return:
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC | MPIG_DEBUG_LEVEL_DATA, "exiting: rsz=" MPIG_SIZE_FMT ", smpi_errno=" MPIG_ERRNO_FMT
	", rmpi_errno=" MPIG_ERRNO_FMT, *rsz, *smpi_errno, *rmpi_errno));
    MPIG_FUNC_EXIT(MPID_STATE_mpig_cm_self_buffer_copy);
    return;
    
  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* mpig_cm_self_buffer_copy() */
/**********************************************************************************************************************************
					     END MISCELLANEOUS INTERNAL FUNCTIONS
**********************************************************************************************************************************/
