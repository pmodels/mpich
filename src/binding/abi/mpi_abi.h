#ifndef MPI_H_ABI
#define MPI_H_ABI

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define MPI_VERSION    4
#define MPI_SUBVERSION 2

typedef intptr_t MPI_Aint;
typedef int64_t  MPI_Offset;
typedef int64_t  MPI_Count;

typedef int MPI_Fint;

typedef struct {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
    int internal[5];
} MPI_Status;

typedef struct MPI_ABI_Op * MPI_Op;
#define MPI_OP_NULL                    (MPI_Op)0x00000020
#define MPI_SUM                        (MPI_Op)0x00000021
#define MPI_MIN                        (MPI_Op)0x00000022
#define MPI_MAX                        (MPI_Op)0x00000023
#define MPI_PROD                       (MPI_Op)0x00000024
#define MPI_BAND                       (MPI_Op)0x00000028
#define MPI_BOR                        (MPI_Op)0x00000029
#define MPI_BXOR                       (MPI_Op)0x0000002a
#define MPI_LAND                       (MPI_Op)0x00000030
#define MPI_LOR                        (MPI_Op)0x00000031
#define MPI_LXOR                       (MPI_Op)0x00000032
#define MPI_MINLOC                     (MPI_Op)0x00000038
#define MPI_MAXLOC                     (MPI_Op)0x00000039
#define MPI_REPLACE                    (MPI_Op)0x0000003c
#define MPI_NO_OP                      (MPI_Op)0x0000003d

typedef struct MPI_ABI_Comm * MPI_Comm;
#define MPI_COMM_NULL                  (MPI_Comm)0x00000100
#define MPI_COMM_WORLD                 (MPI_Comm)0x00000101
#define MPI_COMM_SELF                  (MPI_Comm)0x00000102

typedef struct MPI_ABI_Group * MPI_Group;
#define MPI_GROUP_NULL                 (MPI_Group)0x00000108
#define MPI_GROUP_EMPTY                (MPI_Group)0x00000109

typedef struct MPI_ABI_Win * MPI_Win;
#define MPI_WIN_NULL                   (MPI_Win)0x00000110

typedef struct MPI_ABI_File * MPI_File;
#define MPI_FILE_NULL                  (MPI_File)0x00000118

typedef struct MPI_ABI_Session * MPI_Session;
#define MPI_SESSION_NULL               (MPI_Session)0x00000120

typedef struct MPI_ABI_Message * MPI_Message;
#define MPI_MESSAGE_NULL               (MPI_Message)0x00000128
#define MPI_MESSAGE_NO_PROC            (MPI_Message)0x00000129

typedef struct MPI_ABI_Info * MPI_Info;
#define MPI_INFO_NULL                  (MPI_Info)0x00000130
#define MPI_INFO_ENV                   (MPI_Info)0x00000131

typedef struct MPI_ABI_Errhandler * MPI_Errhandler;
#define MPI_ERRHANDLER_NULL            (MPI_Errhandler)0x00000140
#define MPI_ERRORS_ARE_FATAL           (MPI_Errhandler)0x00000141
#define MPI_ERRORS_RETURN              (MPI_Errhandler)0x00000142
#define MPI_ERRORS_ABORT               (MPI_Errhandler)0x00000143

typedef struct MPI_ABI_Request * MPI_Request;
#define MPI_REQUEST_NULL               (MPI_Request)0x00000180

typedef struct MPI_ABI_Datatype * MPI_Datatype;
#define MPI_DATATYPE_NULL              (MPI_Datatype)0x00000200
#define MPI_AINT                       (MPI_Datatype)0x00000201
#define MPI_COUNT                      (MPI_Datatype)0x00000202
#define MPI_OFFSET                     (MPI_Datatype)0x00000203
#define MPI_PACKED                     (MPI_Datatype)0x00000207
#define MPI_SHORT                      (MPI_Datatype)0x00000208
#define MPI_INT                        (MPI_Datatype)0x00000209
#define MPI_LONG                       (MPI_Datatype)0x0000020a
#define MPI_LONG_LONG                  (MPI_Datatype)0x0000020b
#define MPI_LONG_LONG_INT              MPI_LONG_LONG
#define MPI_UNSIGNED_SHORT             (MPI_Datatype)0x0000020c
#define MPI_UNSIGNED                   (MPI_Datatype)0x0000020d
#define MPI_UNSIGNED_LONG              (MPI_Datatype)0x0000020e
#define MPI_UNSIGNED_LONG_LONG         (MPI_Datatype)0x0000020f
#define MPI_FLOAT                      (MPI_Datatype)0x00000210
#define MPI_C_FLOAT_COMPLEX            (MPI_Datatype)0x00000212
#define MPI_C_COMPLEX                  MPI_C_FLOAT_COMPLEX
#define MPI_CXX_FLOAT_COMPLEX          (MPI_Datatype)0x00000213
#define MPI_DOUBLE                     (MPI_Datatype)0x00000214
#define MPI_C_DOUBLE_COMPLEX           (MPI_Datatype)0x00000216
#define MPI_CXX_DOUBLE_COMPLEX         (MPI_Datatype)0x00000217
#define MPI_LOGICAL                    (MPI_Datatype)0x00000218
#define MPI_INTEGER                    (MPI_Datatype)0x00000219
#define MPI_REAL                       (MPI_Datatype)0x0000021a
#define MPI_COMPLEX                    (MPI_Datatype)0x0000021b
#define MPI_DOUBLE_PRECISION           (MPI_Datatype)0x0000021c
#define MPI_DOUBLE_COMPLEX             (MPI_Datatype)0x0000021d
#define MPI_LONG_DOUBLE                (MPI_Datatype)0x00000220
#define MPI_C_LONG_DOUBLE_COMPLEX      (MPI_Datatype)0x00000224
#define MPI_CXX_LONG_DOUBLE_COMPLEX    (MPI_Datatype)0x00000225
#define MPI_FLOAT_INT                  (MPI_Datatype)0x00000228
#define MPI_DOUBLE_INT                 (MPI_Datatype)0x00000229
#define MPI_LONG_INT                   (MPI_Datatype)0x0000022a
#define MPI_2INT                       (MPI_Datatype)0x0000022b
#define MPI_SHORT_INT                  (MPI_Datatype)0x0000022c
#define MPI_LONG_DOUBLE_INT            (MPI_Datatype)0x0000022d
#define MPI_2REAL                      (MPI_Datatype)0x00000230
#define MPI_2DOUBLE_PRECISION          (MPI_Datatype)0x00000231
#define MPI_2INTEGER                   (MPI_Datatype)0x00000232
#define MPI_C_BOOL                     (MPI_Datatype)0x00000238
#define MPI_CXX_BOOL                   (MPI_Datatype)0x00000239
#define MPI_WCHAR                      (MPI_Datatype)0x0000023c
#define MPI_INT8_T                     (MPI_Datatype)0x00000240
#define MPI_UINT8_T                    (MPI_Datatype)0x00000241
#define MPI_CHAR                       (MPI_Datatype)0x00000243
#define MPI_SIGNED_CHAR                (MPI_Datatype)0x00000244
#define MPI_UNSIGNED_CHAR              (MPI_Datatype)0x00000245
#define MPI_BYTE                       (MPI_Datatype)0x00000247
#define MPI_INT16_T                    (MPI_Datatype)0x00000248
#define MPI_UINT16_T                   (MPI_Datatype)0x00000249
#define MPI_INT32_T                    (MPI_Datatype)0x00000250
#define MPI_UINT32_T                   (MPI_Datatype)0x00000251
#define MPI_INT64_T                    (MPI_Datatype)0x00000258
#define MPI_UINT64_T                   (MPI_Datatype)0x00000259
#define MPIX_LOGICAL1                  (MPI_Datatype)0x000002c0
#define MPI_INTEGER1                   (MPI_Datatype)0x000002c1
#define MPIX_REAL1                     (MPI_Datatype)0x000002c2
#define MPI_CHARACTER                  (MPI_Datatype)0x000002c3
#define MPIX_LOGICAL2                  (MPI_Datatype)0x000002c8
#define MPI_INTEGER2                   (MPI_Datatype)0x000002c9
#define MPI_REAL2                      (MPI_Datatype)0x000002ca
#define MPIX_LOGICAL4                  (MPI_Datatype)0x000002d0
#define MPI_INTEGER4                   (MPI_Datatype)0x000002d1
#define MPI_REAL4                      (MPI_Datatype)0x000002d2
#define MPI_COMPLEX4                   (MPI_Datatype)0x000002d3
#define MPIX_LOGICAL8                  (MPI_Datatype)0x000002d8
#define MPI_INTEGER8                   (MPI_Datatype)0x000002d9
#define MPI_REAL8                      (MPI_Datatype)0x000002da
#define MPI_COMPLEX8                   (MPI_Datatype)0x000002db
#define MPIX_LOGICAL16                 (MPI_Datatype)0x000002e0
#define MPI_INTEGER16                  (MPI_Datatype)0x000002e1
#define MPI_REAL16                     (MPI_Datatype)0x000002e2
#define MPI_COMPLEX16                  (MPI_Datatype)0x000002e3
#define MPI_COMPLEX32                  (MPI_Datatype)0x000002eb

enum {
    // Status indexing - must match MPI_Status definition
    MPI_F_SOURCE        = 0,
    MPI_F_TAG           = 1,
    MPI_F_ERROR         = 2,
    // Fortran status array size and reserved index values (in C)
    MPI_F_STATUS_SIZE   = 8
};

