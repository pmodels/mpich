/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#undef DEBUG_VMPI
#if defined(DEBUG_VMPI)
#define DEBUG_PRINTF(a_) printf a_
#define DEBUG_FFLUSH() fflush(stdout)
#else
#define DEBUG_PRINTF(a)
#define DEBUG_FFLUSH()
#endif

#include "mpidconf.h"

#if defined(MPIG_VMPI)
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "mpig_vmpi.h"
#include <mpi.h>

#if defined(TYPEOF_VMPI_COMM_IS_BASIC)
#define MPIG_VMPI_COMM_INITIALIZER (0)
#else
#define MPIG_VMPI_COMM_INITIALIZER {{0}};
#endif

#if defined(TYPEOF_VMPI_DATATYPE_IS_BASIC)
#define MPIG_VMPI_DATATYPE_INITIALIZER (0)
#else
#define MPIG_VMPI_DATATYPE_INITIALIZER {{0}};
#endif

#if defined(TYPEOF_VMPI_OP_IS_BASIC)
#define MPIG_VMPI_OP_INITIALIZER (0)
#else
#define MPIG_VMPI_OP_INITIALIZER {{0}};
#endif

#if defined(TYPEOF_VMPI_REQUEST_IS_BASIC)
#define MPIG_VMPI_REQUEST_INITIALIZER (0)
#else
#define MPIG_VMPI_REQUEST_INITIALIZER {{0}};
#endif


/*
 * symbols for exporting predefined vendor MPI handles and values
 */
mpig_vmpi_request_t mpig_vmpi_request_null = MPIG_VMPI_REQUEST_INITIALIZER;
int mpig_vmpi_any_source = -1;
int mpig_vmpi_any_tag = -1;
int mpig_vmpi_max_error_string = -1;
int mpig_vmpi_proc_null = -1;
mpig_vmpi_status_t * mpig_vmpi_status_ignore = NULL;
mpig_vmpi_status_t * mpig_vmpi_statuses_ignore = NULL;
int mpig_vmpi_undefined = -1;
int mpig_vmpi_tag_ub = -1;

/* predefined communicators */
mpig_vmpi_comm_t mpig_vmpi_comm_null = MPIG_VMPI_COMM_INITIALIZER;
mpig_vmpi_comm_t mpig_vmpi_comm_world = MPIG_VMPI_COMM_INITIALIZER;
mpig_vmpi_comm_t mpig_vmpi_comm_self = MPIG_VMPI_COMM_INITIALIZER;

/* predefined datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_null = MPIG_VMPI_DATATYPE_INITIALIZER;
/* c basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_byte = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_signed_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_char = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_wchar = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_short = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_short = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_long_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long_long = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_float = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double = MPIG_VMPI_DATATYPE_INITIALIZER;
/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_short_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_float_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_int = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_long_double_int = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran basic datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_logical = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_character = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_precision = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_double_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
mpig_vmpi_datatype_t mpig_vmpi_dt_2integer = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2real = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_complex = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_2double_precision = MPIG_VMPI_DATATYPE_INITIALIZER;
/* fortran size specific datatypes */
mpig_vmpi_datatype_t mpig_vmpi_dt_integer1 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer2 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer4 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_integer16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real4 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_real16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex8 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex16 = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_complex32 = MPIG_VMPI_DATATYPE_INITIALIZER;
/* type representing a packed application buffer */
mpig_vmpi_datatype_t mpig_vmpi_dt_packed = MPIG_VMPI_DATATYPE_INITIALIZER;
/* pseudo datatypes used to manipulate the extent */
mpig_vmpi_datatype_t mpig_vmpi_dt_lb = MPIG_VMPI_DATATYPE_INITIALIZER;
mpig_vmpi_datatype_t mpig_vmpi_dt_ub = MPIG_VMPI_DATATYPE_INITIALIZER;

/* collective operations (for MPI_Reduce, etc.) */
mpig_vmpi_op_t mpig_vmpi_op_null = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_max = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_min = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_sum = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_prod = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_land = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_band = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_lor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_bor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_lxor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_bxor = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_minloc = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_maxloc = MPIG_VMPI_OP_INITIALIZER;
mpig_vmpi_op_t mpig_vmpi_op_replace = MPIG_VMPI_OP_INITIALIZER;

/* predefined error classes */
int mpig_vmpi_err_buffer = -1;
int mpig_vmpi_err_count = -1;
int mpig_vmpi_err_type = -1;
int mpig_vmpi_err_tag = -1;
int mpig_vmpi_err_comm = -1;
int mpig_vmpi_err_rank = -1;
int mpig_vmpi_err_root = -1;
int mpig_vmpi_err_truncate = -1;
int mpig_vmpi_err_group = -1;
int mpig_vmpi_err_op = -1;
int mpig_vmpi_err_request = -1;
int mpig_vmpi_err_topology = -1;
int mpig_vmpi_err_dims = -1;
int mpig_vmpi_err_arg = -1;
int mpig_vmpi_err_other = -1;
int mpig_vmpi_err_unknown = -1;
int mpig_vmpi_err_intern = -1;
int mpig_vmpi_err_in_status = -1;
int mpig_vmpi_err_pending = -1;
int mpig_vmpi_err_file = -1;
int mpig_vmpi_err_access = -1;
int mpig_vmpi_err_amode = -1;
int mpig_vmpi_err_bad_file = -1;
int mpig_vmpi_err_file_exists = -1;
int mpig_vmpi_err_file_in_use = -1;
int mpig_vmpi_err_no_space = -1;
int mpig_vmpi_err_no_such_file = -1;
int mpig_vmpi_err_io = -1;
int mpig_vmpi_err_read_only = -1;
int mpig_vmpi_err_conversion = -1;
int mpig_vmpi_err_dup_datarep = -1;
int mpig_vmpi_err_unsupported_datarep = -1;
int mpig_vmpi_err_info = -1;
int mpig_vmpi_err_info_key = -1;
int mpig_vmpi_err_info_value = -1;
int mpig_vmpi_err_info_nokey = -1;
int mpig_vmpi_err_name = -1;
int mpig_vmpi_err_no_mem = -1;
int mpig_vmpi_err_not_same = -1;
int mpig_vmpi_err_port = -1;
int mpig_vmpi_err_quota = -1;
int mpig_vmpi_err_service = -1;
int mpig_vmpi_err_spawn = -1;
int mpig_vmpi_err_unsupported_operation = -1;
int mpig_vmpi_err_win = -1;
int mpig_vmpi_err_base = -1;
int mpig_vmpi_err_locktype = -1;
int mpig_vmpi_err_keyval = -1;
int mpig_vmpi_err_rma_conflict = -1;
int mpig_vmpi_err_rma_sync = -1;
int mpig_vmpi_err_size = -1;
int mpig_vmpi_err_disp = -1;
int mpig_vmpi_err_assert = -1;


