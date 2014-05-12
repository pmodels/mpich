! ! MPI 3 Data cosnatnts for Fortran.
! From A.1.1
! The module also contains the conversion routines for mpi_status
!
!--------------------------------------------------------------

module mpi_f08_compile_constants

use,intrinsic :: iso_c_binding, only: c_int, c_ptr
use,intrinsic :: iso_fortran_env, only: int32, int64, real32, real64
use :: mpi_f08_types
use :: mpi_c_interface_types, only: c_Aint, c_Count, c_Offset, c_Status

!====================================================================
! Make names brought in from other modules private if they should not
! be exposed in program units using this module
!====================================================================

! Make names from iso_c_binding private
private :: c_int

! Make names from iso_fortran_env private
private :: int32
private :: int64
private :: real32
private :: real64

! Make nmes from mpi_C_types private
private :: c_Aint
private :: c_Count
private :: c_Offset

!=========================================
! Definitions of MPI Constants for Fortran
! Should match the values in  mpif.h
!==========================================

! Implementation Information
! 8.1.1 p335
integer,parameter :: MPI_VERSION    = 3
integer,parameter :: MPI_SUBVERSION = 0


! Null Handles
! A.1.1 p669

type(MPI_Group),parameter      :: MPI_GROUP_NULL      = MPI_Group(134217728 )      ! 0x08000000
type(MPI_Comm),parameter       :: MPI_COMM_NULL       = MPI_Comm(67108864 )        ! 0x04000000
type(MPI_Datatype),parameter   :: MPI_DATATYPE_NULL   = MPI_Datatype(  201326592)  ! 0x0c000000
type(MPI_Request),parameter    :: MPI_REQUEST_NULL    = MPI_Request(738197504)     ! 0x2c000000
type(MPI_Op),parameter         :: MPI_OP_NULL         = MPI_Op(  402653184)        ! 0x18000000
type(MPI_Errhandler),parameter :: MPI_ERRHANDLER_NULL = MPI_Errhandler( 335544320) ! 0x14000000
type(MPI_File),parameter       :: MPI_FILE_NULL       = MPI_File(0)                ! 0x00000000
type(MPI_Info),parameter       :: MPI_INFO_NULL       = MPI_Info(469762048)        ! 0x1c000000
type(MPI_Win),parameter        :: MPI_WIN_NULL        = MPI_Win(  536870912)       ! 0x20000000
type(MPI_Message),parameter    :: MPI_MESSAGE_NULL    = MPI_Message(738197504)     ! 0x2c000000
! The component value for MPI_MESSAGE_NULL is the same as that for MPI_REQUEST_NULL

