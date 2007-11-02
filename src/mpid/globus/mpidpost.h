/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPOST_H_INCLUDED)
#define MPICH2_MPIDPOST_H_INCLUDED

/* STATES:NO WARNINGS */
#include "mpiallstates.h"

/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
int mpig_comm_construct(MPID_Comm * comm);

int mpig_comm_destruct(MPID_Comm * comm);

int mpig_comm_free_hook(MPID_Comm * comm);

#define MPID_DEV_COMM_FUNC_HOOK(func_, old_comm_, new_comm_, mpi_errno_p_)	\
{										\
    *(mpi_errno_p_) = MPIG_ ## func_ ## _HOOK((old_comm_), (new_comm_));	\
}

#define MPID_Dev_comm_create_hook(comm_)								\
{													\
    int MPID_Dev_comm_create_hook_mrc__;								\
    MPID_Dev_comm_create_hook_mrc__ = mpig_comm_construct(comm_);					\
    if (MPID_Dev_comm_create_hook_mrc__) MPID_Abort(NULL, MPID_Dev_comm_create_hook_mrc__, 13, NULL);	\
}

#define MPID_Dev_comm_destroy_hook(comm_)								\
{													\
    int MPID_Dev_comm_destroy_hook_mrc__;								\
    MPID_Dev_comm_destroy_hook_mrc__ = mpig_comm_destruct(comm_);					\
    if (MPID_Dev_comm_destroy_hook_mrc__) MPID_Abort(NULL, MPID_Dev_comm_destroy_hook_mrc__, 13, NULL);	\
}

#define MPIG_COMM_FREE_HOOK(old_comm_, new_comm_) mpig_comm_free_hook(old_comm_)
#define MPIG_COMM_DISCONNECT_HOOK(old_comm_, new_comm_) mpig_comm_free_hook(old_comm_)

#if defined(MPIG_VMPI)
#define MPIG_COMM_DUP_HOOK(old_comm_, new_comm_) mpig_cm_vmpi_comm_dup_hook((old_comm_), (new_comm_))
#define MPIG_COMM_CREATE_HOOK(old_comm_, new_comm_) mpig_cm_vmpi_comm_split_hook((old_comm_), (new_comm_))
#define MPIG_COMM_SPLIT_HOOK(old_comm_, new_comm_) mpig_cm_vmpi_comm_split_hook((old_comm_), (new_comm_))
#define MPIG_INTERCOMM_CREATE_HOOK(old_comm_, new_comm_) mpig_cm_vmpi_intercomm_create_hook((old_comm_), (new_comm_))
#define MPIG_INTERCOMM_MERGE_HOOK(old_comm_, new_comm_) mpig_cm_vmpi_intercomm_merge_hook((old_comm_), (new_comm_))
#else
#define MPIG_COMM_DUP_HOOK(old_comm_, new_comm_) (MPI_SUCCESS)
#define MPIG_COMM_CREATE_HOOK(old_comm_, new_comm_) (MPI_SUCCESS)
#define MPIG_COMM_SPLIT_HOOK(old_comm_, new_comm_) (MPI_SUCCESS)
#define MPIG_INTERCOMM_CREATE_HOOK(old_comm_, new_comm_) (MPI_SUCCESS)
#define MPIG_INTERCOMM_MERGE_HOOK(old_comm_, new_comm_) (MPI_SUCCESS)
#endif

/*
 * MT-RC-NOTE: this routine does not perform insure the pointer to the vtable in the VC and the the pointers within the function
 * table table are consistent across are threads.  as such, it is the responsibily of the calling routine to insure the VC and
 * the table to which it points is up-to-date before calling this routine.
 *
 * MT-RC-FIXME: give the use of this routine in the ADI3 macros below, it may be necessary for this routine to become a function
 * that performs an RC acquire on platforms that require it.  at present, this is not necessary since the vtable pointer set in
 * the VC by communication module is always set in the main thread, the same thread in which all of the MPI routines are called.
 * to guarantee this, the maximum application thread level is presently restricted to MPI_THREAD_SINGLE.  in addition, all
 * communication modules currently insure that the vtable pointer and vtable it points at remain unaltered for the lifetime of
 * the VC.
 *
 * MT-RC-NOTE: as an alternative to adding a RC acquire operation to this routine, the application thread level limitation could
 * be lifted if it was known that all MPI routines calling the ADI3 rotuines in the vtable performed some form of an acquire
 * operation before calling the ADI3 routine.  at one point, most MPI routines acquired a global mutex at the beginning of the
 * routine and released it at the end.  such an operation would be sufficient assuming the vtable pointer and the vtable it
 * pointed to remained constant until after the ADI3 routine is entered.
 */