/*
 * miscellaneous internal variable and function declarations;
 */
static void mpig_vmpi_atexit_handler(void);

static int mpig_vmpi_initialized = 0;
static int mpig_vmpi_call_mpi_finalize = 0;
static int mpig_vmpi_module_ref_count = 0;
static int mpig_vmpi_aborting = 0;


/*
 * initialization and termination functions
 */

/* NOTE: this initialization routine may be called prior to the application calling main.  this is necessary if the vendor MPI is
   an implementation of the MPI-1 standard which requires that the command line parameters be passed to it.  see comments in
   mpig_vmpi_premain.c for additional details. */
int mpig_vmpi_init(int * argc_p, char *** argv_p)
{
    int flag;
    int vrc = MPI_SUCCESS;
#if defined(DEBUG_VMPI)
    int pid = (int) getpid();
#endif
    
    mpig_vmpi_module_ref_count += 1;

    if (mpig_vmpi_initialized) goto fn_return;
    mpig_vmpi_initialized = 1;

#   if defined(HAVE_GLOBUS_DUROC_MODULE)
    {
	/* BIG-UGLY-HACK: Globus DUROC deactivates all of its dependent modules during an atexit handler, including
	   globus_gram_myjob and globus_mp.  during the deactivation of globus_gram_myjob, the vendor MPI is called to free a
	   communicator allocated during the activation of that module.  this is undesirable since this results in the calling of
	   vendor MPI routines after the vendor MPI has been finalized by MPIg.  to solve this problem, we push our own routine
	   onto the atexit stack which calls the vendor MPI finalize routine, allowing the Globus routines to call any vendor MPI
	   routines they wish before the vendor MPI is deactivated. */
	atexit(mpig_vmpi_atexit_handler);
	mpig_vmpi_module_ref_count += 1;
    }
#   endif
    
    /* call the vendor implementation of MPI_Init(), but only if another library/module hasn't already called MPI_Init().  see
       the comments in mpig_vmpi_finalize() for a more detailed description of the problem. */
    DEBUG_PRINTF(("%d: entering mpig_vmpi_init\n", pid)); DEBUG_FFLUSH();
    vrc = MPI_Initialized(&flag);
    if (vrc) goto fn_fail;

    if (!flag)
    {
	DEBUG_PRINTF(("%d: before MPI_Init\n", pid)); DEBUG_FFLUSH();
	MPI_Init(argc_p, argv_p);
	DEBUG_PRINTF(("%d: after MPI_Init\n", pid)); DEBUG_FFLUSH();
	mpig_vmpi_call_mpi_finalize = 1;
    }

    /* set the error handlers for the predefined communicators so that all MPI functions return error codes */
    DEBUG_PRINTF(("%d: before setting error handler\n", pid)); DEBUG_FFLUSH();
#   if defined(HAVE_C_VMPI_COMM_SET_ERRHANDLER)
    {
	vrc = MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
	if (vrc) goto fn_fail;
	vrc = MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
	if (vrc) goto fn_fail;
    }
#   else
    {
	vrc = MPI_Errhandler_set(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
	if (vrc) goto fn_fail;
	vrc = MPI_Errhandler_set(MPI_COMM_SELF, MPI_ERRORS_RETURN);
	if (vrc) goto fn_fail;
    }
#   endif
    DEBUG_PRINTF(("%d: after setting error handler\n", pid)); DEBUG_FFLUSH();
    
    /* copy miscellaneous vendor MPI symbols into externally visible constants */
    *(MPI_Request *) &mpig_vmpi_request_null = MPI_REQUEST_NULL;
    
    mpig_vmpi_any_source = MPI_ANY_SOURCE;
    mpig_vmpi_any_tag = MPI_ANY_TAG;
    mpig_vmpi_max_error_string = MPI_MAX_ERROR_STRING;
    mpig_vmpi_proc_null = MPI_PROC_NULL;
    mpig_vmpi_status_ignore = (mpig_vmpi_status_t *) MPI_STATUS_IGNORE;
    mpig_vmpi_statuses_ignore = (mpig_vmpi_status_t *) MPI_STATUSES_IGNORE;
    mpig_vmpi_undefined = MPI_UNDEFINED;
    mpig_vmpi_tag_ub = MPI_TAG_UB;

    /* copy the vendor MPI predefined communicator handles into externally visible constants */
    *(MPI_Comm *) &mpig_vmpi_comm_null = MPI_COMM_NULL;
    *(MPI_Comm *) &mpig_vmpi_comm_world = MPI_COMM_WORLD;
    *(MPI_Comm *) &mpig_vmpi_comm_self = MPI_COMM_SELF;

    /* copy the vendor MPI predefined datatype handles into externally visible constants */
    *(MPI_Datatype *) &mpig_vmpi_dt_null = MPI_DATATYPE_NULL;
    /* c basic datatypes */
    *(MPI_Datatype *) &mpig_vmpi_dt_byte = MPI_BYTE;
    *(MPI_Datatype *) &mpig_vmpi_dt_char = MPI_CHAR;
#   if defined(HAVE_C_VMPI_SIGNED_CHAR)
    *(MPI_Datatype *) &mpig_vmpi_dt_signed_char = MPI_SIGNED_CHAR;
#   else    
    *(MPI_Datatype *) &mpig_vmpi_dt_signed_char = MPI_CHAR;
#   endif    
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_char = MPI_UNSIGNED_CHAR;
#   if defined(HAVE_C_VMPI_WCHAR)    
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_WCHAR;
#   elif (SIZEOF_WCHAR_T == SIZEOF_CHAR)
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_UNSIGNED_CHAR;
#   elif (SIZEOF_WCHAR_T == SIZEOF_SHORT)
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_UNSIGNED_SHORT;
#   elif (SIZEOF_WCHAR_T == SIZEOF_INT)
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_UNSIGNED;
#   elif (SIZEOF_WCHAR_T == SIZEOF_LONG)
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_UNSIGNED_LONG;
#   else    
    *(MPI_Datatype *) &mpig_vmpi_dt_wchar = MPI_DATATYPE_NULL;
#   endif    
    *(MPI_Datatype *) &mpig_vmpi_dt_short = MPI_SHORT;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_short = MPI_UNSIGNED_SHORT;
    *(MPI_Datatype *) &mpig_vmpi_dt_int = MPI_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned = MPI_UNSIGNED;
    *(MPI_Datatype *) &mpig_vmpi_dt_long = MPI_LONG;
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long = MPI_UNSIGNED_LONG;
#   if defined(HAVE_C_VMPI_LONG_LONG)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long = MPI_LONG_LONG;
#   elif defined(HAVE_C_VMPI_LONG_LONG_INT)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long = MPI_LONG_LONG_INT;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_LONG_LONG_INT)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long_int = MPI_LONG_LONG_INT;
#   elif defined(HAVE_C_VMPI_LONG_LONG)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long_int = MPI_LONG_LONG;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_long_long_int = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_UNSIGNED_LONG_LONG)
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long_long = MPI_UNSIGNED_LONG_LONG;
#   elif defined(HAVE_C_VMPI_LONG_LONG)
    /* NOTE: the vendor MPI is assumed to be running on a homogeneous system.  therefore, we ignore the possibility of corrupting
       the unsigned value if sign extension were to occur from a size change. */
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long_long = MPI_LONG_LONG;
#   elif defined(HAVE_C_VMPI_LONG_LONG_INT)
    /* NOTE: the vendor MPI is assumed to be running on a homogeneous system.  therefore, we ignore the possibility of corrupting
       the unsigned value if sign extension were to occur from a size change. */
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long_long = MPI_LONG_LONG_INT;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_unsigned_long_long = MPI_DATATYPE_NULL;
#   endif
    *(MPI_Datatype *) &mpig_vmpi_dt_float = MPI_FLOAT;
    *(MPI_Datatype *) &mpig_vmpi_dt_double = MPI_DOUBLE;
#   if defined(HAVE_C_VMPI_LONG_DOUBLE)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double = MPI_LONG_DOUBLE;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double = MPI_DATATYPE_NULL;
#   endif
    /* c paired datatypes used predominantly for minloc/maxloc reduce operations */
    *(MPI_Datatype *) &mpig_vmpi_dt_short_int = MPI_SHORT_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_2int = MPI_2INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_long_int = MPI_LONG_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_float_int = MPI_FLOAT_INT;
    *(MPI_Datatype *) &mpig_vmpi_dt_double_int = MPI_DOUBLE_INT;
#   if defined(HAVE_C_VMPI_LONG_DOUBLE_INT)
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double_int = MPI_LONG_DOUBLE_INT;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_long_double_int = MPI_DATATYPE_NULL;
#   endif
    /* fortran basic datatypes */
    *(MPI_Datatype *) &mpig_vmpi_dt_logical = MPI_LOGICAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_character = MPI_CHARACTER;
    *(MPI_Datatype *) &mpig_vmpi_dt_integer = MPI_INTEGER;
    *(MPI_Datatype *) &mpig_vmpi_dt_real = MPI_REAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_double_precision = MPI_DOUBLE_PRECISION;
    *(MPI_Datatype *) &mpig_vmpi_dt_complex = MPI_COMPLEX;
#   if defined(HAVE_C_VMPI_DOUBLE_COMPLEX)
    *(MPI_Datatype *) &mpig_vmpi_dt_double_complex = MPI_DOUBLE_COMPLEX;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_double_complex = MPI_DATATYPE_NULL;
#   endif
    /* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
    *(MPI_Datatype *) &mpig_vmpi_dt_2integer = MPI_2INTEGER;
    *(MPI_Datatype *) &mpig_vmpi_dt_2complex = MPI_2COMPLEX;
    *(MPI_Datatype *) &mpig_vmpi_dt_2real = MPI_2REAL;
    *(MPI_Datatype *) &mpig_vmpi_dt_2double_precision = MPI_2DOUBLE_PRECISION;
#   if defined(HAVE_C_VMPI_2DOUBLE_COMPLEX)
    *(MPI_Datatype *) &mpig_vmpi_dt_2double_complex = MPI_2DOUBLE_COMPLEX;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_2double_complex = MPI_DATATYPE_NULL;
#   endif
    /* fortran size specific datatypes */
#   if defined(HAVE_C_VMPI_INTEGER1)
    *(MPI_Datatype *) &mpig_vmpi_dt_integer1 = MPI_INTEGER1;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_integer1 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_INTEGER2)
    *(MPI_Datatype *) &mpig_vmpi_dt_integer2 = MPI_INTEGER2;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_integer4 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_INTEGER4)
    *(MPI_Datatype *) &mpig_vmpi_dt_integer4 = MPI_INTEGER4;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_integer4 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_INTEGER8)
    *(MPI_Datatype *) &mpig_vmpi_dt_integer8 = MPI_INTEGER8;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_integer8 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_INTEGER16)
    *(MPI_Datatype *) &mpig_vmpi_dt_integer16 = MPI_INTEGER16;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_integer16 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_REAL4)
    *(MPI_Datatype *) &mpig_vmpi_dt_real4 = MPI_REAL4;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_real4 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_REAL8)
    *(MPI_Datatype *) &mpig_vmpi_dt_real8 = MPI_REAL8;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_real8 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_REAL16)
    *(MPI_Datatype *) &mpig_vmpi_dt_real16 = MPI_REAL16;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_real16 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_COMPLEX8)
    *(MPI_Datatype *) &mpig_vmpi_dt_complex8 = MPI_COMPLEX8;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_complex8 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_COMPLEX16)
    *(MPI_Datatype *) &mpig_vmpi_dt_complex16 = MPI_COMPLEX16;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_complex16 = MPI_DATATYPE_NULL;
