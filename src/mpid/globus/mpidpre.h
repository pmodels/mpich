/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIDPRE_H_INCLUDED)
#define MPICH2_MPIDPRE_H_INCLUDED

/* compile time contants */
#if !defined(MPIG_ERR_STRING_SIZE)
#define MPIG_ERR_STRING_SIZE 32768
#endif

#if !defined(MPIG_INT_MAX_STRLEN)
#define MPIG_INT_MAX_STRLEN 11
#endif

#include "mpidconf.h"
#include "mpiallstates.h"

#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif

/* include common tools provided by Globus */
#if defined(HAVE_GLOBUS_COMMON_MODULE)
#include "globus_common.h"
#endif
#if defined(HAVE_GLOBUS_DC_MODULE)
#include "globus_dc.h"
#endif

/* miscellaneous core types */
#if !defined(HAVE_BOOL_T)
typedef int bool_t;
#endif

/* miscellaneous macros */
#define MPIG_QUOTE(a_) MPIG_QUOTE2(a_)
#define MPIG_QUOTE2(a_) #a_

#define MPIG_UNUSED_ARG(arg_) {(void) arg_;}
#define MPIG_UNUSED_VAR(var_) {(void) var_;}

#define MPIG_BOOL_STR(b_) ((b_) ? "TRUE" : "FALSE")
#define MPIG_ENDIAN_STR(e_) (((e_) == MPIG_ENDIAN_LITTLE) ? "little" : (((e_) == MPIG_ENDIAN_BIG) ? "big" : "unknown"))

#define MPIG_MIN(a_, b_) ((a_) <= (b_) ? (a_) : (b_))
#define MPIG_MAX(a_, b_) ((a_) >= (b_) ? (a_) : (b_))

#define MPIG_INVALID_PTR ((void *) -1)

/* NOTE: the values of TRUE and FALSE must match the values set in mpig_vmpi.c */
#undef FALSE
#define FALSE 0
#undef TRUE
#define TRUE 1

/* settings for inlining functions */
#if defined(HAVE_C_INLINE)
#if defined(__GNUC__)
#define MPIG_INLINE_FUNCS 1
#define MPIG_INLINE_HDECL extern inline
#define MPIG_INLINE_HDEF extern inline  /* for functions defined in header files */
#define MPIG_INLINE inline              /* for functions defined in source files */
#else /* !defined(__GNUC__) --> C99? */
#define MPIG_INLINE_FUNCS 1
#define MPIG_INLINE_HDECL inline
#define MPIG_INLINE_HDEF inline
#define MPIG_INLINE inline
#endif /* end if/else __GNUC__ */
#else
#define MPIG_INLINE_HDECL
#undef MPIG_INLINE_HDEF
#define MPIG_INLINE
#endif /* end if/else defined(HAVE_C_INLINE) */


/* declare the existence of MPICH2 and MPIG structures that cannot be defined util later */
struct MPID_Comm;
struct MPID_Datatype;
struct MPID_Request;
struct MPID_Progress_state;
struct mpig_cm;
struct mpig_bc;
struct mpig_pg;
struct mpig_vc;
struct mpig_msg_op_params;
struct mpig_recvq_ras_op;

/* macros to make printing values of types that vary with the platform a little easier */
#define MPIG_HANDLE_VAL(object_) (((object_) != NULL) ? (object_)->handle : 0)
#define MPIG_STR_VAL(str_) ((str_) != NULL ? (str_) : "(null)")

/* determine if comprehensive error checking should be performed */
#if #defined(HAVE_ERROR_CHECKING)
#define MPIG_CHECK_ERRORS TRUE
#else
#define MPIG_CHECK_ERRORS FALSE
#endif


/**********************************************************************************************************************************
                                               BEGIN BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/
typedef struct mpig_genq_entry
{
    void * value;
    struct mpig_genq_entry * prev;
    struct mpig_genq_entry * next;
}
mpig_genq_entry_t;

typedef struct mpig_genq
{
    mpig_genq_entry_t * head;
    mpig_genq_entry_t * tail;
}
mpig_genq_t;
/**********************************************************************************************************************************
                                                END BASIC DATA STRUCTURES SECTON
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/
#include "mpig_thread.i"

#define mpig_thread_get_id() ((unsigned long) mpig_thread_self())

#define MPID_REQUIRES_THREAD_SAFETY
typedef mpig_mutex_t MPID_Thread_mutex_t;

#define MPID_Thread_mutex_create(mutex_, mpi_errno_p_)	\
{							\
    if (mpig_mutex_construct((mutex_)) != 0)		\
    {							\
	*(mpi_errno_p_) = MPI_ERR_OTHER;		\
    }							\
}

#define MPID_Thread_mutex_destroy(mutex_, mpi_errno_p_)	\
{							\
    if (mpig_mutex_destroy((mutex_) != 0)		\
    {							\
	*(mpi_errno_p_) = MPI_ERR_OTHER;		\
    }							\
}

#define MPID_Thread_mutex_lock(mutex_)	\
{					\
    mpig_mutex_lock(mutex_);		\
}

#define MPID_Thread_mutex_unlock(mutex_)	\
{						\
    mpig_mutex_unlock(mutex_);			\
}
/**********************************************************************************************************************************
						END DEVICE THREAD SUPPORT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
                                                       BEGIN UUID SECTION
**********************************************************************************************************************************/
#if defined(HAVE_UUID_GENERATE) && defined(HAVE_UUID_UNPARSE) && defined(HAVE_UUID_UUID_H)

