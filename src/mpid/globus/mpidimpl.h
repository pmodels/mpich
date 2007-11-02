/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDIMPL_H_INCLUDED)
#define MPICH2_MPIDIMPL_H_INCLUDED

#include "mpiimpl.h"

#if defined(HAVE_GLOBUS_COMMON_MODULE)
#include "globus_module.h"
#endif


/*
 * When memory tracing is enabled, mpimem.h redefines malloc(), calloc(), and free() to be invalid statements.  These
 * redefinitions have a negative interaction with the Globus heap routines which may be mapped (using C preprocessor defines)
 * directly to the heap routines supplied as part of the the operating system.  This is particularly a problem when a Globus
 * library routine returns an allocated object and expects to caller to free the object.  One cannot use MPIU_Free() to free the
 * object since MPIU_Free() is expecting a pointer to memory allocated by the memory tracing module.  Therefore, it is necessary
 * to remove the redefinitions of the heap routines, allowing the Globus heap routines to map directly to those provided by the
 * operating system.
 */
#if defined(USE_MEMORY_TRACING)
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif


/*
 * use MPIG_STATIC when declaring module specific (global) variables.  by using MPIG_STATIC, the variable will be exposed in the
 * global scope for a debug build, allowing the value to be obtained at any time from within the debugger.
 *
 * NOTE: do not use MPIG_STATIC for static variables declared within a function!
 */
#if defined(MPIG_DEBUG)
#define MPIG_STATIC
#else
#define MPIG_STATIC static
#endif

#if defined(MPIG_VMPI)
#define MPIG_USING_VMPI (TRUE)
#else
#define MPIG_USING_VMPI (FALSE)
#endif

#if defined(HAVE_GETHOSTNAME) && defined(NEEDS_GETHOSTNAME_DECL) && !defined(gethostname)
int gethostname(char *name, size_t len);
# endif


#if defined(HAVE_GLOBUS_COMMON_MODULE)
#define mpig_get_globus_error_msg(grc_) (globus_error_print_chain(globus_error_peek(grc_)))
#endif
/**********************************************************************************************************************************
						       BEGIN PT2PT SECTION
**********************************************************************************************************************************/
int mpig_adi3_cancel_recv(MPID_Request * rreq);
/**********************************************************************************************************************************
							END PT2PT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATION MODULE SECTION
**********************************************************************************************************************************/
extern mpig_cm_t * const * const mpig_cm_table;
extern const int mpig_cm_table_num_entries;

char * mpig_cm_vtable_last_entry(int foo, float bar, const short * baz, char bif);

#define mpig_cm_get_module_type(cm_) ((cm_)->module_type)

#define mpig_cm_get_name(cm_) ((cm_)->name)

#define mpig_cm_get_vtable(cm_) ((const mpig_cm_vtable_t *)((cm_)->vtable))
/**********************************************************************************************************************************
						END COMMUNICATION MODULE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
int mpig_comm_init(void);

int mpig_comm_finalize(void);

#define mpig_comm_set_local_vc(comm_, rank_, vc_)	\
{							\
    (comm_)->local_vcr[(rank_)] = (vc_);		\
}

#define mpig_comm_get_local_vc(comm_, rank_) ((comm_)->local_vcr[(rank_)])

#define mpig_comm_set_remote_vc(comm_, rank_, vc_)	\
{							\
    (comm_)->vcr[(rank_)] = (vc_);			\
}

#define mpig_comm_get_remote_vc(comm_, rank_) ((comm_)->vcr[(rank_)])

#define mpig_comm_get_my_vc(comm_) \
    ((comm_)->comm_kind == MPID_INTRACOMM) ? ((comm_)->vcr[(comm_)->rank]) : ((comm_)->local_vcr[(comm_)->rank])
/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN TOPOLOGY INFORMATION SECTION
**********************************************************************************************************************************/
int mpig_topology_init(void);

int mpig_topology_finalize(void);

int mpig_topology_comm_construct(MPID_Comm * comm);

int mpig_topology_comm_destruct(MPID_Comm * comm);

#define MPIG_TOPOLOGY_LEVEL_WAN_MASK		((unsigned) 1 << MPIG_TOPOLOGY_LEVEL_WAN)
#define MPIG_TOPOLOGY_LEVEL_LAN_MASK		((unsigned) 1 << MPIG_TOPOLOGY_LEVEL_LAN)
#define MPIG_TOPOLOGY_LEVEL_SAN_MASK		((unsigned) 1 << MPIG_TOPOLOGY_LEVEL_SAN)
#define MPIG_TOPOLOGY_LEVEL_VMPI_MASK		((unsigned) 1 << MPIG_TOPOLOGY_LEVEL_VMPI)
/**********************************************************************************************************************************
						END TOPOLOGY INFORMATION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN DATATYPE SECTION
**********************************************************************************************************************************/
int mpig_datatype_set_my_bc(mpig_bc_t * bc);
int mpig_datatype_process_bc(const mpig_bc_t * bc, mpig_vc_t * vc);
void mpig_datatype_get_src_ctype(const mpig_vc_t * vc, const MPID_Datatype dt, mpig_ctype_t * ctype);
void mpig_datatype_get_my_ctype(const MPI_Datatype dt, mpig_ctype_t * ctype);

#define mpig_datatype_get_src_ctype(/* mpig_vc_t ptr */ vc_, /* MPI_Datatype */ dt_, /* mpig_ctype_t ptr */ ctype_p_)	\
{															\
    int mpig_datatype_get_src_ctype_dt_id__;										\
															\
    MPIU_Assert(HANDLE_GET_KIND(dt_) == MPI_KIND_BUILTIN && HANDLE_GET_MPI_KIND(dt_) == MPID_DATATYPE);			\
    MPID_Datatype_get_basic_id(dt_, mpig_datatype_get_src_ctype_dt_id__);						\
    MPIU_Assert(mpig_datatype_get_src_ctype_dt_id__ < MPIG_DATATYPE_MAX_BASIC_TYPES);					\
    *(ctype_p_) = (mpig_ctype_t) (vc_)->dt_cmap[mpig_datatype_get_src_ctype_dt_id__];					\
}

#define mpig_datatype_get_my_ctype(/* MPI_Datatype */ dt_, /* mpig_ctype_t ptr */ ctype_p_)		\
{													\
    const mpig_vc_t * mpig_datatype_get_my_ctype_vc__;							\
													\
    mpig_pg_get_vc(mpig_process.my_pg, mpig_process.my_pg_rank, &mpig_datatype_get_my_ctype_vc__);	\
    mpig_datatype_get_src_ctype(mpig_datatype_get_my_ctype_vc__, (dt_), (ctype_p_));			\
}

#define mpig_datatype_get_size(dt_, dt_size_p_)				\
{									\
    if (HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN)			\
    {									\
	*(dt_size_p_) = MPID_Datatype_get_basic_size(dt_);		\
    }									\
    else								\
    {									\
	MPID_Datatype * mpig_datatype_get_size_dtp__;			\
									\
	MPID_Datatype_get_ptr((dt_), mpig_datatype_get_size_dtp__);	\
	*(dt_size_p_) = mpig_datatype_get_size_dtp__->size;		\
    }									\
}

#define mpig_datatype_get_info(/* MPI_Datatype */ dt_, /* bool_t ptr */ dt_contig_, /* MPIU_Size_t ptr */ dt_size_,	\
    /* MPIU_Size_t ptr */ dt_nblks_, /* MPI_Aint ptr */ dt_true_lb_)							\
{															\
    if (HANDLE_GET_KIND(dt_) == HANDLE_KIND_BUILTIN)									\
    {															\
	*(dt_contig_) = TRUE;												\
	*(dt_size_) = MPID_Datatype_get_basic_size(dt_);								\
        *(dt_nblks_) = 1;												\
        *(dt_true_lb_) = 0;												\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "datatype - type=basic, dt_contig=%s, dt_size=" MPIG_SIZE_FMT		\
			   ", dt_nblks=" MPIG_SIZE_FMT ", dt_true_lb=" MPIG_AINT_FMT, MPIG_BOOL_STR(*(dt_contig_)),	\
			 (MPIU_Size_t) *(dt_size_), (MPIU_Size_t) *(dt_nblks_), (MPI_Aint) *(dt_true_lb_)));		\
    }															\
    else														\
    {															\
	MPID_Datatype * mpig_datatype_get_info_dtp__;									\
															\
	MPID_Datatype_get_ptr((dt_), mpig_datatype_get_info_dtp__);							\
	*(dt_contig_) = mpig_datatype_get_info_dtp__->is_contig;							\
	*(dt_size_) = mpig_datatype_get_info_dtp__->size;								\
	*(dt_nblks_) = mpig_datatype_get_info_dtp__->n_contig_blocks;							\
        *(dt_true_lb_) = mpig_datatype_get_info_dtp__->true_lb;								\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DT, "datatype - type=app, dt_contig=%s, dt_size=" MPIG_SIZE_FMT		\
			   ", dt_nblks=" MPIG_SIZE_FMT ", dt_true_lb=" MPIG_AINT_FMT, MPIG_BOOL_STR(*(dt_contig_)),	\
			 (MPIU_Size_t) *(dt_size_), (MPIU_Size_t) *(dt_nblks_), (MPI_Aint) *(dt_true_lb_)));		\
    }															\
}
/**********************************************************************************************************************************
						      END DATATYPE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
/*
 * request allocation
 *
 * MT-NOTE: except for the the init and finalize routines, these request allocation routines are thread safe.
 */