// Error classes
enum {
    MPI_SUCCESS                         = 0,
    MPI_ERR_BUFFER                      = 1,
    MPI_ERR_COUNT                       = 2,
    MPI_ERR_TYPE                        = 3,
    MPI_ERR_TAG                         = 4,
    MPI_ERR_COMM                        = 5,
    MPI_ERR_RANK                        = 6,
    MPI_ERR_REQUEST                     = 7,
    MPI_ERR_ROOT                        = 8,
    MPI_ERR_GROUP                       = 9,
    MPI_ERR_OP                          = 10,
    MPI_ERR_TOPOLOGY                    = 11,
    MPI_ERR_DIMS                        = 12,
    MPI_ERR_ARG                         = 13,
    MPI_ERR_UNKNOWN                     = 14,
    MPI_ERR_TRUNCATE                    = 15,
    MPI_ERR_OTHER                       = 16,
    MPI_ERR_INTERN                      = 17,
    MPI_ERR_PENDING                     = 18,
    MPI_ERR_IN_STATUS                   = 19,
    MPI_ERR_ACCESS                      = 20,
    MPI_ERR_AMODE                       = 21,
    MPI_ERR_ASSERT                      = 22,
    MPI_ERR_BAD_FILE                    = 23,
    MPI_ERR_BASE                        = 24,
    MPI_ERR_CONVERSION                  = 25,
    MPI_ERR_DISP                        = 26,
    MPI_ERR_DUP_DATAREP                 = 27,
    MPI_ERR_FILE_EXISTS                 = 28,
    MPI_ERR_FILE_IN_USE                 = 29,
    MPI_ERR_FILE                        = 30,
    MPI_ERR_INFO_KEY                    = 31,
    MPI_ERR_INFO_NOKEY                  = 32,
    MPI_ERR_INFO_VALUE                  = 33,
    MPI_ERR_INFO                        = 34,
    MPI_ERR_IO                          = 35,
    MPI_ERR_KEYVAL                      = 36,
    MPI_ERR_LOCKTYPE                    = 37,
    MPI_ERR_NAME                        = 38,
    MPI_ERR_NO_MEM                      = 39,
    MPI_ERR_NOT_SAME                    = 40,
    MPI_ERR_NO_SPACE                    = 41,
    MPI_ERR_NO_SUCH_FILE                = 42,
    MPI_ERR_PORT                        = 43,
    MPI_ERR_PROC_ABORTED                = 44,
    MPI_ERR_QUOTA                       = 45,
    MPI_ERR_READ_ONLY                   = 46,
    MPI_ERR_RMA_ATTACH                  = 47,
    MPI_ERR_RMA_CONFLICT                = 48,
    MPI_ERR_RMA_RANGE                   = 49,
    MPI_ERR_RMA_SHARED                  = 50,
    MPI_ERR_RMA_SYNC                    = 51,
    MPI_ERR_RMA_FLAVOR                  = 52,
    MPI_ERR_SERVICE                     = 53,
    MPI_ERR_SESSION                     = 54,
    MPI_ERR_SIZE                        = 55,
    MPI_ERR_SPAWN                       = 56,
    MPI_ERR_UNSUPPORTED_DATAREP         = 57,
    MPI_ERR_UNSUPPORTED_OPERATION       = 58,
    MPI_ERR_VALUE_TOO_LARGE             = 59,
    MPI_ERR_WIN                         = 60,
    MPI_ERR_ERRHANDLER                  = 61,
    MPI_T_ERR_CANNOT_INIT               = 1000,
    MPI_T_ERR_NOT_ACCESSIBLE            = 1001,
    MPI_T_ERR_NOT_INITIALIZED           = 1002,
    MPI_T_ERR_NOT_SUPPORTED             = 1003,
    MPI_T_ERR_MEMORY                    = 1004,
    MPI_T_ERR_INVALID                   = 1005,
    MPI_T_ERR_INVALID_INDEX             = 1006,
    MPI_T_ERR_INVALID_ITEM              = 1007,
    MPI_T_ERR_INVALID_SESSION           = 1008,
    MPI_T_ERR_INVALID_HANDLE            = 1009,
    MPI_T_ERR_INVALID_NAME              = 1010,
    MPI_T_ERR_OUT_OF_HANDLES            = 1011,
    MPI_T_ERR_OUT_OF_SESSIONS           = 1012,
    MPI_T_ERR_CVAR_SET_NOT_NOW          = 1013,
    MPI_T_ERR_CVAR_SET_NEVER            = 1014,
    MPI_T_ERR_PVAR_NO_WRITE             = 1015,
    MPI_T_ERR_PVAR_NO_STARTSTOP         = 1016,
    MPI_T_ERR_PVAR_NO_ATOMIC            = 1017,
    MPI_ERR_LASTCODE                    = 0x3fff // half of the minimum required value of INT_MAX
};

// Buffer Address Constants
#define MPI_BOTTOM           ((void*)0)
#define MPI_IN_PLACE         ((void*)1)
#define MPI_BUFFER_AUTOMATIC ((void*)2)

// Constants Specifying Empty or Ignored Input
#define MPI_ARGV_NULL       ((char**)0)
#define MPI_ARGVS_NULL      ((char***)0)
#define MPI_ERRCODES_IGNORE ((int*)0)
#define MPI_STATUS_IGNORE   ((MPI_Status*)0)
#define MPI_STATUSES_IGNORE ((MPI_Status*)0)
#define MPI_UNWEIGHTED      ((int*)4)
#define MPI_WEIGHTS_EMPTY   ((int*)5)

// Other constants
#define MPI_BSEND_OVERHEAD                   512 // MPICH=96, OMPI=128

// String size constants
#define MPI_MAX_DATAREP_STRING               128 // MPICH=OMPI=128 (MPICH has it in `mpio.h`)
#define MPI_MAX_ERROR_STRING                 512 // MPICH was bigger
#define MPI_MAX_INFO_KEY                     256 // MPICH was bigger
#define MPI_MAX_INFO_VAL                    1024 // MPICH was bigger
#define MPI_MAX_LIBRARY_VERSION_STRING      8192 // MPICH was bigger
#define MPI_MAX_OBJECT_NAME                  128 // MPICH was bigger
#define MPI_MAX_PORT_NAME                   1024 // OMPI was bigger
#define MPI_MAX_PROCESSOR_NAME               256 // OMPI was bigger
#define MPI_MAX_STRINGTAG_LEN               1024 // OMPI was bigger (v5.0+)
#define MPI_MAX_PSET_NAME_LEN                512 // OMPI was bigger (v5.0+)

// Mode Constants
// must be powers-of-2 to support OR-ing
enum {
    // Files
    MPI_MODE_APPEND             = 1,
    MPI_MODE_CREATE             = 2,
    MPI_MODE_DELETE_ON_CLOSE    = 4,
    MPI_MODE_EXCL               = 8,
    MPI_MODE_RDONLY             = 16,
    MPI_MODE_RDWR               = 32,
    MPI_MODE_SEQUENTIAL         = 64,
    MPI_MODE_UNIQUE_OPEN        = 128,
    MPI_MODE_WRONLY             = 256,
    // Windows
    MPI_MODE_NOCHECK            = 1024,
    MPI_MODE_NOPRECEDE          = 2048,
    MPI_MODE_NOPUT              = 4096,
    MPI_MODE_NOSTORE            = 8192,
    MPI_MODE_NOSUCCEED          = 16384
};

enum {
    // rank sentinels - must be negative
    MPI_ANY_SOURCE      = -1,
    MPI_PROC_NULL       = -2,
    MPI_ROOT            = -3,

    // tag sentinels - should be negative
    MPI_ANY_TAG         = -31,

    // attribute constant - should be negative
    MPI_KEYVAL_INVALID  = -127,

    // special displacement for sequential access file - should be negative
    MPI_DISPLACEMENT_CURRENT = -255,

    // multi-purpose sentinel - must be negative
    MPI_UNDEFINED       = -32766 // make it match OMPI and MPICH
};

enum {
    // Environmental inquiry keys and Predefined Attribute Keys
    // Threads Constants
    // These values are monotonic; i.e., SINGLE < FUNNELED < SERIALIZED < MULTIPLE.
    MPI_THREAD_SINGLE     = 0, 
    MPI_THREAD_FUNNELED   = 1,
    MPI_THREAD_SERIALIZED = 2,
    MPI_THREAD_MULTIPLE   = 7, // in case we need other threading levels below MULTIPLE

    // Array Datatype Order
    MPI_ORDER_C       = 0xC, // 12
    MPI_ORDER_FORTRAN = 0xF, // 15

    // Array Datatype Distribution
    MPI_DISTRIBUTE_NONE       = 16,
    MPI_DISTRIBUTE_BLOCK      = 17,
    MPI_DISTRIBUTE_CYCLIC     = 18,
    MPI_DISTRIBUTE_DFLT_DARG  = 19,

    // RMA Lock Constants - arbitrary values
    MPI_LOCK_SHARED    = 21,
    MPI_LOCK_EXCLUSIVE = 22,

    // MPI Window Models
    MPI_WIN_UNIFIED  = 31,
    MPI_WIN_SEPARATE = 32,

    // MPI Window Create Flavors
    MPI_WIN_FLAVOR_ALLOCATE = 41,
    MPI_WIN_FLAVOR_CREATE   = 42,
    MPI_WIN_FLAVOR_DYNAMIC  = 43,
    MPI_WIN_FLAVOR_SHARED   = 44,

    // Results of communicator and group comparisons
    MPI_IDENT       = 101,
    MPI_CONGRUENT   = 102,
    MPI_SIMILAR     = 103,
    MPI_UNEQUAL     = 104,

    // MPI_Topo_test
    MPI_GRAPH      = 201,
    MPI_DIST_GRAPH = 202,
    MPI_CART       = 203,

    // Datatype Decoding Constants
    MPI_COMBINER_NAMED              = 301,
    MPI_COMBINER_DUP                = 302,
    MPI_COMBINER_CONTIGUOUS         = 303,
    MPI_COMBINER_VECTOR             = 304,
    MPI_COMBINER_HVECTOR            = 305,
    MPI_COMBINER_INDEXED            = 306,
    MPI_COMBINER_HINDEXED           = 307,
    MPI_COMBINER_INDEXED_BLOCK      = 308,
    MPI_COMBINER_HINDEXED_BLOCK     = 309,
    MPI_COMBINER_STRUCT             = 310,
    MPI_COMBINER_SUBARRAY           = 311,
    MPI_COMBINER_DARRAY             = 312,
    MPI_COMBINER_F90_REAL           = 313,
    MPI_COMBINER_F90_COMPLEX        = 314,
    MPI_COMBINER_F90_INTEGER        = 315,
    MPI_COMBINER_RESIZED            = 316,
    MPI_COMBINER_VALUE_INDEX        = 317,

    // File Position Constants
    MPI_SEEK_CUR                = 601,
    MPI_SEEK_END                = 602,
    MPI_SEEK_SET                = 603,

    // Fortran Datatype Matching Constants
    MPIX_TYPECLASS_LOGICAL      = 801,
    MPI_TYPECLASS_INTEGER       = 802,
    MPI_TYPECLASS_REAL          = 803,
    MPI_TYPECLASS_COMPLEX       = 804,

    // Communicator split type constants - arbitrary values
    MPI_COMM_TYPE_SHARED          = 1001,
    MPI_COMM_TYPE_HW_UNGUIDED     = 1002,
    MPI_COMM_TYPE_HW_GUIDED       = 1003,
    MPI_COMM_TYPE_RESOURCE_GUIDED = 1004,

    // These apply to MPI_COMM_WORLD
    MPI_TAG_UB          = 10001,
    MPI_IO              = 10002,
    MPI_HOST            = 10003,
    MPI_WTIME_IS_GLOBAL = 10004,
    MPI_APPNUM          = 10005,
    MPI_LASTUSEDCODE    = 10006,
    MPI_UNIVERSE_SIZE   = 10007,

    // Predefined Attribute Keys
    // These apply to Windows
    MPI_WIN_BASE            = 20001,
    MPI_WIN_DISP_UNIT       = 20002,
    MPI_WIN_SIZE            = 20003,
    MPI_WIN_CREATE_FLAVOR   = 20004,
    MPI_WIN_MODEL           = 20005
};


typedef int (MPI_Copy_function)(MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Delete_function)(MPI_Comm, int, void *, void *);

typedef void (MPI_User_function)(void *, void *, int *, MPI_Datatype *);
typedef void (MPI_User_function_c)(void *, void *, MPI_Count *, MPI_Datatype *);