#include <uuid/uuid.h>
#define MPIG_UUID_MAX_STR_LEN 36
typedef struct {uuid_t uuid;} mpig_uuid_t;

#elif defined (HAVE_GLOBUS_COMMON_MODULE)

#define MPIG_UUID_MAX_STR_LEN GLOBUS_UUID_TEXTLEN
typedef globus_uuid_t mpig_uuid_t;

#else

#define MPIG_UUID_MAX_STR_LEN (MPI_MAX_PROCESSOR_NAME+10)
typedef struct {char str[MPIG_UUID_MAX_STR_LEN+1];} mpig_uuid_t;

#endif
/**********************************************************************************************************************************
                                                        END UUID SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					       BEGIN COMMUNICATION METHOD SECTION
**********************************************************************************************************************************/
/* FIXME: this list should be generated at configure time, perhaps by scanning files with a pattern of mpig_cm_*.h. */
typedef enum mpig_cm_module_type
{
    MPIG_CM_TYPE_SELF,
    MPIG_CM_TYPE_VMPI,
    MPIG_CM_TYPE_XIO,
    MPIG_CM_TYPE_OTHER,
    MPIG_CM_TYPE_LAST
}
mpig_cm_module_type_t;

#define MPIG_CM_NUM_TYPES (MPIG_CM_TYPE_LAST)

/*
 * <mpi_errno> mpig_cm_init([IN/MOD] cm, [IN/OUT] argc, [IN/OUT] argv)
 *
 * Parameters:
 *
 *   cm [IN/MOD] - communication method object to be initialized
 *
 *   argc [IN/OUT] - number of arguments in the argv array.  this may be modified communication method.
 *
 *   argv [IN/OUT] - array of command line arguments.  arguments may be added or removed as appropriate.
 *
 * Returns: a MPI error code
 */
typedef int (*mpig_cm_init_fn_t)(struct mpig_cm * cm, int * argc, char *** argv);

/*
 * <mpi_errno> mpig_cm_finalize([IN/MOD] cm)
 *
 * shutdown down the communication method.  all pending communication using this method must complete before the routine returns.
 *
 * Parameters:
 *
 *   cm [IN/MOD] - communication method object to be shutdown
 *
 * Returns: a MPI error code
 */
typedef int (*mpig_cm_finalize_fn_t)(struct mpig_cm * cm);

/*
 * <mpi_errno> mpig_cm_add_contact_info([IN/MOD] bc)
 *
 * Add contact information for the current process to the supplied business card.
 *
 * Parameters:
 *
 *   cm [IN] - communication method object
 *
 *   bc [IN/MOD] - business card object in which to store contact information for this process
 *
 * Returns: a MPI error code
 */
typedef int (*mpig_cm_add_contact_info_fn_t)(struct mpig_cm * cm, struct mpig_bc * bc);

/*
 * <mpi_errno> mpig_cm_construct_contact_info([IN] cm, [IN/MOD] vc)
 *
 * extract contact information from the business card (and possibly other sources) and store it in machine processable form
 *
 * Parameters:
 *
 *   cm [IN] - communication method object
 *
 *   vc [IN/MOD] - vc object in which to place contact information
 *
 * Returns: a MPI error code
 *
 * NOTE: this routine assumes the that business card object within the VC has been initialized and contains any information
 * necessary required by the communication method
 */
typedef int (*mpig_cm_construct_vc_contact_info_fn_t)(struct mpig_cm * cm, struct mpig_vc * vc);

/*
 * <mpi_errno> mpig_cm_construct_contact_info([IN] cm, [IN/MOD] vc)
 *
 * release any resources allocated when the machine readable form of the contact information was constructed
 *
 * Parameters:
 *
 *   cm [IN] - communication method object
 *
 *   vc [IN/MOD] - vc object in which to destroy the contact information
 *
 * Returns: a MPI error code
 *
 * NOTE: this routine assumes the that business card object within the VC has been initialized and contains any information
 * necessary required by the communication method
 */
typedef void (*mpig_cm_destruct_vc_contact_info_fn_t)(struct mpig_cm * cm, struct mpig_vc * vc);

/*
 * <mpi_errno> mpig_cm_select_comm_method([IN] cm, [IN/MOD] vc, [OUT] selected)
 *
 * determine if the current process can use the communication method to communicate with the process associated with the supplied
 * VC.  if it can, then the VC will be initialized accordingly.  if the VC has already been selected by another communication
 * method, the routine will return with 'selected' equal to FALSE.
 *
 * Parameters:
 *
 *   cm [IN] - communication method object
 *
 *   vc [IN] - vc object to initialize if the communication method is capable of performing communication with the process
 *
 *   selected [OUT] - TRUE if the communication method can communicate with the remote process; otherwise FALSE
 *
 * Returns: a MPI error code
 *
 * NOTE: this routine assumes the that mpig_cm_extract_contact_info() has be called
 */