#define mpig_comm_get_vc_vtable(comm_, rank_) (((rank_) >= 0) ? ((comm_)->vcr[(rank_)]->vtable) : mpig_cm_other_vc->vtable)
/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/
/*
 * NOTE: the setting, incrementing and decrementing of the request completion counter must be atomic since the progress engine
 * could be completing the request in one thread and the application could be cancelling the request in another thread.  if atomic
 * instructions are used, a write barrier or store-release may be needed to insure that updates to other fields in the request
 * structure are made visible before the completion counter is updated.  the exact operations required depending the on the
 * memory model of the processor architecture.
 */

/*
 * SUPER-IMPORTANT-MT-NOTE: requests with datatypes and communicators attached to them must only be created and destroyed in the
 * application threads.  these objects are not protected by any mutex that is presently acquired by the internal device threads.
 * as such, releasing the reference for these objects from internal device threads would be unsafe.
 */
MPID_Request * mpig_request_create(void);

void mpig_request_destroy(MPID_Request * req);

/*
 * request utility macros
 *
 * NOTE: these macros only reference publicly accessible functions and data, and therefore may be used in MPID macros.
 */
#define mpig_request_get_rank(req_) ((req_)->dev.rank)

#define mpig_request_i_set_ref_count(req_, ref_cnt_)	\
{							\
    (req_)->ref_count = (ref_cnt_);			\
}

#define mpig_request_set_ref_count(req_, ref_cnt_)									\
{															\
    mpig_request_i_set_ref_count((req_), (ref_cnt_));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "setting request ref count: req=" MPIG_HANDLE_FMT	\
	", reqp=" MPIG_PTR_FMT ", ref_count=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->ref_count));		\
}

#define mpig_request_inc_ref_count(req_, was_inuse_p_)										\
{																\
    *(was_inuse_p_) = (((req_)->ref_count)++);											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "incrementing request ref count: req=" MPIG_HANDLE_FMT	\
	", reqp=" MPIG_PTR_FMT ", ref_count=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->ref_count));			\
}

#define mpig_request_dec_ref_count(req_, is_inuse_p_)										\
{																\
    *(is_inuse_p_) = (--((req_)->ref_count));											\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "decrementing request ref count: req=" MPIG_HANDLE_FMT	\
	", reqp=" MPIG_PTR_FMT ", ref_count=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->ref_count));			\
}

#define mpig_request_i_set_cc(req_, cc_)	\
{						\
    (req_)->cc = (cc_);				\
}

#define mpig_request_set_cc(req_, cc_)										\
{														\
    mpig_request_i_set_cc((req_), (cc_));									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "setting request completion count: req=" 	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cc));	\
}

#define mpig_request_inc_cc(req_, was_complete_p_)									\
{															\
    *(was_complete_p_) = !((*(req_)->cc_ptr)++);									\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "incrementing request completion count: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cc));		\
}

#define mpig_request_dec_cc(req_, is_complete_p_)									\
{															\
    *(is_complete_p_) = !(--(*(req_)->cc_ptr));										\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_COUNT | MPIG_DEBUG_LEVEL_REQ, "decrementing request completion count: req="	\
	MPIG_HANDLE_FMT ", reqp=" MPIG_PTR_FMT ", cc=%d", (req_)->handle, MPIG_PTR_CAST(req_), (req_)->cc));		\
}

/*
 * SUPER-IMPORTANT-MT-NOTE: see the note at the top of this section concerning the creation and destruction of requests from
 * internal device threads.
 */
#define mpig_request_complete(req_)						\
{										\
    bool_t mpig_request_complete_is_complete__;					\
										\
    mpig_request_dec_cc((req_), &mpig_request_complete_is_complete__);		\
    if (mpig_request_complete_is_complete__)					\
    {										\
	bool_t mpig_request_complete_is_inuse__;				\
	mpig_request_dec_ref_count((req_), &mpig_request_complete_is_inuse__);	\
	if (mpig_request_complete_is_inuse__ == FALSE)				\
	{									\
	    mpig_request_destroy((req_));					\
	}									\
										\
        mpig_pe_wakeup();							\
    }										\
}