#   endif
#   if defined(HAVE_C_VMPI_COMPLEX32)
    *(MPI_Datatype *) &mpig_vmpi_dt_complex32 = MPI_COMPLEX32;
#   else
    *(MPI_Datatype *) &mpig_vmpi_dt_complex32 = MPI_DATATYPE_NULL;
#   endif
    /* type representing a packed application buffer */
    *(MPI_Datatype *) &mpig_vmpi_dt_packed = MPI_PACKED;
    /* pseudo datatypes used to manipulate the extent */
    *(MPI_Datatype *) &mpig_vmpi_dt_lb = MPI_LB;
    *(MPI_Datatype *) &mpig_vmpi_dt_ub = MPI_UB;

    /* copy the vendor collective operations (for MPI_Reduce, etc.) */
    *(MPI_Op *) &mpig_vmpi_op_null = MPI_OP_NULL;
    *(MPI_Op *) &mpig_vmpi_op_max = MPI_MAX;
    *(MPI_Op *) &mpig_vmpi_op_min = MPI_MIN;
    *(MPI_Op *) &mpig_vmpi_op_sum = MPI_SUM;
    *(MPI_Op *) &mpig_vmpi_op_prod = MPI_PROD;
    *(MPI_Op *) &mpig_vmpi_op_land = MPI_LAND;
    *(MPI_Op *) &mpig_vmpi_op_band = MPI_BAND;
    *(MPI_Op *) &mpig_vmpi_op_lor = MPI_LOR;
    *(MPI_Op *) &mpig_vmpi_op_bor = MPI_BOR;
    *(MPI_Op *) &mpig_vmpi_op_lxor = MPI_LXOR;
    *(MPI_Op *) &mpig_vmpi_op_bxor =  MPI_BXOR;
    *(MPI_Op *) &mpig_vmpi_op_minloc = MPI_MINLOC;
    *(MPI_Op *) &mpig_vmpi_op_maxloc = MPI_MAXLOC;