typedef int (*mpig_cm_select_comm_method_fn_t)(struct mpig_cm * cm, struct mpig_vc * vc, bool_t * selected);

/*
 * <mpi_errno> mpig_cm_get_vc_compatability([IN] cm, [IN] vc1, [IN] vc2, [IN] levels_in, [OUT] levels_out)
 *
 * determine if the two VCs supplied to the routine are capable of communicating at any of the predefined topology levels using
 * the communication method.
 *
 * Parameters:
 *
 *   cm [IN] - communication method object
 *
 *   vc1 [IN] - first VC
 *
 *   vc2 [IN[ - second VC
 *
 *   levels_in [IN] - a bitmask defining the topology levels of interest
 *
 *   levels_out [OUT] - a bitmask containing the topology levels are shared by the two VCs (a subset of levels_in)
 *
 * Returns: a MPI error code
 *
 * NOTE: this routine assumes the that mpig_cm_extract_contact_info() has be called for both VCs
 */
typedef int (*mpig_cm_get_vc_compatability_fn_t)(struct mpig_cm * cm, const struct mpig_vc * vc1, const struct mpig_vc * vc2,
    unsigned levels_in, unsigned * levels_out);

typedef char * (*mpig_cm_vtable_last_entry_fn_t)(int foo, float bar, const short * baz, char bif);

typedef struct mpig_cm_vtable
{
    mpig_cm_init_fn_t				init;
    mpig_cm_finalize_fn_t			finalize;
    mpig_cm_add_contact_info_fn_t		add_contact_info;
    mpig_cm_construct_vc_contact_info_fn_t	construct_vc_contact_info;
    mpig_cm_destruct_vc_contact_info_fn_t	destruct_vc_contact_info;
    mpig_cm_select_comm_method_fn_t		select_comm_method;
    mpig_cm_get_vc_compatability_fn_t		get_vc_compatability;
    mpig_cm_vtable_last_entry_fn_t		vtable_last_entry;
}
mpig_cm_vtable_t;

/*
 * include communication module defintions and declarations
 *
 * FIXME: the list of configuration module header files should be generated at configure time, perhaps by scanning for files who
 * names match the pattern mpig_cm_*.h.
 */
#include "mpig_cm_self.h"
#include "mpig_cm_vmpi.h"
#include "mpig_cm_xio.h"
#include "mpig_cm_other.h"

/*
 * define the mpig_cm structure using any extended information supplied by the communication methods
 */
/* FIXME: the defintion of MPIG_COMM_CM_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_CM_CMU_SELF_DECL)
#define MPIG_CM_CMU_SELF_DECL
#else
#undef MPIG_CM_CMU_DECL_STRUCT_DEFINED
#define MPIG_CM_CMU_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_CM_CMU_VMPI_DECL)
#define MPIG_CM_CMU_VMPI_DECL
#else
#undef MPIG_CM_CMU_DECL_STRUCT_DEFINED
#define MPIG_CM_CMU_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_CM_CMU_XIO_DECL)
#define MPIG_CM_CMU_XIO_DECL
#else
#undef MPIG_CM_CMU_DECL_STRUCT_DEFINED
#define MPIG_CM_CMU_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_CM_CMU_OTHER_DECL)
#define MPIG_CM_CMU_OTHER_DECL
#else
#undef MPIG_CM_CMU_DECL_STRUCT_DEFINED
#define MPIG_CM_CMU_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_CM_CMU_DECL_STRUCT_DEFINED)
#define MPIG_CM_CMU_DECL	\
    union			\
    {				\
	MPIG_CM_CMU_SELF_DECL	\
	MPIG_CM_CMU_VMPI_DECL	\
	MPIG_CM_CMU_XIO_DECL	\
	MPIG_CM_CMU_OTHER_DECL	\
    } cmu;
#else
#define MPIG_CM_CMU_DECL
#endif

typedef struct mpig_cm
{
    mpig_cm_module_type_t module_type;
    const char * name;
    mpig_cm_vtable_t * vtable;
    MPIG_CM_CMU_DECL
}
mpig_cm_t;
/**********************************************************************************************************************************
						END COMMUNICATION METHOD SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN COMMUNICATOR SECTION
**********************************************************************************************************************************/
#define MPIG_COMM_NUM_CONTEXTS 4

/* set of processes which will have to talk together at a given level */
typedef struct mpig_comm_set
{
    int size;               /* number of processes in the set */
    int root_index;         /* position of the root process in the set */
    int my_rank_index;      /* position of the current process in the set */
    int * set;               /* array of process ids in the set */
}
mpig_comm_set_t;