! Error Classes
! A.1.1, pp 661 - 663
integer,parameter ::  MPI_SUCCESS          = 0
integer,parameter ::  MPI_ERR_BUFFER       = 1
integer,parameter ::  MPI_ERR_COUNT        = 2
integer,parameter ::  MPI_ERR_TYPE         = 3
integer,parameter ::  MPI_ERR_TAG          = 4
integer,parameter ::  MPI_ERR_COMM         = 5
integer,parameter ::  MPI_ERR_RANK         = 6
integer,parameter ::  MPI_ERR_REQUEST      = 19
integer,parameter ::  MPI_ERR_ROOT         = 7
integer,parameter ::  MPI_ERR_GROUP        = 8
integer,parameter ::  MPI_ERR_OP           = 9
integer,parameter ::  MPI_ERR_TOPOLOGY     = 10
integer,parameter ::  MPI_ERR_DIMS         = 11
integer,parameter ::  MPI_ERR_ARG          = 12
integer,parameter ::  MPI_ERR_UNKNOWN      = 13
integer,parameter ::  MPI_ERR_TRUNCATE     = 14
integer,parameter ::  MPI_ERR_OTHER        = 15
integer,parameter ::  MPI_ERR_INTERN       = 16
integer,parameter ::  MPI_ERR_PENDING      = 18
integer,parameter ::  MPI_ERR_IN_STATUS    = 17
integer,parameter ::  MPI_ERR_ACCESS       = 20
integer,parameter ::  MPI_ERR_AMODE        = 21
integer,parameter ::  MPI_ERR_ASSERT       = 53
integer,parameter ::  MPI_ERR_BAD_FILE     = 22
integer,parameter ::  MPI_ERR_BASE         = 46
integer,parameter ::  MPI_ERR_CONVERSION   = 23
integer,parameter ::  MPI_ERR_DISP         = 52
integer,parameter ::  MPI_ERR_DUP_DATAREP  = 24
integer,parameter ::  MPI_ERR_FILE_EXISTS  = 25
integer,parameter ::  MPI_ERR_FILE_IN_USE  = 26
integer,parameter ::  MPI_ERR_FILE         = 27
integer,parameter ::  MPI_ERR_INFO_KEY     = 29
integer,parameter ::  MPI_ERR_INFO_NOKEY   = 31
integer,parameter ::  MPI_ERR_INFO_VALUE   = 30
integer,parameter ::  MPI_ERR_INFO         = 28
integer,parameter ::  MPI_ERR_IO           = 32
integer,parameter ::  MPI_ERR_KEYVAL       = 48
integer,parameter ::  MPI_ERR_LOCKTYPE     = 47
integer,parameter ::  MPI_ERR_NAME         = 33
integer,parameter ::  MPI_ERR_NO_MEM       = 34
integer,parameter ::  MPI_ERR_NOT_SAME     = 35
integer,parameter ::  MPI_ERR_NO_SPACE     = 36
integer,parameter ::  MPI_ERR_NO_SUCH_FILE = 37
integer,parameter ::  MPI_ERR_PORT         = 38
integer,parameter ::  MPI_ERR_QUOTA        = 39
integer,parameter ::  MPI_ERR_READ_ONLY    = 40
integer,parameter ::  MPI_ERR_RMA_ATTACH   = 56
integer,parameter ::  MPI_ERR_RMA_CONFLICT = 49
integer,parameter ::  MPI_ERR_RMA_RANGE    = 55
integer,parameter ::  MPI_ERR_RMA_SHARED   = 57
integer,parameter ::  MPI_ERR_RMA_SYNC     = 50
integer,parameter ::  MPI_ERR_RMA_FLAVOR   = 58
integer,parameter ::  MPI_ERR_SERVICE      = 41
integer,parameter ::  MPI_ERR_SIZE         = 51
integer,parameter ::  MPI_ERR_SPAWN        = 42
integer,parameter ::  MPI_ERR_UNSUPPORTED_DATAREP  = 43
integer,parameter ::  MPI_ERR_UNSUPPORTED_OPERATION  = 44
integer,parameter ::  MPI_ERR_WIN          = 45
integer,parameter ::  MPI_ERR_LASTCODE     = 1073741823

! Assorted Constants
! A.1.1 p. 663
integer,parameter ::  MPI_PROC_NULL      = -1
integer,parameter ::  MPI_ANY_SOURCE     = -2
integer,parameter ::  MPI_ANY_TAG        = -1
integer,parameter ::  MPI_UNDEFINED      = -32766
integer,parameter ::  MPI_BSEND_OVERHEAD = 96
integer,parameter ::  MPI_KEYVAL_INVALID = 603979776
integer,parameter ::  MPI_LOCK_EXCLUSIVE = 234
integer,parameter ::  MPI_LOCK_SHARED    = 235
integer,parameter ::  MPI_ROOT           = -3

! No Process Message Handle
! A.1.1 p. 663
type(MPI_Message),parameter    :: MPI_MESSAGE_NO_PROC = MPI_Message(1811939328)