extern mpig_mutex_t mpig_request_alloc_mutex;

void mpig_request_alloc_init(void);

void mpig_request_alloc_finalize(void);

#define mpig_request_alloc(req_p_)				\
{								\
    /* acquire a request from the object allocation pool */	\
    mpig_mutex_lock(&mpig_request_alloc_mutex);			\
    {								\
	*(req_p_) = MPIU_Handle_obj_alloc(&MPID_Request_mem);	\
    }								\
    mpig_mutex_unlock(&mpig_request_alloc_mutex);		\
								\
    mpig_request_mutex_construct(*(req_p_));			\
}

#define mpig_request_free(req_)						\
{									\
    mpig_request_mutex_destruct(req_);					\
									\
    /* release the request back to the object allocation pool */	\
    mpig_mutex_lock(&mpig_request_alloc_mutex);				\
    {									\
	MPIU_Handle_obj_free(&MPID_Request_mem, (req_));		\
    }									\
    mpig_mutex_unlock(&mpig_request_alloc_mutex);			\
}

/*
 * request functions
 */
const char * mpig_request_type_get_string(mpig_request_type_t req_type);

/*
 * request accessor macros
 *
 * MT-NOTE: these routines are not thread safe.  the calling routing is responsible for performing the necessary operations to
 * insure atomicity.
 *
 * MT-RC-NOTE: neither do these routines provide any coherence guarantees when a request is shared between multiple threads.  the
 * calling routine is responsible for performing the necessary mutex lock/unlock or RC acquire/release operations to insure that
 * changes made to the request object are made visible to other threads accessing the object.
 */
#define mpig_request_i_set_envelope(req_, rank_, tag_, ctx_)	\
{								\
    (req_)->dev.rank = (rank_);					\
    (req_)->dev.tag = (tag_);					\
    (req_)->dev.ctx = (ctx_);					\
}

#define mpig_request_set_envelope(req_, rank_, tag_, ctx_)								\
{															\
    mpig_request_i_set_envelope((req_), (rank_), (tag_), (ctx_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set envelope: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", rank=%d, tag=%d, ctx=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->dev.rank,				\
	(req_)->dev.tag, (req_)->dev.ctx));										\
}

#define mpig_request_i_get_envelope(req_, rank_p_, tag_p_, ctx_p_)	\
{									\
    *(rank_p_) = (req_)->dev.rank;					\
    *(tag_p_) = (req_)->dev.tag;					\
    *(ctx_p_) = (req_)->dev.ctx;					\
}

#define mpig_request_get_envelope(req_, rank_p_, tag_p_, ctx_p_)							\
{															\
    mpig_request_i_get_envelope((req_), (rank_p_), (tag_p_), (ctx_p_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - get envelope: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", rank=%d, tag=%d, ctx=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->dev.rank,				\
	(req_)->dev.tag, (req_)->dev.ctx));										\
}

#define mpig_request_get_rank(req_) ((req_)->dev.rank)

#define mpig_request_i_set_buffer(req_, buf_, cnt_, dt_)	\
{								\
    (req_)->dev.buf = (buf_);					\
    (req_)->dev.cnt = (cnt_);					\
    (req_)->dev.dt = (dt_);					\
}

#define mpig_request_set_buffer(req_, buf_, cnt_, dt_)									\
{															\
    mpig_request_i_set_buffer((req_), (buf_), (cnt_), (dt_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set buffer: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT, (req_)->handle, MPIG_PTR_CAST(req_),			\
	MPIG_PTR_CAST((req_)->dev.buf), (req_)->dev.cnt, (req_)->dev.dt));						\
}

#define mpig_request_i_get_buffer(req_, buf_p_, cnt_p_, dt_p_)	\
{								\
    *(buf_p_) = (req_)->dev.buf;				\
    *(cnt_p_) = (req_)->dev.cnt;				\
    *(dt_p_) = (req_)->dev.dt;					\
}

#define mpig_request_get_buffer(req_, buf_p_, cnt_p_, dt_p_)								\
{															\
    mpig_request_i_get_buffer((req_), (buf_p_), (cnt_p_), (dt_p_));							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - get buffer: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", buf=" MPIG_PTR_FMT ", cnt=%d, dt=" MPIG_HANDLE_FMT, (req_)->handle, MPIG_PTR_CAST(req_),			\
	MPIG_PTR_CAST((req_)->dev.buf), (req_)->dev.cnt, (req_)->dev.dt));						\
}

#define mpig_request_get_dt(req_) ((req_)->dev.dt)

#define mpig_request_i_set_remote_req_id(req_, remote_req_id_)	\
{								\
    (req_)->dev.remote_req_id = (remote_req_id_);		\
}

#define mpig_request_set_remote_req_id(req_, remote_req_id_)								\
{															\
    mpig_request_i_set_remote_req_id((req_), (remote_req_id_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set remote req id: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", id=" MPIG_HANDLE_FMT, (req_)->handle, MPIG_PTR_CAST(req_), (req_)->dev.remote_req_id));			\
}

#define mpig_request_get_remote_req_id(req_) ((req_)->dev.remote_req_id)

#define mpig_request_add_comm_ref(req_, comm_)	\
{						\
    if ((req_)->comm == NULL)			\
    {						\
	(req_)->comm = (comm_);			\
	MPIR_Comm_add_ref(comm_);		\
    }						\
}

#define mpig_request_release_comm_ref(req_)	\
{						\
    if (req->comm != NULL)			\
    {						\
	MPIR_Comm_release(req->comm);		\
        req->comm = NULL;			\
    }						\
}    

#define mpig_request_add_dt_ref(req_, dt_)											\
{																\
    if ((req_)->dev.dtp == NULL && HANDLE_GET_KIND(dt_) != HANDLE_KIND_BUILTIN && HANDLE_GET_KIND(dt_) != HANDLE_KIND_INVALID)	\
    {																\
	MPID_Datatype_get_ptr((dt_), (req_)->dev.dtp);										\
	MPID_Datatype_add_ref((req_)->dev.dtp);											\
    }																\
}

#define mpig_request_release_dt_ref(req_)	\
{						\
    if (req->dev.dtp != NULL)			\
    {						\
	MPID_Datatype_release(req->dev.dtp);	\
        req->dev.dtp = NULL;			\
    }						\
}

#define mpig_request_get_type(req_) ((req_)->dev.type)

#define mpig_request_i_set_type(req_, type_)	\
{						\
    (req_)->dev.type = (type_);			\
}

#define mpig_request_set_type(req_, type_)											\
{																\
    mpig_request_i_set_type((req_), (type_));											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set request type: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT		\
	", type=%s",(req_)->handle, MPIG_PTR_CAST(req_), mpig_request_type_get_string((req_)->dev.type)));			\
}

#define mpig_request_i_set_vc(req_, vc_)	\
{						\
    (req_)->dev.vc = (vc_);			\
}

#define mpig_request_set_vc(req_, vc_)										\
{														\
    mpig_request_i_set_vc((req_), (vc_));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_REQ, "request - set vc: req=" MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT	\
	", vc=" MPIG_PTR_FMT, (req_)->handle, MPIG_PTR_CAST(req_), MPIG_PTR_CAST(vc_)));			\
}

#define mpig_request_get_vc(req_) ((req_)->dev.vc)

#define mpig_request_set_cm_destruct_fn(req_, fn_)	\
{							\
    (req_)->dev.cm_destruct = (fn_);			\
}

/*
 * thread safety and release consistency macros
 */