#define mpig_request_release(req_)						\
{										\
    bool_t mpig_request_release_is_inuse__;					\
										\
    mpig_request_dec_ref_count((req_), &mpig_request_release_is_inuse__);	\
    if (mpig_request_release_is_inuse__ == FALSE)				\
    {										\
	mpig_request_destroy(req_);						\
    }										\
}

/*
 * Request routines implemented as macros
 *
 * SUPER-IMPORTANT-MT-NOTE: see the note at the top of this section concerning the creation and destruction of requests from
 * internal device threads.
 */
#define MPID_Request_create() mpig_request_create()

#define MPID_Request_release(req_) mpig_request_release(req_)

#define MPID_Request_set_completed(req_)	\
{						\
    *(req_)->cc_ptr = 0;			\
    mpig_pe_wakeup();				\
}
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN ADI3 MAPPING SECTION
**********************************************************************************************************************************/
/* MT-RC-FIXME: at present, mpig_comm_get_vc_vtable does not perform a RC acquire.  see description above. */
#undef MPID_Send
#define MPID_Send(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_send((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Isend
#define MPID_Isend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_isend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Rsend
#define MPID_Rsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_rsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Irsend
#define MPID_Irsend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_irsend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Ssend
#define MPID_Ssend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_ssend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Issend
#define MPID_Issend(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_issend((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Recv
#define MPID_Recv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, status_ ,reqp_)						\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_recv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_),	\
	(status_), (reqp_)))

#undef MPID_Irecv
#define MPID_Irecv(buf_, cnt_, dt_, rank_, tag_, comm_, ctxoff_, reqp_)								\
     (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_irecv((buf_), (cnt_), (dt_), (rank_), (tag_), (comm_), (ctxoff_), (reqp_)))

#undef MPID_Probe
#define MPID_Probe(rank_, tag_, comm_, ctxoff_, status_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_probe((rank_), (tag_), (comm_), (ctxoff_), (status_)))

#undef MPID_Iprobe
#define MPID_Iprobe(rank_, tag_, comm_, ctxoff_, flag_p_, status_)							\
    (mpig_comm_get_vc_vtable((comm_), (rank_))->adi3_iprobe((rank_), (tag_), (comm_), (ctxoff_), (flag_p_), (status_)))

#undef MPID_Cancel_send
#define MPID_Cancel_send(sreq_) (mpig_comm_get_vc_vtable((sreq_)->comm, mpig_request_get_rank(sreq_))->adi3_cancel_send(sreq_))

#undef MPID_Cancel_recv
#define MPID_Cancel_recv(rreq_) (mpig_comm_get_vc_vtable((rreq_)->comm, mpig_request_get_rank(rreq_))->adi3_cancel_recv(rreq_))
/**********************************************************************************************************************************
						    END ADI3 MAPPING SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
extern mpig_pe_count_t mpig_pe_total_ops_count;
extern int mpig_pe_active_ops_count;
extern int mpig_pe_active_ras_ops_count;
extern int mpig_pe_polling_required_count;

#define HAVE_MPID_PROGRESS_START_MACRO
#undef MPID_Progress_start
#define MPID_Progress_start(state_)			\
{							\
    (state_)->dev.count = mpig_pe_total_ops_count;	\
    mpig_cm_vmpi_pe_start(state_);			\
    mpig_cm_xio_pe_start(state_);			\
}

#define HAVE_MPID_PROGRESS_END_MACRO
#undef MPID_Progress_end
#define MPID_Progress_end(state_)	\
{					\
    mpig_cm_vmpi_pe_end(state_);	\
    mpig_cm_xio_pe_end(state_);		\
}

#define HAVE_MPID_PROGRESS_POKE_MACRO
#undef MPID_Progress_poke
#define MPID_Progress_poke() MPID_Progress_test()

#define mpig_pe_register_cm(cm_info_p_)											\
{															\
    mpig_pe_polling_required_count += (cm_info_p_)->polling_required ? 1 : 0;						\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "progress engine registered with core: "	\
	"polling_required=%s, total_polling_required_count=%d", MPIG_BOOL_STR((cm_info_p_)->polling_required),		\
    mpig_pe_polling_required_count));											\
}

#define mpig_pe_unregister_cm(cm_info_p_)	\
{						\
}

#define mpig_pe_start_op()													\
{																\
    /* MT-NOTE: this routine should only be called by the application thread, so it is safe to update the counter without	\
       locking a  mutex. the communication modules are responsible for any locking they might require. */			\
    mpig_pe_active_ops_count += 1;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "start of operation: active_ops_count=%d",		\
	mpig_pe_active_ops_count));												\
}