! Fortran Support Method Specific Constants
! A.1.1 p. 664
logical,parameter :: MPI_SUBARRAYS_SUPPORTED        = .true.
logical,parameter :: MPI_ASYNC_PROTECTS_NONBLOCKING = .true.  ! Value differs from mpif.h

! Variable Address Size
! A.1.1 p. 664
integer,parameter :: MPI_ADDRESS_KIND = c_Aint     ! Defined in mpi_c_interface_types
integer,parameter :: MPI_COUNT_KIND   = c_Count    ! Defined in mpi_c_interface_types
integer,parameter :: MPI_INTEGER_KIND = KIND(0)
integer,parameter :: MPI_OFFSET_KIND  = c_Offset   ! Defined in mpi_c_interface_types

! Error Handling Specifiers
! A.1.1 p. 664
type(MPI_Errhandler),parameter :: MPI_ERRORS_ARE_FATAL = MPI_Errhandler(1409286144)
type(MPI_Errhandler),parameter :: MPI_ERRORS_RETURN    = MPI_Errhandler(1409286145)

! Maximum Sizes for Strings
! A.1.1 p. 664
integer,parameter ::  MPI_MAX_DATAREP_STRING         = 127
integer,parameter ::  MPI_MAX_ERROR_STRING           = 1023
integer,parameter ::  MPI_MAX_INFO_KEY               = 254
integer,parameter ::  MPI_MAX_INFO_VAL               = 1023
integer,parameter ::  MPI_MAX_LIBRARY_VERSION_STRING = 8191
integer,parameter ::  MPI_MAX_OBJECT_NAME            = 127
integer,parameter ::  MPI_MAX_PORT_NAME              = 255
integer,parameter ::  MPI_MAX_PROCESSOR_NAME         = 128-1