#define mpig_request_mutex_construct(req_)	mpig_mutex_construct(&(req_)->dev.mutex)
#define mpig_request_mutex_destruct(req_)	mpig_mutex_destruct(&(req_)->dev.mutex)
#define mpig_request_mutex_lock(req_)										\
{														\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - acquiring mutex: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, MPIG_PTR_CAST(req_)));				\
    mpig_mutex_lock(&(req_)->dev.mutex);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - mutex acquired: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, MPIG_PTR_CAST(req_)));				\
}
#define mpig_request_mutex_unlock(req_)										\
{														\
    mpig_mutex_unlock(&(req_)->dev.mutex);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_REQ, "request - mutex released: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT, (req_)->handle, MPIG_PTR_CAST(req_)));				\
}

#define mpig_request_mutex_lock_conditional(req_, cond_)	{if (cond_) mpig_request_mutex_lock(req_);}
#define mpig_request_mutex_unlock_conditional(req_, cond_)	{if (cond_) mpig_request_mutex_unlock(req_);}

#define mpig_request_rc_acq(req_, needed_)	mpig_request_mutex_lock(req_)
#define mpig_request_rc_rel(req_, needed_)	mpig_request_mutex_unlock(req_)

/*
 * request construction and destruction
 *
 * MT-RC-NOTE: on release consistent systems, if the request object is to be used by any thread other than the one in which it
 * was created, then the construction of the object must be followed by a RC release or mutex unlock.  for systems using super
 * lazy release consistency models, such as the one used in Treadmarks, it would be necessary to perform an acquire/lock in the
 * mpig_request_construct() immediately after creating the mutex.  The calling routine would need to perform a release after the
 * temp VC was completely initialized.  we have chosen to assume that none of the the systems upon which MPIG will run have that
 * lazy of a RC model.
 */

/*
 * SUPER-IMPORTANT-MT-NOTE: see the note at the top of the request section in mpidpost.h concerning the creation and destruction
 * of requests from internal device threads.
 */
#define mpig_request_construct(req_)						\
{										\
    /* set MPICH fields */							\
    mpig_request_i_set_ref_count((req_), 1);					\
    (req_)->kind = MPID_REQUEST_UNDEFINED;					\
    (req_)->cc_ptr = &(req_)->cc;						\
    mpig_request_i_set_cc((req_), 0);						\
    (req_)->comm = NULL;							\
    (req_)->partner_request = NULL;						\
    MPIR_Status_set_empty(&(req_)->status);					\
    (req_)->status.MPI_ERROR = MPI_SUCCESS;					\
										\
    /* set device fields */							\
    mpig_request_i_set_type((req_), MPIG_REQUEST_TYPE_UNDEFINED);		\
    mpig_request_i_set_buffer((req_), NULL, 0, MPI_DATATYPE_NULL);		\
    mpig_request_i_set_envelope((req_), MPI_PROC_NULL, MPI_ANY_TAG, -1);	\
    (req_)->dev.dtp = NULL;							\
    mpig_request_i_set_remote_req_id((req_), MPI_REQUEST_NULL);			\
    (req_)->dev.cm_destruct = (mpig_request_cm_destruct_fn_t) NULL;		\
    mpig_request_i_set_vc((req_), NULL);					\
    (req_)->dev.recvq_unreg_ras_op = NULL;					\
    (req_)->dev.next = NULL;							\
}

/*
 * SUPER-IMPORTANT-MT-NOTE: see the note at the top of the request section in mpidpost.h concerning the creation and destruction
 * of requests from internal device threads.
 */
#define mpig_request_destruct(req_)							\
{											\
    /* call communication module's request destruct function */				\
    if ((req_)->dev.cm_destruct != NULL)						\
    {											\
	(req_)->dev.cm_destruct(req_);							\
	(req_)->dev.cm_destruct = (mpig_request_cm_destruct_fn_t) MPIG_INVALID_PTR;	\
    }											\
											\
    /* release and communicator and datatype objects associated with the request */	\
    mpig_request_release_comm_ref(req_);						\
    mpig_request_release_dt_ref(req_);							\
											\
    /* resset MPICH fields */								\
    mpig_request_i_set_ref_count((req_), 0);						\
    (req_)->kind = MPID_REQUEST_UNDEFINED;						\
    (req_)->cc_ptr = MPIG_INVALID_PTR;							\
    mpig_request_i_set_cc((req_), 0);							\
    /* (req_)->comm = NULL; -- cleared by mpig_request_release_comm() */		\
    (req_)->partner_request = MPIG_INVALID_PTR;						\
    MPIR_Status_set_empty(&(req_)->status);						\
    (req_)->status.MPI_ERROR = MPI_ERR_INTERN;						\
											\
    /* reset device fields */								\
    mpig_request_i_set_type((req_), MPIG_REQUEST_TYPE_FIRST);				\
    mpig_request_i_set_buffer((req_), MPIG_INVALID_PTR, -1, MPI_DATATYPE_NULL);		\
    mpig_request_i_set_envelope((req_), MPI_PROC_NULL, MPI_ANY_TAG, -1);		\
    /* (req_)->dev.dtp = NULL; -- cleared by mpig_request_release_dt() */		\
    mpig_request_i_set_remote_req_id((req_), MPI_REQUEST_NULL);				\
    mpig_request_i_set_vc((req_), MPIG_INVALID_PTR);					\
    (req_)->dev.recvq_unreg_ras_op = MPIG_INVALID_PTR;					\
    (req_)->dev.next = MPIG_INVALID_PTR;						\
}

#define mpig_request_set_params(req_, kind_, type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_)	\
{														\
    /* set MPICH fields */											\
    mpig_request_set_ref_count((req_), (ref_cnt_));								\
    (req_)->kind = (kind_);											\
    (req_)->cc_ptr = &(req_)->cc;										\
    mpig_request_set_cc((req_), (cc_));										\
														\
    /* set device fields */											\
    mpig_request_set_type((req_), (type_));									\
    mpig_request_set_buffer((req_), (buf_), (cnt_), (dt_));							\
    mpig_request_set_envelope((req_), (rank_), (tag_), (ctx_));							\
    mpig_request_set_vc(req_, (vc_));										\
}

#define mpig_request_create_sreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_, sreq_p_)		\
{															\
    mpig_request_alloc(sreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");	\
    mpig_request_construct(*(sreq_p_));											\
    mpig_request_set_params(*(sreq_p_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_),					\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));					\
}

#define mpig_request_create_isreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_, sreq_p_)	\
{															\
    mpig_request_alloc(sreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "send request");	\
    mpig_request_construct(*(sreq_p_));											\
    mpig_request_set_params(*(sreq_p_), MPID_REQUEST_SEND, (type_), (ref_cnt_), (cc_),					\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));					\
    mpig_request_add_comm_ref(*(sreq_p_), (comm_));									\
    mpig_request_add_dt_ref(*(sreq_p_), (dt_));										\
}

#define mpig_request_create_psreq(type_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, sreq_p_)			\
{																\
    mpig_request_alloc(sreq_p_);												\
    MPIU_ERR_CHKANDJUMP1((*(sreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent send request");	\
    mpig_request_construct(*(sreq_p_));												\
    mpig_request_set_params(*(sreq_p_), MPID_PREQUEST_SEND, (type_), (ref_cnt_), (cc_),						\
			    (buf_), (cnt_), (dt_),(rank_), (tag_), (ctx_), NULL);						\
    mpig_request_add_comm_ref(*(sreq_p_), (comm_));										\
    mpig_request_add_dt_ref(*(sreq_p_), (dt_));											\
}

#define mpig_request_construct_rreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, vc_)	\
{													\
    mpig_request_set_params((rreq_), MPID_REQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),	\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));			\
}

#define mpig_request_create_rreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, rreq_p_, vc_)			\
{															\
    mpig_request_alloc(rreq_p_);											\
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");	\
    mpig_request_construct(*(rreq_p_));											\
    mpig_request_construct_rreq(*(rreq_p_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));	\
}

#define mpig_request_construct_irreq(rreq_, ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_)	\
{														\
    mpig_request_set_params((rreq_), MPID_REQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),		\
			    (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (vc_));				\
    mpig_request_add_comm_ref((rreq_), (comm_));								\
    mpig_request_add_dt_ref((rreq_), (dt_));									\
}

#define mpig_request_create_irreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, vc_, rreq_p_)			 \
{																 \
    mpig_request_alloc(rreq_p_);												 \
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "receive request");		 \
    mpig_request_construct(*(rreq_p_));												 \
    mpig_request_construct_irreq(*(rreq_p_), (ref_cnt_), (cc_), (buf_), (cnt_), (dt_), (rank_), (tag_), (ctx_), (comm_), (vc_)); \
}

#define mpig_request_create_prreq(ref_cnt_, cc_, buf_, cnt_, dt_, rank_, tag_, ctx_, comm_, rreq_p_)				 \
{																 \
    mpig_request_alloc(rreq_p_);												 \
    MPIU_ERR_CHKANDJUMP1((*(rreq_p_) == NULL), mpi_errno, MPI_ERR_OTHER, "**nomem", "**nomem %s", "persistent receive request"); \
    mpig_request_construct(*(rreq_p_));												 \
    mpig_request_set_params(*(rreq_p_), MPID_PREQUEST_RECV, MPIG_REQUEST_TYPE_RECV, (ref_cnt_), (cc_),				 \
			    (buf_), (cnt_), (dt), (rank_), (tag_), (ctx_), NULL);						 \
    mpig_request_add_comm_ref(*(rreq_p_), (comm_));										 \
    mpig_request_add_dt_ref(*(rreq_p_), (dt_));											 \
}
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS DATA SECTION
**********************************************************************************************************************************/
#define mpig_process_mutex_construct()	mpig_mutex_construct(&mpig_process.mutex)
#define mpig_process_mutex_destruct()	mpig_mutex_destruct(&mpig_process.mutex)
#define mpig_process_mutex_lock()								\
{												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS, "process local data - acquiring mutex"));	\
    mpig_mutex_lock(&mpig_process.mutex);							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS, "process local data - mutex acquired"));	\
}
#define mpig_process_mutex_unlock()								\
{												\
    mpig_mutex_unlock(&mpig_process.mutex);							\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS, "process local data - mutex released"));	\
}
/**********************************************************************************************************************************
						    END PROCESS DATA SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN DATA CONVERSION
**********************************************************************************************************************************/
/*
 * NOTE: the data conversion routines do not perform type conversion.  they expect the data (and data pointers) to match the type
 * indicated by the routine name.
 *
 * MT-RC-NOTE: the data conversion do not make any effort to insure that the data being converted has been acquired or that the
 * converted data is released.  it is the responsibility of the calling routine to perform any mutex lock/unlock or RC
 * acquire/release operations that are necessary to insure that data manipulated by these routines are visible by the necessary
 * threads.
 */