#define mpig_pe_end_op()													\
{																\
    /* NOTE_MT: this routine should only be called by the application thread, so it is safe to update the counter without	\
       locking a mutex.  the communication modules are responsible for any locking they might require. */			\
    mpig_pe_active_ops_count -= 1;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "end of operation: active_ops_count=%d",		\
	mpig_pe_active_ops_count));												\
    MPIU_Assert(mpig_pe_active_ops_count >= 0);											\
}

#define mpig_pe_start_ras_op()													\
{																\
    /* MT-NOTE: this routine should only be called by the application thread, so it is safe to update the counter without	\
       locking a  mutex. the communication modules are responsible for any locking they might require. */			\
    mpig_pe_active_ras_ops_count += 1;												\
    mpig_pe_active_ops_count += 1;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "start of receive-any-source operation: "		\
	"active_ras_ops_count=%d, active_ops_count=%d", mpig_pe_active_ras_ops_count, mpig_pe_active_ops_count));		\
}

#define mpig_pe_end_ras_op()													\
{																\
    /* NOTE_MT: this routine should only be called by the application thread, so it is safe to update the counter without	\
       locking a mutex.  the communication modules are responsible for any locking they might require. */			\
    mpig_pe_active_ras_ops_count -= 1;												\
    mpig_pe_active_ops_count -= 1;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "end of receive-any-source operation: "		\
	"active_ras_ops_count=%d, active_ops_count=%d", mpig_pe_active_ras_ops_count, mpig_pe_active_ops_count));		\
    MPIU_Assert(mpig_pe_active_ops_count >= 0);											\
}

#define mpig_pe_wakeup()													\
{																\
    /* MT: this routine should only be called by the application thread, so it is safe to update the counter without locking a	\
       mutex.  the communication modules are responsible for any locking they might require. */					\
    mpig_pe_total_ops_count += 1;												\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_PROGRESS | MPIG_DEBUG_LEVEL_COUNT, "PE wakeup; op count incremented: op_count=%u",	\
	mpig_pe_total_ops_count));												\
    /* mpig_cm_xio_pe_wakeup(); -- XXX: MT-APP-NOTE: what is really needed here??? probably nothing until we support		\
       multithreaded applications */												\
}

#define mpig_pe_cm_has_active_ops(cm_info_p_) ((cm_info_p_)->active_ops + mpig_pe_active_ras_ops_count > 0)

#define mpig_pe_cm_owns_all_active_ops(cm_info_p_) ((cm_info_p_)->active_ops == mpig_pe_active_ops_count)

/* this routine returns TRUE if an operation has completed since the last call to MPID_Progress_start() or MPID_Progress_wait();
   otherwise, FALSE is return.  this routine's primarily use is to determine if a CM can perform a blocking operation during its
   pe_wait() routine.  if an operation has completed that the routine calling might be interested in, then the pe_wait() routine
   may not block. */
#define mpig_pe_op_has_completed(state_) ((state_)->dev.count != mpig_pe_total_ops_count)

/* this routine will return TRUE if any communication module other than the one associated with the supplied PE info structure
   requires polling in order to make progress; otherwise FALSE is returned.  in some cases, like probe, it is only important that
   progress be made on outstanding requests that keep the current operation from completing.  returning to the MPICH layer when
   requests complete is not necessary, only when the current operation completes.  in these cases, the CM implementing the
   operation may block if this routine returns FALSE. */
#define mpig_pe_polling_required(cm_info_p_) \
    (mpig_pe_polling_required_count - ((cm_info_p_)->polling_required ? 1 : 0) > 0)
/**********************************************************************************************************************************
						   END PROGRESS ENGINE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN PROCESS ID SECTION
**********************************************************************************************************************************/
/*
 * FIXME: XXX: gpids need to be bigger than two integers.  ideally, we would like to use a uuid supplied by globus to identify
 * the process group and an integer to represent the process rank within the process group; however, this would require reduce
 * the uuid through some hashing algorithm and thus increase the probability of two unique process groups having the same process
 * id.
 */
   