! Named Predefined Datatypes - C types
! A.1.1 pp 665 - 666
type(MPI_Datatype),parameter :: MPI_CHAR                   = MPI_Datatype( 1275068673) ! 0x4c000101
type(MPI_Datatype),parameter :: MPI_SHORT                  = MPI_Datatype( 1275068931) ! 0x4c000203
type(MPI_Datatype),parameter :: MPI_INT                    = MPI_Datatype( 1275069445) ! 0x4c000405
type(MPI_Datatype),parameter :: MPI_LONG                   = MPI_Datatype( 1275070471) ! 0x4c000807
type(MPI_Datatype),parameter :: MPI_LONG_LONG_INT          = MPI_Datatype( 1275070473) ! 0x4c000809
type(MPI_Datatype),parameter :: MPI_LONG_LONG              = MPI_LONG_LONG_INT
type(MPI_Datatype),parameter :: MPI_SIGNED_CHAR            = MPI_Datatype( 1275068696) ! 0x4c000118
type(MPI_Datatype),parameter :: MPI_UNSIGNED_CHAR          = MPI_Datatype( 1275068674) ! 0x4c000102
type(MPI_Datatype),parameter :: MPI_UNSIGNED_SHORT         = MPI_Datatype( 1275068932) ! 0x4c000204
type(MPI_Datatype),parameter :: MPI_UNSIGNED               = MPI_Datatype( 1275069446) ! 0x4c000406
type(MPI_Datatype),parameter :: MPI_UNSIGNED_LONG          = MPI_Datatype( 1275070472) ! 0x4c000808
type(MPI_Datatype),parameter :: MPI_UNSIGNED_LONG_LONG     = MPI_Datatype( 1275070489) ! 0x4c000819
type(MPI_Datatype),parameter :: MPI_FLOAT                  = MPI_Datatype( 1275069450) ! 0x4c00040a
type(MPI_Datatype),parameter :: MPI_DOUBLE                 = MPI_Datatype( 1275070475) ! 0x4c00080b
type(MPI_Datatype),parameter :: MPI_LONG_DOUBLE            = MPI_Datatype( 1275072524) ! 0x4c00100c
type(MPI_Datatype),parameter :: MPI_WCHAR                  = MPI_Datatype( 1275069454) ! 0x4c00040e
type(MPI_Datatype),parameter :: MPI_C_BOOL                 = MPI_Datatype( 1275068735) ! 0x4c00013f
type(MPI_Datatype),parameter :: MPI_INT8_T                 = MPI_Datatype( 1275068727) ! 0x4c000137
type(MPI_Datatype),parameter :: MPI_INT16_T                = MPI_Datatype( 1275068984) ! 0x4c000238
type(MPI_Datatype),parameter :: MPI_INT32_T                = MPI_Datatype( 1275069497) ! 0x4c000439
type(MPI_Datatype),parameter :: MPI_INT64_T                = MPI_Datatype( 1275070522) ! 0x4c00083a
type(MPI_Datatype),parameter :: MPI_UINT8_T                = MPI_Datatype( 1275068731) ! 0x4c00013b
type(MPI_Datatype),parameter :: MPI_UINT16_T               = MPI_Datatype( 1275068988) ! 0x4c00023c
type(MPI_Datatype),parameter :: MPI_UINT32_T               = MPI_Datatype( 1275069501) ! 0x4c00043d
type(MPI_Datatype),parameter :: MPI_UINT64_T               = MPI_Datatype( 1275070526) ! 0x4c00083e
type(MPI_Datatype),parameter :: MPI_AINT                   = MPI_Datatype( 1275070531) ! 0x4c000843
type(MPI_Datatype),parameter :: MPI_COUNT                  = MPI_Datatype( 1275070533) ! 0x4c000845
type(MPI_Datatype),parameter :: MPI_OFFSET                 = MPI_Datatype( 1275070532) ! 0x4c000844
type(MPI_Datatype),parameter :: MPI_C_FLOAT_COMPLEX        = MPI_Datatype( 1275070528) ! 0x4c000840
type(MPI_Datatype),parameter :: MPI_C_COMPLEX              = MPI_C_FLOAT_COMPLEX
type(MPI_Datatype),parameter :: MPI_C_DOUBLE_COMPLEX       = MPI_Datatype( 1275072577) ! 0x4c001041
type(MPI_Datatype),parameter :: MPI_C_LONG_DOUBLE_COMPLEX  = MPI_Datatype( 1275076674) ! 0x4c002042
type(MPI_Datatype),parameter :: MPI_BYTE                   = MPI_Datatype( 1275068685) ! 0x4c00010d
type(MPI_Datatype),parameter :: MPI_PACKED                 = MPI_Datatype( 1275068687) ! 0x4c00010f


! Named Predefined Datatypes - Fortran types
! A.1.1 p. 666
integer,private,parameter :: dik = kind(0)     ! Default Integer Kind
integer,private,parameter :: drk = kind(0.0)   ! Default Real Kind
integer,private,parameter :: ddk = kind(0.0d0) ! Default Double Precision Kind