#define mpig_dc_put_int32(buf_, data_) mpig_dc_put_uint32((buf_), (data_))
#define mpig_dc_putn_int32(buf_, data_, num_) mpig_dc_put_uint32((buf_), (data_), (num_))
#define mpig_dc_get_int32(endian_, buf_, data_p_) mpig_dc_get_uint32((endian_), (buf_), (data_p_))
#define mpig_dc_getn_int32(endian_, buf_, data_, num_) mpig_dc_get_uint32((endian_), (buf_, (data_), (num_))
#define mpig_dc_put_int64(buf_, data_) mpig_dc_put_uint64((buf_), (data_))
#define mpig_dc_putn_int64(buf_, data_, num_) mpig_dc_put_uint64((buf_), (data_), (num_))
#define mpig_dc_get_int64(endian_, buf_, data_p_) mpig_dc_get_uint64((endian_), (buf_), (data_p_))
#define mpig_dc_getn_int64(endian_, buf_, data_, num_) mpig_dc_get_uint64((endian_), (buf_), (data_), (num_))

#define mpig_dc_put_uint32(buf_, data_)					\
{									\
    unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
    unsigned char * mpig_dc_data__ = (unsigned char *) &(data_);	\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
}

#define mpig_dc_putn_uint32(buf_, data_, num_)			\
{								\
    memcpy((void *)(buf_), (void *)(data_), (num_) * 4);	\
}

#define mpig_dc_get_uint32(endian_, buf_, data_p_)				\
{										\
    if ((endian_) == MPIG_MY_ENDIAN)						\
    {										\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
	unsigned char * mpig_dc_data_p__ = (unsigned char *)(data_p_);		\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
    }										\
    else									\
    {										\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
	unsigned char * mpig_dc_data_p__ = (unsigned char *)(data_p_) + 3;	\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
    }										\
}

#define mpig_dc_getn_uint32(endian_, buf_, data_, num_)			\
{									\
    if ((endian_) == MPIG_MY_ENDIAN)					\
    {									\
	memcpy((void *)(buf_), (void *)(data_), (num_) * 4);		\
    }									\
    else								\
    {									\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);	\
	unsigned char * mpig_dc_data__ = (unsigned char *)(data_) + 3;	\
									\
	for (n__ = 0; n__ < (num_); n__++)				\
	{								\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    mpig_dc_data__ += 8;					\
	}								\
    }									\
}

#define mpig_dc_put_uint64(buf_, data_)					\
{									\
    unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
    unsigned char * mpig_dc_data__ = (unsigned char *) &(data_);	\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
    *(mpig_dc_buf__)++ = *(mpig_dc_data__)++;				\
}

#define mpig_dc_putn_uint64(buf_, data_, num_)			\
{								\
    memcpy((void *)(buf_), (void *)(data_), (num_) * 8);	\
}

#define mpig_dc_get_uint64(endian_, buf_, data_p_)				\
{										\
    if ((endian_) == MPIG_MY_ENDIAN)						\
    {										\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
	unsigned char * mpig_dc_data_p__ = (unsigned char *)(data_p_);		\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)++ = *(mpig_dc_buf__)++;				\
    }										\
    else									\
    {										\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);		\
	unsigned char * mpig_dc_data_p__ = (unsigned char *)(data_p_) + 7;	\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
	*(mpig_dc_data_p__)-- = *(mpig_dc_buf__)++;				\
    }										\
}

#define mpig_dc_getn_uint64(endian_, buf_, data_, num_)			\
{									\
    if ((endian_) == MPIG_MY_ENDIAN)					\
    {									\
	memcpy((void *)(buf_), (void *)(data_), (num_) * 8);		\
    }									\
    else								\
    {									\
	unsigned char * mpig_dc_buf__ = (unsigned char *)(buf_);	\
	unsigned char * mpig_dc_data__ = (unsigned char *)(data_p) + 7;	\
									\
	for (n__ = 0; n__ < (num_); n__++)				\
	{								\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    *(mpig_dc_data__)-- = *(mpig_dc_buf__)++;			\
	    mpig_dc_data__ += 16;					\
	}								\
    }									\
}
/**********************************************************************************************************************************
						       END DATA CONVERSION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: the i/o vector routines are not thread safe.  it is the responsibility of the calling routine to insure that a i/o
 * vector object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data structure
 * containing i/o vector object.
 *
 * MT-RC-NOTE: on release consistent systems, if the i/o vector object is to be used by any thread other than the one in which it
 * was created, then the construction of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex is
 * not used as the means of insuring atomic access to the i/o vector, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */
MPIU_Size_t mpig_iov_unpack_fn(const void * buf, MPIU_Size_t buf_size, mpig_iov_t * iov);

#define mpig_iov_construct(iov_, max_entries_)			\
{								\
    ((mpig_iov_t *)(iov_))->max_entries = (max_entries_);	\
    mpig_iov_reset(iov_, 0);					\
}

#define mpig_iov_destruct(iov_)	\
{				\
    ((mpig_iov_t *)(iov_))->max_entries = -1;				\
    ((mpig_iov_t *)(iov_))->free_entry = -1;				\
    ((mpig_iov_t *)(iov_))->cur_entry = -1;				\
    ((mpig_iov_t *)(iov_))->num_bytes = 0;				\
}

#define mpig_iov_reset(iov_, num_prealloc_entries_)			\
{									\
    ((mpig_iov_t *)(iov_))->free_entry = (num_prealloc_entries_);	\
    ((mpig_iov_t *)(iov_))->cur_entry = 0;				\
    ((mpig_iov_t *)(iov_))->num_bytes = 0;				\
}

#define mpig_iov_nullify(iov_)			\
{						\
    ((mpig_iov_t *)(iov_))->max_entries = 0;	\
}

#define mpig_iov_is_null(iov_) (((mpig_iov_t *)(iov_))->max_entries == 0)

#define mpig_iov_set_entry(iov_, entry_, buf_, bytes_)					\
{											\
    MPIU_Assert((entry_) < ((mpig_iov_t *)(iov_))->max_entries);			\
    ((mpig_iov_t *)(iov_))->iov[entry_].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(buf_);	\
    ((mpig_iov_t *)(iov_))->iov[entry_].MPID_IOV_LEN = (bytes_);			\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);					\
}