typedef struct mpig_comm
{
    /* topology information */
    int * topology_depths;
    int ** topology_colors;
    int ** topology_ranks;
    int ** topology_cluster_ids;
    int ** topology_cluster_sizes;
    mpig_comm_set_t * topology_comm_sets;
    int topology_max_depth;
    
    /* data structures for tracking active communicators to prevent MPID_Finalize from returning before all outstanding
       communication has completed. */
    bool_t app_ref;
    mpig_genq_entry_t active_list;
}
mpig_comm_t;

/* FIXME: the defintion of MPIG_COMM_CMS_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_COMM_CMS_SELF_DECL)
#define MPIG_COMM_CMS_SELF_DECL
#else
#undef MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#define MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CMS_VMPI_DECL)
#define MPIG_COMM_CMS_VMPI_DECL
#else
#undef MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#define MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CMS_XIO_DECL)
#define MPIG_COMM_CMS_XIO_DECL
#else
#undef MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#define MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_COMM_CMS_OTHER_DECL)
#define MPIG_COMM_CMS_OTHER_DECL
#else
#undef MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#define MPIG_COMM_CMS_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_COMM_CMS_DECL_STRUCT_DEFINED)
#define MPIG_COMM_CMS_DECL		\
    struct				\
    {					\
	MPIG_COMM_CMS_SELF_DECL		\
	MPIG_COMM_CMS_VMPI_DECL		\
	MPIG_COMM_CMS_XIO_DECL		\
	MPIG_COMM_CMS_OTHER_DECL	\
    } cms;
#else
#define MPIG_COMM_CMS_DECL
#endif

#define MPID_DEV_COMM_DECL \
    mpig_comm_t dev;	   \
    MPIG_COMM_CMS_DECL

#define HAVE_DEV_COMM_HOOK
#define HAVE_DEV_COMM_FUNC_HOOKS
/**********************************************************************************************************************************
						    END COMMUNICATOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						     BEGIN DATATYPE SECTION
**********************************************************************************************************************************/
#define MPIG_DATATYPE_MAX_BASIC_TYPES 64

typedef enum mpig_ctype
{
    MPIG_CTYPE_NONE = 0,
    MPIG_CTYPE_CHAR,
    MPIG_CTYPE_SHORT,
    MPIG_CTYPE_INT,
    MPIG_CTYPE_LONG,
    MPIG_CTYPE_LONG_LONG,
    MPIG_CTYPE_FLOAT,
    MPIG_CTYPE_DOUBLE,
    MPIG_CTYPE_LONG_DOUBLE,
    MPIG_CTYPE_LAST
}
mpig_ctype_t;

/* FIXME: the defintion of MPIG_DATATYPE_CMS_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_DATATYPE_CMS_SELF_DECL)
#define MPIG_DATATYPE_CMS_SELF_DECL
#else
#undef MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CMS_VMPI_DECL)
#define MPIG_DATATYPE_CMS_VMPI_DECL
#else
#undef MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CMS_XIO_DECL)
#define MPIG_DATATYPE_CMS_XIO_DECL
#else
#undef MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_DATATYPE_CMS_OTHER_DECL)
#define MPIG_DATATYPE_CMS_OTHER_DECL
#else
#undef MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#define MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_DATATYPE_CMS_DECL_STRUCT_DEFINED)
#define MPIG_DATATYPE_CMS_DECL		\
    struct				\
    {					\
	MPIG_DATATYPE_CMS_SELF_DECL	\
	MPIG_DATATYPE_CMS_VMPI_DECL	\
	MPIG_DATATYPE_CMS_XIO_DECL	\
	MPIG_DATATYPE_CMS_OTHER_DECL	\
    } cms;
#else
#define MPIG_DATATYPE_CMS_DECL
#endif

#define MPID_DEV_DATATYPE_DECL \
    MPIG_DATATYPE_CMS_DECL

/* the inclusion of the datatype header file must come after the defintion of MPID_DEV_DATATYPE_DECL.  if it does not, then the
   device and communication method data structures will not be included in the MPID_Datatype object. */
#include "mpid_datatype.h"
/**********************************************************************************************************************************
						      END DATATYPE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN REQUEST SECTION
**********************************************************************************************************************************/

/*
 * SUPER-IMPORTANT-MT-NOTE: see the note at the top of the request section in mpidpost.h concerning the creation and destruction
 * of requests from internal device threads.
 */

typedef enum mpig_request_type
{
    MPIG_REQUEST_TYPE_FIRST = 0,
    MPIG_REQUEST_TYPE_UNDEFINED,
    MPIG_REQUEST_TYPE_INTERNAL,
    MPIG_REQUEST_TYPE_RECV,
    MPIG_REQUEST_TYPE_SEND,
    MPIG_REQUEST_TYPE_RSEND,
    MPIG_REQUEST_TYPE_SSEND,
    MPIG_REQUEST_TYPE_BSEND,
    MPIG_REQUEST_TYPE_LAST
}
mpig_request_type_t;

typedef void (*mpig_request_cm_destruct_fn_t)(struct MPID_Request * req);