type(MPI_Datatype),parameter,private :: MPIx_C8       = MPI_Datatype( 1275070494) ! 0x4c00081e
type(MPI_Datatype),parameter,private :: MPIx_C16      = MPI_Datatype( 1275072546) ! 0x4c001022
type(MPI_Datatype),parameter,private :: MPIx_C32      = MPI_Datatype( 1275076652) ! 0x4c00202c
type(MPI_Datatype),parameter,private :: MPIx_2R4      = MPI_Datatype( 1275070497) ! 0x4c000821
type(MPI_Datatype),parameter,private :: MPIx_2R8      = MPI_Datatype( 1275072545) ! 0x4c001021
type(MPI_Datatype),parameter,private :: MPIx_L4       = MPI_Datatype( 1275069469) ! 0x4c00041d
type(MPI_Datatype),parameter,private :: MPIx_L8       = MPI_Datatype( 1275070493) ! 0x4c00081d
type(MPI_Datatype),parameter,private :: MPIx_R4       = MPI_Datatype( 1275069468) ! 0x4c00041c
type(MPI_Datatype),parameter,private :: MPIx_R8       = MPI_Datatype( 1275070495) ! 0x4c00081f
type(MPI_Datatype),parameter,private :: MPIx_R16      = MPI_Datatype( 1275072555) ! 0x4c00102b
type(MPI_Datatype),parameter,private :: MPIx_I4       = MPI_Datatype( 1275069467) ! 0x4c00041b
type(MPI_Datatype),parameter,private :: MPIx_I8       = MPI_Datatype( 1275070491) ! 0x4c00081b
type(MPI_Datatype),parameter,private :: MPIx_2I4      = MPI_Datatype( 1275070496) ! 0x4c000820
type(MPI_Datatype),parameter,private :: MPIx_2I8      = MPI_Datatype( 1275072544) ! 0x4c001020
type(MPI_Datatype),parameter :: MPI_INTEGER           = merge(MPIx_I4, MPIx_I8,  dik==int32)
type(MPI_Datatype),parameter :: MPI_REAL              = merge(MPIx_R4, MPIx_R8,  drk==real32)
type(MPI_Datatype),parameter :: MPI_DOUBLE_PRECISION  = merge(MPIx_R8, MPIx_R16, ddk==real64)
type(MPI_Datatype),parameter :: MPI_COMPLEX           = merge(MPIx_C8, MPIx_C16, drk==real32)
type(MPI_Datatype),parameter :: MPI_LOGICAL           = merge(MPIx_L4, MPIx_L8,  dik==int32)
type(MPI_Datatype),parameter :: MPI_CHARACTER         = MPI_Datatype( 1275068698) ! 0x4c00011a

! Named Predefined Datatypes - C++ types
! A.1.1 p. 666
type(MPI_Datatype),parameter :: MPI_CXX_BOOL                = MPI_Datatype( 1275068723) ! 0x4C000133
type(MPI_Datatype),parameter :: MPI_CXX_FLOAT_COMPLEX       = MPI_Datatype( 1275070516) ! 0x4C000834
type(MPI_Datatype),parameter :: MPI_CXX_DOUBLE_COMPLEX      = MPI_Datatype( 1275072565) ! 0x4C001035
type(MPI_Datatype),parameter :: MPI_CXX_LONG_DOUBLE_COMPLEX = MPI_Datatype( 1275076662) ! 0x4C002036

! Optional datatypes (Fortran)
! A.1.1 p. 666
type(MPI_Datatype),parameter :: MPI_DOUBLE_COMPLEX    = merge(MPIx_C16,MPIx_C32, ddk==real64)
type(MPI_Datatype),parameter :: MPI_INTEGER1          = MPI_Datatype( 1275068717) ! 0x4c00012d
type(MPI_Datatype),parameter :: MPI_INTEGER2          = MPI_Datatype( 1275068975) ! 0x4c00022f
type(MPI_Datatype),parameter :: MPI_INTEGER4          = MPI_Datatype( 1275069488) ! 0x4c000430
type(MPI_Datatype),parameter :: MPI_INTEGER8          = MPI_Datatype( 1275070513) ! 0x4c000831
type(MPI_Datatype),parameter :: MPI_INTEGER16         = MPI_DATATYPE_NULL
type(MPI_Datatype),parameter :: MPI_REAL2             = MPI_DATATYPE_NULL
type(MPI_Datatype),parameter :: MPI_REAL4             = MPI_Datatype( 1275069479) ! 0x4c000427
type(MPI_Datatype),parameter :: MPI_REAL8             = MPI_Datatype( 1275070505) ! 0x4c000829
type(MPI_Datatype),parameter :: MPI_REAL16            = MPI_Datatype( 1275072555) ! 0x4c00102b
type(MPI_Datatype),parameter :: MPI_COMPLEX4          = MPI_DATATYPE_NULL
type(MPI_Datatype),parameter :: MPI_COMPLEX8          = MPI_Datatype( 1275070504) ! 0x4c000828
type(MPI_Datatype),parameter :: MPI_COMPLEX16         = MPI_Datatype( 1275072554) ! 0x4c00102a
type(MPI_Datatype),parameter :: MPI_COMPLEX32         = MPI_Datatype( 1275076652) ! 0x4c00202c