#define mpig_iov_add_entry(iov_, buf_, bytes_)									\
{														\
    MPIU_Assert(((mpig_iov_t *)(iov_))->free_entry < ((mpig_iov_t *)(iov_))->max_entries);			\
    ((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry].MPID_IOV_BUF = (MPID_IOV_BUF_CAST)(buf_);	\
    ((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry].MPID_IOV_LEN = (bytes_);			\
    ((mpig_iov_t *)(iov_))->free_entry += 1;									\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);								\
}

#define mpig_iov_inc_num_inuse_entries(iov_, n_)						\
{												\
    ((mpig_iov_t *)(iov_))->free_entry += (n_);							\
    MPIU_Assert(((mpig_iov_t *)(iov_))->free_entry <= ((mpig_iov_t *)(iov_))->max_entries);	\
}

#define mpig_iov_inc_current_entry(iov_, n_)	\
{						\
    ((mpig_iov_t *)(iov_))->cur_entry += (n_);	\
}

#define mpig_iov_inc_num_bytes(iov_, bytes_)		\
{							\
    ((mpig_iov_t *)(iov_))->num_bytes += (bytes_);	\
}

#define mpig_iov_dec_num_bytes(iov_, bytes_)		\
{							\
    ((mpig_iov_t *)(iov_))->num_bytes -= (bytes_);	\
}

#define mpig_iov_get_base_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[0])

#define mpig_iov_get_current_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->cur_entry])

#define mpig_iov_get_next_free_entry_ptr(iov_) (&((mpig_iov_t *)(iov_))->iov[((mpig_iov_t *)(iov_))->free_entry])

#define mpig_iov_get_num_entries(iov_) (((mpig_iov_t *)(iov_))->max_entries)

#define mpig_iov_get_num_inuse_entries(iov_) (((mpig_iov_t *)(iov_))->free_entry - ((mpig_iov_t *)(iov_))-> cur_entry)

#define mpig_iov_get_num_free_entries(iov_) (((mpig_iov_t *)(iov_))->max_entries - ((mpig_iov_t *)(iov_))->free_entry)

#define mpig_iov_get_num_bytes(iov_) (((mpig_iov_t *)(iov_))->num_bytes)

#define mpig_iov_unpack(buf_, nbytes_, iov_) mpig_iov_unpack_fn((buf_), (nbytes_), (mpig_iov_t *)(iov_));
/**********************************************************************************************************************************
						     END I/O VECTOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN DATA BUFFER SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: the data buffer routines are not thread safe.  it is the responsibility of the calling routine to insure that a data
 * buffer object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data structure
 * containing data buffer object.
 *
 * MT-RC-NOTE: on release consistent systems, if the data buffer object is to be used by any thread other than the one in which
 * it was created, then the construction of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex
 * is not used as the means of insuring atomic access to the data bufffer, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */

int mpig_databuf_create(MPIU_Size_t size, mpig_databuf_t ** dbufp);

void mpig_databuf_destroy(mpig_databuf_t * dbuf);

#define mpig_databuf_construct(dbuf_, size_)		\
{							\
    ((mpig_databuf_t *)(dbuf_))->size = (size_);	\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;		\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;		\
}

#define mpig_databuf_destruct(dbuf_)		\
{						\
    ((mpig_databuf_t *)(dbuf_))->size = 0;	\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;	\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;	\
}
								
#define mpig_databuf_reset(dbuf_)		\
{						\
    ((mpig_databuf_t *)(dbuf_))->eod = 0;	\
    ((mpig_databuf_t *)(dbuf_))->pos = 0;	\
}

#define mpig_databuf_get_base_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t))

#define mpig_databuf_get_pos_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t) + ((mpig_databuf_t *)(dbuf_))->pos)

#define mpig_databuf_get_eod_ptr(dbuf_) ((char *)(dbuf_) + sizeof(mpig_databuf_t) + ((mpig_databuf_t *)(dbuf_))->eod)

#define mpig_databuf_get_size(dbuf_) (((mpig_databuf_t *)(dbuf_))->size)

#define mpig_databuf_get_eod(dbuf_) (((mpig_databuf_t *)(dbuf_))->eod)

#define mpig_databuf_set_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->size - (val_) <= ((mpig_databuf_t *)(dbuf_))->size);				\
    ((mpig_databuf_t *)(dbuf_))->eod = (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - set eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_inc_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod + (val_) <= ((mpig_databuf_t *)(dbuf_))->size);				\
    ((mpig_databuf_t *)(dbuf_))->eod += (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - inc eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_dec_eod(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod - (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->eod -= (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - dec eod: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_get_pos(dbuf_) (((mpig_databuf_t *)(dbuf_))->pos)

#define mpig_databuf_set_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->eod - (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->pos = (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - set pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_inc_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->pos + (val_) <= ((mpig_databuf_t *)(dbuf_))->eod);					\
    ((mpig_databuf_t *)(dbuf_))->pos += (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - inc pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_dec_pos(dbuf_, val_)											\
{																\
    MPIU_Assert(((mpig_databuf_t *)(dbuf_))->pos - (val_) <= ((mpig_databuf_t *)(dbuf_))->pos);					\
    ((mpig_databuf_t *)(dbuf_))->pos -= (MPIU_Size_t)(val_);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_DATABUF,										\
	"databuf - dec pos: databuf=" MPIG_PTR_FMT ", val=" MPIG_SIZE_FMT ", size=" MPIG_SIZE_FMT ", pos=" MPIG_SIZE_FMT	\
	", eod=" MPIG_SIZE_FMT ", eodp=" MPIG_PTR_FMT ", posp=" MPIG_PTR_FMT, MPIG_PTR_CAST(dbuf_), (MPIU_Size_t)(val_),	\
	((mpig_databuf_t *)(dbuf_))->size, ((mpig_databuf_t *)(dbuf_))->pos, ((mpig_databuf_t *)(dbuf_))->eod,			\
	MPIG_PTR_CAST(mpig_databuf_get_eod_ptr(dbuf_)), MPIG_PTR_CAST(mpig_databuf_get_pos_ptr(dbuf_))));			\
}

#define mpig_databuf_get_remaining_bytes(dbuf_) (mpig_databuf_get_eod(dbuf_) - mpig_databuf_get_pos(dbuf_))

#define mpig_databuf_get_free_bytes(dbuf_) (mpig_databuf_get_size(dbuf_) - mpig_databuf_get_eod(dbuf_))
/**********************************************************************************************************************************
						     END DATA BUFFER SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN STRING SPACE SECTION
**********************************************************************************************************************************/
typedef struct mpig_strspace
{
    char * base;
    size_t size;
    size_t eod;
    size_t pos;
}
mpig_strspace_t;

int mpig_strspace_grow(mpig_strspace_t * space, size_t growth);

int mpig_strspace_import_string(mpig_strspace_t * space, const char * str);

int mpig_strspace_add_element(mpig_strspace_t * space, const char * str, size_t growth);

int mpig_strspace_extract_next_element(mpig_strspace_t * space, size_t growth, char ** str_p);

#define mpig_strspace_construct(space_)	\
{					\
    (space_)->base = NULL;		\
    (space_)->size = 0;			\
    (space_)->eod = 0;			\
    (space_)->pos = 0;			\
}

#define mpig_strspace_destruct(space_)			\
{							\
    if ((space_)->base) MPIU_Free((space_)->base);	\
    (space_)->base = MPIG_INVALID_PTR;			\
    (space_)->size = 0;					\
    (space_)->eod = 0;					\
    (space_)->pos = 0;					\
}

#define mpig_strspace_reset(space_)	\
{					\
    (space_)->eod = 0;			\
    (space_)->pos = 0;			\
}

#define mpig_strspace_get_base_ptr(space_) ((space_)->base)

#define mpig_strspace_get_eod_ptr(space_) ((space_)->base + (space_)->eod)

#define mpig_strspace_get_pos_ptr(space_) ((space_)->base + (space_)->pos)

#define mpig_strspace_set_eod(space_, val_)	\
{						\
    (space_)->eod = (val_);			\
}

#define mpig_strspace_inc_eod(space_, val_)	\
{						\
    (space_)->eod += (val_);			\
}

#define mpig_strspace_dec_eod(space_, val_)	\
{						\
    (space_)->eod -= (val_);			\
}

#define mpig_strspace_set_pos(space_, val_)	\
{						\
    (space_)->pos = (val_);			\
}

#define mpig_strspace_inc_pos(space_, val_)	\
{						\
    (space_)->pos += (val_);			\
}

#define mpig_strspace_dec_pos(space_, val_)	\
{						\
    (space_)->pos -= (val_);			\
}

#define mpig_strspace_get_size(space_) ((space_)->size)

#define mpig_strspace_get_eod(space_) ((space_)->eod)

#define mpig_strspace_get_remaining_bytes(space_) ((space_)->eod - (space_)->pos)

#define mpig_strspace_get_free_bytes(space_) ((space_)->size - (space_)->eod)

#define mpig_strspace_free_extracted_element(elem_)	\
{							\
    MPIU_Free(elem_);					\
}
/**********************************************************************************************************************************
						    END STRING SPACE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN BUSINESS CARD SECTION
**********************************************************************************************************************************/
/*
 * NOTE: to insure that the memory associated with the strings returned by mpig_bc_get_contact() and mpig_bc_serialize_object()
 * is freed, the caller is responsible mpig_bc_free_contact() and mpig_bc_free_serialized_object() respectively.
 *
 * MT-NOTE: the business card routines are not thread safe.  it is the responsibility of the calling routine to insure that a
 * business card object is accessed atomically.  this is typically handled by performing a mutex lock/unlock on the data
 * structure containing business card object.
 *
 * MT-RC-NOTE: on release consistent systems, if the business card object is to be used by any thread other than the one in which
 * it was created, then the creation of the object must be followed by a RC release or mutex unlock.  furthermore, if a mutex is
 * not used as the means of insuring atomic access to the business card, it will also be necessary to perform RC acquires and
 * releases to guarantee that up-to-date data and internal state is visible at the appropriate times.  (see additional notes in
 * sections for other objects concerning issues with super lazy release consistent systems like Treadmarks.)
 */

int mpig_bc_copy(const mpig_bc_t * orig_bc, mpig_bc_t * new_bc);
    
int mpig_bc_add_contact(mpig_bc_t * bc, const char * key, const char * value);

int mpig_bc_get_contact(const mpig_bc_t * bc, const char * key, char ** value, bool_t * found_p);

void mpig_bc_free_contact(char * value);

int mpig_bc_serialize_object(mpig_bc_t * bc, char ** str);

void mpig_bc_free_serialized_object(char * str);

int mpig_bc_deserialize_object(const char *, mpig_bc_t * bc);


#define mpig_bc_construct(bc_)	\
{				\
    (bc_)->str_begin = NULL;	\
    (bc_)->str_end = NULL;	\
    (bc_)->str_size = 0;	\
    (bc_)->str_left = 0;	\
}

#define mpig_bc_destruct(bc_)				\
{							\
    if ((bc_)->str_begin) MPIU_Free((bc_)->str_begin);	\
    (bc_)->str_begin = MPIG_INVALID_PTR;		\
    (bc_)->str_end = MPIG_INVALID_PTR;			\
    (bc_)->str_size = 0;				\
    (bc_)->str_left = 0;				\
}
/**********************************************************************************************************************************
						    END BUSINESS CARD SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/
/* MT-NOTE: the following routine locks the VC and associated PG mutexes.  previous routines on the call stack must not be
   holding those mutexes. */
void mpig_vc_release_ref(mpig_vc_t * vc);

/* MT-NOTE: the following routines assume the VC mutex is already held by the current context */
int mpig_vc_construct_contact_info(mpig_vc_t * vc);

void mpig_vc_destruct_contact_info(mpig_vc_t * vc);

int mpig_vc_select_comm_method(mpig_vc_t * vc);

/* NOTE: the following routine should never be called.  it exists only to help identify problems with missing entries in the VC
   vtable defined by a communication module */
double mpig_vc_vtable_last_entry(float foo, int bar, const short * baz, char bif);

/*
 * constructor/destructor macros
 *
 * MT-RC-NOTE: on release consistent systems, if the virtual connection object is to be used by any thread other than the one in
 * which it was created, then the construction of the object must be followed by a RC release or mutex unlock.  for systems using
 * super lazy release consistency models, such as the one used in Treadmarks, it would be necessary to perform an acquire/lock in
 * the mpig_vc_construct() immediately after creating the mutex.  The calling routine would need to perform a release after the
 * temp VC was completely initialized.  we have chosen to assume that none of the the systems upon which MPIG will run have that
 * lazy of a RC model.
 */
#define mpig_vc_construct(vc_)				\
{							\
    mpig_vc_mutex_construct(vc_);			\
    mpig_vc_i_set_ref_count((vc_), 0);			\
    mpig_vc_set_initialized((vc_), FALSE);		\
    mpig_vc_set_cm((vc_), NULL);			\
    mpig_vc_set_vtable((vc_), NULL);			\
    mpig_vc_set_pg_info((vc_), NULL, -1);		\
    mpig_bc_construct(mpig_vc_get_bc(vc_));		\
    (vc_)->cms.ci_initialized = FALSE;			\
    (vc_)->cms.topology_num_levels = -1;		\
    (vc_)->cms.topology_levels = 0;			\
    (vc_)->cms.app_num = -1;				\
    (vc_)->cms.lan_id = NULL;				\
    (vc_)->lpid = -1;					\
}

#define mpig_vc_destruct(vc_)						\
{									\
    if ((vc_)->vtable != NULL && (vc_)->vtable->vc_destruct != NULL)	\
    {									\
	(vc_)->vtable->vc_destruct(vc_);				\
    }									\
    mpig_vc_destruct_contact_info(vc_);					\
    mpig_vc_i_set_ref_count((vc_), 0);					\
    mpig_vc_set_initialized((vc_), FALSE);				\
    mpig_vc_set_cm((vc_), MPIG_INVALID_PTR);				\
    mpig_vc_set_vtable((vc_), MPIG_INVALID_PTR);			\
    mpig_vc_set_pg_info((vc_), MPIG_INVALID_PTR, -1);			\
    mpig_bc_destruct(mpig_vc_get_bc(vc_));				\
    (vc_)->cms.ci_initialized = FALSE;					\
    (vc_)->cms.topology_num_levels = -1;				\
    (vc_)->cms.topology_levels = 0;					\
    (vc_)->cms.app_num = -1;						\
    (vc_)->cms.lan_id = MPIG_INVALID_PTR;				\
    (vc_)->lpid = -1;							\
    mpig_vc_mutex_destruct(vc_);					\
}

/*
 * reference counting macros
 *
 * MT-NOTE: these macros are not thread safe.  their use will likely require locking the PG mutex, although performing a RC
 * acquire/release may prove sufficient, depending on the level of synchronization required.
 */
#define mpig_vc_i_set_ref_count(vc_, ref_count_)	\
{							\
    (vc_)->ref_count = (ref_count_);			\
}

#define mpig_vc_set_ref_count(vc_, ref_count_)									\
{														\
    mpig_vc_i_set_ref_count((vc_), (ref_count_));								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	"VC - set ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", MPIG_PTR_CAST(vc_), (vc_)->ref_count));	\
}

#define mpig_vc_inc_ref_count(vc_, was_inuse_p_)									\
{															\
    if ((vc_)->vtable->vc_inc_ref_count == NULL)									\
    {															\
	*(was_inuse_p_) = ((vc_)->ref_count++);										\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	    "VC - increment ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", MPIG_PTR_CAST(vc_), (vc_)->ref_count));	\
    }															\
    else														\
    {															\
	(vc_)->vtable->vc_inc_ref_count((vc_), (was_inuse_p_));								\
    }															\
}