typedef mpig_mutex_t mpig_request_mutex_t;

#define MPIG_REQUEST_DEV_DECL													\
    struct mpig_request														\
    {																\
	/* mutex tp protect data and insure RC systems see updates */								\
	mpig_request_mutex_t mutex;												\
																\
	/* request type (combine with the top-level 'kind' field to determine exact nature of the request) */			\
	mpig_request_type_t type;												\
																\
	/* message envelope data (rank, tag, context id) */									\
	int rank;														\
	int tag;														\
	int ctx;														\
																\
	/* application buffer */												\
	void * buf;														\
	int cnt;														\
	MPI_Datatype dt;													\
																\
	/* pointer to datatype for reference counting purposes.  the datatype must be kept alive until the request is complete, \
	   even if the application were to free it. */										\
	struct MPID_Datatype * dtp;												\
																\
	/* handle of an associated remote request.  among other things, this information allows a remote cancel send handler to \
	   identify the correct request to remove from the unexpected queue.  this field is not used by the VMPI communication	\
	   module, but is required by the receive queue code, and thus is part of the general request structure. */		\
	int remote_req_id;													\
																\
	/* pointer to the communication method's request destruct function */							\
	mpig_request_cm_destruct_fn_t cm_destruct;										\
																\
	/* the virtual connection used to satisfy this request */								\
	mpig_vc_t * vc;														\
																\
	/* pointer to recvq receive-any-source operation object handing the pending unregister of the vendor MPI request */	\
	struct mpig_recvq_ras_op * recvq_unreg_ras_op;										\
																\
	/* pointer allowing the request to be inserted into any number of lists/queues */					\
	struct MPID_Request * next;												\
    } dev;

/* FIXME: the defintion of MPIG_REQUEST_CMS_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_REQUEST_CMS_SELF_DECL)
#define MPIG_REQUEST_CMS_SELF_DECL
#else
#undef MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMS_VMPI_DECL)
#define MPIG_REQUEST_CMS_VMPI_DECL
#else
#undef MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMS_XIO_DECL)
#define MPIG_REQUEST_CMS_XIO_DECL
#else
#undef MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMS_OTHER_DECL)
#define MPIG_REQUEST_CMS_OTHER_DECL
#else
#undef MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMS_DECL_UNION_DEFINED
#endif

#if defined(MPIG_REQUEST_CMS_DECL_UNION_DEFINED)
#define MPIG_REQUEST_CMS_DECL		\
    union				\
    {					\
	MPIG_REQUEST_CMS_SELF_DECL	\
	MPIG_REQUEST_CMS_VMPI_DECL	\
	MPIG_REQUEST_CMS_XIO_DECL	\
	MPIG_REQUEST_CMS_OTHER_DECL	\
    } cms;
#else
#define MPIG_REQUEST_CMS_DECL
#endif

/* FIXME: the defintion of MPIG_REQUEST_CMU_DECL should be generated by configure based on a list of communication modules. */
#if !defined(MPIG_REQUEST_CMU_SELF_DECL)
#define MPIG_REQUEST_CMU_SELF_DECL
#else
#undef MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMU_VMPI_DECL)
#define MPIG_REQUEST_CMU_VMPI_DECL
#else
#undef MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMU_XIO_DECL)
#define MPIG_REQUEST_CMU_XIO_DECL
#else
#undef MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_REQUEST_CMU_OTHER_DECL)
#define MPIG_REQUEST_CMU_OTHER_DECL
#else
#undef MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#define MPIG_REQUEST_CMU_DECL_UNION_DEFINED
#endif

#if defined(MPIG_REQUEST_CMU_DECL_UNION_DEFINED)
#define MPIG_REQUEST_CMU_DECL		\
    union				\
    {					\
	MPIG_REQUEST_CMU_SELF_DECL	\
	MPIG_REQUEST_CMU_VMPI_DECL	\
	MPIG_REQUEST_CMU_XIO_DECL	\
	MPIG_REQUEST_CMU_OTHER_DECL	\
    } cmu;
#else
#define MPIG_REQUEST_CMU_DECL
#endif

#define MPID_DEV_REQUEST_DECL	\
    MPIG_REQUEST_DEV_DECL	\
    MPIG_REQUEST_CMS_DECL	\
    MPIG_REQUEST_CMU_DECL
/**********************************************************************************************************************************
						       END REQUEST SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN STATUS SECTION
**********************************************************************************************************************************/
#if defined(HAVE_GLOBUS_DC_MODULE)
#define MPID_DEV_STATUS_SET_EMPTY_HOOK(status_)		\
{							\
    (status_)->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL;	\
}

#define MPID_DEV_STATUS_SET_PROC_NULL_HOOK(status_)	\
{							\
    (status_)->mpig_dc_format = GLOBUS_DC_FORMAT_LOCAL;	\
}