! Datatypes for reduction functions (C)
! A.1.1 p. 667
type(MPI_Datatype),parameter :: MPI_FLOAT_INT         = MPI_Datatype(-1946157056) ! 0x8c000000
type(MPI_Datatype),parameter :: MPI_DOUBLE_INT        = MPI_Datatype(-1946157055) ! 0x8c000001
type(MPI_Datatype),parameter :: MPI_LONG_INT          = MPI_Datatype(-1946157054) ! 0x8c000002
type(MPI_Datatype),parameter :: MPI_2INT              = MPI_Datatype( 1275070486) ! 0x4c000816
type(MPI_Datatype),parameter :: MPI_SHORT_INT         = MPI_Datatype(-1946157053) ! 0x8c000003
type(MPI_Datatype),parameter :: MPI_LONG_DOUBLE_INT   = MPI_Datatype(-1946157052) ! 0x8c000004

! Datatypes for reduction functions (Fortran)
! A.1.1 p. 667
type(MPI_Datatype),parameter :: MPI_2REAL             = merge(MPIx_2R4, MPIx_2R8,drk==real32)
type(MPI_Datatype),parameter :: MPI_2DOUBLE_PRECISION = MPI_Datatype( 1275072547) ! 0x4c001023
type(MPI_Datatype),parameter :: MPI_2integer          = merge(MPIx_2I4, MPIx_2I8,dik==int32)
!!!!!! Seems like MPI_2DOUBLE_PRECISION not handled correctly here.
!!!!!! But the mpi.h.32 and mpi.h.64 had the same value.

! Reserved communicators
! A.1.1 p. 667
type(MPI_Comm),parameter  ::  MPI_COMM_WORLD  = MPI_Comm( 1140850688) ! 0x44000000
type(MPI_Comm),parameter  ::  MPI_COMM_SELF   = MPI_Comm( 1140850689) ! 0x44000001

! Communicator split type constants
! A.1.1 p. 667
integer,parameter :: MPI_COMM_TYPE_SHARED = 1

! Results of communicator and group comparisons
! A.1.1 p. 667
integer,parameter :: MPI_IDENT     = 0
integer,parameter :: MPI_CONGRUENT = 1
integer,parameter :: MPI_SIMILAR   = 2
integer,parameter :: MPI_UNEQUAL   = 3

! Environmental inquiry info key
! A.1.1 p. 667
type(MPI_Info),parameter :: MPI_INFO_ENV = MPI_Info(999922) ! **** NEED VALUE

! Environmental inquiry keys
! A.1.1 p. 668
integer,parameter ::  MPI_TAG_UB          = 1681915906 !  0x64400002
integer,parameter ::  MPI_HOST            = 1681915908 !  0x64400004
integer,parameter ::  MPI_IO              = 1681915910 !  0x64400006
integer,parameter ::  MPI_WTIME_IS_GLOBAL = 1681915912 !  0x64400008