#define mpig_vc_dec_ref_count(vc_, is_inuse_p_)										\
{															\
    if ((vc_)->vtable->vc_dec_ref_count == NULL)									\
    {															\
	*(is_inuse_p_) = (--(vc_)->ref_count);										\
	MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_VC,						\
	    "VC - decrement ref count: vc=" MPIG_PTR_FMT ", ref_count=%d", MPIG_PTR_CAST(vc_), (vc_)->ref_count));	\
    }															\
    else														\
    {															\
	(vc_)->vtable->vc_dec_ref_count((vc_), (is_inuse_p_));								\
    }															\
}

#define mpig_vc_get_ref_count(vc_) ((vc_)->ref_count)

/*
 * miscellaneous accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex.  alternatively, it may be sufficient
 * for the get routines to be preceeded by a RC acquire and the set routines to be followed a RC release.  the specific method
 * depends on the level of synchronization required.
 */
#define mpig_vc_set_initialized(vc_, flag_)	\
{						\
    (vc_)->initialized = (flag_);		\
}

#define mpig_vc_is_initialized(vc_) ((vc_)->initialized)

#define mpig_vc_set_cm(vc_, cm_)	\
{					\
    (vc_)->cm = (cm_);			\
}

#define mpig_vc_get_cm(vc_) ((vc_)->cm)

#define mpig_vc_get_cm_module_type(vc_) ((vc_)->cm->module_type)

#define mpig_vc_get_cm_name(vc_) ((vc_)->cm->name)

#define mpig_vc_get_cm_vtable(vc_) mpig_cm_get_vtable((vc_)->cm)

#define mpig_vc_set_vtable(vc_, vtable_)	\
{						\
    (vc_)->vtable = (vtable_);			\
}

#define mpig_vc_get_vtable(vc_, func_) ((const mpig_vc_vtable_t *)((vc_)->vtable))

#define mpig_vc_set_pg_info(vc_, pg_, pg_rank_)	\
{						\
    (vc_)->pg = (pg_);				\
    (vc_)->pg_rank = (pg_rank_);		\
}

#define mpig_vc_get_pg(vc_) ((vc_)->pg)

#define mpig_vc_get_pg_rank(vc_) ((vc_)->pg_rank)

#define mpig_vc_get_bc(vc_) (&(vc_)->bc)

#define mpig_vc_get_num_topology_levels(vc_) ((vc_)->cms.topology_num_levels)

#define mpig_vc_get_topology_levels(vc_) ((vc_)->cms.topology_levels)

#define mpig_vc_get_app_num(vc_) ((vc_)->cms.app_num)

#define mpig_vc_get_lan_id(vc_) ((vc_)->cms.lan_id)

/* Thread safety and release consistency macros */
#define mpig_vc_mutex_construct(vc_)	mpig_mutex_construct(&(vc_)->mutex)
#define mpig_vc_mutex_destruct(vc_)	mpig_mutex_destruct(&(vc_)->mutex)
#define mpig_vc_mutex_lock(vc_)						\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,	\
	"VC - acquiring mutex: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc_)));	\
    mpig_mutex_lock(&(vc_)->mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,	\
	"VC - mutex acquired: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc_)));	\
}
#define mpig_vc_mutex_unlock(vc_)					\
{									\
    mpig_mutex_unlock(&(vc_)->mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,	\
	"VC - mutex released: vc=" MPIG_PTR_FMT, MPIG_PTR_CAST(vc_)));	\
}

#define mpig_vc_mutex_lock_conditional(vc_, cond_)	{if (cond_) mpig_vc_mutex_lock(vc_);}
#define mpig_vc_mutex_unlock_conditional(vc_, cond_)	{if (cond_) mpig_vc_mutex_unlock(vc_);}

#define mpig_vc_rc_acq(vc_, needed_)	mpig_vc_mutex_lock(vc_)
#define mpig_vc_rc_rel(vc_, needed_)	mpig_vc_mutex_unlock(vc_)
/**********************************************************************************************************************************
						 END VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/
/*
 * mpig_vcrt object definition
 */
typedef struct mpig_vcrt
{
    mpig_mutex_t mutex;

    /* number of references to this object */
    int ref_count;

    /* number of entries in the table */
    int size;

    /* array of virtual connection references (pointers to VCs) */
    mpig_vc_t * vcr_table[1];
}
mpig_vcrt_t;


int mpig_vcrt_serialize_object(mpig_vcrt_t * vcrt, char ** str_p);

void mpig_vcrt_free_serialized_object(char * str);

int mpig_vcrt_deserialize_object(char * str, mpig_vcrt_t ** vcrt_p);

#define mpig_vcrt_set_vc(vcrt_, p_, vc_)	\
{						\
    (vcrt_)->vcr_table[p_] = (vc_);		\
}
	
#define mpig_vcrt_size(vcrt_) ((vcrt_)->size)

/* Thread safety and release consistency macros */
#define mpig_vcrt_mutex_construct(vcrt_)	mpig_mutex_construct(&(vcrt_)->mutex)
#define mpig_vcrt_mutex_destruct(vcrt_)		mpig_mutex_destruct(&(vcrt_)->mutex)
#define mpig_vcrt_mutex_lock(vcrt_)						\
{										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,		\
    "VCRT - acquiring mutex: vcrt=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_)));	\
    mpig_mutex_lock(&(vcrt_)->mutex);						\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,		\
	"VCRT - mutex acquired: vcrt=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_)));	\
}
#define mpig_vcrt_mutex_unlock(vcrt_)						\
{										\
    mpig_mutex_unlock(&(vcrt_)->mutex);						\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_VC,		\
	"VCRT - mutex released: vcrt=" MPIG_PTR_FMT, MPIG_PTR_CAST(vcrt_)));	\
}

#define mpig_vcrt_mutex_lock_conditional(vcrt_, cond_)	    {if (cond_) mpig_vcrt_mutex_lock(vcrt_);}
#define mpig_vcrt_mutex_unlock_conditional(vcrt_, cond_)    {if (cond_) mpig_vcrt_mutex_unlock(vcrt_);}