int MPID_GPID_Get(MPID_Comm * comm, int rank, int gpid[]);
/**********************************************************************************************************************************
						     END PROCESS ID SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)

void mpig_debug_init(void);

extern globus_debug_handle_t mpig_debug_handle;
extern time_t mpig_debug_start_tv_sec;

#define MPIG_DEBUG_LEVEL_NAMES "ERROR FUNC ADI3 PT2PT COLL DYNAMIC WIN THREADS PROGRESS DATA COUNT " \
    "REQ DT COMM TOPO CM VC PG BC RECVQ CEMT PM DATABUF MSGHDR MPICH2 APP" 

typedef enum mpig_debug_levels
{
    MPIG_DEBUG_LEVEL_ERROR =		1 << 0,		/* errors (and warnings) */
    MPIG_DEBUG_LEVEL_FUNC =		1 << 1,		/* function entry/exit */
    MPIG_DEBUG_LEVEL_ADI3 =		1 << 2,		/* ADI3 function entry/exit */
    MPIG_DEBUG_LEVEL_PT2PT =		1 << 3,		/* point-to-point operations */
    MPIG_DEBUG_LEVEL_COLL =		1 << 4,		/* collective operations */
    MPIG_DEBUG_LEVEL_DYNAMIC =		1 << 5,		/* spawn and connect/accept operations */
    MPIG_DEBUG_LEVEL_WIN =		1 << 6,		/* window (rma) objects and operations */
    MPIG_DEBUG_LEVEL_THREADS =		1 << 7,		/* thread operations and state */
    MPIG_DEBUG_LEVEL_PROGRESS =		1 << 8,		/* progress engine */
    MPIG_DEBUG_LEVEL_DATA =		1 << 9,		/* movement of user data */
    MPIG_DEBUG_LEVEL_COUNT =		1 << 10,	/* changes to counters (reference, completion, etc.) */
    MPIG_DEBUG_LEVEL_REQ =		1 << 11,	/* request objects and operations */
    MPIG_DEBUG_LEVEL_DT =		1 << 12,	/* datatype objects and operations */
    MPIG_DEBUG_LEVEL_COMM =		1 << 13,	/* communicator objects and operations */
    MPIG_DEBUG_LEVEL_TOPO =		1 << 14,	/* topology aware information and operations */
    MPIG_DEBUG_LEVEL_CM =		1 << 15,	/* communication method interface and implementation */
    MPIG_DEBUG_LEVEL_VC =		1 << 16,	/* virtual connection ojbects */
    MPIG_DEBUG_LEVEL_PG =		1 << 17,	/* process group objects */
    MPIG_DEBUG_LEVEL_BC =		1 << 18,	/* business card operations */
    MPIG_DEBUG_LEVEL_RECVQ =		1 << 19,	/* global receive queue */
    MPIG_DEBUG_LEVEL_CEMT =		1 << 20,	/* connection establishment, maintenance, and teardown */
    MPIG_DEBUG_LEVEL_PM =		1 << 21,	/* process management */
    MPIG_DEBUG_LEVEL_DATABUF =		1 << 22,	/* internal data buffers */
    MPIG_DEBUG_LEVEL_MSGHDR =		1 << 23,	/* packing and unpacking of messsage headers */
    MPIG_DEBUG_LEVEL_MPICH2 =		1 << 24,	/* MPICH2 debugging output */
    MPIG_DEBUG_LEVEL_APP =		1 << 25		/* application */
}
mpig_debug_levels_t;

#define MPIG_DEBUG_STMT(levels_, a_)  if ((levels_) & mpig_debug_handle.levels) {a_}

#if defined(HAVE_C99_VARIADIC_MACROS) || defined(HAVE_GNU_VARIADIC_MACROS)
void mpig_debug_printf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...);
void mpig_debug_uprintf_fn(unsigned levels, const char * filename, const char * funcname, int line, const char * fmt, ...);
#define MPIG_DEBUG_PRINTF(a_) mpig_debug_printf a_
#define MPIG_DEBUG_UPRINTF(a_) mpig_debug_uprintf a_
#endif

#if defined(HAVE_C99_VARIADIC_MACROS)