#   if defined(HAVE_C_VMPI_REPLACE)
    *(MPI_Op *) &mpig_vmpi_op_replace = MPI_REPLACE;
#   else
    *(MPI_Op *) &mpig_vmpi_op_replace = MPI_OP_NULL;
#   endif
    
    /* copy the vendor MPI predefined error class values into the externally visible constants */
#   if defined(HAVE_C_VMPI_ERR_ACCESS)
    mpig_vmpi_err_access = MPI_ERR_ACCESS;
#   else
    mpig_vmpi_err_access = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_AMODE)
    mpig_vmpi_err_amode = MPI_ERR_AMODE;
#   else
    mpig_vmpi_err_amode = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_arg = MPI_ERR_ARG;
#   if defined(HAVE_C_VMPI_ERR_ASSERT)
    mpig_vmpi_err_assert = MPI_ERR_ASSERT;
#   else
    mpig_vmpi_err_assert = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_BAD_FILE)
    mpig_vmpi_err_bad_file = MPI_ERR_BAD_FILE;
#   else
    mpig_vmpi_err_bad_file = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_BASE)
    mpig_vmpi_err_base = MPI_ERR_BASE;
#   else
    mpig_vmpi_err_base = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_buffer = MPI_ERR_BUFFER;
    mpig_vmpi_err_comm = MPI_ERR_COMM;
#   if defined(HAVE_C_VMPI_ERR_CONVERSION)
    mpig_vmpi_err_conversion = MPI_ERR_CONVERSION;
#   else
    mpig_vmpi_err_conversion = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_count = MPI_ERR_COUNT;
    mpig_vmpi_err_dims = MPI_ERR_DIMS;
#   if defined(HAVE_C_VMPI_ERR_DISP)
    mpig_vmpi_err_disp = MPI_ERR_DISP;
#   else
    mpig_vmpi_err_disp = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_DUP_DATAREP)
    mpig_vmpi_err_dup_datarep = MPI_ERR_DUP_DATAREP;
#   else
    mpig_vmpi_err_dup_datarep = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_FILE)
    mpig_vmpi_err_file = MPI_ERR_FILE;
#   else
    mpig_vmpi_err_file = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_FILE_EXISTS)
    mpig_vmpi_err_file_exists = MPI_ERR_FILE_EXISTS;
#   else
    mpig_vmpi_err_file_exists = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_FILE_IN_USE)
    mpig_vmpi_err_file_in_use = MPI_ERR_FILE_IN_USE;
#   else
    mpig_vmpi_err_file_in_use = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_group = MPI_ERR_GROUP;
    mpig_vmpi_err_in_status = MPI_ERR_IN_STATUS;
#   if defined(HAVE_C_VMPI_ERR_INFO)
    mpig_vmpi_err_info = MPI_ERR_INFO;
#   else
    mpig_vmpi_err_info = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_INFO_KEY)
    mpig_vmpi_err_info_key = MPI_ERR_INFO_KEY;
#   else
    mpig_vmpi_err_info_key = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_INFO_NOKEY)
    mpig_vmpi_err_info_nokey = MPI_ERR_INFO_NOKEY;
#   else
    mpig_vmpi_err_info_nokey = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_INFO_VALUE)
    mpig_vmpi_err_info_value = MPI_ERR_INFO_VALUE;
#   else
    mpig_vmpi_err_info_value = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_intern = MPI_ERR_INTERN;
#   if defined(HAVE_C_VMPI_ERR_IO)
    mpig_vmpi_err_io = MPI_ERR_IO;
#   else
    mpig_vmpi_err_io = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_KEYVAL)
    mpig_vmpi_err_keyval = MPI_ERR_KEYVAL;
#   else
    mpig_vmpi_err_keyval = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_LOCKTYPE)
    mpig_vmpi_err_locktype = MPI_ERR_LOCKTYPE;
#   else
    mpig_vmpi_err_locktype = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_NAME)
    mpig_vmpi_err_name = MPI_ERR_NAME;
#   else
    mpig_vmpi_err_name = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_NO_MEM)
    mpig_vmpi_err_no_mem = MPI_ERR_NO_MEM;
#   else
    mpig_vmpi_err_no_mem = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_NO_SPACE)
    mpig_vmpi_err_no_space = MPI_ERR_NO_SPACE;
#   else
    mpig_vmpi_err_no_space = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_NO_SUCH_FILE)
    mpig_vmpi_err_no_such_file = MPI_ERR_NO_SUCH_FILE;
#   else
    mpig_vmpi_err_no_such_file = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_NOT_SAME)
    mpig_vmpi_err_not_same = MPI_ERR_NOT_SAME;
#   else
    mpig_vmpi_err_not_same = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_op = MPI_ERR_OP;
    mpig_vmpi_err_other = MPI_ERR_OTHER;
    mpig_vmpi_err_pending = MPI_ERR_PENDING;