#define mpig_vcrt_rc_acq(vcrt_, needed_)	mpig_vcrt_mutex_lock(vcrt_)
#define mpig_vcrt_rc_rel(vcrt_, needed_)	mpig_vcrt_mutex_unlock(vcrt_)
/**********************************************************************************************************************************
					END VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS GROUP SECTION
**********************************************************************************************************************************/
int mpig_pg_init(void);

int mpig_pg_finalize(void);

int mpig_pg_acquire_ref(const char * pg_id, int pg_size, bool_t locked, mpig_pg_t ** pg_p, bool_t * committed_p);

void mpig_pg_release_ref(mpig_pg_t * pg);

void mpig_pg_commit(mpig_pg_t * pg);

/*
 *reference counting macros
 *
 * MT-NOTE: these macros are not thread safe.  their use will likely require locking the PG mutex, although performing a RC
 * acquire/release may prove sufficient, depending on the level of synchronization required.
 */
#define mpig_pg_inc_ref_count(pg_)											\
{															\
    (pg_)->ref_count++;													\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_PG,							\
	"PG - increment ref count: pg=" MPIG_PTR_FMT ", ref_count=%d", MPIG_PTR_CAST(pg_), (pg_)->ref_count));		\
}


#define mpig_pg_dec_ref_count(pg_, is_inuse_p_)										\
{															\
    *(is_inuse_p_) = (--(pg_)->ref_count);										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_PG,							\
	"PG - decrement ref count: pg=" MPIG_PTR_FMT ", ref_count=%d", MPIG_PTR_CAST(pg_), (pg_)->ref_count));		\
}

/*
 * miscellaneous accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex.  alternatively, it may be sufficient
 * for the get routines to be preceeded by a RC acquire and the set routines to be followed a RC release.  the specific method
 * depends on the level of synchronization required.
 */
#define mpig_pg_get_size(pg_) ((pg_)->size)

#define mpig_pg_get_rank(pg_) ((pg_)->rank)

/* MT-NOTE: this routine should only be called if the VC is known to have a reference count greater than zero.  if the reference
   count could be zero indicating the object is being or has been destroyed, then use mpig_pg_get_vc_ref() */
#define mpig_pg_get_vc(pg_, rank_, vc_p_)	\
{						\
    *(vc_p_) = &(pg_)->vct[rank_];		\
}

/*
 * process group ID accessor macros
 *
 * MT-NOTE: these macros are not thread safe.  their use may require locking the PG mutex or performing a RC acquire.
 */
#define mpig_pg_get_id(pg_) ((pg_)->id)

#define mpig_pg_compare_ids(id1_, id2_) (strcmp((id1_), (id2_)))

/*
 * macros for accessing static process group information given a VC
 *
 * MT-RC-NOTE: these macros assume that the information be access is static for the lifetime of the PG object, and that some form
 * of RC acquire has been performed to insure the information for a newly created object is up-to-date and not stale data from
 * previous object utilizing the same memory space.  acquiring the VC mutex, or performing a RC acuqire on the VC, is the most
 * common way to insure that the PG data is current.
 */
#define mpig_vc_get_pg_id(vc_) ((vc_)->pg->id)

#define mpig_vc_get_pg_size(vc_) ((vc_)->pg->size)

/*
 * thread saftey and release consistency macros
 */
#define mpig_pg_mutex_construct(pg_)	mpig_mutex_construct(&(pg_)->mutex)
#define mpig_pg_mutex_destruct(pg_)     mpig_mutex_destruct(&(pg_)->mutex)
#define mpig_pg_mutex_lock(pg_)						\
{									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG,	\
	"PG - acquiring mutex: pg=" MPIG_PTR_FMT, MPIG_PTR_CAST(pg_)));	\
    mpig_mutex_lock(&(pg_)->mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG,	\
	"PG - mutex acquired: pg=" MPIG_PTR_FMT, MPIG_PTR_CAST(pg_)));	\
}
#define mpig_pg_mutex_unlock(pg_)					\
{									\
    mpig_mutex_unlock(&(pg_)->mutex);					\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_THREADS | MPIG_DEBUG_LEVEL_PG,	\
	"PG - mutex released: pg=" MPIG_PTR_FMT, MPIG_PTR_CAST(pg_)));	\
}
#define mpig_pg_mutex_lock_conditional(pg_, cond_)	{if (cond_) mpig_pg_mutex_lock(pg_);}
#define mpig_pg_mutex_unlock_conditional(pg_, cond_)	{if (cond_) mpig_pg_mutex_unlock(pg_);}
/**********************************************************************************************************************************
						    END PROCESS GROUP SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN RECEIVE QUEUE SECTION
**********************************************************************************************************************************/
/*
 * MT-NOTE: with the exception of the init and finalize routines, the receive queue routines are thread safe.
 *
 * the routines that return a request, return it with its mutex locked by the current context.  this allows the calling routine
 * an opportunity to initialize an enqueued request before it is accessible to a thread which has dequeued it.  likewise, it
 * allows another thread still initializing a request to complete before the request is made available to other routines.
 *
 * the routines that take a request as a parameter expect that the calling routine will acquire the request mutex either before
 * calling the recvq routine or directly after calling the recvq routine.  In the latter case, acquiring the mutex is only
 * necessary if the request be found and dequeued.  Acquiring the mutex guarantees that no other thread is in the process of
 * initializing the request.
 */
int mpig_recvq_init(void);

int mpig_recvq_finalize(void);

int mpig_recvq_find_unexp_and_extract_status(int rank, int tag, MPID_Comm * comm, int ctx, bool_t * found_p,
    MPI_Status * status);

struct MPID_Request * mpig_recvq_find_unexp_and_deq(int rank, int tag, int ctx, MPI_Request sreq_id);

bool_t mpig_recvq_deq_unexp_rreq(struct MPID_Request * rreq);

int mpig_recvq_cancel_posted_rreq(struct MPID_Request * rreq, bool_t * cancelled_p);

int mpig_recvq_deq_unexp_or_enq_posted(int rank, int tag, int ctx, const mpig_msg_op_params_t * recv_any_source_params,
    int * found_p, MPID_Request ** rreq_p);

int mpig_recvq_deq_posted_or_enq_unexp(mpig_vc_t * vc, int rank, int tag, int ctx, int * found_p, MPID_Request ** rreq_p);

#if defined(MPIG_VMPI)
bool_t mpig_recvq_deq_posted_ras_req(struct MPID_Request * rreq, bool_t vcancelled);

int mpig_recvq_unregister_ras_vreqs(void);

#define mpig_recvq_ras_op_signal_complete(ras_op_)	\
{							\
    (ras_op_)->complete = TRUE;				\
    if ((ras_op_)->cond_in_use)				\
    {							\
	mpig_cond_signal(&(ras_op_)->cond);		\
    }							\
}

#define mpig_recvq_ras_op_get_rreq(ras_op_) ((ras_op_)->req)

#define mpig_recvq_ras_op_set_cancelled_state(ras_op_, flag_)		\
{                                                                       \
    (ras_op_)->cancelled = (flag_);                                     \
}

#define mpig_recvq_ras_op_get_cancelled_state(ras_op_) ((ras_op_)->cancelled)
#endif
/**********************************************************************************************************************************
						    END RECEIVE QUEUE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN PROCESS MANAGEMENT SECTION
**********************************************************************************************************************************/
struct mpig_pm;

int mpig_pm_init(int * argc, char *** argv);

int mpig_pm_finalize(void);

int mpig_pm_abort(int exit_code);

int mpig_pm_exchange_business_cards(mpig_bc_t * bc, mpig_bc_t ** bcs_p);

int mpig_pm_free_business_cards(mpig_bc_t * bcs);

int mpig_pm_get_pg_size(int * pg_size);

int mpig_pm_get_pg_rank(int * pg_rank);

int mpig_pm_get_pg_id(const char ** pg_id_p);

int mpig_pm_get_app_num(const mpig_bc_t * bc, int * app_num);

float * mpig_pm_vtable_last_entry(double foo, int bar, const float * baz, short bif);


typedef int (*mpig_pm_init_fn_t)(int * argc, char *** argv, struct mpig_pm * pm, bool_t * my_pm);

typedef int (*mpig_pm_finalize_fn_t)(struct mpig_pm * pm);

typedef int (*mpig_pm_abort_fn_t)(struct mpig_pm * pm, int exit_code);

typedef int (*mpig_pm_exchange_business_cards_fn_t)(struct mpig_pm * pm, mpig_bc_t * bc, mpig_bc_t ** bcs_p);

typedef int (*mpig_pm_free_business_cards_fn_t)(struct mpig_pm * pm, mpig_bc_t * bcs);

typedef int (*mpig_pm_get_pg_size_fn_t)(struct mpig_pm * pm, int * pg_size);

typedef int (*mpig_pm_get_pg_rank_fn_t)(struct mpig_pm * pm, int * pg_rank);

typedef int (*mpig_pm_get_pg_id_fn_t)(struct mpig_pm * pm, const char ** pg_id_p);

typedef int (*mpig_pm_get_app_num_fn_t)(struct mpig_pm * pm, const mpig_bc_t * bc, int * app_num);