typedef int (MPI_Grequest_cancel_function)(void *, int);
typedef int (MPI_Grequest_free_function)(void *);
typedef int (MPI_Grequest_query_function)(void *, MPI_Status *);

typedef int (MPI_Datarep_conversion_function)(void *, MPI_Datatype, int, void *, MPI_Offset, void *);
typedef int (MPI_Datarep_extent_function)(MPI_Datatype datatype, MPI_Aint *, void *);
typedef int (MPI_Datarep_conversion_function_c)(void *, MPI_Datatype, MPI_Count, void *, MPI_Offset, void *);

typedef int (MPI_Comm_copy_attr_function)(MPI_Comm, int, void *, void *, void *, int *);
typedef int (MPI_Comm_delete_attr_function)(MPI_Comm, int, void *, void *);
typedef int (MPI_Type_copy_attr_function)(MPI_Datatype, int, void *, void *, void *, int *);
typedef int (MPI_Type_delete_attr_function)(MPI_Datatype, int, void *, void *);
typedef int (MPI_Win_copy_attr_function)(MPI_Win, int, void *, void *, void *, int *);
typedef int (MPI_Win_delete_attr_function)(MPI_Win, int, void *, void *);

typedef void (MPI_Comm_errhandler_function)(MPI_Comm *, int *, ...);
typedef void (MPI_File_errhandler_function)(MPI_File *, int *, ...);
typedef void (MPI_Win_errhandler_function)(MPI_Win *, int *, ...);
typedef void (MPI_Session_errhandler_function)(MPI_Session *, int *, ...);

typedef MPI_Comm_errhandler_function MPI_Comm_errhandler_fn;
typedef MPI_File_errhandler_function MPI_File_errhandler_fn;
typedef MPI_Win_errhandler_function MPI_Win_errhandler_fn;
typedef MPI_Session_errhandler_function MPI_Session_errhandler_fn;

#define MPI_NULL_COPY_FN         ((MPI_Copy_function*)0x0)
#define MPI_DUP_FN               ((MPI_Copy_function*)0x1)
#define MPI_NULL_DELETE_FN       ((MPI_Delete_function*)0x0)
#define MPI_COMM_NULL_COPY_FN    ((MPI_Comm_copy_attr_function*)0x0)
#define MPI_COMM_DUP_FN          ((MPI_Comm_copy_attr_function*)0x1)
#define MPI_COMM_NULL_DELETE_FN  ((MPI_Comm_delete_attr_function*)0x0)
#define MPI_TYPE_NULL_COPY_FN    ((MPI_Type_copy_attr_function*)0x0)
#define MPI_TYPE_DUP_FN          ((MPI_Type_copy_attr_function*)0x1)
#define MPI_TYPE_NULL_DELETE_FN  ((MPI_Type_delete_attr_function*)0x0)
#define MPI_WIN_NULL_COPY_FN     ((MPI_Win_copy_attr_function*)0x0)
#define MPI_WIN_DUP_FN           ((MPI_Win_copy_attr_function*)0x1)
#define MPI_WIN_NULL_DELETE_FN   ((MPI_Win_delete_attr_function*)0x0)
#define MPI_CONVERSION_FN_NULL   ((MPI_Datarep_conversion_function*)0x0)
#define MPI_CONVERSION_FN_NULL_C ((MPI_Datarep_conversion_function_c*)0x0)

/* MPI_T */

typedef struct MPI_T_enum_t * MPI_T_enum;
typedef struct MPI_T_cvar_handle_t * MPI_T_cvar_handle;
typedef struct MPI_T_pvar_handle_t * MPI_T_pvar_handle;
typedef struct MPI_T_pvar_session_t * MPI_T_pvar_session;
typedef struct MPI_T_event_registration_t * MPI_T_event_registration;
typedef struct MPI_T_event_instance_t * MPI_T_event_instance;

typedef enum  MPI_T_cb_safety {
    MPI_T_CB_REQUIRE_NONE,
    MPI_T_CB_REQUIRE_MPI_RESTRICTED,
    MPI_T_CB_REQUIRE_THREAD_SAFE,
    MPI_T_CB_REQUIRE_ASYNC_SIGNAL_SAFE
} MPI_T_cb_safety;

typedef enum MPI_T_source_order {
    MPI_T_SOURCE_ORDERED,
    MPI_T_SOURCE_UNORDERED
} MPI_T_source_order;

#define MPI_T_ENUM_NULL (MPI_T_enum)0
#define MPI_T_CVAR_HANDLE_NULL (MPI_T_cvar_handle)0
#define MPI_T_PVAR_SESSION_NULL (MPI_T_pvar_session)0
#define MPI_T_PVAR_HANDLE_NULL (MPI_T_pvar_handle)0
#define MPI_T_PVAR_ALL_HANDLES (MPI_T_pvar_handle)1

enum {
    MPI_T_VERBOSITY_USER_BASIC,
    MPI_T_VERBOSITY_USER_DETAIL,
    MPI_T_VERBOSITY_USER_ALL,
    MPI_T_VERBOSITY_TUNER_BASIC,
    MPI_T_VERBOSITY_TUNER_DETAIL,
    MPI_T_VERBOSITY_TUNER_ALL,
    MPI_T_VERBOSITY_MPIDEV_BASIC,
    MPI_T_VERBOSITY_MPIDEV_DETAIL,
    MPI_T_VERBOSITY_MPIDEV_ALL
};

enum {
    MPI_T_BIND_NO_OBJECT,
    MPI_T_BIND_MPI_COMM,
    MPI_T_BIND_MPI_DATATYPE,
    MPI_T_BIND_MPI_ERRHANDLER,
    MPI_T_BIND_MPI_FILE,
    MPI_T_BIND_MPI_GROUP,
    MPI_T_BIND_MPI_OP,
    MPI_T_BIND_MPI_REQUEST,
    MPI_T_BIND_MPI_SESSION,
    MPI_T_BIND_MPI_WIN,
    MPI_T_BIND_MPI_MESSAGE,
    MPI_T_BIND_MPI_INFO
};

enum {
    MPI_T_SCOPE_CONSTANT,
    MPI_T_SCOPE_READONLY,
    MPI_T_SCOPE_LOCAL,
    MPI_T_SCOPE_GROUP,
    MPI_T_SCOPE_GROUP_EQ,
    MPI_T_SCOPE_ALL,
    MPI_T_SCOPE_ALL_EQ
};

enum {
    MPI_T_PVAR_CLASS_STATE,
    MPI_T_PVAR_CLASS_LEVEL,
    MPI_T_PVAR_CLASS_SIZE,
    MPI_T_PVAR_CLASS_PERCENTAGE,
    MPI_T_PVAR_CLASS_HIGHWATERMARK,
    MPI_T_PVAR_CLASS_LOWWATERMARK,
    MPI_T_PVAR_CLASS_COUNTER,
    MPI_T_PVAR_CLASS_AGGREGATE,
    MPI_T_PVAR_CLASS_TIMER,
    MPI_T_PVAR_CLASS_GENERIC
};

typedef void (MPI_T_event_cb_function)(MPI_T_event_instance event_instance, MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, void *user_data);
typedef void (MPI_T_event_free_cb_function)(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, void *user_data);
typedef void (MPI_T_event_dropped_cb_function)(MPI_Count count, MPI_T_event_registration event_registration, int source_index, MPI_T_cb_safety cb_safety, void *user_data);

/* MPI global variables */
extern MPI_Fint * MPI_F_STATUS_IGNORE;
extern MPI_Fint * MPI_F_STATUSES_IGNORE;