#define MPID_DEV_REQUEST_EXTRACT_STATUS_HOOK(req_, status_)	\
{								\
    (status_)->mpig_dc_format = (req_)->status.mpig_dc_format;	\
}
#endif
/**********************************************************************************************************************************
						       END STATUS SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						      BEGIN DATA CONVERSION
**********************************************************************************************************************************/
typedef enum mpig_endian
{
    MPIG_ENDIAN_UNKNOWN = 0,
    MPIG_ENDIAN_LITTLE,
    MPIG_ENDIAN_BIG
}
mpig_endian_t;
/**********************************************************************************************************************************
						       END DATA CONVERSION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN I/O VECTOR SECTION
**********************************************************************************************************************************/
typedef struct mpig_iov
{
    int max_entries;
    int free_entry;
    int cur_entry;
    size_t num_bytes;
    MPID_IOV iov[1];
}
mpig_iov_t;

#define MPIG_IOV_DECL(iov_var_name_, entries_)	\
    struct					\
    {						\
	int max_entries;			\
	int free_entry;				\
	int cur_entry;				\
	size_t num_bytes;			\
	MPID_IOV iov[entries_];			\
    } iov_var_name_[1]
/**********************************************************************************************************************************
						     END I/O VECTOR SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						    BEGIN DATA BUFFER SECTION
**********************************************************************************************************************************/
typedef struct mpig_databuf
{
    MPIU_Size_t size;
    MPIU_Size_t eod;
    MPIU_Size_t pos;
}
mpig_databuf_t;

#define MPIG_DATABUF_DECL(dbuf_var_name_, size_)	\
    struct						\
    {							\
	MPIU_Size_t size;				\
	MPIU_Size_t eod;				\
	MPIU_Size_t pos;				\
	char data[size_];				\
    } dbuf_var_name_[1]
/**********************************************************************************************************************************
						     END DATA BUFFER SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN BUSINESS CARD SECTION
**********************************************************************************************************************************/
typedef struct mpig_bc
{
    char * str_begin;
    char * str_end;
    size_t str_size;
    size_t str_left;
}
mpig_bc_t;
/**********************************************************************************************************************************
						    END BUSINESS CARD SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						BEGIN VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/
typedef int (*mpig_vc_adi3_send_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_isend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_rsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_irsend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_ssend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_issend_fn_t)(const void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_recv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, MPI_Status * status, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_irecv_fn_t)(void * buf, int cnt, MPI_Datatype dt, int rank, int tag, struct MPID_Comm * comm,
    int ctxoff, struct MPID_Request ** reqp);

typedef int (*mpig_vc_adi3_probe_fn_t)(int rank, int tag, struct MPID_Comm * comm, int ctxoff, MPI_Status * status);

typedef int (*mpig_vc_adi3_iprobe_fn_t)(int rank, int tag, struct MPID_Comm * comm, int ctxoff, int * found_p,
    MPI_Status * status);

typedef int (*mpig_vc_adi3_cancel_recv_fn_t)(struct MPID_Request * rreq);
	     
typedef int (*mpig_vc_adi3_cancel_send_fn_t)(struct MPID_Request * sreq);
	     
typedef int (*mpig_vc_recv_any_source_fn_t)(struct mpig_vc * vc, struct MPID_Request * rreq);

typedef void(*mpig_vc_inc_ref_count_fn_t)(struct mpig_vc * vc, bool_t * was_inuse);

typedef void (*mpig_vc_dec_ref_count_fn_t)(struct mpig_vc * vc, bool_t * inuse);

typedef void (*mpig_vc_destruct_fn_t)(struct mpig_vc * vc);

typedef double (*mpig_vc_vtable_last_entry_fn_t)(float foo, int bar, const short * baz, char bif);

typedef struct mpig_vc_vtable
{
    mpig_vc_adi3_send_fn_t		adi3_send;
    mpig_vc_adi3_isend_fn_t		adi3_isend;
    mpig_vc_adi3_rsend_fn_t		adi3_rsend;
    mpig_vc_adi3_irsend_fn_t		adi3_irsend;
    mpig_vc_adi3_ssend_fn_t		adi3_ssend;
    mpig_vc_adi3_issend_fn_t		adi3_issend;
    mpig_vc_adi3_recv_fn_t		adi3_recv;
    mpig_vc_adi3_irecv_fn_t		adi3_irecv;
    mpig_vc_adi3_probe_fn_t		adi3_probe;
    mpig_vc_adi3_iprobe_fn_t		adi3_iprobe;
    mpig_vc_adi3_cancel_recv_fn_t	adi3_cancel_recv;
    mpig_vc_adi3_cancel_send_fn_t	adi3_cancel_send;
    mpig_vc_recv_any_source_fn_t	recv_any_source;
    mpig_vc_inc_ref_count_fn_t		vc_inc_ref_count;
    mpig_vc_dec_ref_count_fn_t		vc_dec_ref_count;
    mpig_vc_destruct_fn_t		vc_destruct;
    mpig_vc_vtable_last_entry_fn_t	vtable_last_entry;
}
mpig_vc_vtable_t;

/*
 * create a union containing information specific to the communication methods.
 *
 * FIXME: the defintion of MPIG_VC_CMU_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_VC_CMU_SELF_DECL)
#define MPIG_VC_CMU_SELF_DECL
#else
#undef MPIG_VC_CMU_DECL_UNION_DEFINED
#define MPIG_VC_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CMU_VMPI_DECL)
#define MPIG_VC_CMU_VMPI_DECL
#else
#undef MPIG_VC_CMU_DECL_UNION_DEFINED
#define MPIG_VC_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CMU_XIO_DECL)
#define MPIG_VC_CMU_XIO_DECL
#else
#undef MPIG_VC_CMU_DECL_UNION_DEFINED
#define MPIG_VC_CMU_DECL_UNION_DEFINED
#endif
#if !defined(MPIG_VC_CMU_OTHER_DECL)
#define MPIG_VC_CMU_OTHER_DECL
#else
#undef MPIG_VC_CMU_DECL_UNION_DEFINED
#define MPIG_VC_CMU_DECL_UNION_DEFINED
#endif

#if defined(MPIG_VC_CMU_DECL_UNION_DEFINED)
#define MPIG_VC_CMU_DECL		\
    union			\
    {				\
	MPIG_VC_CMU_SELF_DECL	\
	MPIG_VC_CMU_VMPI_DECL	\
	MPIG_VC_CMU_XIO_DECL	\
	MPIG_VC_CMU_OTHER_DECL	\
    } cmu;
#else
#define MPIG_VC_CMU_DECL
#endif

/* create a structure containing information specific to the communication methods/modules.  this structure should be used to
 * store any information for a communication method/module that must persist independent of the method selected.  intially, this
 * structure was created to store information used to determine if two connections can speak to each other at the desired
 * topology level and thus contains some top-level information pertinent to topology discovery.
 *
 * NOTE: information not needed for computing topology information should reside in the union above to conserve space.
 *
 * FIXME: the defintion of MPIG_VC_CMS_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_VC_CMS_SELF_DECL)
#define MPIG_VC_CMS_SELF_DECL
#endif
#if !defined(MPIG_VC_CMS_VMPI_DECL)
#define MPIG_VC_CMS_VMPI_DECL
#endif
#if !defined(MPIG_VC_CMS_XIO_DECL)
#define MPIG_VC_CMS_XIO_DECL
#endif
#if !defined(MPIG_VC_CMS_OTHER_DECL)
#define MPIG_VC_CMS_OTHER_DECL
#endif

#define MPIG_VC_CMS_DECL					\
    struct							\
    {								\
	/* contact information successfully initialized */	\
	bool_t ci_initialized;					\
								\
	/* topology information */				\
	int topology_num_levels;				\
	unsigned topology_levels;				\
	int app_num;						\
	char * lan_id;						\
								\
	MPIG_VC_CMS_SELF_DECL					\
	MPIG_VC_CMS_VMPI_DECL					\
	MPIG_VC_CMS_XIO_DECL					\
	MPIG_VC_CMS_OTHER_DECL					\
    } cms;

