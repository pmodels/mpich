/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED)
#define MPICH2_MPIG_CM_VMPI_H_INCLUDED 1

/* expose the communication module's vtable so that it is accessible to other modules in the device */
extern struct mpig_cm mpig_cm_vmpi;


/* define the structure to be included in the communication method structure (CMS) of a VC object */
#define MPIG_VC_CMS_VMPI_DECL	\
    struct			\
    {				\
	mpig_uuid_t job_id;	\
	int cw_rank;		\
    } vmpi;


#if defined(MPIG_VMPI)
#include "mpig_vmpi.h"

/* change any occurences of MPID and MPID_ to MPIG and MPIG_ respectively.  this symbols are found in macros that construct
   symbol names.  One example of such macros are those that convert object handles to object pointers in
   src/include/mpiimpl.h. */
#define MPID				MPIG
#define MPID_				MPIG_


/* define the structure to be included in the communcation method structure (CMS) of a communicator object */
#define MPIG_COMM_CMS_VMPI_DECL				\
    struct						\
    {							\
	mpig_vmpi_comm_t comms[MPIG_COMM_NUM_CONTEXTS];	\
	int rank;					\
	int remote_size;				\
	int * remote_ranks_mtov;			\
	int * remote_ranks_vtom;			\
    } vmpi;

/* define the structure to be included in the communication method structure (CMS) of a datatype object */
#define MPIG_DATATYPE_CMS_VMPI_DECL	\
    struct				\
    {					\
	mpig_vmpi_datatype_t dt;	\
    } vmpi;

/* define the structure to be included in the communication method struct (CMS) of a request object.  NOTE: the req and
   pe_table_index fields must be in the structure rather than the union.  if the request is a receive any source request, then
   this information must be maintained even when the request is not owned by the VMPI CM. */
#define MPIG_REQUEST_CMS_VMPI_DECL				\
    struct							\
    {								\
	mpig_vmpi_request_t req;				\
	int pe_table_index;					\
	bool_t cancelling;					\
    } vmpi;

/* define the structure to be included in the communication method structure (CMS) of the MPIG process structure */
#define MPIG_PROCESS_CMS_VMPI_DECL	\
    struct				\
    {					\
	int cw_rank;			\
	int cw_size;			\
    } vmpi;


/*
 * macros to assist in handling vendor MPI errors
 */
#if defined(HAVE_ERROR_CHECKING) || defined(MPIG_DEBUG)
#define MPIG_ERR_VMPI_SET(vrc_, vfcname_, mpi_errno_p_)										 \
{																 \
    int MPIG_ERR_VMPI_SET_vrc__;												 \
    int MPIG_ERR_VMPI_SET_error_class__ = MPIG_VMPI_ERR_UNKNOWN;								 \
    int MPIG_ERR_VMPI_SET_error_str_len__;											 \
    char MPIG_ERR_VMPI_SET_error_str__[MPIG_VMPI_MAX_ERROR_STRING + 1];								 \
																 \
    MPIG_ERR_VMPI_SET_vrc__ = mpig_vmpi_error_class((vrc_), &MPIG_ERR_VMPI_SET_error_class__);					 \
    MPIU_Assert(MPIG_ERR_VMPI_SET_vrc__ == MPI_SUCCESS && "ERROR: vendor MPI_Error_class failed");				 \
																 \
    MPIG_ERR_VMPI_SET_vrc__ = mpig_vmpi_error_string((vrc_), MPIG_ERR_VMPI_SET_error_str__, &MPIG_ERR_VMPI_SET_error_str_len__); \
    MPIU_Assert(MPIG_ERR_VMPI_SET_vrc__ == MPI_SUCCESS && "ERROR: vendor MPI_Error_string failed");				 \
																 \
    MPIU_ERR_SET2(*(mpi_errno_p_), mpig_cm_vmpi_error_class_vtom(MPIG_ERR_VMPI_SET_error_class__), "**globus|vmpi_fn_failed",	 \
	"**globus|vmpi_fn_failed %s %s", (vfcname_), MPIG_ERR_VMPI_SET_error_str__);						 \
    MPIG_DEBUG_PRINTF((MPIG_DEBUG_LEVEL_ERROR, "ERROR: call to vendor %s failed: %s", (vfcname_),				 \
	(MPIG_ERR_VMPI_SET_error_str__)));											 \
}
#else
#define MPIG_ERR_VMPI_SET(vrc_, vfcname_, mpi_errno_p_)					\
{											\
    int MPIG_ERR_VMPI_SET_error_class__;						\
    mpig_vmpi_error_class((vrc_), &MPIG_ERR_VMPI_SET_error_class__);			\
    *(mpi_errno_p_) = mpig_cm_vmpi_error_class_vtom(MPIG_ERR_VMPI_SET_error_class__);	\
}
#endif