! Collective operations
! A.1.1 p. 668
type(MPI_Op),parameter :: MPI_MAX     = MPI_Op( 1476395009) ! 0x58000001
type(MPI_Op),parameter :: MPI_MIN     = MPI_Op( 1476395010) ! 0x58000002
type(MPI_Op),parameter :: MPI_SUM     = MPI_Op( 1476395011) ! 0x58000003
type(MPI_Op),parameter :: MPI_PROD    = MPI_Op( 1476395012) ! 0x58000004
type(MPI_Op),parameter :: MPI_MAXLOC  = MPI_Op( 1476395020) ! 0x5800000c
type(MPI_Op),parameter :: MPI_MINLOC  = MPI_Op( 1476395019) ! 0x5800000b
type(MPI_Op),parameter :: MPI_BAND    = MPI_Op( 1476395014) ! 0x58000006
type(MPI_Op),parameter :: MPI_BOR     = MPI_Op( 1476395016) ! 0x58000008
type(MPI_Op),parameter :: MPI_BXOR    = MPI_Op( 1476395018) ! 0x5800000a
type(MPI_Op),parameter :: MPI_LAND    = MPI_Op( 1476395013) ! 0x58000005
type(MPI_Op),parameter :: MPI_LOR     = MPI_Op( 1476395015) ! 0x58000007
type(MPI_Op),parameter :: MPI_LXOR    = MPI_Op( 1476395017) ! 0x58000009
type(MPI_Op),parameter :: MPI_REPLACE = MPI_Op( 1476395021) ! 0x5800000d
type(MPI_Op),parameter :: MPI_NO_OP   = MPI_Op( 1476395022) ! 0x5800000e

! Empty group
! A.1.1 p 669
type(MPI_Group),parameter :: MPI_GROUP_EMPTY  = MPI_Group( 1207959552) ! 0x48000000

! Topologies
! A.1.1 p 669
integer,parameter :: MPI_GRAPH      = 1
integer,parameter :: MPI_CART       = 2
integer,parameter :: MPI_DIST_GRAPH = 3

! Predefined Attribute Keys
! A.1.1 p 671
integer,parameter :: MPI_APPNUM            = 1681915918
integer,parameter :: MPI_LASTUSEDCODE      = 1681915916
integer,parameter :: MPI_UNIVERSE_SIZE     = 1681915914
integer,parameter :: MPI_WIN_BASE          = 1711276034
integer,parameter :: MPI_WIN_DISP_UNIT     = 1711276038
integer,parameter :: MPI_WIN_SIZE          = 1711276036
integer,parameter :: MPI_WIN_CREATE_FLAVOR = 1711276040
integer,parameter :: MPI_WIN_MODEL         = 1711276042

! MPI Window Create Flavors
! A.1.1 p 671
integer,parameter :: MPI_WIN_FLAVOR_CREATE   = 1
integer,parameter :: MPI_WIN_FLAVOR_ALLOCATE = 2
integer,parameter :: MPI_WIN_FLAVOR_DYNAMIC  = 3
integer,parameter :: MPI_WIN_FLAVOR_SHARED   = 4

! MPI Window Models
! A.1.1 p 671
integer,parameter :: MPI_WIN_SEPARATE = 1
integer,parameter :: MPI_WIN_UNIFIED  = 2

! Mode Constants
! A.1.1 p 672
integer,parameter :: MPI_MODE_APPEND          = 128
integer,parameter :: MPI_MODE_CREATE          = 1
integer,parameter :: MPI_MODE_DELETE_ON_CLOSE = 16
integer,parameter :: MPI_MODE_EXCL            = 64
integer,parameter :: MPI_MODE_NOCHECK         = 1024
integer,parameter :: MPI_MODE_NOPRECEDE       = 8192
integer,parameter :: MPI_MODE_NOPUT           = 4096
integer,parameter :: MPI_MODE_NOSTORE         = 2048
integer,parameter :: MPI_MODE_NOSUCCEED       = 16384
integer,parameter :: MPI_MODE_RDONLY          = 2
integer,parameter :: MPI_MODE_RDWR            = 8
integer,parameter :: MPI_MODE_SEQUENTIAL      = 256
integer,parameter :: MPI_MODE_UNIQUE_OPEN     = 32
integer,parameter :: MPI_MODE_WRONLY          = 4