typedef mpig_mutex_t mpig_vc_mutex_t;

typedef struct mpig_vc
{
    /* mutex tp protect data and insure RC systems see updates */
    mpig_vc_mutex_t mutex;
    
    /* number of references to this VC object */
    int ref_count;

    /* virtual function table containing ADI3 routines, etc. */
    mpig_vc_vtable_t * vtable;

    /* process group to which the process associated with this VC belongs, and the rank of that process in the process group */
    struct mpig_pg * pg;
    int pg_rank;

    /* local process id */
    int lpid;

    /* communication method type, associated functions, and module specific data structures */
    mpig_cm_t * cm;
    MPIG_VC_CMU_DECL
    MPIG_VC_CMS_DECL
    
    /* business card containing information on how to connect to the remote process */
    mpig_bc_t bc;

    /* status of the VC */
    bool_t initialized;
    
    /* map of MPI datatypes to basic C types */
    unsigned char dt_cmap[MPIG_DATATYPE_MAX_BASIC_TYPES];
}
mpig_vc_t;
/**********************************************************************************************************************************
						 END VIRTUAL CONNECTION SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
					BEGIN VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/
/*
 * MPID_VCRT/MPID_VCR
 *
 * MPID_VCRT is the virtual connection reference table object.  MPID_VCR is an array of virtual connection references, allowing
 * the MPICH and device layers fast access to the items in the table.
 *
 * FIXME: These structures should not be exposed as fields in the MPICH layer of MPID_Comm object.  A better technique would be to
 * define an interface that allows the MPICH layer to access the information without having any exposure to to the data
 * structures themselves.  We should work with the MPICH folks to define such an interface.
 */
typedef struct mpig_vcrt * MPID_VCRT;
typedef struct mpig_vc * MPID_VCR;
/**********************************************************************************************************************************
					 END VIRTUAL CONNECTION REFERENCE TABLE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN PROCESS GROUP SECTION
**********************************************************************************************************************************/
typedef mpig_mutex_t mpig_pg_mutex_t;

