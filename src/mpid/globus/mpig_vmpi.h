/*
 * Globus device code:          Copyright 2005 Northern Illinois University
 * Borrowed MPICH-G2 code:      Copyright 2000 Argonne National Laboratory and Northern Illinois University
 * Borrowed MPICH2 device code: Copyright 2001 Argonne National Laboratory
 *
 * XXX: INSERT POINTER TO OFFICIAL COPYRIGHT TEXT
 */

#if !defined(MPICH2_MPIG_VMPI_H_INCLUDED)
#define MPICH2_MPIG_VMPI_H_INCLUDED 1

#if defined(__cplusplus)
extern "C"
{
#endif

/*
 * types large enough to hold vendor MPI object handles and structures
 */
typedef TYPEOF_VMPI_COMM mpig_vmpi_comm_t;
typedef TYPEOF_VMPI_DATATYPE mpig_vmpi_datatype_t;
typedef TYPEOF_VMPI_OP mpig_vmpi_op_t;
typedef TYPEOF_VMPI_REQUEST mpig_vmpi_request_t;
typedef TYPEOF_VMPI_STATUS mpig_vmpi_status_t;
typedef TYPEOF_MPICH2_AINT mpig_vmpi_aint_t;

/*
 * externally accessible symbols containing vendor MPI values
 */
/* miscellaneous values */
extern mpig_vmpi_request_t mpig_vmpi_request_null;
#define MPIG_VMPI_REQUEST_NULL (*(const mpig_vmpi_request_t *) &mpig_vmpi_request_null)
extern int mpig_vmpi_any_source;
#define MPIG_VMPI_ANY_SOURCE (*(const int *) &mpig_vmpi_any_source)
extern int mpig_vmpi_any_tag;
#define MPIG_VMPI_ANY_TAG (*(const int *) &mpig_vmpi_any_tag)
extern int mpig_vmpi_max_error_string;
#define MPIG_VMPI_MAX_ERROR_STRING (*(const int *) &mpig_vmpi_max_error_string)
extern int mpig_vmpi_proc_null;
#define MPIG_VMPI_PROC_NULL (*(const int *) &mpig_vmpi_proc_null)
extern int mpig_vmpi_undefined;
#define MPIG_VMPI_UNDEFINED (*(const int *) &mpig_vmpi_undefined)
extern int mpig_vmpi_tag_ub;
#define MPIG_VMPI_TAG_UB (*(const int *) &mpig_vmpi_tag_ub)

/* miscellaneous pointers.  NOTE: these pointers cannot use the const qualifier since they are frequently passed into functions
   as arguments that are not declared as constant. */
extern mpig_vmpi_status_t * mpig_vmpi_status_ignore;
#define MPIG_VMPI_STATUS_IGNORE ((mpig_vmpi_status_t *) mpig_vmpi_status_ignore)
extern mpig_vmpi_status_t * mpig_vmpi_statuses_ignore;
#define MPIG_VMPI_STATUSES_IGNORE ((mpig_vmpi_status_t *) mpig_vmpi_statuses_ignore)

/* predefined communicators */
extern mpig_vmpi_comm_t mpig_vmpi_comm_null;
#define MPIG_VMPI_COMM_NULL (*(const mpig_vmpi_comm_t *) &mpig_vmpi_comm_null)
extern mpig_vmpi_comm_t mpig_vmpi_comm_world;
#define MPIG_VMPI_COMM_WORLD (*(const mpig_vmpi_comm_t *) &mpig_vmpi_comm_world)
extern mpig_vmpi_comm_t mpig_vmpi_comm_self;
#define MPIG_VMPI_COMM_SELF (*(const mpig_vmpi_comm_t *) &mpig_vmpi_comm_self)

/* predefined datatypes */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_null;
#define MPIG_VMPI_DATATYPE_NULL (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_null)
/* c basic datatypes */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_byte;
#define MPIG_VMPI_BYTE (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_byte)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_char;
#define MPIG_VMPI_CHAR (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_char)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_signed_char;
#define MPIG_VMPI_SIGNED_CHAR (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_signed_char)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_char;
#define MPIG_VMPI_UNSIGNED_CHAR (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_unsigned_char)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_wchar;
#define MPIG_VMPI_WCHAR (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_wchar)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_short;
#define MPIG_VMPI_SHORT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_short)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_short;
#define MPIG_VMPI_UNSIGNED_SHORT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_unsigned_short)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_int;
#define MPIG_VMPI_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned;
#define MPIG_VMPI_UNSIGNED (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_unsigned)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long;
#define MPIG_VMPI_LONG (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long;
#define MPIG_VMPI_UNSIGNED_LONG (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_unsigned_long)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long_long;
#define MPIG_VMPI_LONG_LONG (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long_long)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long_long_int;
#define MPIG_VMPI_LONG_LONG_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long_long_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_unsigned_long_long;
#define MPIG_VMPI_UNSIGNED_LONG_LONG (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_unsigned_long_long)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_float;
#define MPIG_VMPI_FLOAT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_float)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_double;
#define MPIG_VMPI_DOUBLE (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_double)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long_double;
#define MPIG_VMPI_LONG_DOUBLE (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long_double)
/* c paired datatypes used predominantly for minloc/maxloc reduce operations */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_short_int;
#define MPIG_VMPI_SHORT_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_short_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2int;
#define MPIG_VMPI_2INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long_int;
#define MPIG_VMPI_LONG_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_float_int;
#define MPIG_VMPI_FLOAT_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_float_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_double_int;
#define MPIG_VMPI_DOUBLE_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_double_int)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_long_double_int;
#define MPIG_VMPI_LONG_DOUBLE_INT (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_long_double_int)
/* fortran basic datatypes */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_logical;
#define MPIG_VMPI_LOGICAL (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_logical)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_character;
#define MPIG_VMPI_CHARACTER (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_character)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer;
#define MPIG_VMPI_INTEGER (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_real;
#define MPIG_VMPI_REAL (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_real)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_double_precision;
#define MPIG_VMPI_DOUBLE_PRECISION (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_double_precision)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_complex;
#define MPIG_VMPI_COMPLEX (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_complex)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_double_complex;
#define MPIG_VMPI_DOUBLE_COMPLEX (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_double_complex)
/* fortran paired datatypes used predominantly for minloc/maxloc reduce operations */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2integer;
#define MPIG_VMPI_2INTEGER (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2integer)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2complex;
#define MPIG_VMPI_2COMPLEX (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2complex)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2real;
#define MPIG_VMPI_2REAL (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2real)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2double_complex;
#define MPIG_VMPI_2DOUBLE_COMPLEX (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2double_complex)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_2double_precision;
#define MPIG_VMPI_2DOUBLE_PRECISION (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_2double_precision)
/* fortran size specific datatypes */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer1;
#define MPIG_VMPI_INTEGER1 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer1)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer2;
#define MPIG_VMPI_INTEGER2 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer2)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer4;
#define MPIG_VMPI_INTEGER4 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer4)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer8;
#define MPIG_VMPI_INTEGER8 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer8)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_integer16;
#define MPIG_VMPI_INTEGER16 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_integer16)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_real4;
#define MPIG_VMPI_REAL4 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_real4)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_real8;
#define MPIG_VMPI_REAL8 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_real8)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_real16;
#define MPIG_VMPI_REAL16 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_real16)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_complex8;
#define MPIG_VMPI_COMPLEX8 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_complex8)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_complex16;
#define MPIG_VMPI_COMPLEX16 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_complex16)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_complex32;
#define MPIG_VMPI_COMPLEX32 (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_complex32)
/* type representing a packed application buffer */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_packed;
#define MPIG_VMPI_PACKED (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_packed)
/* pseudo datatypes used to manipulate the extent */
extern mpig_vmpi_datatype_t mpig_vmpi_dt_lb;
#define MPIG_VMPI_LB (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_lb)
extern mpig_vmpi_datatype_t mpig_vmpi_dt_ub;
#define MPIG_VMPI_UB (*(const mpig_vmpi_datatype_t *) &mpig_vmpi_dt_ub)

/* collective operations (for MPI_Reduce, etc.) */
extern mpig_vmpi_op_t mpig_vmpi_op_null;
#define MPIG_VMPI_OP_NULL (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_null)
extern mpig_vmpi_op_t mpig_vmpi_op_max;
#define MPIG_VMPI_MAX (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_max)
extern mpig_vmpi_op_t mpig_vmpi_op_min;
#define MPIG_VMPI_MIN (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_min)
extern mpig_vmpi_op_t mpig_vmpi_op_sum;
#define MPIG_VMPI_SUM (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_sum)
extern mpig_vmpi_op_t mpig_vmpi_op_prod;
#define MPIG_VMPI_PROD (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_prod)
extern mpig_vmpi_op_t mpig_vmpi_op_land;
#define MPIG_VMPI_LAND (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_land)
extern mpig_vmpi_op_t mpig_vmpi_op_band;
#define MPIG_VMPI_BAND (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_band)
extern mpig_vmpi_op_t mpig_vmpi_op_lor;
#define MPIG_VMPI_LOR (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_lor)
extern mpig_vmpi_op_t mpig_vmpi_op_bor;
#define MPIG_VMPI_BOR (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_bor)
extern mpig_vmpi_op_t mpig_vmpi_op_lxor;
#define MPIG_VMPI_LXOR (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_lxor)
extern mpig_vmpi_op_t mpig_vmpi_op_bxor;
#define MPIG_VMPI_BXOR (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_bxor)
extern mpig_vmpi_op_t mpig_vmpi_op_minloc;
#define MPIG_VMPI_MINLOC (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_minloc)
extern mpig_vmpi_op_t mpig_vmpi_op_maxloc;
#define MPIG_VMPI_MAXLOC (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_maxloc)
extern mpig_vmpi_op_t mpig_vmpi_op_replace;
#define MPIG_VMPI_REPLACE (*(const mpig_vmpi_op_t *) &mpig_vmpi_op_replace)

/* error classes */
#define MPIG_VMPI_SUCCESS (0)  /* the MPI standard defines MPI_SUCCESS to always be zero */
extern int mpig_vmpi_err_buffer;
#define MPIG_VMPI_ERR_BUFFER (*(const int *) &mpig_vmpi_err_buffer)
extern int mpig_vmpi_err_count;
#define MPIG_VMPI_ERR_COUNT (*(const int *) &mpig_vmpi_err_count)
extern int mpig_vmpi_err_type;
#define MPIG_VMPI_ERR_TYPE (*(const int *) &mpig_vmpi_err_type)
extern int mpig_vmpi_err_tag;
#define MPIG_VMPI_ERR_TAG (*(const int *) &mpig_vmpi_err_tag)
extern int mpig_vmpi_err_comm;
#define MPIG_VMPI_ERR_COMM (*(const int *) &mpig_vmpi_err_comm)
extern int mpig_vmpi_err_rank;
#define MPIG_VMPI_ERR_RANK (*(const int *) &mpig_vmpi_err_rank)
extern int mpig_vmpi_err_root;
#define MPIG_VMPI_ERR_ROOT (*(const int *) &mpig_vmpi_err_root)
extern int mpig_vmpi_err_truncate;
#define MPIG_VMPI_ERR_TRUNCATE (*(const int *) &mpig_vmpi_err_truncate)
extern int mpig_vmpi_err_group;
#define MPIG_VMPI_ERR_GROUP (*(const int *) &mpig_vmpi_err_group)
extern int mpig_vmpi_err_op;
#define MPIG_VMPI_ERR_OP (*(const int *) &mpig_vmpi_err_op)
extern int mpig_vmpi_err_request;
#define MPIG_VMPI_ERR_REQUEST (*(const int *) &mpig_vmpi_err_request)
extern int mpig_vmpi_err_topology;
#define MPIG_VMPI_ERR_TOPOLOGY (*(const int *) &mpig_vmpi_err_topology)
extern int mpig_vmpi_err_dims;
#define MPIG_VMPI_ERR_DIMS (*(const int *) &mpig_vmpi_err_dims)
extern int mpig_vmpi_err_arg;
#define MPIG_VMPI_ERR_ARG (*(const int *) &mpig_vmpi_err_arg)
extern int mpig_vmpi_err_other;
#define MPIG_VMPI_ERR_OTHER (*(const int *) &mpig_vmpi_err_other)
extern int mpig_vmpi_err_unknown;
#define MPIG_VMPI_ERR_UNKNOWN (*(const int *) &mpig_vmpi_err_unknown)
extern int mpig_vmpi_err_intern;
#define MPIG_VMPI_ERR_INTERN (*(const int *) &mpig_vmpi_err_intern)
extern int mpig_vmpi_err_in_status;
#define MPIG_VMPI_ERR_IN_STATUS (*(const int *) &mpig_vmpi_err_in_status)
extern int mpig_vmpi_err_pending;
#define MPIG_VMPI_ERR_PENDING (*(const int *) &mpig_vmpi_err_pending)
extern int mpig_vmpi_err_file;
#define MPIG_VMPI_ERR_FILE (*(const int *) &mpig_vmpi_err_file)
extern int mpig_vmpi_err_access;
#define MPIG_VMPI_ERR_ACCESS (*(const int *) &mpig_vmpi_err_access)
extern int mpig_vmpi_err_amode;
#define MPIG_VMPI_ERR_AMODE (*(const int *) &mpig_vmpi_err_amode)
extern int mpig_vmpi_err_bad_file;
#define MPIG_VMPI_ERR_BAD_FILE (*(const int *) &mpig_vmpi_err_bad_file)
extern int mpig_vmpi_err_file_exists;
#define MPIG_VMPI_ERR_FILE_EXISTS (*(const int *) &mpig_vmpi_err_file_exists)
extern int mpig_vmpi_err_file_in_use;
#define MPIG_VMPI_ERR_FILE_IN_USE (*(const int *) &mpig_vmpi_err_file_in_use)
extern int mpig_vmpi_err_no_space;
#define MPIG_VMPI_ERR_NO_SPACE (*(const int *) &mpig_vmpi_err_no_space)
extern int mpig_vmpi_err_no_such_file;
#define MPIG_VMPI_ERR_NO_SUCH_FILE (*(const int *) &mpig_vmpi_err_no_such_file)
extern int mpig_vmpi_err_io;
#define MPIG_VMPI_ERR_IO (*(const int *) &mpig_vmpi_err_io)
extern int mpig_vmpi_err_read_only;
#define MPIG_VMPI_ERR_READ_ONLY (*(const int *) &mpig_vmpi_err_read_only)
extern int mpig_vmpi_err_conversion;
#define MPIG_VMPI_ERR_CONVERSION (*(const int *) &mpig_vmpi_err_conversion)
extern int mpig_vmpi_err_dup_datarep;
#define MPIG_VMPI_ERR_DUP_DATAREP (*(const int *) &mpig_vmpi_err_dup_datarep)
extern int mpig_vmpi_err_unsupported_datarep;
#define MPIG_VMPI_ERR_UNSUPPORTED_DATAREP (*(const int *) &mpig_vmpi_err_unsupported_datarep)
extern int mpig_vmpi_err_info;
#define MPIG_VMPI_ERR_INFO (*(const int *) &mpig_vmpi_err_info)
extern int mpig_vmpi_err_info_key;
#define MPIG_VMPI_ERR_INFO_KEY (*(const int *) &mpig_vmpi_err_info_key)
extern int mpig_vmpi_err_info_value;
#define MPIG_VMPI_ERR_INFO_VALUE (*(const int *) &mpig_vmpi_err_info_value)
extern int mpig_vmpi_err_info_nokey;
#define MPIG_VMPI_ERR_INFO_NOKEY (*(const int *) &mpig_vmpi_err_info_nokey)
extern int mpig_vmpi_err_name;
#define MPIG_VMPI_ERR_NAME (*(const int *) &mpig_vmpi_err_name)
extern int mpig_vmpi_err_no_mem;
#define MPIG_VMPI_ERR_NO_MEM (*(const int *) &mpig_vmpi_err_no_mem)
extern int mpig_vmpi_err_not_same;
#define MPIG_VMPI_ERR_NOT_SAME (*(const int *) &mpig_vmpi_err_not_same)
extern int mpig_vmpi_err_port;
#define MPIG_VMPI_ERR_PORT (*(const int *) &mpig_vmpi_err_port)
extern int mpig_vmpi_err_quota;
#define MPIG_VMPI_ERR_QUOTA (*(const int *) &mpig_vmpi_err_quota)
extern int mpig_vmpi_err_service;
#define MPIG_VMPI_ERR_SERVICE (*(const int *) &mpig_vmpi_err_service)
extern int mpig_vmpi_err_spawn;
#define MPIG_VMPI_ERR_SPAWN (*(const int *) &mpig_vmpi_err_spawn)
extern int mpig_vmpi_err_unsupported_operation;
#define MPIG_VMPI_ERR_UNSUPPORTED_OPERATION (*(const int *) &mpig_vmpi_err_unsupported_operation)
extern int mpig_vmpi_err_win;
#define MPIG_VMPI_ERR_WIN (*(const int *) &mpig_vmpi_err_win)
extern int mpig_vmpi_err_base;
#define MPIG_VMPI_ERR_BASE (*(const int *) &mpig_vmpi_err_base)
extern int mpig_vmpi_err_locktype;
#define MPIG_VMPI_ERR_LOCKTYPE (*(const int *) &mpig_vmpi_err_locktype)
extern int mpig_vmpi_err_keyval;
#define MPIG_VMPI_ERR_KEYVAL (*(const int *) &mpig_vmpi_err_keyval)
extern int mpig_vmpi_err_rma_conflict;
#define MPIG_VMPI_ERR_RMA_CONFLICT (*(const int *) &mpig_vmpi_err_rma_conflict)
extern int mpig_vmpi_err_rma_sync;
#define MPIG_VMPI_ERR_RMA_SYNC (*(const int *) &mpig_vmpi_err_rma_sync)
extern int mpig_vmpi_err_size;
#define MPIG_VMPI_ERR_SIZE (*(const int *) &mpig_vmpi_err_size)
extern int mpig_vmpi_err_disp;
#define MPIG_VMPI_ERR_DISP (*(const int *) &mpig_vmpi_err_disp)
extern int mpig_vmpi_err_assert;
#define MPIG_VMPI_ERR_ASSERT (*(const int *) &mpig_vmpi_err_assert)

/*
 * translation functions used to access vendor MPI routines and data structures
 */
int mpig_vmpi_init(int * argc, char *** argv);

int mpig_vmpi_finalize(void);

int mpig_vmpi_abort(mpig_vmpi_comm_t comm, int exit_code);

/* point-to-point functions */
int mpig_vmpi_send(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm);

int mpig_vmpi_isend(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request);

int mpig_vmpi_rsend(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm);

int mpig_vmpi_irsend(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request);

int mpig_vmpi_ssend(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm);

int mpig_vmpi_issend(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int dest, int tag, mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request);

int mpig_vmpi_probe(int src, int tag, mpig_vmpi_comm_t comm, mpig_vmpi_status_t * status);

int mpig_vmpi_iprobe(int src, int tag, mpig_vmpi_comm_t comm, int * flag, mpig_vmpi_status_t * status);

int mpig_vmpi_recv(void * buf, int cnt, mpig_vmpi_datatype_t dt, int src, int tag, mpig_vmpi_comm_t comm,
    mpig_vmpi_status_t * status);

int mpig_vmpi_irecv(void * buf, int cnt, mpig_vmpi_datatype_t dt, int src, int tag, mpig_vmpi_comm_t comm,
    mpig_vmpi_request_t * request);

int mpig_vmpi_cancel(mpig_vmpi_request_t * request);

int mpig_vmpi_request_free(mpig_vmpi_request_t * request);

int mpig_vmpi_test(mpig_vmpi_request_t * request, int * flag, mpig_vmpi_status_t * status);

int mpig_vmpi_wait(mpig_vmpi_request_t * request, mpig_vmpi_status_t * status);

int mpig_vmpi_testany(int incount, mpig_vmpi_request_t * requests, int * index, int * flag, mpig_vmpi_status_t * status);

int mpig_vmpi_waitany(int incount, mpig_vmpi_request_t * requests, int * index, mpig_vmpi_status_t * status);

int mpig_vmpi_testsome(int incount, mpig_vmpi_request_t * requests, int * outcount, int * indicies, mpig_vmpi_status_t * statuses);

int mpig_vmpi_waitsome(int incount, mpig_vmpi_request_t * requests, int * outcount, int * indicies, mpig_vmpi_status_t * statuses);

int mpig_vmpi_test_cancelled(mpig_vmpi_status_t * status, int * flag);

/* collective communication functions */
int mpig_vmpi_barrier(mpig_vmpi_comm_t comm);

int mpig_vmpi_bcast(const void * buf, int cnt, mpig_vmpi_datatype_t dt, int root, mpig_vmpi_comm_t comm);

int mpig_vmpi_gather(const void * send_buf, int send_cnt, mpig_vmpi_datatype_t send_dt, void * recv_buf, int recv_cnt,
    mpig_vmpi_datatype_t recv_dt, int root, mpig_vmpi_comm_t comm);

int mpig_vmpi_gatherv(const void * send_buf, int send_cnt, mpig_vmpi_datatype_t send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, mpig_vmpi_datatype_t recv_dt, int root, mpig_vmpi_comm_t comm);

int mpig_vmpi_allgather(const void * send_buf, int send_cnt, mpig_vmpi_datatype_t send_dt, void * recv_buf, int recv_cnt,
    mpig_vmpi_datatype_t recv_dt, mpig_vmpi_comm_t comm);

int mpig_vmpi_allgatherv(const void * send_buf, int send_cnt, mpig_vmpi_datatype_t send_dt, void * recv_buf,
    const int * recv_cnts, const int * recv_displs, mpig_vmpi_datatype_t recv_dt, mpig_vmpi_comm_t comm);

int mpig_vmpi_allreduce(const void * send_buf, void * recv_buf, int cnt, mpig_vmpi_datatype_t dt,
    mpig_vmpi_op_t op, mpig_vmpi_comm_t comm);

/* communicator functions */
int mpig_vmpi_comm_size(mpig_vmpi_comm_t comm, int * size_p);

int mpig_vmpi_comm_rank(mpig_vmpi_comm_t comm, int * rank_p);

int mpig_vmpi_comm_dup(mpig_vmpi_comm_t old_comm, mpig_vmpi_comm_t * new_comm_p);

int mpig_vmpi_comm_split(mpig_vmpi_comm_t old_comm, int color, int key, mpig_vmpi_comm_t * new_comm_p);

int mpig_vmpi_intercomm_create(mpig_vmpi_comm_t local_comm, int local_leader, mpig_vmpi_comm_t peer_comm,
    int remote_leader, int tag, mpig_vmpi_comm_t * new_intercomm_p);

int mpig_vmpi_intercomm_merge(mpig_vmpi_comm_t intercomm, int high, mpig_vmpi_comm_t * new_intracomm_p);

int mpig_vmpi_comm_free(mpig_vmpi_comm_t * comm_p);

int mpig_vmpi_comm_get_attr(mpig_vmpi_comm_t comm, int key, void * val_p, int * flag);

/* datatype functions */
int mpig_vmpi_type_contiguous(int cnt, mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_vector(int cnt, int blklen, int  stride, mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_create_hvector(int cnt, int blklen, mpig_vmpi_aint_t stride, mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_indexed(int cnt, int * blklens, int * displs, mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_create_hindexed(int cnt, int * blklens, mpig_vmpi_aint_t * displs, mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * new_dt_p);

#if defined(HAVE_VMPI_TYPE_CREATE_INDEXED_BLOCK)
int mpig_vmpi_type_create_indexed_block(int cnt, int blklen, int * displs, mpig_vmpi_datatype_t old_dt,
    mpig_vmpi_datatype_t * new_dt_p);
#endif

int mpig_vmpi_type_create_struct(int cnt, int * blklens, mpig_vmpi_aint_t * displs, mpig_vmpi_datatype_t * old_dts,
    mpig_vmpi_datatype_t * new_dt_p);

int mpig_vmpi_type_dup(mpig_vmpi_datatype_t old_dt, mpig_vmpi_datatype_t * new_dt_p);

#if defined(HAVE_VMPI_TYPE_CREATE_RESIZED)
int mpig_vmpi_type_create_resized(mpig_vmpi_datatype_t old_dt, mpig_vmpi_aint_t lb, mpig_vmpi_aint_t extent,
    mpig_vmpi_datatype_t * new_dt_p);
#endif

int mpig_vmpi_type_lb(mpig_vmpi_datatype_t dt, mpig_vmpi_aint_t * displ);

int mpig_vmpi_type_ub(mpig_vmpi_datatype_t dt, mpig_vmpi_aint_t * displ);

int mpig_vmpi_type_commit(mpig_vmpi_datatype_t * dt_p);

int mpig_vmpi_type_free(mpig_vmpi_datatype_t * dt_p);

int mpig_vmpi_get_count(mpig_vmpi_status_t * status, mpig_vmpi_datatype_t dt, int * cnt_p);

/* error extraction and conversion functions */
int mpig_vmpi_error_class(int vendor_errno, int * vendor_class_p);

int mpig_vmpi_error_string(int vendor_errno, char * string, int * result_length);

/*
 * utility functions that are not defined in the MPI standard but are needed to transition between MPICH2 and the vendor MPI
 */
void mpig_vmpi_status_set_source(mpig_vmpi_status_t * status, int source);

int mpig_vmpi_status_get_source(const mpig_vmpi_status_t * status);

void mpig_vmpi_status_set_tag(mpig_vmpi_status_t * status, int tag);

int mpig_vmpi_status_get_tag(const mpig_vmpi_status_t * status);

void mpig_vmpi_status_set_error(mpig_vmpi_status_t * status, int error_code);

int mpig_vmpi_status_get_error(const mpig_vmpi_status_t * status);

int mpig_vmpi_comm_is_null(const mpig_vmpi_comm_t comm);

int mpig_vmpi_datatype_is_null(const mpig_vmpi_datatype_t dt);

int mpig_vmpi_op_is_null(const mpig_vmpi_op_t op);

int mpig_vmpi_request_is_null(const mpig_vmpi_request_t req);

#if defined(TYPEOF_VMPI_COMM_IS_BASIC)
#define mpig_vmpi_comm_is_null(comm_) ((comm_) == MPIG_VMPI_COMM_NULL)
#endif
#if defined(TYPEOF_VMPI_DATATYPE_IS_BASIC)
#define mpig_vmpi_datatype_is_null(dt_) ((dt_) == MPIG_VMPI_DATATYPE_NULL)
#endif
#if defined(TYPEOF_VMPI_OP_IS_BASIC)
#define mpig_vmpi_op_is_null(op_) ((op_) == MPIG_VMPI_OP_NULL)
#endif
#if defined(TYPEOF_VMPI_REQUEST_IS_BASIC)
#define mpig_vmpi_request_is_null(req_) ((req_) == MPIG_VMPI_REQUEST_NULL)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* !defined(MPICH2_MPIG_VMPI_H_INCLUDED) */