#   if defined(HAVE_C_VMPI_ERR_PORT)
    mpig_vmpi_err_port = MPI_ERR_PORT;
#   else
    mpig_vmpi_err_port = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_QUOTA)
    mpig_vmpi_err_quota = MPI_ERR_QUOTA;
#   else
    mpig_vmpi_err_quota = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_rank = MPI_ERR_RANK;
#   if defined(HAVE_C_VMPI_ERR_READ_ONLY)
    mpig_vmpi_err_read_only = MPI_ERR_READ_ONLY;
#   else
    mpig_vmpi_err_read_only = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_request = MPI_ERR_REQUEST;
#   if defined(HAVE_C_VMPI_ERR_RMA_CONFLICT)
    mpig_vmpi_err_rma_conflict = MPI_ERR_RMA_CONFLICT;
#   else
    mpig_vmpi_err_rma_conflict = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_RMA_SYNC)
    mpig_vmpi_err_rma_sync = MPI_ERR_RMA_SYNC;
#   else
    mpig_vmpi_err_rma_sync = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_root = MPI_ERR_ROOT;
#   if defined(HAVE_C_VMPI_ERR_SERVICE)
    mpig_vmpi_err_service = MPI_ERR_SERVICE;
#   else
    mpig_vmpi_err_service = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_SIZE)
    mpig_vmpi_err_size = MPI_ERR_SIZE;
#   else
    mpig_vmpi_err_size = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_SPAWN)
    mpig_vmpi_err_spawn = MPI_ERR_SPAWN;
#   else
    mpig_vmpi_err_spawn = MPI_ERR_UNKNOWN;
#   endif
    mpig_vmpi_err_tag = MPI_ERR_TAG;
    mpig_vmpi_err_topology = MPI_ERR_TOPOLOGY;
    mpig_vmpi_err_truncate = MPI_ERR_TRUNCATE;
    mpig_vmpi_err_type = MPI_ERR_TYPE;
    mpig_vmpi_err_unknown = MPI_ERR_UNKNOWN;
#   if defined(HAVE_C_VMPI_ERR_UNSUPPORTED_DATAREP)
    mpig_vmpi_err_unsupported_datarep = MPI_ERR_UNSUPPORTED_DATAREP;
#   else
    mpig_vmpi_err_unsupported_datarep = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_UNSUPPORTED_OPERATION)
    mpig_vmpi_err_unsupported_operation = MPI_ERR_UNSUPPORTED_OPERATION;
#   else
    mpig_vmpi_err_unsupported_operation = MPI_ERR_UNKNOWN;
#   endif
#   if defined(HAVE_C_VMPI_ERR_WIN)
    mpig_vmpi_err_win = MPI_ERR_WIN;
#   else
    mpig_vmpi_err_win = MPI_ERR_UNKNOWN;
#   endif

#   if defined(DEBUG_VMPI)
    {
	int exists;
	int tag_ub;
	int vrc;

	DEBUG_PRINTF(("%d: extracting tag upper bound\n", pid)); DEBUG_FFLUSH();
	
#       if defined(HAVE_C_VMPI_COMM_GET_ATTR)
	{
	    vrc = MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &tag_ub, &exists);
	}
#       else
	{
	    vrc = MPI_Attr_get(MPI_COMM_WORLD, MPI_TAG_UB, &tag_ub, &exists);
	}
#       endif

	DEBUG_PRINTF(("%d: finished extracting tag upper bound: vrc=%d, exists=%d, tag_ub=%d\n", pid, vrc, exists, tag_ub));
	DEBUG_FFLUSH();
    }
#   endif
    
  fn_return:
    DEBUG_PRINTF(("%d: exiting from mpig_vmpi_init\n", pid)); DEBUG_FFLUSH();
    return vrc;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	DEBUG_PRINTF(("%d: entering mpig_vmpi_init fn_fail block\n", pid)); DEBUG_FFLUSH();
	if (mpig_vmpi_call_mpi_finalize)
	{
	    MPI_Finalize();
	}
	DEBUG_PRINTF(("%d: exiting mpig_vmpi_init fn_fail block\n", pid)); DEBUG_FFLUSH();

	goto fn_return;
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_vmpi_init() */

int mpig_vmpi_finalize(void)
{
    int vrc = MPIG_VMPI_SUCCESS;
#if defined(DEBUG_VMPI)
    int pid = (int) getpid();
#endif

    DEBUG_PRINTF(("%d: entering mpig_vmpi_init\n", pid)); DEBUG_FFLUSH();
    mpig_vmpi_module_ref_count -= 1;
    if (mpig_vmpi_module_ref_count == 0)
    {
	/*
	 * call the vendor implementation of MPI_Finalize(), but only if we also called MPI_Init().  if some other library/module
	 * called MPI_Init(), then we should let them decide when to call MPI_Finalize().
	 *
	 * this is particularily important for globus/nexus which delays calling MPI_Finalize() until exit() is called.  nexus does
	 * this so that it can be activated and deactivated multiple times, something MPI can't handle.  also, nexus keeps an
	 * outstanding receive posted until exit() is called, and calling MPI_Finalize() before that receive is cancelled causes
	 * some implementations (ex: SGI) to hang.
	 */
	if (mpig_vmpi_call_mpi_finalize)
	{
	    DEBUG_PRINTF(("%d: before MPI_Finalize\n", pid)); DEBUG_FFLUSH();
	    vrc = MPI_Finalize();
	    if (vrc) goto fn_fail;
	    DEBUG_PRINTF(("%d: after MPI_Finalize\n", pid)); DEBUG_FFLUSH();
	}
    }

  fn_return:
    DEBUG_PRINTF(("%d: exiting mpig_vmpi_finalize\n", pid)); DEBUG_FFLUSH();
    return vrc;

  fn_fail:
    {   /* --BEGIN ERROR HANDLING-- */
	DEBUG_PRINTF(("%d: entering mpig_vmpi_finalize fn_fail block\n", pid)); DEBUG_FFLUSH();
	goto fn_return;
	DEBUG_PRINTF(("%d: exiting mpig_vmpi_finalize fn_fail block\n", pid)); DEBUG_FFLUSH();
    }   /* --END ERROR HANDLING-- */
}
/* end mpig_vmpi_finalize() */