typedef struct mpig_pg
{
    /* mutex to protect data and insure RC systems see updates */
    mpig_pg_mutex_t mutex;

    /* number of references to this PG object */
    int ref_count;

    /* committed state of PG object.  committed PG objects may be destroyed when the reference count reaches zero. */
    bool_t committed;
    
    /* Number of processes in the process group */
    int size;

    /* unique id for the process group id.  this is required for MPI-2 dynamic process functionality. */
    char * id;
    
    /* next pointer used to maintain a list of all process groups known to this process */
    struct mpig_pg * next;

    /* table of virtual connection objects.  at present this is an array of VC objects.  someday we may make this an array of VC
       references, or perhaps something more wacky like a distributed array to reduce memory consumption when scaling to a very
       large number of processors.  thus, it is important to use mpig_pg_get_vc() instead of directly referencing this field. */
    mpig_vc_t vct[1];
}
mpig_pg_t;
/**********************************************************************************************************************************
						    END PROCESS GROUP SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						  BEGIN PROGRESS ENGINE SECTION
**********************************************************************************************************************************/
typedef unsigned long mpig_pe_count_t;
/*
 * MPID_PROGRESS_STATE_DECL
 *
 * This state object is used to prevent MPID_Progress_wait() from erroneous blocking if progress has occurred between
 * MPID_Progress_start() and MPID_Progress_wait().  The state object is allocated on the stack, and initialized by
 * MPID_Progress_start().  MPID_Progress_end() is only called if MPID_Progress_wait() is not, so any cleanup of data structures
 * in the state object must occur in both routines.
 *
 * FIXME: the defintion of MPIG_PE_STATE_CMS_DECL should be generated by configure based on a list of communication modules.
 */
#if !defined(MPIG_PE_STATE_CMS_SELF_DECL)
#define MPIG_PE_STATE_CMS_SELF_DECL
#else
#undef MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CMS_VMPI_DECL)
#define MPIG_PE_STATE_CMS_VMPI_DECL
#else
#undef MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CMS_XIO_DECL)
#define MPIG_PE_STATE_CMS_XIO_DECL
#else
#undef MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#endif
#if !defined(MPIG_PE_STATE_CMS_OTHER_DECL)
#define MPIG_PE_STATE_CMS_OTHER_DECL
#else
#undef MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#define MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED
#endif

#if defined(MPIG_PE_STATE_CMS_DECL_STRUCT_DEFINED)
#define MPIG_PE_STATE_CMS_DECL		\
    struct mpig_pe_state_cm		\
    {					\
	MPIG_PE_STATE_CMS_SELF_DECL	\
	MPIG_PE_STATE_CMS_VMPI_DECL	\
	MPIG_PE_STATE_CMS_XIO_DECL	\
	MPIG_PE_STATE_CMS_OTHER_DECL	\
    } cm;
#else
#define MPIG_PE_STATE_CMS_DECL
#endif

typedef struct mpig_pe_state
{
    /* snapshot of the progress engine completion counter */
    mpig_pe_count_t count;
}
mpig_pe_state_t;

#define MPID_PROGRESS_STATE_DECL	\
    mpig_pe_state_t dev;		\
    MPIG_PE_STATE_CMS_DECL

typedef struct mpig_pe_info
{
    int active_ops;		/* number of active operations */
    bool_t polling_required;	/* polling is required for the CM to make progress on avtive operations */
}
mpig_pe_info_t;
/**********************************************************************************************************************************
						   END PROGRESS ENGINE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						       BEGIN PT2PT SECTION
**********************************************************************************************************************************/
typedef struct mpig_msg_op_params
{
    void * buf;
    int cnt;
    MPI_Datatype dt;
    int rank;
    int tag;
    struct MPID_Comm * comm;
    int ctxoff;
}
mpig_msg_op_params_t;
/**********************************************************************************************************************************
							END PT2PT SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						   BEGIN RECEIVE QUEUE SECTION
**********************************************************************************************************************************/
typedef struct mpig_recvq_ras_op
{
    struct MPID_Request * req;
    bool_t complete;
    bool_t cancelled;
    bool_t cond_in_use;
    mpig_cond_t cond;
    mpig_genq_entry_t q_entry;
}
mpig_recvq_ras_op_t;
/**********************************************************************************************************************************
						    END RECEIVE QUEUE SECTION
**********************************************************************************************************************************/


/**********************************************************************************************************************************
						 BEGIN DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/
#if defined(MPIG_DEBUG)

/* enable MPICH layer debugging as well */
#if !defined(USE_DBG_LOGGING)
#define USE_DBG_LOGGING
#endif

/* include the macros that set the enter/exit macros to call MPIR_ENTER/EXIT_FUNC */
#include "mpifunclog.h"

#elif FALSE && defined(USE_DBG_LOGGING)
/* XXX: if the MPICH layer debugging has been enabled, then enable the device layer debugging as well? */
#define MPIG_DEBUG
#endif
/**********************************************************************************************************************************
						  END DEBUGGING OUTPUT SECTION
**********************************************************************************************************************************/

#endif /* MPICH2_MPIDPRE_H_INCLUDED */