/* MPI functions */
int MPI_Abort(MPI_Comm comm, int errorcode);
int MPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int MPI_Accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int MPI_Add_error_class(int *errorclass);
int MPI_Add_error_code(int errorclass, int *errorcode);
int MPI_Add_error_string(int errorcode, const char *string);
int MPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Allgather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Allgatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);
int MPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Allreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Allreduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoall_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int MPI_Alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int MPI_Alltoallw_init(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Attr_delete(MPI_Comm comm, int keyval);
int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int MPI_Barrier(MPI_Comm comm);
int MPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int MPI_Bcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm);
int MPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Bcast_init_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Bsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Bsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Buffer_attach(void *buffer, int size);
int MPI_Buffer_attach_c(void *buffer, MPI_Count size);
int MPI_Buffer_detach(void *buffer_addr, int *size);
int MPI_Buffer_detach_c(void *buffer_addr, MPI_Count *size);
int MPI_Buffer_flush(void);
int MPI_Buffer_iflush(MPI_Request *request);
int MPI_Cancel(MPI_Request *request);
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);
int MPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart);
int MPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);
int MPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank);
int MPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank);
int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);
int MPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);
int MPI_Cartdim_get(MPI_Comm comm, int *ndims);
int MPI_Close_port(const char *port_name);
int MPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_attach_buffer(MPI_Comm comm, void *buffer, int size);
int MPI_Comm_attach_buffer_c(MPI_Comm comm, void *buffer, MPI_Count size);
int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
int MPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int MPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Errhandler *errhandler);
int MPI_Comm_create_from_group(MPI_Group group, const char *stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newcomm);
int MPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm);
int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state);
int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);
int MPI_Comm_detach_buffer(MPI_Comm comm, void *buffer_addr, int *size);
int MPI_Comm_detach_buffer_c(MPI_Comm comm, void *buffer_addr, MPI_Count *size);
int MPI_Comm_disconnect(MPI_Comm *comm);
int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int MPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
int MPI_Comm_flush_buffer(MPI_Comm comm);
int MPI_Comm_free(MPI_Comm *comm);
int MPI_Comm_free_keyval(int *comm_keyval);
int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler);
int MPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used);
int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
int MPI_Comm_get_parent(MPI_Comm *parent);
int MPI_Comm_group(MPI_Comm comm, MPI_Group *group);
int MPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);
int MPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request);
int MPI_Comm_iflush_buffer(MPI_Comm comm, MPI_Request *request);
int MPI_Comm_join(int fd, MPI_Comm *intercomm);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);
int MPI_Comm_remote_size(MPI_Comm comm, int *size);
int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
int MPI_Comm_set_info(MPI_Comm comm, MPI_Info info);
int MPI_Comm_set_name(MPI_Comm comm, const char *comm_name);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int MPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int MPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm);
int MPI_Comm_test_inter(MPI_Comm comm, int *flag);
int MPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);
int MPI_Dims_create(int nnodes, int ndims, int dims[]);
int MPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int MPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int MPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[]);
int MPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted);
int MPI_Errhandler_free(MPI_Errhandler *errhandler);
int MPI_Error_class(int errorcode, int *errorclass);
int MPI_Error_string(int errorcode, char *string, int *resultlen);
int MPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Exscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Exscan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Exscan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_F_sync_reg(void *buf);
int MPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);
int MPI_File_call_errhandler(MPI_File fh, int errorcode);
int MPI_File_close(MPI_File *fh);
int MPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler);
int MPI_File_delete(const char *filename, MPI_Info info);
int MPI_File_get_amode(MPI_File fh, int *amode);
int MPI_File_get_atomicity(MPI_File fh, int *flag);
int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset *disp);
int MPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler);
int MPI_File_get_group(MPI_File fh, MPI_Group *group);
int MPI_File_get_info(MPI_File fh, MPI_Info *info_used);
int MPI_File_get_position(MPI_File fh, MPI_Offset *offset);
int MPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset);
int MPI_File_get_size(MPI_File fh, MPI_Offset *size);
int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype, MPI_Aint *extent);
int MPI_File_get_type_extent_c(MPI_File fh, MPI_Datatype datatype, MPI_Count *extent);
int MPI_File_get_view(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep);
int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_all_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_at_all_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_all_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_at_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iwrite_shared_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh);
int MPI_File_preallocate(MPI_File fh, MPI_Offset size);
int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_all_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_all_begin(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
int MPI_File_read_all_begin_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status);
int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_all_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype);
int MPI_File_read_at_all_begin_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
int MPI_File_read_ordered(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_ordered_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_ordered_begin(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
int MPI_File_read_ordered_begin_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
int MPI_File_read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_read_shared_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);
int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence);
int MPI_File_set_atomicity(MPI_File fh, int flag);
int MPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler);
int MPI_File_set_info(MPI_File fh, MPI_Info info);
int MPI_File_set_size(MPI_File fh, MPI_Offset size);
int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info);
int MPI_File_sync(MPI_File fh);
int MPI_File_write(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_all_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_all_begin(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
int MPI_File_write_all_begin_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status *status);
int MPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype);
int MPI_File_write_at_all_begin_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_write_at_all_end(MPI_File fh, const void *buf, MPI_Status *status);
int MPI_File_write_ordered(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_ordered_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_ordered_begin(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
int MPI_File_write_ordered_begin_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype);
int MPI_File_write_ordered_end(MPI_File fh, const void *buf, MPI_Status *status);
int MPI_File_write_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_write_shared_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int MPI_Finalize(void);
int MPI_Finalized(int *flag);
int MPI_Free_mem(void *base);
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Gather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Gatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Get_c(void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int MPI_Get_accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, void *result_addr, MPI_Count result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int MPI_Get_address(const void *location, MPI_Aint *address);
int MPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);
int MPI_Get_count_c(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int MPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count);
int MPI_Get_elements_c(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int MPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int MPI_Get_hw_resource_info(MPI_Info *hw_info);
int MPI_Get_library_version(char *version, int *resultlen);
int MPI_Get_processor_name(char *name, int *resultlen);
int MPI_Get_version(int *version, int *subversion);
int MPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[], int reorder, MPI_Comm *comm_graph);
int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]);
int MPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank);
int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]);
int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
int MPI_Grequest_complete(MPI_Request request);
int MPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request);
int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
int MPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int MPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int MPI_Group_free(MPI_Group *group);
int MPI_Group_from_session_pset(MPI_Session session, const char *pset_name, MPI_Group *newgroup);
int MPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int MPI_Group_rank(MPI_Group group, int *rank);
int MPI_Group_size(MPI_Group group, int *size);
int MPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]);
int MPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int MPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iallreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int MPI_Ialltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int MPI_Ibarrier(MPI_Comm comm, MPI_Request *request);
int MPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ibcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Ibsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iexscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Igather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Igatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status);
int MPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
int MPI_Imrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
int MPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int MPI_Ineighbor_alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int MPI_Info_create(MPI_Info *info);
int MPI_Info_create_env(int argc, char *argv[], MPI_Info *info);
int MPI_Info_delete(MPI_Info info, const char *key);
int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo);
int MPI_Info_free(MPI_Info *info);
int MPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag);
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys);
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key);
int MPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag);
int MPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag);
int MPI_Info_set(MPI_Info info, const char *key, const char *value);
int MPI_Init(int *argc, char ***argv);
int MPI_Init_thread(int *argc, char ***argv, int required, int *provided);
int MPI_Initialized(int *flag);
int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
int MPI_Intercomm_create_from_groups(MPI_Group local_group, int local_leader, MPI_Group remote_group, int remote_leader, const char *stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newintercomm);
int MPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_scatter_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Ireduce_scatter_block_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Irsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Is_thread_main(int *flag);
int MPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatter_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Iscatterv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int MPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int MPI_Isendrecv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int MPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int MPI_Isendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int MPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Issend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
int MPI_Keyval_free(int *keyval);
int MPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name);
int MPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);
int MPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
int MPI_Mrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
int MPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_allgather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_allgatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoall_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int MPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int MPI_Neighbor_alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int MPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Neighbor_alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Op_commutative(MPI_Op op, int *commute);
int MPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);
int MPI_Op_create_c(MPI_User_function_c *user_fn, int commute, MPI_Op *op);
int MPI_Op_free(MPI_Op *op);
int MPI_Open_port(MPI_Info info, char *port_name);
int MPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm);
int MPI_Pack_c(const void *inbuf, MPI_Count incount, MPI_Datatype datatype, void *outbuf, MPI_Count outsize, MPI_Count *position, MPI_Comm comm);
int MPI_Pack_external(const char *datarep, const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);
int MPI_Pack_external_c(const char *datarep, const void *inbuf, MPI_Count incount, MPI_Datatype datatype, void *outbuf, MPI_Count outsize, MPI_Count *position);
int MPI_Pack_external_size(const char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size);
int MPI_Pack_external_size_c(const char *datarep, MPI_Count incount, MPI_Datatype datatype, MPI_Count *size);
int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);
int MPI_Pack_size_c(MPI_Count incount, MPI_Datatype datatype, MPI_Comm comm, MPI_Count *size);
int MPI_Parrived(MPI_Request request, int partition, int *flag);
int MPI_Pcontrol(const int level, ...);
int MPI_Pready(int partition, MPI_Request request);
int MPI_Pready_list(int length, const int array_of_partitions[], MPI_Request request);
int MPI_Pready_range(int partition_low, int partition_high, MPI_Request request);
int MPI_Precv_init(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Psend_init(const void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name);
int MPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Put_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win);
int MPI_Query_thread(int *provided);
int MPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int MPI_Raccumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Recv_init_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int MPI_Reduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int MPI_Reduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Reduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);
int MPI_Reduce_local_c(const void *inbuf, void *inoutbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op);
int MPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Reduce_scatter_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Reduce_scatter_block_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Reduce_scatter_block_init_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Reduce_scatter_init_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Register_datarep(const char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);
int MPI_Register_datarep_c(const char *datarep, MPI_Datarep_conversion_function_c *read_conversion_fn, MPI_Datarep_conversion_function_c *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);
int MPI_Remove_error_class(int errorclass);
int MPI_Remove_error_code(int errorcode);
int MPI_Remove_error_string(int errorcode);
int MPI_Request_free(MPI_Request *request);
int MPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status);
int MPI_Request_get_status_all(int count, MPI_Request array_of_requests[], int *flag, MPI_Status *array_of_statuses);
int MPI_Request_get_status_any(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
int MPI_Request_get_status_some(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int MPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int MPI_Rget_c(void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int MPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int MPI_Rget_accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, void *result_addr, MPI_Count result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int MPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int MPI_Rput_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int MPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Rsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Rsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Scan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int MPI_Scan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Scan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatter_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Scatter_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatterv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int MPI_Scatterv_init(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Scatterv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Send_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Send_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Sendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int MPI_Session_attach_buffer(MPI_Session session, void *buffer, int size);
int MPI_Session_attach_buffer_c(MPI_Session session, void *buffer, MPI_Count size);
int MPI_Session_call_errhandler(MPI_Session session, int errorcode);
int MPI_Session_create_errhandler(MPI_Session_errhandler_function *session_errhandler_fn, MPI_Errhandler *errhandler);
int MPI_Session_detach_buffer(MPI_Session session, void *buffer_addr, int *size);
int MPI_Session_detach_buffer_c(MPI_Session session, void *buffer_addr, MPI_Count *size);
int MPI_Session_finalize(MPI_Session *session);
int MPI_Session_flush_buffer(MPI_Session session);
int MPI_Session_get_errhandler(MPI_Session session, MPI_Errhandler *errhandler);
int MPI_Session_get_info(MPI_Session session, MPI_Info *info_used);
int MPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n, int *pset_len, char *pset_name);
int MPI_Session_get_num_psets(MPI_Session session, MPI_Info info, int *npset_names);
int MPI_Session_get_pset_info(MPI_Session session, const char *pset_name, MPI_Info *info);
int MPI_Session_iflush_buffer(MPI_Session session, MPI_Request *request);
int MPI_Session_init(MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session);
int MPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler);
int MPI_Sizeof(void *x, int *size);
int MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Ssend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Ssend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int MPI_Start(MPI_Request *request);
int MPI_Startall(int count, MPI_Request array_of_requests[]);
int MPI_Status_get_error(MPI_Status *status, int *error);
int MPI_Status_get_source(MPI_Status *status, int *source);
int MPI_Status_get_tag(MPI_Status *status, int *tag);
int MPI_Status_set_cancelled(MPI_Status *status, int flag);
int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count);
int MPI_Status_set_elements_c(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
int MPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
int MPI_Status_set_error(MPI_Status *status, int error);
int MPI_Status_set_source(MPI_Status *status, int source);
int MPI_Status_set_tag(MPI_Status *status, int tag);
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
int MPI_Test_cancelled(const MPI_Status *status, int *flag);
int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status *array_of_statuses);
int MPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
int MPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int MPI_Topo_test(MPI_Comm comm, int *status);
int MPI_Type_commit(MPI_Datatype *datatype);
int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_contiguous_c(MPI_Count count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_darray_c(int size, int rank, int ndims, const MPI_Count array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);
int MPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);
int MPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);
int MPI_Type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_hindexed_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_hindexed_block_c(MPI_Count count, MPI_Count blocklength, const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_hvector_c(MPI_Count count, MPI_Count blocklength, MPI_Count stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_indexed_block_c(MPI_Count count, MPI_Count blocklength, const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state);
int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype);
int MPI_Type_create_resized_c(MPI_Datatype oldtype, MPI_Count lb, MPI_Count extent, MPI_Datatype *newtype);
int MPI_Type_create_struct(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int MPI_Type_create_struct_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int MPI_Type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_create_subarray_c(int ndims, const MPI_Count array_of_sizes[], const MPI_Count array_of_subsizes[], const MPI_Count array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval);
int MPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_free(MPI_Datatype *datatype);
int MPI_Type_free_keyval(int *type_keyval);
int MPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag);
int MPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]);
int MPI_Type_get_contents_c(MPI_Datatype datatype, MPI_Count max_integers, MPI_Count max_addresses, MPI_Count max_large_counts, MPI_Count max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Count array_of_large_counts[], MPI_Datatype array_of_datatypes[]);
int MPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);
int MPI_Type_get_envelope_c(MPI_Datatype datatype, MPI_Count *num_integers, MPI_Count *num_addresses, MPI_Count *num_large_counts, MPI_Count *num_datatypes, int *combiner);
int MPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
int MPI_Type_get_extent_c(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
int MPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
int MPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen);
int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);
int MPI_Type_get_true_extent_c(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int MPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int MPI_Type_get_value_index(MPI_Datatype value_type, MPI_Datatype index_type, MPI_Datatype *pair_type);
int MPI_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_indexed_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype);
int MPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int MPI_Type_set_name(MPI_Datatype datatype, const char *type_name);
int MPI_Type_size(MPI_Datatype datatype, int *size);
int MPI_Type_size_c(MPI_Datatype datatype, MPI_Count *size);
int MPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size);
int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Type_vector_c(MPI_Count count, MPI_Count blocklength, MPI_Count stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int MPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm);
int MPI_Unpack_c(const void *inbuf, MPI_Count insize, MPI_Count *position, void *outbuf, MPI_Count outcount, MPI_Datatype datatype, MPI_Comm comm);
int MPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);
int MPI_Unpack_external_c(const char datarep[], const void *inbuf, MPI_Count insize, MPI_Count *position, void *outbuf, MPI_Count outcount, MPI_Datatype datatype);
int MPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name);
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int MPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status *array_of_statuses);
int MPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status);
int MPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int MPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int MPI_Win_allocate_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int MPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int MPI_Win_allocate_shared_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int MPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);
int MPI_Win_call_errhandler(MPI_Win win, int errorcode);
int MPI_Win_complete(MPI_Win win);
int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int MPI_Win_create_c(void *base, MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int MPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);
int MPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Errhandler *errhandler);
int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state);
int MPI_Win_delete_attr(MPI_Win win, int win_keyval);
int MPI_Win_detach(MPI_Win win, const void *base);
int MPI_Win_fence(int assert, MPI_Win win);
int MPI_Win_flush(int rank, MPI_Win win);
int MPI_Win_flush_all(MPI_Win win);
int MPI_Win_flush_local(int rank, MPI_Win win);
int MPI_Win_flush_local_all(MPI_Win win);
int MPI_Win_free(MPI_Win *win);
int MPI_Win_free_keyval(int *win_keyval);
int MPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
int MPI_Win_get_group(MPI_Win win, MPI_Group *group);
int MPI_Win_get_info(MPI_Win win, MPI_Info *info_used);
int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);
int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);
int MPI_Win_lock_all(int assert, MPI_Win win);
int MPI_Win_post(MPI_Group group, int assert, MPI_Win win);
int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);
int MPI_Win_set_info(MPI_Win win, MPI_Info info);
int MPI_Win_set_name(MPI_Win win, const char *win_name);
int MPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);
int MPI_Win_shared_query_c(MPI_Win win, int rank, MPI_Aint *size, MPI_Aint *disp_unit, void *baseptr);
int MPI_Win_start(MPI_Group group, int assert, MPI_Win win);
int MPI_Win_sync(MPI_Win win);
int MPI_Win_test(MPI_Win win, int *flag);
int MPI_Win_unlock(int rank, MPI_Win win);
int MPI_Win_unlock_all(MPI_Win win);
int MPI_Win_wait(MPI_Win win);