typedef float * (*mpig_pm_vtable_last_entry_fn_t)(double foo, int bar, const float * baz, short bif);


typedef struct mpig_pm_vtable
{
    mpig_pm_init_fn_t init;
    mpig_pm_finalize_fn_t finalize;
    mpig_pm_abort_fn_t abort;
    mpig_pm_exchange_business_cards_fn_t exchange_business_cards;
    mpig_pm_free_business_cards_fn_t free_business_cards;
    mpig_pm_get_pg_size_fn_t get_pg_size;
    mpig_pm_get_pg_rank_fn_t get_pg_rank;
    mpig_pm_get_pg_id_fn_t get_pg_id;
    mpig_pm_get_app_num_fn_t get_app_num;
    mpig_pm_vtable_last_entry_fn_t vtable_last_entry;
}
mpig_pm_vtable_t;

typedef struct mpig_pm
{
    const char * name;
    mpig_pm_vtable_t * vtable;
}
mpig_pm_t;

#define mpig_pm_get_name(pm_) ((pm_)->name)
#define mpig_pm_get_vtable(pm_) ((const mpig_pm_vtable_t *)((pm_)->vtable))
/**********************************************************************************************************************************
						 END PROCESS MANAGEMENT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN CONNECT-ACCEPT SECTION
**********************************************************************************************************************************/
/* NOTE: mpig_connection_t must be defined by the communication module providing connect/accept service */

int mpig_port_open(MPID_Info * info, char * port_name);

int mpig_port_close(const char * port_name);

int mpig_port_accept(const char * port_name, mpig_vc_t ** port_vc);

int mpig_port_connect(const char * port_name, mpig_vc_t ** port_vc);

int mpig_port_vc_send(mpig_vc_t * port_vc, void * buf, MPIU_Size_t nbytes);

int mpig_port_vc_recv(mpig_vc_t * port_vc, void * buf, MPIU_Size_t nbytes);

mpig_endian_t mpig_port_vc_get_endian(mpig_vc_t * port_vc);

int mpig_port_vc_close(mpig_vc_t * port_vc);
/**********************************************************************************************************************************
						   END CONNECT-ACCEPT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                                    BEGIN VENDOR MPI SECTION
**********************************************************************************************************************************/
#if defined(MPIG_VMPI)
void mpig_vmpi_error_to_mpich2_error(int vendor_errno, int * mpi_errno_p);
#endif
/**********************************************************************************************************************************
                                                     END VENDOR MPI SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                                       BEGIN UUID SECTION
**********************************************************************************************************************************/
#if defined(HAVE_UUID_GENERATE) && defined(HAVE_UUID_UNPARSE) && defined(HAVE_UUID_UUID_H)

#define mpig_uuid_nullify(uuid_p_)      \
{                                       \
    uuid_clear((uuid_p_)->uuid);        \
}
#define mpig_uuid_generate(uuid_p_) (uuid_generate((uuid_p_)->uuid), MPI_SUCCESS)
#define mpig_uuid_copy(uuid_src_p_, uuid_dest_p_)               \
{                                                               \
    uuid_copy((uuid_dest_p_)->uuid, (uuid_src_p_)->uuid);       \
}
#define mpig_uuid_parse(str_, uuid_p_) (uuid_parse((str_), (uuid_p_)->uuid), MPI_SUCCESS)
#define mpig_uuid_unparse(uuid_p_, str_)        \
{                                               \
    uuid_unparse((uuid_p_)->uuid, (str_));      \
}
#define mpig_uuid_compare(uuid1_p_, uuid2_p_) ((uuid_compare((uuid1_p_)->uuid, (uuid2_p_)->uuid) == 0) ? TRUE : FALSE)
#define mpig_uuid_is_null(uuid_p_) (uuid_is_null((uuid_p_)->uuid) ? TRUE : FALSE)

#elif defined (HAVE_GLOBUS_COMMON_MODULE)

#define mpig_uuid_nullify(uuid_p_)                              \
{                                                               \
    memset((uuid_p_)->binary.bytes, 0, 16);                     \
    memset((uuid_p_)->text, 0, MPIG_UUID_MAX_STR_LEN + 1);      \
}
#define mpig_uuid_generate(uuid_p_) (globus_uuid_create(uuid_p_) == GLOBUS_SUCCESS ? MPI_SUCCESS : MPI_ERR_OTHER)
#define mpig_uuid_copy(uuid_src_p_, uuid_dest_p_)                                       \
{                                                                                       \
    memcpy((uuid_dest_p_)->binary.bytes, (uuid_src_p_)->binary.bytes, 16);              \
    MPIU_Strncpy((uuid_dest_p_)->text, (uuid_src_p_)->text, GLOBUS_UUID_TEXTLEN + 1);   \
}
#define mpig_uuid_parse(str_, uuid_p_) ((globus_uuid_import((uuid_p_), (str_)) == GLOBUS_SUCCESS) ? MPI_SUCCESS : MPI_ERR_OTHER)
#define mpig_uuid_unparse(uuid_p_, str_)                                \
{                                                                       \
    MPIU_Strncpy((str_), (uuid_p_)->text, MPIG_UUID_MAX_STR_LEN + 1);   \
}
#define mpig_uuid_compare(uuid1_p_, uuid2_p_) (GLOBUS_UUID_MATCH(*(uuid1_p_), *(uuid2_p_)) ? TRUE : FALSE)
#define mpig_uuid_is_null(uuid_p_) ((uuid_p_)->text[0] == '\0')  ? TRUE : FALSE)

#else

#define mpig_uuid_nullify(uuid_p_)                              \
{                                                               \
    memset((uuid_p_)->str, 0, MPIG_UUID_MAX_STR_LEN + 1);       \
}
#define mpig_uuid_generate(uuid_p_) ((mpig_process.my_hostname[0] != '\0') ? (MPIU_Snprintf((uuid_p_)->str,                     \
        MPIG_UUID_MAX_STR_LEN + 1, "%s-%d", mpig_process.my_hostname, (int) mpig_process.my_pid), MPI_SUCCESS) : MPI_ERR_OTHER)
#define mpig_uuid_copy(uuid_src_p_, uuid_dest_p_)                                       \
{                                                                                       \
    MPIU_Strncpy((uuid_dest_p_)->str, (uuid_src_p_)->str, MPIG_UUID_MAX_STR_LEN + 1);   \
}
#define mpig_uuid_parse(str_, uuid_p_) (MPIU_Strncpy((uuid_p_)->str, (str_), MPIG_UUID_MAX_STR_LEN + 1), MPI_SUCCESS)
#define mpig_uuid_unparse(uuid_p_, str_)                                \
{                                                                       \
    MPIU_Strncpy((str_), (uuid_p_)->str, MPIG_UUID_MAX_STR_LEN + 1);    \
}
#define mpig_uuid_compare(uuid1_p_, uuid2_p_) ((strncmp((uuid1_p_)->str, (uuid2_p_)->str, MPIG_UUID_MAX_STR_LEN + 1) == 0) ?    \
    TRUE : FALSE)

#define mpig_uuid_is_null(uuid_p_) ((uuid_p_)->str[0] == '\0')  ? TRUE : FALSE)

#endif
/**********************************************************************************************************************************
                                                        END UUID SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                               BEGIN BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/
#include "mpig_bds_genq.i"
/**********************************************************************************************************************************
                                                END BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#undef FCNAME
#define FCNAME fcname

#if defined(MPIG_DEBUG)
#define MPIG_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIG_FUNCNAME_CHECK()
#define MPIG_FUNC_EXIT(a_)
#define MPIG_RMA_FUNC_ENTER(a_) MPIG_FUNCNAME_CHECK()
#define MPIG_RMA_FUNC_EXIT(a_)
#else
#define MPIG_STATE_DECL(a_) MPIDI_STATE_DECL(a_)
#define MPIG_FUNC_ENTER(a_) MPIDI_FUNC_ENTER(a_)
#define MPIG_FUNC_EXIT(a_) MPIDI_FUNC_EXIT(a_)
#define MPIG_RMA_FUNC_ENTER(a_) MPIDI_RMA_FUNC_ENTER(a_)
#define MPIG_RMA_FUNC_EXIT(a_) MPIDI_RMA_FUNC_EXIT(a_)
#endif
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN USAGE STAT ROUTINES
**********************************************************************************************************************************/

void mpig_usage_init(void);

void mpig_usage_finalize(void);

/**********************************************************************************************************************************
						     END USAGE STAT ROUTINES
**********************************************************************************************************************************/

#if defined(HAVE_C_INLINE)
#if defined(__GNUC__)
#define MPIG_EMIT_INLINE_FUNCS 1
#undef MPIG_INLINE_HDECL
#undef MPIG_INLINE_HDEF
#define MPIG_INLINE_HDEF
#else /* !defined(__GNUC__) --> C99? */
#define MPIG_EMIT_INLINE_FUNCS 1
#undef MPIG_INLINE_HDECL
#define MPIG_INLINE_HDECL extern
#undef MPIG_INLINE_HDEF
#endif /* end if/else __GNUC__ */
#else
#define MPIG_INLINE_HDECL
#undef MPIG_INLINE_HDEF
#define MPIG_INLINE
#endif /* end if/else defined(HAVE_C_INLINE) */


#endif /* MPICH2_MPIDIMPL_H_INCLUDED */