! Datatype Decoding Constants
! A.1.1 p 672
integer,parameter :: MPI_COMBINER_NAMED             = 1
integer,parameter :: MPI_COMBINER_DUP               = 2
integer,parameter :: MPI_COMBINER_CONTIGUOUS        = 3
integer,parameter :: MPI_COMBINER_VECTOR            = 4
integer,parameter :: MPI_COMBINER_HVECTOR_INTEGER   = 5
integer,parameter :: MPI_COMBINER_HVECTOR           = 6
integer,parameter :: MPI_COMBINER_INDEXED           = 7
integer,parameter :: MPI_COMBINER_HINDEXED_INTEGER  = 8
integer,parameter :: MPI_COMBINER_HINDEXED          = 9
integer,parameter :: MPI_COMBINER_INDEXED_BLOCK     = 10
integer,parameter :: MPI_COMBINER_STRUCT_INTEGER    = 11
integer,parameter :: MPI_COMBINER_STRUCT            = 12
integer,parameter :: MPI_COMBINER_SUBARRAY          = 13
integer,parameter :: MPI_COMBINER_DARRAY            = 14
integer,parameter :: MPI_COMBINER_F90_REAL          = 15
integer,parameter :: MPI_COMBINER_F90_COMPLEX       = 16
integer,parameter :: MPI_COMBINER_F90_INTEGER       = 17
integer,parameter :: MPI_COMBINER_RESIZED           = 18
integer,parameter :: MPI_COMBINER_HINDEXED_BLOCK    = 19

! Threads Constants
! A.1.1 p 672
integer,parameter :: MPI_THREAD_SINGLE     = 0
integer,parameter :: MPI_THREAD_FUNNELED   = 1
integer,parameter :: MPI_THREAD_SERIALIZED = 2
integer,parameter :: MPI_THREAD_MULTIPLE   = 3

! File Operation Constants, Part 1
! A.1.1 p 673
integer(mpi_offset_kind),parameter :: MPI_DISPLACEMENT_CURRENT = -54278278

! File Operation Constants, Part 2
! A.1.1 p 673
integer,parameter :: MPI_DISTRIBUTE_BLOCK     = 121
integer,parameter :: MPI_DISTRIBUTE_CYCLIC    = 122
integer,parameter :: MPI_DISTRIBUTE_DFLT_DARG = -49767
integer,parameter :: MPI_DISTRIBUTE_NONE      = 123
integer,parameter :: MPI_ORDER_C              = 56
integer,parameter :: MPI_ORDER_FORTRAN        = 57
integer,parameter :: MPI_SEEK_CUR             = 602
integer,parameter :: MPI_SEEK_END             = 604
integer,parameter :: MPI_SEEK_SET             = 600

! F90 Datatype Matching Cosntants
! A.1.1 p 673
integer,parameter :: MPI_TYPECLASS_COMPLEX = 3
integer,parameter :: MPI_TYPECLASS_integer = 2
integer,parameter :: MPI_TYPECLASS_REAL    = 1

! MPI_T_* names
! A.1.1 pp 674-676
! The MPI_T_* names in the tables on pages 674-676 are for C only
! and are omitted here

!=============================================

! Added constants defined in mpich 3 mpif.h file
! These are removed - See table 2.1, page 18 in the spec.
! but kept here for backward compatibility
type(MPI_Datatype),parameter :: MPI_LB                     = MPI_Datatype( 1275068432) ! 0x4c000010
type(MPI_Datatype),parameter :: MPI_UB                     = MPI_Datatype( 1275068433) ! 0x4c000011

! Additional reduction function data types defined by mpich 3, not in spec
type(MPI_Datatype),parameter :: MPI_2COMPLEX        = MPI_Datatype( 1275072548) ! 0x4C002025
type(MPI_Datatype),parameter :: MPI_2DOUBLE_COMPLEX = MPI_Datatype( 1275076645) ! 0x4C001024
!!!!!! Similar mishandling here for -r8 case.


END MODULE mpi_f08_compile_constants