MPI_Aint MPI_Aint_add(MPI_Aint base, MPI_Aint disp);
MPI_Aint MPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2);
double MPI_Wtick(void);
double MPI_Wtime(void);

int MPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status);
int MPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status);
MPI_Fint MPI_Comm_c2f(MPI_Comm comm);
MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint MPI_Type_c2f(MPI_Datatype datatype);
MPI_Datatype MPI_Type_f2c(MPI_Fint datatype);
MPI_Fint MPI_File_c2f(MPI_File file);
MPI_File MPI_File_f2c(MPI_Fint file);
MPI_Fint MPI_Group_c2f(MPI_Group group);
MPI_Group MPI_Group_f2c(MPI_Fint group);
MPI_Fint MPI_Info_c2f(MPI_Info info);
MPI_Info MPI_Info_f2c(MPI_Fint info);
MPI_Fint MPI_Message_c2f(MPI_Message message);
MPI_Message MPI_Message_f2c(MPI_Fint message);
MPI_Fint MPI_Op_c2f(MPI_Op op);
MPI_Op MPI_Op_f2c(MPI_Fint op);
MPI_Fint MPI_Request_c2f(MPI_Request request);
MPI_Request MPI_Request_f2c(MPI_Fint request);
MPI_Fint MPI_Session_c2f(MPI_Session session);
MPI_Session MPI_Session_f2c(MPI_Fint session);
MPI_Fint MPI_Win_c2f(MPI_Win win);
MPI_Win MPI_Win_f2c(MPI_Fint win);

/* MPI_T functions */
int MPI_T_category_changed(int *update_number);
int MPI_T_category_get_categories(int cat_index, int len, int indices[]);
int MPI_T_category_get_cvars(int cat_index, int len, int indices[]);
int MPI_T_category_get_events(int cat_index, int len, int indices[]);
int MPI_T_category_get_index(const char *name, int *cat_index);
int MPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_cvars, int *num_pvars, int *num_categories);
int MPI_T_category_get_num(int *num_cat);
int MPI_T_category_get_num_events(int cat_index, int *num_events);
int MPI_T_category_get_pvars(int cat_index, int len, int indices[]);
int MPI_T_cvar_get_index(const char *name, int *cvar_index);
int MPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *scope);
int MPI_T_cvar_get_num(int *num_cvar);
int MPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle, int *count);
int MPI_T_cvar_handle_free(MPI_T_cvar_handle *handle);
int MPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf);
int MPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf);
int MPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len);
int MPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name, int *name_len);
int MPI_T_event_callback_get_info(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info *info_used);
int MPI_T_event_callback_set_info(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info info);
int MPI_T_event_copy(MPI_T_event_instance event_instance, void *buffer);
int MPI_T_event_get_index(const char *name, int *event_index);
int MPI_T_event_get_info(int event_index, char *name, int *name_len, int *verbosity, MPI_Datatype array_of_datatypes[], MPI_Aint array_of_displacements[], int *num_elements, MPI_T_enum *enumtype, MPI_Info *info, char *desc, int *desc_len, int *bind);
int MPI_T_event_get_num(int *num_events);
int MPI_T_event_get_source(MPI_T_event_instance event_instance, int *source_index);
int MPI_T_event_get_timestamp(MPI_T_event_instance event_instance, MPI_Count *event_timestamp);
int MPI_T_event_handle_alloc(int event_index, void *obj_handle, MPI_Info info, MPI_T_event_registration *event_registration);
int MPI_T_event_handle_free(MPI_T_event_registration event_registration, void *user_data, MPI_T_event_free_cb_function free_cb_function);
int MPI_T_event_handle_get_info(MPI_T_event_registration event_registration, MPI_Info *info_used);
int MPI_T_event_handle_set_info(MPI_T_event_registration event_registration, MPI_Info info);
int MPI_T_event_read(MPI_T_event_instance event_instance, int element_index, void *buffer);
int MPI_T_event_register_callback(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info info, void *user_data, MPI_T_event_cb_function event_cb_function);
int MPI_T_event_set_dropped_handler(MPI_T_event_registration event_registration, MPI_T_event_dropped_cb_function dropped_cb_function);
int MPI_T_finalize(void);
int MPI_T_init_thread(int required, int *provided);
int MPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index);
int MPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *readonly, int *continuous, int *atomic);
int MPI_T_pvar_get_num(int *num_pvar);
int MPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle, MPI_T_pvar_handle *handle, int *count);
int MPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);
int MPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int MPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int MPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int MPI_T_pvar_session_create(MPI_T_pvar_session *session);
int MPI_T_pvar_session_free(MPI_T_pvar_session *session);
int MPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int MPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int MPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf);
int MPI_T_source_get_info(int source_index, char *name, int *name_len, char *desc, int *desc_len, MPI_T_source_order *ordering, MPI_Count *ticks_per_second, MPI_Count *max_ticks, MPI_Info *info);
int MPI_T_source_get_num(int *num_sources);
int MPI_T_source_get_timestamp(int source_index, MPI_Count *timestamp);