#define MPIG_ERR_VMPI_SETANDSTMT(vrc_, vfcname_, mpi_errno_p_, stmt_)	\
{									\
    MPIG_ERR_VMPI_SET((vrc_), (vfcname_), (mpi_errno_p_));		\
    { stmt_ ; }								\
}

#define MPIG_ERR_VMPI_CHKANDSET(vrc_, vfcname_, mpi_errno_p_)	\
{								\
    if (vrc_)							\
    {								\
	MPIG_ERR_VMPI_SET((vrc_), (vfcname_), (mpi_errno_p_));	\
    }								\
}

#define MPIG_ERR_VMPI_CHKANDSTMT(vrc_, vfcname_, mpi_errno_p_, stmt_)	\
{									\
    if (vrc_)								\
    {									\
	MPIG_ERR_VMPI_SET((vrc_), (vfcname_), (mpi_errno_p_));		\
	{ stmt_ ; }							\
    }									\
}

#define MPIG_ERR_VMPI_CHKANDJUMP(vrc_, vfcname_, mpi_errno_p_)				\
{											\
    MPIG_ERR_VMPI_CHKANDSTMT((vrc_), (vfcname_), (mpi_errno_p_), {goto fn_fail;});	\
}

#endif /* defined(MPIG_VMPI) */


/*
 * globally visible functions and macros
 */
#if defined(MPIG_VMPI)

void mpig_cm_vmpi_pe_start(struct MPID_Progress_state * state);

void mpig_cm_vmpi_pe_end(struct MPID_Progress_state * state);

int mpig_cm_vmpi_pe_wait(struct MPID_Progress_state * state);

int mpig_cm_vmpi_pe_test(void);

int mpig_cm_vmpi_pe_poke(void);


int mpig_cm_vmpi_comm_dup_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm);

int mpig_cm_vmpi_comm_split_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm);

int mpig_cm_vmpi_intercomm_create_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm);

int mpig_cm_vmpi_intercomm_merge_hook(struct MPID_Comm * orig_comm, struct MPID_Comm * new_comm);

int mpig_cm_vmpi_comm_free_hook(struct MPID_Comm * comm);

void mpig_cm_vmpi_comm_destruct_hook(struct MPID_Comm * comm);