#define mpig_debug_printf(levels_, fmt_, ...)									\
{														\
    if ((levels_) & mpig_debug_handle.levels)									\
    {														\
	mpig_debug_uprintf_fn((levels_), __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## __VA_ARGS__);	\
    }														\
}

#define mpig_debug_uprintf(levels_, fmt_, ...)								\
{													\
    mpig_debug_uprintf_fn((levels_), __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## __VA_ARGS__);	\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_UTIL_PRINTF a_
#define MPIG_DEBUG_OLD_UTIL_PRINTF(fmt_, ...)				\
{									\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPICH2, fmt_, ## __VA_ARGS__);	\
}

#elif defined(HAVE_GNU_VARIADIC_MACROS)

#define mpig_debug_printf(levels_, fmt_, args_...)							\
{													\
    if ((levels_) & mpig_debug_handle.levels)								\
    {													\
	mpig_debug_uprintf_fn((levels_), __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## args_);	\
    }													\
}

#define mpig_debug_uprintf(levels_, fmt_, args_...)						\
{												\
    mpig_debug_uprintf_fn((levels_), __FILE__, MPIU_QUOTE(FUNCNAME), __LINE__, fmt_, ## args_);	\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) MPIG_DEBUG_OLD_UTIL_PRINTF a_
#define MPIG_DEBUG_OLD_UTIL_PRINTF(fmt_, args,...)		\
{								\
    mpig_debug_printf(MPIG_DEBUG_LEVEL_MPICH2, fmt_, ## args_);	\
}

#else /* the compiler does not support variadic macros */

#define MPIG_DEBUG_PRINTF(a_) mpig_debug_printf(a_)
#define MPIG_DEBUG_UPRINTF(a_) mpig_debug_uprintf(a_)

typedef struct mpig_debug_thread_state
{
    const char * filename;
    const char * funcname;
    int line;
}
mpig_debug_thread_state_t;

extern globus_thread_once_t mpig_debug_create_state_key_once;
extern globus_thread_key_t mpig_debug_state_key;

void mpig_debug_printf_fn(unsigned levels, const char * fmt, ...);
void mpig_debug_uprintf_fn(unsigned levels, const char * fmt, ...);
void mpig_debug_old_util_printf_fn(const char * fmt, ...);
void mpig_debug_create_state_key(void);

#define	mpig_debug_save_state(filename_, funcname_, line_)				\
{											\
    mpig_debug_thread_state_t * mpig_debug_save_state_state__;				\
    globus_thread_once(&mpig_debug_create_state_key_once, mpig_debug_create_state_key);	\
    mpig_debug_save_state_state__ = globus_thread_getspecific(mpig_debug_state_key);	\
    if (mpig_debug_save_state_state__ == NULL)						\
    {											\
	mpig_debug_save_state_state__ = MPIU_Malloc(sizeof(mpig_debug_thread_state_t));	\
	MPIU_Assertp((mpig_debug_save_state_state__ != NULL) &&				\
	    "ERROR: unable to allocate memory for thread specific debugging state");	\
	globus_thread_setspecific(mpig_debug_state_key, mpig_debug_save_state_state__);	\
    }											\
    mpig_debug_save_state_state__->filename = (filename_);				\
    mpig_debug_save_state_state__->funcname = (funcname_);				\
    mpig_debug_save_state_state__->line = (line_);					\
}

#define	mpig_debug_retrieve_state(filename_p_, funcname_p_, line_p_)				\
{												\
    mpig_debug_thread_state_t * mpig_debug_retrieve_state_state__;				\
    mpig_debug_retrieve_state_state__ = globus_thread_getspecific(mpig_debug_state_key);	\
    *(filename_p_) = mpig_debug_retrieve_state_state__->filename;				\
    *(funcname_p_) = mpig_debug_retrieve_state_state__->funcname;				\
    *(line_p_) = mpig_debug_retrieve_state_state__->line;					\
}

#define	mpig_debug_printf(a_)							\
{										\
    if (mpig_debug_handle.levels)						\
    {										\
	mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
	mpig_debug_printf_fn a_;						\
    }										\
}

#define	mpig_debug_uprintf(a_)							\
{										\
    if (mpig_debug_handle.levels)						\
    {										\
	mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
	mpig_debug_uprintf_fn a_;						\
    }										\
}

#undef MPIU_DBG_PRINTF
#define MPIU_DBG_PRINTF(a_) mpig_debug_old_util_printf a_
#define mpig_debug_old_util_printf(a_)						\
{										\
    if (mpig_debug_handle.levels)						\
    {										\
	mpig_debug_save_state(__FILE__, MPIU_QUOTE(FUNCNAME), __LINE__);	\
	mpig_debug_old_util_printf_fn a_;					\
    }										\
}
#endif /* variadic macros */


/* override the stardard MPICH2 enter/exit debug logging macros */
#undef MPIR_FUNC_ENTER
#define MPIR_FUNC_ENTER(state_)					\
{								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "entering"));	\
    MPIG_FUNCNAME_CHECK();					\
}

#undef MPIR_FUNC_EXIT
#define MPIR_FUNC_EXIT(state_)					\
{								\
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_FUNC, "exiting"));	\
}

/* macro to verify that the function name in FUNCNAME is really the name of the function */
#if defined(MPIG_VERIFY_FUNCNAME)
#if defined(HAVE__FUNC__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __func__) == 0)
#elif defined (HAVE__FUNCTION__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __FUNCTION__) == 0)
#elif defined (HAVE_CAP__FUNC__)
#define MPIG_FUNCNAME_CHECK() MPIU_Assert(strcmp(MPIU_QUOTE(FUNCNAME), __FUNC__) == 0)
#else
#define MPIG_FUNCNAME_CHECK()
#endif
#else
#define MPIG_FUNCNAME_CHECK()
#endif

#else /* !defined(MPIG_DEBUG) */

#define MPIG_DEBUG_TEST(levels_) (FALSE)
#define MPIG_DEBUG_STMT(levels_, a_)
#define	MPIG_DEBUG_PRINTF(a_)

#if !defined(MPIR_FUNC_ENTER)
#define MPIR_FUNC_ENTER(state_)
#endif
#if !defined(MPIR_FUNC_EXIT)
#define MPIR_FUNC_EXIT(state_)
#endif

#endif /* end if/else defined(MPIG_DEBUG) */

#if (!defined(NDEBUG) && defined(HAVE_ERROR_CHECKING))
#define MPIG_Assert(a_)							\
{									\
    if (!(a_))								\
    {									\
	MPID_Abort(NULL, MPI_SUCCESS, 1, "Assertion failed in file "	\
	    __FILE__ " at line " MPIU_QUOTE(__LINE__) ": "		\
	    MPIU_QUOTE(a_) "\n");					\
    }									\
}
#else
#define MPIG_Assert(a_)
#endif

#define MPIG_Assertp(a_)						\
{									\
    if (!(a_))								\
    {									\
	MPID_Abort(NULL, MPI_SUCCESS, 1, "Assertion failed in file "	\
	    __FILE__ " at line " MPIU_QUOTE(__LINE__) ": "		\
	    MPIU_QUOTE(a_) "\n");					\
    }									\
}

#undef MPIU_Assert
#define MPIU_Assert(a_) MPIG_Assert(a_)

#undef MPIU_Assertp
#define MPIU_Assertp(a_) MPIG_Assertp(a_)
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS DATA SECTION
**********************************************************************************************************************************/
/* FIXME: the defintion of MPIG_PROCESS_CMS_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_PROCESS_CMS_SELF_DECL)
#define MPIG_PROCESS_CMS_SELF_DECL
#else
#undef MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CMS_VMPI_DECL)
#define MPIG_PROCESS_CMS_VMPI_DECL
#else
#undef MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CMS_XIO_DECL)
#define MPIG_PROCESS_CMS_XIO_DECL
#else
#undef MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PROCESS_CMS_OTHER_DECL)
#define MPIG_PROCESS_CMS_OTHER_DECL
#else
#undef MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#define MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_PROCESS_CMS_DECL_STRUCT_DEFINED)
#define MPIG_PROCESS_CMS_DECL		\
    struct				\
    {					\
	MPIG_PROCESS_CMS_SELF_DECL	\
	MPIG_PROCESS_CMS_VMPI_DECL	\
	MPIG_PROCESS_CMS_XIO_DECL	\
	MPIG_PROCESS_CMS_OTHER_DECL	\
    } cms;
#else
#define MPIG_PROCESS_CMS_DECL
#endif

/* define useful names for function counting */
#define MPIG_FUNC_CNT_FIRST MPID_STATE_MPI_ABORT
#define MPIG_FUNC_CNT_LAST MPID_STATE_MPI_WTIME
#define MPIG_FUNC_CNT_NUMFUNCS (MPIG_FUNC_CNT_LAST - MPIG_FUNC_CNT_FIRST + 1)

typedef struct mpig_process
{
    /*
     * pointer to the the process group to which this process belongs, the size of the process group, and the rank of the
     * processs within the process group.  in addition, the size of the subjob to which this process belongs, and the rank of the
     * process within the subjob.
     *
     * NOTE: DONT NOT REORDER THESE FIELDS AS A STATIC INITIALIZER IN mpid_env.c SETS THEM BASED ON THEIR ORDER!
     */
    struct mpig_pg * my_pg;
    const char * my_pg_id;
    int my_pg_size;
    int my_pg_rank;

    int my_sj_size;
    int my_sj_rank;

    /* process id and hostname associated with the local process */
    char my_hostname[MPI_MAX_PROCESSOR_NAME];
    pid_t my_pid;

    /* mutex to protect and insure coherence of data in the process structure */
    mpig_mutex_t mutex;

    MPIG_PROCESS_CMS_DECL
    
#   if defined(HAVE_GLOBUS_USAGE_MODULE)
    /* usage stats tracking vars */
    struct timeval start_time;
    int64_t nbytes_sent;
    int64_t vmpi_nbytes_sent;
    
    int function_count[MPIG_FUNC_CNT_NUMFUNCS];
#   endif
}
mpig_process_t;

extern mpig_process_t mpig_process;
/**********************************************************************************************************************************
						    END PROCESS DATA SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN USAGE STATISTICS SECTION
**********************************************************************************************************************************/
#if defined(HAVE_GLOBUS_USAGE_MODULE)
#define MPIG_USAGE_INC_NBYTES_SENT(nbytes_)	\
{						\
    mpig_process.nbytes_sent += (nbytes_);	\
}

#define MPIG_USAGE_INC_VMPI_NBYTES_SENT(nbytes_)	\
{							\
    mpig_process.vmpi_nbytes_sent += (nbytes_);		\
}

#define MPIG_USAGE_FUNC_ENTER(state_)					\
{									\
    if(state_ >= MPIG_FUNC_CNT_FIRST && state_ <= MPIG_FUNC_CNT_LAST)	\
    {									\
        mpig_process.function_count[state_ - MPIG_FUNC_CNT_FIRST]++;	\
    }									\
}
#else
#define MPIG_USAGE_INC_NBYTES_SENT(nbytes_)
#define MPIG_USAGE_INC_VMPI_NBYTES_SENT(nbytes_)
#define MPIG_USAGE_FUNC_ENTER(state_)
#endif

#define MPIG_MPI_FUNC_ENTER(state_)	\
{					\
    MPIR_FUNC_ENTER(state_);		\
    MPIG_USAGE_FUNC_ENTER(state_);	\
}

#define MPIG_MPI_FUNC_EXIT(state_)	\
{					\
    MPIR_FUNC_EXIT(state_);		\
}

/* override the enter/exit debug logging macros to count function calls */
#undef MPID_MPI_FUNC_ENTER
#undef MPID_MPI_FUNC_EXIT
#undef MPID_MPI_PT2PT_FUNC_ENTER
#undef MPID_MPI_PT2PT_FUNC_EXIT
#undef MPID_MPI_PT2PT_FUNC_ENTER_FRONT
#undef MPID_MPI_PT2PT_FUNC_EXIT_FRONT
#undef MPID_MPI_PT2PT_FUNC_ENTER_BACK
#undef MPID_MPI_PT2PT_FUNC_ENTER_BOTH
#undef MPID_MPI_PT2PT_FUNC_EXIT_BACK
#undef MPID_MPI_PT2PT_FUNC_EXIT_BOTH
#undef MPID_MPI_COLL_FUNC_ENTER
#undef MPID_MPI_COLL_FUNC_EXIT
#undef MPID_MPI_RMA_FUNC_ENTER
#undef MPID_MPI_RMA_FUNC_EXIT
#undef MPID_MPI_INIT_FUNC_ENTER
#undef MPID_MPI_INIT_FUNC_EXIT
#undef MPID_MPI_FINALIZE_FUNC_ENTER
#undef MPID_MPI_FINALIZE_FUNC_EXIT

#define MPID_MPI_FUNC_ENTER(a)                  MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_FUNC_EXIT(a)                   MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER(a)            MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT(a)             MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_FRONT(a)      MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_FRONT(a)       MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BACK(a)       MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_ENTER_BOTH(a)       MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BACK(a)        MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_PT2PT_FUNC_EXIT_BOTH(a)        MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_COLL_FUNC_ENTER(a)             MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_COLL_FUNC_EXIT(a)              MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_RMA_FUNC_ENTER(a)              MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_RMA_FUNC_EXIT(a)               MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_INIT_FUNC_ENTER(a)             MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_INIT_FUNC_EXIT(a)              MPIG_MPI_FUNC_EXIT(a)
#define MPID_MPI_FINALIZE_FUNC_ENTER(a)         MPIG_MPI_FUNC_ENTER(a)
#define MPID_MPI_FINALIZE_FUNC_EXIT(a)          MPIG_MPI_FUNC_EXIT(a)
/**********************************************************************************************************************************
						  END USAGE STATISTICS SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDPOST_H_INCLUDED */