int mpig_vmpi_abort(mpig_vmpi_comm_t comm, int exit_code)
{
    mpig_vmpi_aborting = 1;
    return MPI_Abort(*(MPI_Comm *) &comm, exit_code);
}
/* end mpig_vmpi_abort() */

static void mpig_vmpi_atexit_handler(void)
{
    if (!mpig_vmpi_aborting)
    {
#	if 0
	{
	    /* this assertion is correct if the program runs to completion, but it happens to call exit before deactivating all
	       conents using the vendor MPI, then the assertion erroneous */
	    if (mpig_vmpi_module_ref_count != 1)
	    {
		int pid = (int) getpid();
	    
		fprintf(stderr, "Assertion failed in process %d at line %d in %s: (mpig_vmpi_module_ref_count <%d> == 1)\n",
		    pid, __LINE__, __FILE__, mpig_vmpi_module_ref_count);
		fflush(stderr);
		MPI_Abort(MPI_COMM_WORLD, 1);
	    }
	}
#	endif
	
	mpig_vmpi_finalize();
    }
}
/* end mpig_vmpi_premain_atexit() */

/*
 * point-to-point functions
 */
int mpig_vmpi_send(const void * buf, int cnt, mpig_vmpi_datatype_t dt, const int dest, const int tag,
    const mpig_vmpi_comm_t comm)
{
    return MPI_Send((void *) buf, cnt, *(MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm);
}

int mpig_vmpi_isend(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int dest, int tag, const mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request)
{
    return MPI_Isend((void *) buf, cnt, *(const MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm, (MPI_Request *) request);
}

int mpig_vmpi_rsend(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int dest, int tag, const mpig_vmpi_comm_t comm)
{
    return MPI_Rsend((void *) buf, cnt, *(MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm);
}

int mpig_vmpi_irsend(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int dest, int tag, const mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request)
{
    return MPI_Irsend((void *) buf, cnt, *(MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm, (MPI_Request *) request);
}

int mpig_vmpi_ssend(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int dest, int tag, const mpig_vmpi_comm_t comm)
{
    return MPI_Ssend((void *) buf, cnt, *(MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm);
}

int mpig_vmpi_issend(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int dest, int tag, const mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request)
{
    return MPI_Issend((void *) buf, cnt, *(MPI_Datatype *) &dt, dest, tag, *(MPI_Comm *) &comm, (MPI_Request *) request);
}

int mpig_vmpi_probe(int src, int tag, const mpig_vmpi_comm_t comm, mpig_vmpi_status_t * status)
{
    return MPI_Probe(src, tag, *(MPI_Comm *) &comm, (MPI_Status *) status);
}

int mpig_vmpi_iprobe(int src, int tag, const mpig_vmpi_comm_t comm, int * flag, mpig_vmpi_status_t * status)
{
    return MPI_Iprobe(src, tag, *(MPI_Comm *) &comm, flag, (MPI_Status *) status);
}

int mpig_vmpi_recv(void * buf, int cnt, const mpig_vmpi_datatype_t dt, int src, int tag, const mpig_vmpi_comm_t comm,
    mpig_vmpi_status_t * status)
{
    return MPI_Recv(buf, cnt, *(MPI_Datatype *) &dt, src, tag, *(MPI_Comm *) &comm, (MPI_Status *) status);
}

int mpig_vmpi_irecv(void * buf, int cnt, const mpig_vmpi_datatype_t dt, int src, int tag, const mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request)
{
    return MPI_Irecv(buf, cnt, *(MPI_Datatype *) &dt, src, tag, *(MPI_Comm *) &comm, (MPI_Request *) request);
}

int mpig_vmpi_cancel(mpig_vmpi_request_t * const request)
{
    return MPI_Cancel((MPI_Request *) request);
}

int mpig_vmpi_request_free(mpig_vmpi_request_t * const request)
{
    return MPI_Request_free((MPI_Request *) request);
}

int mpig_vmpi_test(mpig_vmpi_request_t * const request, int * const flag, mpig_vmpi_status_t * const status)
{
    return MPI_Test((MPI_Request *) request, flag, (MPI_Status *) status);
}

int mpig_vmpi_wait(mpig_vmpi_request_t * const request, mpig_vmpi_status_t * const status)
{
    return MPI_Wait((MPI_Request *) request, (MPI_Status *) status);
}

int mpig_vmpi_testany(const int incount, mpig_vmpi_request_t * const requests, int * const index, int * flag,
    mpig_vmpi_status_t * const status)
{
    return MPI_Testany(incount, (MPI_Request *) requests, index, flag, (MPI_Status *) status);
}

int mpig_vmpi_waitany(const int incount, mpig_vmpi_request_t * const requests, int * const index,
    mpig_vmpi_status_t * const status)
{
    return MPI_Waitany(incount, (MPI_Request *) requests, index, (MPI_Status *) status);
}

int mpig_vmpi_testsome(const int incount, mpig_vmpi_request_t * const requests, int * const outcount, int * const indicies,
    mpig_vmpi_status_t * const statuses)
{
    return MPI_Testsome(incount, (MPI_Request *) requests, outcount, indicies, (MPI_Status *) statuses);
}

int mpig_vmpi_waitsome(const int incount, mpig_vmpi_request_t * const requests, int * const outcount, int * const indicies,
    mpig_vmpi_status_t * const statuses)
{
    return MPI_Waitsome(incount, (MPI_Request *) requests, outcount, indicies, (MPI_Status *) statuses);
}

int mpig_vmpi_test_cancelled(mpig_vmpi_status_t * status, int * flag)
{
    return MPI_Test_cancelled((MPI_Status *) status, flag);
}


/*
 * collective functions
 */
int mpig_vmpi_barrier(const mpig_vmpi_comm_t comm)
{
    return MPI_Barrier(*(MPI_Comm *) &comm);
}

int mpig_vmpi_bcast(const void * buf, int cnt, const mpig_vmpi_datatype_t dt, int root, const mpig_vmpi_comm_t comm)
{
    return MPI_Bcast((void *) buf, cnt, *(MPI_Datatype *) &dt, root, *(MPI_Comm *) &comm);
}

int mpig_vmpi_gather(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t send_dt, void * recv_buf, int recv_cnt,
    const mpig_vmpi_datatype_t recv_dt, int root, const mpig_vmpi_comm_t comm)
{
    return MPI_Gather((void *) send_buf, send_cnt, *(MPI_Datatype *) &send_dt, recv_buf, recv_cnt, *(MPI_Datatype *) &recv_dt,
	root, *(MPI_Comm *) &comm);
}

int mpig_vmpi_gatherv(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, const mpig_vmpi_datatype_t recv_dt, int root, const mpig_vmpi_comm_t comm)
{
    return MPI_Gatherv((void *) send_buf, send_cnt, *(MPI_Datatype *) &send_dt, recv_buf, (int *) recv_cnts, (int *) recv_displs,
	*(MPI_Datatype *) &recv_dt, root, *(MPI_Comm *) &comm);
}

int mpig_vmpi_allgather(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t send_dt, void * recv_buf, int recv_cnt,
    const mpig_vmpi_datatype_t recv_dt, const mpig_vmpi_comm_t comm)
{
    return MPI_Allgather((void *) send_buf, send_cnt, *(MPI_Datatype *) &send_dt, recv_buf, recv_cnt, *(MPI_Datatype *) &recv_dt,
	*(MPI_Comm *) &comm);
}

int mpig_vmpi_allgatherv(const void * send_buf, int send_cnt, const mpig_vmpi_datatype_t send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, const mpig_vmpi_datatype_t recv_dt, const mpig_vmpi_comm_t comm)
{
    return MPI_Allgatherv((void *) send_buf, send_cnt, *(MPI_Datatype *) &send_dt, recv_buf, (int *) recv_cnts,
	(int *) recv_displs, *(MPI_Datatype *) &recv_dt, *(MPI_Comm *) &comm);
}

int mpig_vmpi_allreduce(const void * send_buf, void * recv_buf, int cnt, const mpig_vmpi_datatype_t dt,
    const mpig_vmpi_op_t op, const mpig_vmpi_comm_t comm)
{
    return MPI_Allreduce((void *) send_buf, recv_buf, cnt, *(MPI_Datatype *) &dt, *(MPI_Op *) &op, *(MPI_Comm *) &comm);
}


/*
 * communicator functions
 */
int mpig_vmpi_comm_size(const mpig_vmpi_comm_t comm, int * const size_p)
{
    return MPI_Comm_size(*(MPI_Comm *) &comm, size_p);
}

int mpig_vmpi_comm_rank(const mpig_vmpi_comm_t comm, int * const rank_p)
{
    return MPI_Comm_rank(*(MPI_Comm *) &comm, rank_p);
}

int mpig_vmpi_comm_dup(const mpig_vmpi_comm_t old_comm, mpig_vmpi_comm_t * const new_comm_p)
{
    return MPI_Comm_dup(*(MPI_Comm *) &old_comm, (MPI_Comm *) new_comm_p);
}

int mpig_vmpi_comm_split(const mpig_vmpi_comm_t old_comm, const int color, const int key, mpig_vmpi_comm_t * const new_comm_p)
{
    return MPI_Comm_split(*(MPI_Comm *) &old_comm, color, key, (MPI_Comm *) new_comm_p);
}

int mpig_vmpi_intercomm_create(const mpig_vmpi_comm_t local_comm, const int local_leader, const mpig_vmpi_comm_t peer_comm,
    const int remote_leader, const int tag, mpig_vmpi_comm_t * const new_intercomm_p)
{
    return MPI_Intercomm_create(*(MPI_Comm *) &local_comm, local_leader, *(MPI_Comm *) &peer_comm, remote_leader, tag,
	(MPI_Comm *) new_intercomm_p);
}

int mpig_vmpi_intercomm_merge(const mpig_vmpi_comm_t intercomm, const int high, mpig_vmpi_comm_t * const new_intracomm_p)
{
    return MPI_Intercomm_merge(*(MPI_Comm *) &intercomm, high, (MPI_Comm *) new_intracomm_p);
}

int mpig_vmpi_comm_free(mpig_vmpi_comm_t * const comm_p)
{
    return MPI_Comm_free((MPI_Comm *) comm_p);
}

int mpig_vmpi_comm_get_attr(const mpig_vmpi_comm_t comm, const int key, void * const val_p, int * const flag_p)
{
#   if defined(HAVE_C_VMPI_COMM_GET_ATTR)
    {
	return MPI_Comm_get_attr(comm, key, val_p, flag_p);
    }
#   else
    {
	return MPI_Attr_get(comm, key, val_p, flag_p);
    }
#   endif
}


/*
 * datatype functions
 */
#if !defined(HAVE_C_VMPI_TYPE_CREATE_HVECTOR)
#undef MPI_Type_create_hvector
#define MPI_Type_create_hvector MPI_Type_hvector
#endif

#if !defined(HAVE_C_VMPI_TYPE_CREATE_HINDEXED)
#undef MPI_Type_create_hindexed
#define MPI_Type_create_hindexed MPI_Type_hindexed
#endif

#if !defined(HAVE_C_VMPI_TYPE_CREATE_STRUCT)
#undef MPI_Type_create_struct
#define MPI_Type_create_struct MPI_Type_struct
#endif

int mpig_vmpi_type_contiguous(const int cnt, const mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * const new_dt_p)
{
    return MPI_Type_contiguous(cnt, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
}

int mpig_vmpi_type_vector(const int cnt, const int blklen, const int stride, const mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * const new_dt_p)
{
    return MPI_Type_vector(cnt, blklen, stride, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
}

int mpig_vmpi_type_create_hvector(const int cnt, const int blklen, const mpig_vmpi_aint_t stride,
    const mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * const new_dt_p)
{
    return MPI_Type_create_hvector(cnt, blklen, (MPI_Aint) stride, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
}

int mpig_vmpi_type_indexed(const int cnt, int * const blklens, int * const displs, const mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * const new_dt_p)
{
    return MPI_Type_indexed(cnt, blklens, displs, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
}

#if defined(HAVE_C_VMPI_TYPE_CREATE_INDEXED_BLOCK)
int mpig_vmpi_type_create_indexed_block(const int cnt, const int blklen, int * const displs, const mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * const new_dt_p)
{
    return MPI_Type_create_indexed_block(cnt, blklen, displs, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
}
#endif

int mpig_vmpi_type_create_hindexed(const int cnt, int * const blklens, mpig_vmpi_aint_t * const displs,
    const mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * const new_dt_p)
{
#   if (SIZEOF_MPICH2_AINT == SIZEOF_VMPI_AINT)
    {
	return MPI_Type_create_hindexed(cnt, blklens, (MPI_Aint *) displs, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
    }
#   else
#       error FIXME: translate array of MPICH2 MPI_Aints to array of vendor MPI_Aints
#   endif
}

int mpig_vmpi_type_create_struct(const int cnt, int * const blklens, mpig_vmpi_aint_t * const displs,
    mpig_vmpi_datatype_t * const old_dts, mpig_vmpi_datatype_t * const new_dt_p)
{
#   if (SIZEOF_MPICH2_AINT == SIZEOF_VMPI_AINT)
    {
	return MPI_Type_create_struct(cnt, blklens, (MPI_Aint *) displs, (MPI_Datatype *) old_dts, (MPI_Datatype *) new_dt_p);
    }
#   else
#       error FIXME: translate array of MPICH2 MPI_Aints to array of vendor MPI_Aints
#   endif
}

int mpig_vmpi_type_dup(const mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * const new_dt_p)
{
#   if defined(HAVE_C_VMPI_TYPE_DUP)
    {
	return MPI_Type_dup(*(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
    }
#   else
    {
	return MPI_Type_contiguous(1, *(MPI_Datatype *) &old_dt, (MPI_Datatype *) new_dt_p);
    }
#   endif
}

#if defined(HAVE_C_VMPI_TYPE_CREATE_RESIZED)
int mpig_vmpi_type_create_resized(mpig_vmpi_datatype_t old_dt, mpig_vmpi_aint_t lb, mpig_vmpi_aint_t extent,
    mpig_vmpi_datatype_t * new_dt_p)
{
    return MPI_Type_create_resized(*(MPI_Datatype *) &old_dt, (MPI_Aint) lb, (MPI_Aint) extent, (MPI_Datatype *) new_dt_p);
}
#endif

int mpig_vmpi_type_lb(mpig_vmpi_datatype_t dt, mpig_vmpi_aint_t * displ)
{
#   if (SIZEOF_MPICH2_AINT == SIZEOF_VMPI_AINT)
    {
	return MPI_Type_lb(*(MPI_Datatype *) &dt, (MPI_Aint *) displ);
    }
#   else
    {
	const MPI_Aint vdispl;
	int vmpi_errno;
	
	vmpi_errno = MPI_Type_lb(*(MPI_Datatype *) &dt, &vdispl);
	if (vmpi_errno == MPI_SUCCESS)
	{
	    *displ = (mpig_vmpi_aint_t) vdispl;
	}

	return vmpi_errno;
    }
    }
#   endif
}

int mpig_vmpi_type_ub(mpig_vmpi_datatype_t dt, mpig_vmpi_aint_t * displ)
{
#   if (SIZEOF_MPICH2_AINT == SIZEOF_VMPI_AINT)
    {
	return MPI_Type_ub(*(MPI_Datatype *) &dt, (MPI_Aint *) displ);
    }
#   else
    {
	const MPI_Aint vdispl;
	int vmpi_errno;
	
	vmpi_errno = MPI_Type_ub(*(MPI_Datatype *) &dt, &vdispl);
	if (vmpi_errno == MPI_SUCCESS)
	{
	    *displ = (mpig_vmpi_aint_t) vdispl;
	}

	return vmpi_errno;
    }
#   endif
}

int mpig_vmpi_type_commit(mpig_vmpi_datatype_t * const dt_p)
{
    return MPI_Type_commit((MPI_Datatype *) dt_p);
}

int mpig_vmpi_type_free(mpig_vmpi_datatype_t * const dt_p)
{
    return MPI_Type_free((MPI_Datatype *) dt_p);
}

int mpig_vmpi_get_count(mpig_vmpi_status_t * const status, const mpig_vmpi_datatype_t dt, int * const count_p)
{
    return MPI_Get_count((MPI_Status *) status, *(MPI_Datatype *) &dt, count_p);
}


/*
 * error extraction and conversion functions
 */
int mpig_vmpi_error_class(const int vendor_errno, int * const vendor_class_p)
{
    return MPI_Error_class(vendor_errno, vendor_class_p);
}

int mpig_vmpi_error_string(const int vendor_errno, char * const string, int * const result_length)
{
    return MPI_Error_string(vendor_errno, string, result_length);
}


/*
 * utility functions not defined in the MPI standard but needed to transition between MPICH2 and the vendor MPI
 */
void mpig_vmpi_status_set_source(mpig_vmpi_status_t * const status, const int source)
{
    ((MPI_Status *) status)->MPI_SOURCE = source;
}

int mpig_vmpi_status_get_source(const mpig_vmpi_status_t * const status)
{
    return ((const MPI_Status *) status)->MPI_SOURCE;
}

void mpig_vmpi_status_set_tag(mpig_vmpi_status_t * const status, const int tag)
{
    ((MPI_Status *) status)->MPI_TAG = tag;
}

int mpig_vmpi_status_get_tag(const mpig_vmpi_status_t * const status)
{
    return ((const MPI_Status *) status)->MPI_TAG;
}

void mpig_vmpi_status_set_error(mpig_vmpi_status_t * const status, const int error_code)
{
    ((MPI_Status *) status)->MPI_ERROR = error_code;
}

int mpig_vmpi_status_get_error(const mpig_vmpi_status_t * status)
{
    return ((const MPI_Status *) status)->MPI_ERROR;
}

#if !defined(TYPEOF_VMPI_COMM_IS_BASIC)
int mpig_vmpi_comm_is_null(const mpig_vmpi_comm_t comm)
{
    return ((*(const MPI_Comm *) &comm) == MPI_COMM_NULL);
}
#endif

#if !defined(TYPEOF_VMPI_DATATYPE_IS_BASIC)
int mpig_vmpi_datatype_is_null(const mpig_vmpi_datatype_t dt)
{
    return ((*(const MPI_Datatype *) &dt) == MPI_DATATYPE_NULL);
}
#endif

#if !defined(TYPEOF_VMPI_OP_IS_BASIC)
int mpig_vmpi_op_is_null(const mpig_vmpi_op_t op)
{
    return ((*(const MPI_Op *) &op) == MPI_OP_NULL);
}
#endif

#if !defined(TYPEOF_VMPI_REQUEST_IS_BASIC)
int mpig_vmpi_request_is_null(const mpig_vmpi_request_t req)
{
    return ((*(const MPI_Request *) &req) == MPI_REQUEST_NULL);
}
#endif


#endif /* defined(MPIG_VMPI) */