int mpig_cm_vmpi_type_contiguous_hook(int cnt, MPI_Datatype old_dt, struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_vector_hook(int cnt, int blklen, mpig_vmpi_aint_t stride, int stride_in_bytes, MPI_Datatype old_dt,
    struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_indexed_hook(int cnt, int * blklens, void * displs, int displs_in_bytes, MPI_Datatype old_dt,
    struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_blockindexed_hook(int cnt, int blklen, void * displs, int displs_in_bytes, MPI_Datatype old_dt,
    struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_struct_hook(int cnt, int * blklens, mpig_vmpi_aint_t * displs, MPI_Datatype * old_dts,
    struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_create_resized_hook(MPI_Datatype old_dt, MPI_Aint lb, MPI_Aint extent, struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_dup_hook(MPI_Datatype old_dt, struct MPID_Datatype * new_dtp);

int mpig_cm_vmpi_type_commit_hook(struct MPID_Datatype * dtp);

int mpig_cm_vmpi_type_free_hook(struct MPID_Datatype * dtp);

#define MPID_DEV_TYPE_CONTIGUOUS_HOOK(cnt_, old_dt_, new_dt_p_, mpi_errno_p_)			\
{												\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_contiguous_hook((cnt_), (old_dt_), (new_dt_p_));	\
}

#define MPID_DEV_TYPE_VECTOR_HOOK(cnt_, blklen_, stride_, stride_in_bytes_, old_dt_, new_dt_p_, mpi_errno_p_)		\
{															\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_vector_hook((cnt_), (blklen_), (stride_), (stride_in_bytes_), (old_dt_),	\
	(new_dt_p_));													\
}

#define MPID_DEV_TYPE_INDEXED_HOOK(cnt_, blklens_, displs_, displs_in_bytes_, old_dt_, new_dt_p_, mpi_errno_p_)		\
{															\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_indexed_hook((cnt_), (blklens_), (displs_), (displs_in_bytes_), (old_dt_),	\
	(new_dt_p_));													\
}

#define MPID_DEV_TYPE_BLOCKINDEXED_HOOK(cnt_, blklen_, displs_, displs_in_bytes_, old_dt_, new_dt_p_, mpi_errno_p_)	\
{															\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_blockindexed_hook((cnt_), (blklen_), (displs_), (displs_in_bytes_), (old_dt_),	\
	(new_dt_p_));													\
}

#define MPID_DEV_TYPE_STRUCT_HOOK(cnt_, blklens_, displs_, old_dts_, new_dt_p_, mpi_errno_p_)			\
{														\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_struct_hook((cnt_), (blklens_), (displs_), (old_dts_), (new_dt_p_));	\
}

#define MPID_DEV_TYPE_CREATE_RESIZED_HOOK(old_dt_, lb_, extent_, new_dt_p_, mpi_errno_p_)		\
{													\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_create_resized_hook((old_dt_), (lb_), (extent_), (new_dt_p_));	\
}

#define MPID_DEV_TYPE_DUP_HOOK(old_dt_, new_dt_p_, mpi_errno_p_)		\
{										\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_dup_hook((old_dt_), (new_dt_p_));	\
}

#define MPID_DEV_TYPE_COMMIT_HOOK(dt_p_, mpi_errno_p_)		\
{								\
    *(mpi_errno_p_) = mpig_cm_vmpi_type_commit_hook((dt_p_));	\
}

#define MPID_DEV_TYPE_FREE_HOOK(dt_p_)		\
{						\
    mpig_cm_vmpi_type_free_hook((dt_p_));	\
}


int mpig_cm_vmpi_iprobe_any_source(int tag, struct MPID_Comm * comm, int ctxoff, int * flag, MPI_Status * status);

int mpig_cm_vmpi_cancel_recv_any_source(struct MPID_Request * rreq, bool_t * cancelled_p);

int mpig_cm_vmpi_register_recv_any_source(const struct mpig_msg_op_params * ras_params, bool_t * found_p,
    struct MPID_Request * ras_req);

int mpig_cm_vmpi_unregister_recv_any_source(struct mpig_recvq_ras_op * recvq_ras_op);


int mpig_cm_vmpi_error_class_vtom(int vendor_error_class);


extern mpig_thread_t mpig_cm_vmpi_main_thread;

#define mpig_cm_vmpi_thread_is_main() (mpig_thread_equal(mpig_cm_vmpi_main_thread, mpig_thread_self()))
#define mpig_cm_vmpi_comm_get_remote_vsize(comm_) ((comm_)->cms.vmpi.remote_size)

/* NOTE: the vendor MPI CM is not used for intraprocess communication and therefore must be subtracted from the total number of
   vendor MPI processes when determining if any vendor MPI processes exist in the communicator */
#define mpig_cm_vmpi_comm_has_remote_vprocs(comm_)							\
    (mpig_cm_vmpi_comm_get_remote_vsize(comm_) - ((comm_)->comm_kind == MPID_INTRACOMM ? 1 : 0) > 0)

#endif /* defined(MPIG_VMPI) */


/*
 * macro implementations of progress engine interface functions (these should really be in the PE info structure)
 */
#if defined(MPIG_VMPI)

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_poke() mpig_cm_vmpi_progess_test()

#else /* !defined(MPIG_VMPI) */

#define mpig_cm_vmpi_pe_start(state_)

#define mpig_cm_vmpi_pe_end(state_)

#define mpig_cm_vmpi_pe_wait(state_) (MPI_SUCCESS)

#define mpig_cm_vmpi_pe_test() (MPI_SUCCESS)

#define mpig_cm_vmpi_pe_poke() (MPI_SUCCESS)

#endif /* else !defined(MPIG_VMPI) */

#endif /* !defined(MPICH2_MPIG_CM_VMPI_H_INCLUDED) */