/* PMPI functions */
int PMPI_Abort(MPI_Comm comm, int errorcode);
int PMPI_Accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Add_error_class(int *errorclass);
int PMPI_Add_error_code(int errorclass, int *errorcode);
int PMPI_Add_error_string(int errorcode, const char *string);
int PMPI_Allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allgather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allgatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);
int PMPI_Allreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Allreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Allreduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Allreduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoall_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Alltoallw_init(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Attr_delete(MPI_Comm comm, int keyval);
int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
int PMPI_Barrier(MPI_Comm comm);
int PMPI_Barrier_init(MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm);
int PMPI_Bcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm);
int PMPI_Bcast_init(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Bcast_init_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Bsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Bsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Bsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Bsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Buffer_attach(void *buffer, int size);
int PMPI_Buffer_attach_c(void *buffer, MPI_Count size);
int PMPI_Buffer_detach(void *buffer_addr, int *size);
int PMPI_Buffer_detach_c(void *buffer_addr, MPI_Count *size);
int PMPI_Buffer_flush(void);
int PMPI_Buffer_iflush(MPI_Request *request);
int PMPI_Cancel(MPI_Request *request);
int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int coords[]);
int PMPI_Cart_create(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart);
int PMPI_Cart_get(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]);
int PMPI_Cart_map(MPI_Comm comm, int ndims, const int dims[], const int periods[], int *newrank);
int PMPI_Cart_rank(MPI_Comm comm, const int coords[], int *rank);
int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest);
int PMPI_Cart_sub(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm);
int PMPI_Cartdim_get(MPI_Comm comm, int *ndims);
int PMPI_Close_port(const char *port_name);
int PMPI_Comm_accept(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_attach_buffer(MPI_Comm comm, void *buffer, int size);
int PMPI_Comm_attach_buffer_c(MPI_Comm comm, void *buffer, MPI_Count size);
int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
int PMPI_Comm_connect(const char *port_name, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_function *comm_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_Comm_create_from_group(MPI_Group group, const char *stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newcomm);
int PMPI_Comm_create_group(MPI_Comm comm, MPI_Group group, int tag, MPI_Comm *newcomm);
int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, MPI_Comm_delete_attr_function *comm_delete_attr_fn, int *comm_keyval, void *extra_state);
int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);
int PMPI_Comm_detach_buffer(MPI_Comm comm, void *buffer_addr, int *size);
int PMPI_Comm_detach_buffer_c(MPI_Comm comm, void *buffer_addr, MPI_Count *size);
int PMPI_Comm_disconnect(MPI_Comm *comm);
int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
int PMPI_Comm_dup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm);
int PMPI_Comm_flush_buffer(MPI_Comm comm);
int PMPI_Comm_free(MPI_Comm *comm);
int PMPI_Comm_free_keyval(int *comm_keyval);
int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, void *attribute_val, int *flag);
int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *errhandler);
int PMPI_Comm_get_info(MPI_Comm comm, MPI_Info *info_used);
int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
int PMPI_Comm_get_parent(MPI_Comm *parent);
int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group);
int PMPI_Comm_idup(MPI_Comm comm, MPI_Comm *newcomm, MPI_Request *request);
int PMPI_Comm_idup_with_info(MPI_Comm comm, MPI_Info info, MPI_Comm *newcomm, MPI_Request *request);
int PMPI_Comm_iflush_buffer(MPI_Comm comm, MPI_Request *request);
int PMPI_Comm_join(int fd, MPI_Comm *intercomm);
int PMPI_Comm_rank(MPI_Comm comm, int *rank);
int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);
int PMPI_Comm_remote_size(MPI_Comm comm, int *size);
int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
int PMPI_Comm_set_info(MPI_Comm comm, MPI_Info info);
int PMPI_Comm_set_name(MPI_Comm comm, const char *comm_name);
int PMPI_Comm_size(MPI_Comm comm, int *size);
int PMPI_Comm_spawn(const char *command, char *argv[], int maxprocs, MPI_Info info, int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int PMPI_Comm_spawn_multiple(int count, char *array_of_commands[], char **array_of_argv[], const int array_of_maxprocs[], const MPI_Info array_of_info[], int root, MPI_Comm comm, MPI_Comm *intercomm, int array_of_errcodes[]);
int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
int PMPI_Comm_split_type(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm);
int PMPI_Comm_test_inter(MPI_Comm comm, int *flag);
int PMPI_Compare_and_swap(const void *origin_addr, const void *compare_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Win win);
int PMPI_Dims_create(int nnodes, int ndims, int dims[]);
int PMPI_Dist_graph_create(MPI_Comm comm_old, int n, const int sources[], const int degrees[], const int destinations[], const int weights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int PMPI_Dist_graph_create_adjacent(MPI_Comm comm_old, int indegree, const int sources[], const int sourceweights[], int outdegree, const int destinations[], const int destweights[], MPI_Info info, int reorder, MPI_Comm *comm_dist_graph);
int PMPI_Dist_graph_neighbors(MPI_Comm comm, int maxindegree, int sources[], int sourceweights[], int maxoutdegree, int destinations[], int destweights[]);
int PMPI_Dist_graph_neighbors_count(MPI_Comm comm, int *indegree, int *outdegree, int *weighted);
int PMPI_Errhandler_free(MPI_Errhandler *errhandler);
int PMPI_Error_class(int errorcode, int *errorclass);
int PMPI_Error_string(int errorcode, char *string, int *resultlen);
int PMPI_Exscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Exscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Exscan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Exscan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_F_sync_reg(void *buf);
int PMPI_Fetch_and_op(const void *origin_addr, void *result_addr, MPI_Datatype datatype, int target_rank, MPI_Aint target_disp, MPI_Op op, MPI_Win win);
int PMPI_File_call_errhandler(MPI_File fh, int errorcode);
int PMPI_File_close(MPI_File *fh);
int PMPI_File_create_errhandler(MPI_File_errhandler_function *file_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_File_delete(const char *filename, MPI_Info info);
int PMPI_File_get_amode(MPI_File fh, int *amode);
int PMPI_File_get_atomicity(MPI_File fh, int *flag);
int PMPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset, MPI_Offset *disp);
int PMPI_File_get_errhandler(MPI_File file, MPI_Errhandler *errhandler);
int PMPI_File_get_group(MPI_File fh, MPI_Group *group);
int PMPI_File_get_info(MPI_File fh, MPI_Info *info_used);
int PMPI_File_get_position(MPI_File fh, MPI_Offset *offset);
int PMPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset);
int PMPI_File_get_size(MPI_File fh, MPI_Offset *size);
int PMPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype, MPI_Aint *extent);
int PMPI_File_get_type_extent_c(MPI_File fh, MPI_Datatype datatype, MPI_Count *extent);
int PMPI_File_get_view(MPI_File fh, MPI_Offset *disp, MPI_Datatype *etype, MPI_Datatype *filetype, char *datarep);
int PMPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_all_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_at_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_at_all_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iread_shared_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_all_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_at_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_iwrite_shared_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Request *request);
int PMPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh);
int PMPI_File_preallocate(MPI_File fh, MPI_Offset size);
int PMPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_all_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_all_begin(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
int PMPI_File_read_all_begin_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status);
int PMPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_at_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_at_all_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype);
int PMPI_File_read_at_all_begin_c(MPI_File fh, MPI_Offset offset, void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
int PMPI_File_read_ordered(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_ordered_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_ordered_begin(MPI_File fh, void *buf, int count, MPI_Datatype datatype);
int PMPI_File_read_ordered_begin_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
int PMPI_File_read_shared(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_read_shared_c(MPI_File fh, void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);
int PMPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence);
int PMPI_File_set_atomicity(MPI_File fh, int flag);
int PMPI_File_set_errhandler(MPI_File file, MPI_Errhandler errhandler);
int PMPI_File_set_info(MPI_File fh, MPI_Info info);
int PMPI_File_set_size(MPI_File fh, MPI_Offset size);
int PMPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info);
int PMPI_File_sync(MPI_File fh);
int PMPI_File_write(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_all(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_all_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_all_begin(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
int PMPI_File_write_all_begin_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_write_all_end(MPI_File fh, const void *buf, MPI_Status *status);
int PMPI_File_write_at(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_at_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_at_all(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_at_all_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype);
int PMPI_File_write_at_all_begin_c(MPI_File fh, MPI_Offset offset, const void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_write_at_all_end(MPI_File fh, const void *buf, MPI_Status *status);
int PMPI_File_write_ordered(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_ordered_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_ordered_begin(MPI_File fh, const void *buf, int count, MPI_Datatype datatype);
int PMPI_File_write_ordered_begin_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype);
int PMPI_File_write_ordered_end(MPI_File fh, const void *buf, MPI_Status *status);
int PMPI_File_write_shared(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_File_write_shared_c(MPI_File fh, const void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Status *status);
int PMPI_Finalize(void);
int PMPI_Finalized(int *flag);
int PMPI_Free_mem(void *base);
int PMPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Gather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Gatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Gatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Get(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Get_c(void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Get_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Get_accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, void *result_addr, MPI_Count result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win);
int PMPI_Get_address(const void *location, MPI_Aint *address);
int PMPI_Get_count(const MPI_Status *status, MPI_Datatype datatype, int *count);
int PMPI_Get_count_c(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int PMPI_Get_elements(const MPI_Status *status, MPI_Datatype datatype, int *count);
int PMPI_Get_elements_c(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int PMPI_Get_elements_x(const MPI_Status *status, MPI_Datatype datatype, MPI_Count *count);
int PMPI_Get_hw_resource_info(MPI_Info *hw_info);
int PMPI_Get_library_version(char *version, int *resultlen);
int PMPI_Get_processor_name(char *name, int *resultlen);
int PMPI_Get_version(int *version, int *subversion);
int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, const int indx[], const int edges[], int reorder, MPI_Comm *comm_graph);
int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, int indx[], int edges[]);
int PMPI_Graph_map(MPI_Comm comm, int nnodes, const int indx[], const int edges[], int *newrank);
int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, int neighbors[]);
int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
int PMPI_Grequest_complete(MPI_Request request);
int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn, MPI_Grequest_free_function *free_fn, MPI_Grequest_cancel_function *cancel_fn, void *extra_state, MPI_Request *request);
int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Group_excl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int PMPI_Group_free(MPI_Group *group);
int PMPI_Group_from_session_pset(MPI_Session session, const char *pset_name, MPI_Group *newgroup);
int PMPI_Group_incl(MPI_Group group, int n, const int ranks[], MPI_Group *newgroup);
int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], MPI_Group *newgroup);
int PMPI_Group_rank(MPI_Group group, int *rank);
int PMPI_Group_size(MPI_Group group, int *size);
int PMPI_Group_translate_ranks(MPI_Group group1, int n, const int ranks1[], MPI_Group group2, int ranks2[]);
int PMPI_Group_union(MPI_Group group1, MPI_Group group2, MPI_Group *newgroup);
int PMPI_Iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallreduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iallreduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallw(const void *sendbuf, const int sendcounts[], const int sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const int rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Ialltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Ibarrier(MPI_Comm comm, MPI_Request *request);
int PMPI_Ibcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ibcast_c(void *buffer, MPI_Count count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ibsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Ibsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Iexscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iexscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Igather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Igatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Improbe(int source, int tag, MPI_Comm comm, int *flag, MPI_Message *message, MPI_Status *status);
int PMPI_Imrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
int PMPI_Imrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Message *message, MPI_Request *request);
int PMPI_Ineighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Ineighbor_alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Request *request);
int PMPI_Info_create(MPI_Info *info);
int PMPI_Info_create_env(int argc, char *argv[], MPI_Info *info);
int PMPI_Info_delete(MPI_Info info, const char *key);
int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo);
int PMPI_Info_free(MPI_Info *info);
int PMPI_Info_get(MPI_Info info, const char *key, int valuelen, char *value, int *flag);
int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys);
int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key);
int PMPI_Info_get_string(MPI_Info info, const char *key, int *buflen, char *value, int *flag);
int PMPI_Info_get_valuelen(MPI_Info info, const char *key, int *valuelen, int *flag);
int PMPI_Info_set(MPI_Info info, const char *key, const char *value);
int PMPI_Init(int *argc, char ***argv);
int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided);
int PMPI_Initialized(int *flag);
int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, MPI_Comm peer_comm, int remote_leader, int tag, MPI_Comm *newintercomm);
int PMPI_Intercomm_create_from_groups(MPI_Group local_group, int local_leader, MPI_Group remote_group, int remote_leader, const char *stringtag, MPI_Info info, MPI_Errhandler errhandler, MPI_Comm *newintercomm);
int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, MPI_Comm *newintracomm);
int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Irecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Ireduce_scatter_block_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Irsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Irsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Is_thread_main(int *flag);
int PMPI_Iscan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatter_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Iscatterv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request);
int PMPI_Isend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isendrecv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int PMPI_Isendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Request *request);
int PMPI_Issend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Issend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Keyval_create(MPI_Copy_function *copy_fn, MPI_Delete_function *delete_fn, int *keyval, void *extra_state);
int PMPI_Keyval_free(int *keyval);
int PMPI_Lookup_name(const char *service_name, MPI_Info info, char *port_name);
int PMPI_Mprobe(int source, int tag, MPI_Comm comm, MPI_Message *message, MPI_Status *status);
int PMPI_Mrecv(void *buf, int count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
int PMPI_Mrecv_c(void *buf, MPI_Count count, MPI_Datatype datatype, MPI_Message *message, MPI_Status *status);
int PMPI_Neighbor_allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgather_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgather_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_allgather_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgatherv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_allgatherv_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_allgatherv_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint displs[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoall_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoall_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoall_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallv(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoallv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm);
int PMPI_Neighbor_alltoallv_init(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], MPI_Datatype sendtype, void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], MPI_Datatype recvtype, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallw(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Neighbor_alltoallw_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm);
int PMPI_Neighbor_alltoallw_init(const void *sendbuf, const int sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const int recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Neighbor_alltoallw_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint sdispls[], const MPI_Datatype sendtypes[], void *recvbuf, const MPI_Count recvcounts[], const MPI_Aint rdispls[], const MPI_Datatype recvtypes[], MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Op_commutative(MPI_Op op, int *commute);
int PMPI_Op_create(MPI_User_function *user_fn, int commute, MPI_Op *op);
int PMPI_Op_create_c(MPI_User_function_c *user_fn, int commute, MPI_Op *op);
int PMPI_Op_free(MPI_Op *op);
int PMPI_Open_port(MPI_Info info, char *port_name);
int PMPI_Pack(const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, int outsize, int *position, MPI_Comm comm);
int PMPI_Pack_c(const void *inbuf, MPI_Count incount, MPI_Datatype datatype, void *outbuf, MPI_Count outsize, MPI_Count *position, MPI_Comm comm);
int PMPI_Pack_external(const char *datarep, const void *inbuf, int incount, MPI_Datatype datatype, void *outbuf, MPI_Aint outsize, MPI_Aint *position);
int PMPI_Pack_external_c(const char *datarep, const void *inbuf, MPI_Count incount, MPI_Datatype datatype, void *outbuf, MPI_Count outsize, MPI_Count *position);
int PMPI_Pack_external_size(const char *datarep, int incount, MPI_Datatype datatype, MPI_Aint *size);
int PMPI_Pack_external_size_c(const char *datarep, MPI_Count incount, MPI_Datatype datatype, MPI_Count *size);
int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, int *size);
int PMPI_Pack_size_c(MPI_Count incount, MPI_Datatype datatype, MPI_Comm comm, MPI_Count *size);
int PMPI_Parrived(MPI_Request request, int partition, int *flag);
int PMPI_Pcontrol(const int level, ...);
int PMPI_Pready(int partition, MPI_Request request);
int PMPI_Pready_list(int length, const int array_of_partitions[], MPI_Request request);
int PMPI_Pready_range(int partition_low, int partition_high, MPI_Request request);
int PMPI_Precv_init(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
int PMPI_Psend_init(const void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Publish_name(const char *service_name, MPI_Info info, const char *port_name);
int PMPI_Put(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Put_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win);
int PMPI_Query_thread(int *provided);
int PMPI_Raccumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Raccumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int PMPI_Recv_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Recv_init_c(void *buf, MPI_Count count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int PMPI_Reduce_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int PMPI_Reduce_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_local(const void *inbuf, void *inoutbuf, int count, MPI_Datatype datatype, MPI_Op op);
int PMPI_Reduce_local_c(const void *inbuf, void *inoutbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op);
int PMPI_Reduce_scatter(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter_block(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter_block_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Reduce_scatter_block_init(const void *sendbuf, void *recvbuf, int recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_scatter_block_init_c(const void *sendbuf, void *recvbuf, MPI_Count recvcount, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_scatter_init(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Reduce_scatter_init_c(const void *sendbuf, void *recvbuf, const MPI_Count recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Register_datarep(const char *datarep, MPI_Datarep_conversion_function *read_conversion_fn, MPI_Datarep_conversion_function *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);
int PMPI_Register_datarep_c(const char *datarep, MPI_Datarep_conversion_function_c *read_conversion_fn, MPI_Datarep_conversion_function_c *write_conversion_fn, MPI_Datarep_extent_function *dtype_file_extent_fn, void *extra_state);
int PMPI_Remove_error_class(int errorclass);
int PMPI_Remove_error_code(int errorcode);
int PMPI_Remove_error_string(int errorcode);
int PMPI_Request_free(MPI_Request *request);
int PMPI_Request_get_status(MPI_Request request, int *flag, MPI_Status *status);
int PMPI_Request_get_status_all(int count, MPI_Request array_of_requests[], int *flag, MPI_Status *array_of_statuses);
int PMPI_Request_get_status_any(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
int PMPI_Request_get_status_some(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int PMPI_Rget(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rget_c(void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rget_accumulate(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, void *result_addr, int result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Rget_accumulate_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, void *result_addr, MPI_Count result_count, MPI_Datatype result_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Op op, MPI_Win win, MPI_Request *request);
int PMPI_Rput(const void *origin_addr, int origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, int target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rput_c(const void *origin_addr, MPI_Count origin_count, MPI_Datatype origin_datatype, int target_rank, MPI_Aint target_disp, MPI_Count target_count, MPI_Datatype target_datatype, MPI_Win win, MPI_Request *request);
int PMPI_Rsend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Rsend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Rsend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Rsend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Scan_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
int PMPI_Scan_init(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scan_init_c(const void *sendbuf, void *recvbuf, MPI_Count count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatter_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatter_init(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatter_init_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatterv(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatterv_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);
int PMPI_Scatterv_init(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Scatterv_init_c(const void *sendbuf, const MPI_Count sendcounts[], const MPI_Aint displs[], MPI_Datatype sendtype, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Info info, MPI_Request *request);
int PMPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Send_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Send_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Send_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Sendrecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Sendrecv_c(const void *sendbuf, MPI_Count sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, MPI_Count recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Sendrecv_replace_c(void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int sendtag, int source, int recvtag, MPI_Comm comm, MPI_Status *status);
int PMPI_Session_attach_buffer(MPI_Session session, void *buffer, int size);
int PMPI_Session_attach_buffer_c(MPI_Session session, void *buffer, MPI_Count size);
int PMPI_Session_call_errhandler(MPI_Session session, int errorcode);
int PMPI_Session_create_errhandler(MPI_Session_errhandler_function *session_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_Session_detach_buffer(MPI_Session session, void *buffer_addr, int *size);
int PMPI_Session_detach_buffer_c(MPI_Session session, void *buffer_addr, MPI_Count *size);
int PMPI_Session_finalize(MPI_Session *session);
int PMPI_Session_flush_buffer(MPI_Session session);
int PMPI_Session_get_errhandler(MPI_Session session, MPI_Errhandler *errhandler);
int PMPI_Session_get_info(MPI_Session session, MPI_Info *info_used);
int PMPI_Session_get_nth_pset(MPI_Session session, MPI_Info info, int n, int *pset_len, char *pset_name);
int PMPI_Session_get_num_psets(MPI_Session session, MPI_Info info, int *npset_names);
int PMPI_Session_get_pset_info(MPI_Session session, const char *pset_name, MPI_Info *info);
int PMPI_Session_iflush_buffer(MPI_Session session, MPI_Request *request);
int PMPI_Session_init(MPI_Info info, MPI_Errhandler errhandler, MPI_Session *session);
int PMPI_Session_set_errhandler(MPI_Session session, MPI_Errhandler errhandler);
int PMPI_Sizeof(void *x, int *size);
int PMPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Ssend_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int PMPI_Ssend_init(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Ssend_init_c(const void *buf, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request);
int PMPI_Start(MPI_Request *request);
int PMPI_Startall(int count, MPI_Request array_of_requests[]);
int PMPI_Status_get_error(MPI_Status *status, int *error);
int PMPI_Status_get_source(MPI_Status *status, int *source);
int PMPI_Status_get_tag(MPI_Status *status, int *tag);
int PMPI_Status_set_cancelled(MPI_Status *status, int flag);
int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype, int count);
int PMPI_Status_set_elements_c(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
int PMPI_Status_set_elements_x(MPI_Status *status, MPI_Datatype datatype, MPI_Count count);
int PMPI_Status_set_error(MPI_Status *status, int error);
int PMPI_Status_set_source(MPI_Status *status, int source);
int PMPI_Status_set_tag(MPI_Status *status, int tag);
int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
int PMPI_Test_cancelled(const MPI_Status *status, int *flag);
int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag, MPI_Status *array_of_statuses);
int PMPI_Testany(int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status *status);
int PMPI_Testsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int PMPI_Topo_test(MPI_Comm comm, int *status);
int PMPI_Type_commit(MPI_Datatype *datatype);
int PMPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_contiguous_c(MPI_Count count, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_darray(int size, int rank, int ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_darray_c(int size, int rank, int ndims, const MPI_Count array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);
int PMPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);
int PMPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed_block(int count, int blocklength, const MPI_Aint array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hindexed_block_c(MPI_Count count, MPI_Count blocklength, const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_hvector_c(MPI_Count count, MPI_Count blocklength, MPI_Count stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_indexed_block(int count, int blocklength, const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_indexed_block_c(MPI_Count count, MPI_Count blocklength, const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, MPI_Type_delete_attr_function *type_delete_attr_fn, int *type_keyval, void *extra_state);
int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, MPI_Aint extent, MPI_Datatype *newtype);
int PMPI_Type_create_resized_c(MPI_Datatype oldtype, MPI_Count lb, MPI_Count extent, MPI_Datatype *newtype);
int PMPI_Type_create_struct(int count, const int array_of_blocklengths[], const MPI_Aint array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int PMPI_Type_create_struct_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], const MPI_Datatype array_of_types[], MPI_Datatype *newtype);
int PMPI_Type_create_subarray(int ndims, const int array_of_sizes[], const int array_of_subsizes[], const int array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_create_subarray_c(int ndims, const MPI_Count array_of_sizes[], const MPI_Count array_of_subsizes[], const MPI_Count array_of_starts[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_delete_attr(MPI_Datatype datatype, int type_keyval);
int PMPI_Type_dup(MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_free(MPI_Datatype *datatype);
int PMPI_Type_free_keyval(int *type_keyval);
int PMPI_Type_get_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val, int *flag);
int PMPI_Type_get_contents(MPI_Datatype datatype, int max_integers, int max_addresses, int max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Datatype array_of_datatypes[]);
int PMPI_Type_get_contents_c(MPI_Datatype datatype, MPI_Count max_integers, MPI_Count max_addresses, MPI_Count max_large_counts, MPI_Count max_datatypes, int array_of_integers[], MPI_Aint array_of_addresses[], MPI_Count array_of_large_counts[], MPI_Datatype array_of_datatypes[]);
int PMPI_Type_get_envelope(MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner);
int PMPI_Type_get_envelope_c(MPI_Datatype datatype, MPI_Count *num_integers, MPI_Count *num_addresses, MPI_Count *num_large_counts, MPI_Count *num_datatypes, int *combiner);
int PMPI_Type_get_extent(MPI_Datatype datatype, MPI_Aint *lb, MPI_Aint *extent);
int PMPI_Type_get_extent_c(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
int PMPI_Type_get_extent_x(MPI_Datatype datatype, MPI_Count *lb, MPI_Count *extent);
int PMPI_Type_get_name(MPI_Datatype datatype, char *type_name, int *resultlen);
int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, MPI_Aint *true_extent);
int PMPI_Type_get_true_extent_c(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int PMPI_Type_get_true_extent_x(MPI_Datatype datatype, MPI_Count *true_lb, MPI_Count *true_extent);
int PMPI_Type_get_value_index(MPI_Datatype value_type, MPI_Datatype index_type, MPI_Datatype *pair_type);
int PMPI_Type_indexed(int count, const int array_of_blocklengths[], const int array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_indexed_c(MPI_Count count, const MPI_Count array_of_blocklengths[], const MPI_Count array_of_displacements[], MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *datatype);
int PMPI_Type_set_attr(MPI_Datatype datatype, int type_keyval, void *attribute_val);
int PMPI_Type_set_name(MPI_Datatype datatype, const char *type_name);
int PMPI_Type_size(MPI_Datatype datatype, int *size);
int PMPI_Type_size_c(MPI_Datatype datatype, MPI_Count *size);
int PMPI_Type_size_x(MPI_Datatype datatype, MPI_Count *size);
int PMPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Type_vector_c(MPI_Count count, MPI_Count blocklength, MPI_Count stride, MPI_Datatype oldtype, MPI_Datatype *newtype);
int PMPI_Unpack(const void *inbuf, int insize, int *position, void *outbuf, int outcount, MPI_Datatype datatype, MPI_Comm comm);
int PMPI_Unpack_c(const void *inbuf, MPI_Count insize, MPI_Count *position, void *outbuf, MPI_Count outcount, MPI_Datatype datatype, MPI_Comm comm);
int PMPI_Unpack_external(const char datarep[], const void *inbuf, MPI_Aint insize, MPI_Aint *position, void *outbuf, int outcount, MPI_Datatype datatype);
int PMPI_Unpack_external_c(const char datarep[], const void *inbuf, MPI_Count insize, MPI_Count *position, void *outbuf, MPI_Count outcount, MPI_Datatype datatype);
int PMPI_Unpublish_name(const char *service_name, MPI_Info info, const char *port_name);
int PMPI_Wait(MPI_Request *request, MPI_Status *status);
int PMPI_Waitall(int count, MPI_Request array_of_requests[], MPI_Status *array_of_statuses);
int PMPI_Waitany(int count, MPI_Request array_of_requests[], int *indx, MPI_Status *status);
int PMPI_Waitsome(int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status *array_of_statuses);
int PMPI_Win_allocate(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_allocate_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_allocate_shared_c(MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, void *baseptr, MPI_Win *win);
int PMPI_Win_attach(MPI_Win win, void *base, MPI_Aint size);
int PMPI_Win_call_errhandler(MPI_Win win, int errorcode);
int PMPI_Win_complete(MPI_Win win);
int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int PMPI_Win_create_c(void *base, MPI_Aint size, MPI_Aint disp_unit, MPI_Info info, MPI_Comm comm, MPI_Win *win);
int PMPI_Win_create_dynamic(MPI_Info info, MPI_Comm comm, MPI_Win *win);
int PMPI_Win_create_errhandler(MPI_Win_errhandler_function *win_errhandler_fn, MPI_Errhandler *errhandler);
int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, MPI_Win_delete_attr_function *win_delete_attr_fn, int *win_keyval, void *extra_state);
int PMPI_Win_delete_attr(MPI_Win win, int win_keyval);
int PMPI_Win_detach(MPI_Win win, const void *base);
int PMPI_Win_fence(int assert, MPI_Win win);
int PMPI_Win_flush(int rank, MPI_Win win);
int PMPI_Win_flush_all(MPI_Win win);
int PMPI_Win_flush_local(int rank, MPI_Win win);
int PMPI_Win_flush_local_all(MPI_Win win);
int PMPI_Win_free(MPI_Win *win);
int PMPI_Win_free_keyval(int *win_keyval);
int PMPI_Win_get_attr(MPI_Win win, int win_keyval, void *attribute_val, int *flag);
int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
int PMPI_Win_get_group(MPI_Win win, MPI_Group *group);
int PMPI_Win_get_info(MPI_Win win, MPI_Info *info_used);
int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);
int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);
int PMPI_Win_lock_all(int assert, MPI_Win win);
int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win);
int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);
int PMPI_Win_set_info(MPI_Win win, MPI_Info info);
int PMPI_Win_set_name(MPI_Win win, const char *win_name);
int PMPI_Win_shared_query(MPI_Win win, int rank, MPI_Aint *size, int *disp_unit, void *baseptr);
int PMPI_Win_shared_query_c(MPI_Win win, int rank, MPI_Aint *size, MPI_Aint *disp_unit, void *baseptr);
int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win);
int PMPI_Win_sync(MPI_Win win);
int PMPI_Win_test(MPI_Win win, int *flag);
int PMPI_Win_unlock(int rank, MPI_Win win);
int PMPI_Win_unlock_all(MPI_Win win);
int PMPI_Win_wait(MPI_Win win);

MPI_Aint PMPI_Aint_add(MPI_Aint base, MPI_Aint disp);
MPI_Aint PMPI_Aint_diff(MPI_Aint addr1, MPI_Aint addr2);
double PMPI_Wtick(void);
double PMPI_Wtime(void);

int PMPI_Status_c2f(const MPI_Status *c_status, MPI_Fint *f_status);
int PMPI_Status_f2c(const MPI_Fint *f_status, MPI_Status *c_status);
MPI_Fint PMPI_Comm_c2f(MPI_Comm comm);
MPI_Comm PMPI_Comm_f2c(MPI_Fint comm);
MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler);
MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler);
MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype);
MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype);
MPI_Fint PMPI_File_c2f(MPI_File file);
MPI_File PMPI_File_f2c(MPI_Fint file);
MPI_Fint PMPI_Group_c2f(MPI_Group group);
MPI_Group PMPI_Group_f2c(MPI_Fint group);
MPI_Fint PMPI_Info_c2f(MPI_Info info);
MPI_Info PMPI_Info_f2c(MPI_Fint info);
MPI_Fint PMPI_Message_c2f(MPI_Message message);
MPI_Message PMPI_Message_f2c(MPI_Fint message);
MPI_Fint PMPI_Op_c2f(MPI_Op op);
MPI_Op PMPI_Op_f2c(MPI_Fint op);
MPI_Fint PMPI_Request_c2f(MPI_Request request);
MPI_Request PMPI_Request_f2c(MPI_Fint request);
MPI_Fint PMPI_Session_c2f(MPI_Session session);
MPI_Session PMPI_Session_f2c(MPI_Fint session);
MPI_Fint PMPI_Win_c2f(MPI_Win win);
MPI_Win PMPI_Win_f2c(MPI_Fint win);

/* PMPI_T functions */
int PMPI_T_category_changed(int *update_number);
int PMPI_T_category_get_categories(int cat_index, int len, int indices[]);
int PMPI_T_category_get_cvars(int cat_index, int len, int indices[]);
int PMPI_T_category_get_events(int cat_index, int len, int indices[]);
int PMPI_T_category_get_index(const char *name, int *cat_index);
int PMPI_T_category_get_info(int cat_index, char *name, int *name_len, char *desc, int *desc_len, int *num_cvars, int *num_pvars, int *num_categories);
int PMPI_T_category_get_num(int *num_cat);
int PMPI_T_category_get_num_events(int cat_index, int *num_events);
int PMPI_T_category_get_pvars(int cat_index, int len, int indices[]);
int PMPI_T_cvar_get_index(const char *name, int *cvar_index);
int PMPI_T_cvar_get_info(int cvar_index, char *name, int *name_len, int *verbosity, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *scope);
int PMPI_T_cvar_get_num(int *num_cvar);
int PMPI_T_cvar_handle_alloc(int cvar_index, void *obj_handle, MPI_T_cvar_handle *handle, int *count);
int PMPI_T_cvar_handle_free(MPI_T_cvar_handle *handle);
int PMPI_T_cvar_read(MPI_T_cvar_handle handle, void *buf);
int PMPI_T_cvar_write(MPI_T_cvar_handle handle, const void *buf);
int PMPI_T_enum_get_info(MPI_T_enum enumtype, int *num, char *name, int *name_len);
int PMPI_T_enum_get_item(MPI_T_enum enumtype, int indx, int *value, char *name, int *name_len);
int PMPI_T_event_callback_get_info(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info *info_used);
int PMPI_T_event_callback_set_info(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info info);
int PMPI_T_event_copy(MPI_T_event_instance event_instance, void *buffer);
int PMPI_T_event_get_index(const char *name, int *event_index);
int PMPI_T_event_get_info(int event_index, char *name, int *name_len, int *verbosity, MPI_Datatype array_of_datatypes[], MPI_Aint array_of_displacements[], int *num_elements, MPI_T_enum *enumtype, MPI_Info *info, char *desc, int *desc_len, int *bind);
int PMPI_T_event_get_num(int *num_events);
int PMPI_T_event_get_source(MPI_T_event_instance event_instance, int *source_index);
int PMPI_T_event_get_timestamp(MPI_T_event_instance event_instance, MPI_Count *event_timestamp);
int PMPI_T_event_handle_alloc(int event_index, void *obj_handle, MPI_Info info, MPI_T_event_registration *event_registration);
int PMPI_T_event_handle_free(MPI_T_event_registration event_registration, void *user_data, MPI_T_event_free_cb_function free_cb_function);
int PMPI_T_event_handle_get_info(MPI_T_event_registration event_registration, MPI_Info *info_used);
int PMPI_T_event_handle_set_info(MPI_T_event_registration event_registration, MPI_Info info);
int PMPI_T_event_read(MPI_T_event_instance event_instance, int element_index, void *buffer);
int PMPI_T_event_register_callback(MPI_T_event_registration event_registration, MPI_T_cb_safety cb_safety, MPI_Info info, void *user_data, MPI_T_event_cb_function event_cb_function);
int PMPI_T_event_set_dropped_handler(MPI_T_event_registration event_registration, MPI_T_event_dropped_cb_function dropped_cb_function);
int PMPI_T_finalize(void);
int PMPI_T_init_thread(int required, int *provided);
int PMPI_T_pvar_get_index(const char *name, int var_class, int *pvar_index);
int PMPI_T_pvar_get_info(int pvar_index, char *name, int *name_len, int *verbosity, int *var_class, MPI_Datatype *datatype, MPI_T_enum *enumtype, char *desc, int *desc_len, int *bind, int *readonly, int *continuous, int *atomic);
int PMPI_T_pvar_get_num(int *num_pvar);
int PMPI_T_pvar_handle_alloc(MPI_T_pvar_session session, int pvar_index, void *obj_handle, MPI_T_pvar_handle *handle, int *count);
int PMPI_T_pvar_handle_free(MPI_T_pvar_session session, MPI_T_pvar_handle *handle);
int PMPI_T_pvar_read(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int PMPI_T_pvar_readreset(MPI_T_pvar_session session, MPI_T_pvar_handle handle, void *buf);
int PMPI_T_pvar_reset(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_session_create(MPI_T_pvar_session *session);
int PMPI_T_pvar_session_free(MPI_T_pvar_session *session);
int PMPI_T_pvar_start(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_stop(MPI_T_pvar_session session, MPI_T_pvar_handle handle);
int PMPI_T_pvar_write(MPI_T_pvar_session session, MPI_T_pvar_handle handle, const void *buf);
int PMPI_T_source_get_info(int source_index, char *name, int *name_len, char *desc, int *desc_len, MPI_T_source_order *ordering, MPI_Count *ticks_per_second, MPI_Count *max_ticks, MPI_Info *info);
int PMPI_T_source_get_num(int *num_sources);
int PMPI_T_source_get_timestamp(int source_index, MPI_Count *timestamp);

#if defined(__cplusplus)
}
#endif

#endif  /* MPI_H_ABI */
