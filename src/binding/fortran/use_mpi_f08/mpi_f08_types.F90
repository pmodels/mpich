! MPI 3 Data types for Fortran.
! From A.1.2, pp 676-677
! Note added MPI_Message type not in the spec by mistake
!
!--------------------------------------------------------------

module mpi_f08_types
use, intrinsic :: iso_c_binding, only: c_int
use :: mpi_c_interface_types, only: c_Count, c_Status

implicit none

private :: c_int, c_Count, c_Status

! F08 derived types
! A.1.2  p 677

type, bind(C) :: MPI_Comm
    integer :: MPI_VAL
end type MPI_Comm

type, bind(C) :: MPI_Datatype
    integer :: MPI_VAL
end type MPI_Datatype

type, bind(C) :: MPI_Errhandler
    integer :: MPI_VAL
end type MPI_Errhandler

type, bind(C) :: MPI_File
    integer :: MPI_VAL
end type MPI_File

type, bind(C) :: MPI_Group
    integer :: MPI_VAL
end type MPI_Group

type, bind(C) :: MPI_Info
    integer :: MPI_VAL
end type MPI_Info

type, bind(C) :: MPI_Op
    integer :: MPI_VAL
end type MPI_Op

type, bind(C) :: MPI_Request
    integer :: MPI_VAL
end type MPI_Request

type, bind(C) :: MPI_Win
    integer :: MPI_VAL
end type MPI_Win

type, bind(C) :: MPI_Message
    integer :: MPI_VAL
end type MPI_Message

! Fortran 2008 struct for status. Must be consistent with mpi.h, mpif.h
type, bind(C) :: MPI_Status
    integer :: count_lo;
    integer :: count_hi_and_cancelled;
    integer :: MPI_SOURCE
    integer :: MPI_TAG
    integer :: MPI_ERROR
end type MPI_Status

! Fortran subscript constants
! 3.2.5  p 30, and A.1.1 p 664
integer,parameter :: MPI_SOURCE = 3
integer,parameter :: MPI_TAG    = 4
integer,parameter :: MPI_ERROR  = 5
integer,parameter :: MPI_STATUS_SIZE = 5

interface assignment(=)
    module procedure mpi_status_f08_assgn_c
    module procedure mpi_status_c_assgn_f08
end interface

private :: mpi_status_f08_assgn_c
private :: mpi_status_c_assgn_f08
private :: mpi_status_f_assgn_c
private :: mpi_status_c_assgn_f

! Required operator overloads for == and /= for opaque handles
! 2.5.1 pp 12-13

interface operator(==)
    module procedure mpi_comm_eq
    module procedure mpi_datatype_eq
    module procedure mpi_errhandler_eq
    module procedure mpi_file_eq
    module procedure mpi_group_eq
    module procedure mpi_info_eq
    module procedure mpi_op_eq
    module procedure mpi_request_eq
    module procedure mpi_win_eq
    module procedure mpi_message_eq

    module procedure mpi_comm_f08_eq_f
    module procedure mpi_comm_f_eq_f08
    module procedure mpi_datatype_f08_eq_f
    module procedure mpi_datatype_f_eq_f08
    module procedure mpi_errhandler_f08_eq_f
    module procedure mpi_errhandler_f_eq_f08
    module procedure mpi_file_f08_eq_f
    module procedure mpi_file_f_eq_f08
    module procedure mpi_group_f08_eq_f
    module procedure mpi_group_f_eq_f08
    module procedure mpi_info_f08_eq_f
    module procedure mpi_info_f_eq_f08
    module procedure mpi_op_f08_eq_f
    module procedure mpi_op_f_eq_f08
    module procedure mpi_request_f08_eq_f
    module procedure mpi_request_f_eq_f08
    module procedure mpi_win_f08_eq_f
    module procedure mpi_win_f_eq_f08
    module procedure mpi_message_f08_eq_f
    module procedure mpi_message_f_eq_f08
end interface

private :: mpi_comm_eq
private :: mpi_datatype_eq
private :: mpi_errhandler_eq
private :: mpi_file_eq
private :: mpi_group_eq
private :: mpi_info_eq
private :: mpi_op_eq
private :: mpi_request_eq
private :: mpi_win_eq
private :: mpi_message_eq

private :: mpi_comm_f08_eq_f
private :: mpi_comm_f_eq_f08
private :: mpi_datatype_f08_eq_f
private :: mpi_datatype_f_eq_f08
private :: mpi_errhandler_f08_eq_f
private :: mpi_errhandler_f_eq_f08
private :: mpi_file_f08_eq_f
private :: mpi_file_f_eq_f08
private :: mpi_group_f08_eq_f
private :: mpi_group_f_eq_f08
private :: mpi_info_f08_eq_f
private :: mpi_info_f_eq_f08
private :: mpi_op_f08_eq_f
private :: mpi_op_f_eq_f08
private :: mpi_request_f08_eq_f
private :: mpi_request_f_eq_f08
private :: mpi_win_f08_eq_f
private :: mpi_win_f_eq_f08
private :: mpi_message_f08_eq_f
private :: mpi_message_f_eq_f08

interface operator(/=)
    module procedure mpi_comm_neq
    module procedure mpi_datatype_neq
    module procedure mpi_errhandler_neq
    module procedure mpi_file_neq
    module procedure mpi_group_neq
    module procedure mpi_info_neq
    module procedure mpi_op_neq
    module procedure mpi_request_neq
    module procedure mpi_win_neq
    module procedure mpi_message_neq

    module procedure mpi_comm_f08_ne_f
    module procedure mpi_comm_f_ne_f08
    module procedure mpi_datatype_f08_ne_f
    module procedure mpi_datatype_f_ne_f08
    module procedure mpi_errhandler_f08_ne_f
    module procedure mpi_errhandler_f_ne_f08
    module procedure mpi_file_f08_ne_f
    module procedure mpi_file_f_ne_f08
    module procedure mpi_group_f08_ne_f
    module procedure mpi_group_f_ne_f08
    module procedure mpi_info_f08_ne_f
    module procedure mpi_info_f_ne_f08
    module procedure mpi_op_f08_ne_f
    module procedure mpi_op_f_ne_f08
    module procedure mpi_request_f08_ne_f
    module procedure mpi_request_f_ne_f08
    module procedure mpi_win_f08_ne_f
    module procedure mpi_win_f_ne_f08
    module procedure mpi_message_f08_ne_f
    module procedure mpi_message_f_ne_f08
end interface

private :: mpi_comm_neq
private :: mpi_datatype_neq
private :: mpi_errhandler_neq
private :: mpi_file_neq
private :: mpi_group_neq
private :: mpi_info_neq
private :: mpi_op_neq
private :: mpi_request_neq
private :: mpi_win_neq
private :: mpi_message_neq

private :: mpi_comm_f08_ne_f
private :: mpi_comm_f_ne_f08
private :: mpi_datatype_f08_ne_f
private :: mpi_datatype_f_ne_f08
private :: mpi_errhandler_f08_ne_f
private :: mpi_errhandler_f_ne_f08
private :: mpi_file_f08_ne_f
private :: mpi_file_f_ne_f08
private :: mpi_group_f08_ne_f
private :: mpi_group_f_ne_f08
private :: mpi_info_f08_ne_f
private :: mpi_info_f_ne_f08
private :: mpi_op_f08_ne_f
private :: mpi_op_f_ne_f08
private :: mpi_request_f08_ne_f
private :: mpi_request_f_ne_f08
private :: mpi_win_f08_ne_f
private :: mpi_win_f_ne_f08
private :: mpi_message_f08_ne_f
private :: mpi_message_f_ne_f08


! MPI_Sizeof in 17.1.9

interface MPI_Sizeof
    module procedure MPI_Sizeof_character
    module procedure MPI_Sizeof_logical

    module procedure MPI_Sizeof_xint8
    module procedure MPI_Sizeof_xint16
    module procedure MPI_Sizeof_xint32
    module procedure MPI_Sizeof_xint64
    module procedure MPI_Sizeof_xreal32
    module procedure MPI_Sizeof_xreal64
    module procedure MPI_Sizeof_xreal128
    module procedure MPI_Sizeof_xcomplex32
    module procedure MPI_Sizeof_xcomplex64
    module procedure MPI_Sizeof_xcomplex128
end interface

private :: MPI_Sizeof_character
private :: MPI_Sizeof_logical

private :: MPI_Sizeof_xint8
private :: MPI_Sizeof_xint16
private :: MPI_Sizeof_xint32
private :: MPI_Sizeof_xint64
private :: MPI_Sizeof_xreal32
private :: MPI_Sizeof_xreal64
private :: MPI_Sizeof_xreal128
private :: MPI_Sizeof_xcomplex32
private :: MPI_Sizeof_xcomplex64
private :: MPI_Sizeof_xcomplex128

contains

!--> MPI_Sizeof in 17.1.9,  specifics

subroutine MPI_Sizeof_character (x, size, ierror)
    character, dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_character

subroutine MPI_Sizeof_logical (x, size, ierror)
    logical, dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_logical

subroutine MPI_Sizeof_xint8 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: int8
    integer(int8),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_xint8

subroutine MPI_Sizeof_xint16 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: int16
    integer(int16),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_xint16

subroutine MPI_Sizeof_xint32 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: int32
    integer(int32),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_xint32

subroutine MPI_Sizeof_xint64 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: int64
    integer(int64),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    if (present(ierror)) ierror = 0
end subroutine MPI_Sizeof_xint64

subroutine MPI_Sizeof_xreal32 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real32, int32
    real(real32),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xreal32

subroutine MPI_Sizeof_xreal64 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real64
    real(real64),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xreal64

subroutine MPI_Sizeof_xreal128 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real128
    real(real128),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xreal128

subroutine MPI_Sizeof_xcomplex32 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real32
    complex(real32),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xcomplex32

subroutine MPI_Sizeof_xcomplex64 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real64
    complex(real64),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xcomplex64

subroutine MPI_Sizeof_xcomplex128 (x, size, ierror)
    use,intrinsic :: iso_fortran_env, only: real128
    complex(real128),dimension(..) :: x
    integer, intent(out) :: size
    integer, optional,  intent(out) :: ierror

    size = storage_size(x)/8
    ierror = 0
end subroutine MPI_Sizeof_xcomplex128

elemental subroutine mpi_status_f08_assgn_c (status_f08, status_c)
    ! Defined status_f08 = status_c
    type(MPI_Status),intent(out) :: status_f08
    type(c_Status),intent(in)    :: status_c

    status_f08%count_lo   = status_c%count_lo
    status_f08%count_hi_and_cancelled  = status_c%count_hi_and_cancelled
    status_f08%mpi_source = status_c%mpi_source
    status_f08%mpi_tag    = status_c%mpi_tag
    status_f08%mpi_error  = status_c%mpi_error
end subroutine mpi_status_f08_assgn_c

elemental subroutine mpi_status_c_assgn_f08 (status_c, status_f08)
    ! Defined status_c = status_f08
    type(c_Status),intent(out) :: status_c
    type(MPI_Status),intent(in) :: status_f08

    status_c%count_lo   = status_f08%count_lo
    status_c%count_hi_and_cancelled  = status_f08%count_hi_and_cancelled
    status_c%mpi_source = status_f08%mpi_source
    status_c%mpi_tag    = status_f08%mpi_tag
    status_c%mpi_error  = status_f08%mpi_error
end subroutine mpi_status_c_assgn_f08

subroutine mpi_status_f_assgn_c (status_f, status_c)
    ! Defined status_f = status_c
    use,intrinsic :: iso_fortran_env, only: int32
    integer,intent(out) :: status_f(MPI_STATUS_SIZE)
    type (c_Status),intent(in) :: status_c

    status_f(1) = status_c%count_lo
    status_f(2) = status_c%count_hi_and_cancelled
    status_f(MPI_SOURCE) = status_c%mpi_source
    status_f(MPI_TAG)    = status_c%mpi_tag
    status_f(MPI_ERROR)  = status_c%mpi_error
end subroutine mpi_status_f_assgn_c

subroutine mpi_status_c_assgn_f (status_c, status_f)
    ! Defined status_c = status_f
    use,intrinsic :: iso_fortran_env, only: int32
    integer,intent(in) :: status_f(MPI_STATUS_SIZE)
    integer(C_count) :: cnt
    type(c_Status),intent(out) :: status_c

    status_c%count_lo   = status_f(1);
    status_c%count_hi_and_cancelled  = status_f(2);
    status_c%mpi_source = status_f(MPI_SOURCE)
    status_c%mpi_tag    = status_f(MPI_TAG)
    status_c%mpi_error  = status_f(MPI_ERROR)
end subroutine mpi_status_c_assgn_f

! int MPI_Status_f082c(const MPI_F08_status *status_f08, MPI_Status *status_c)

function MPI_Status_f082c (status_f08, status_c) &
          bind(C, name="MPI_Status_f082c") result (res)
    use,intrinsic :: iso_c_binding, only: c_int
    type(MPI_Status),intent(in):: status_f08
    type(c_Status),intent(out) :: status_c
    integer(c_int)             :: res

    status_c = status_f08
    res = 0
end function MPI_Status_f082c

function PMPI_Status_f082c (status_f08, status_c) &
            bind(C, name="PMPI_Status_f082c") result (res)
    use,intrinsic :: iso_c_binding, only: c_int
    type(MPI_Status),intent(in):: status_f08
    type(c_Status),intent(out) :: status_c
    integer(c_int)             :: res

    status_c = status_f08
    res = 0
end function PMPI_Status_f082c

! int MPI_Status_c2f08(const MPI_Status *status_c, MPI_F08_status *status_f08)

function MPI_Status_c2f08 (status_c, status_f08) &
              bind(C, name="MPI_Status_c2f08") result (res)
    use,intrinsic :: iso_c_binding, only: c_int
    type(MPI_Status),intent(in):: status_c
    type(c_Status),intent(out) :: status_f08
    integer(c_int)             :: res

    status_f08 = status_c
    res = 0
end function MPI_Status_c2f08

function PMPI_Status_c2f08 (status_c, status_f08) &
              bind(C, name="PMPI_Status_c2f08") result (res)
    use,intrinsic :: iso_c_binding, only: c_int
    type(MPI_Status),intent(in):: status_c
    type(c_Status),intent(out) :: status_f08
    integer(c_int)             :: res

    status_f08 = status_c
    res = 0
end function PMPI_Status_c2f08

function mpi_comm_eq (x, y) result(res)
    type(MPI_Comm), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_comm_eq

function mpi_datatype_eq (x, y) result(res)
    type(MPI_Datatype), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_datatype_eq

function mpi_errhandler_eq (x, y) result(res)
    type(MPI_Errhandler), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_errhandler_eq

function mpi_file_eq (x, y) result(res)
    type(MPI_File), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_File_eq

function mpi_group_eq (x, y) result(res)
    type(MPI_Group), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_group_eq

function mpi_info_eq (x, y) result(res)
    type(MPI_Info), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_info_eq

function mpi_op_eq (x, y) result(res)
    type(MPI_Op), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_op_eq

function mpi_request_eq (x, y) result(res)
    type(MPI_Request), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_request_eq

function mpi_win_eq (x, y) result(res)
    type(MPI_Win), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_win_eq

function mpi_message_eq (x, y) result(res)
    type(MPI_Message), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function mpi_message_eq

function mpi_comm_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_comm_f08_eq_f

function mpi_comm_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_comm_f_eq_f08

function mpi_datatype_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_datatype_f08_eq_f

function mpi_datatype_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_datatype_f_eq_f08

function mpi_errhandler_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_errhandler_f08_eq_f

function mpi_errhandler_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_errhandler_f_eq_f08

function mpi_file_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_file_f_eq_f08

function mpi_file_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_file_f08_eq_f

function mpi_group_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_group_f08_eq_f

function mpi_group_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_group_f_eq_f08

function mpi_info_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_info_f08_eq_f

function mpi_info_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_info_f_eq_f08

function mpi_op_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_op_f08_eq_f

function mpi_op_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_op_f_eq_f08

function mpi_request_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_request_f08_eq_f

function mpi_request_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_request_f_eq_f08

function mpi_win_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_win_f08_eq_f

function mpi_win_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_win_f_eq_f08

function mpi_message_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_message_f08_eq_f

function mpi_message_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function mpi_message_f_eq_f08

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  Non-equal part
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

function mpi_comm_neq (x, y) result(res)
    type(MPI_Comm), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_comm_neq

function mpi_datatype_neq (x, y) result(res)
    type(MPI_Datatype), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_datatype_neq

function mpi_errhandler_neq (x, y) result(res)
    type(MPI_Errhandler), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_errhandler_neq

function mpi_file_neq (x, y) result(res)
    type(MPI_File), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_File_neq

function mpi_group_neq (x, y) result(res)
    type(MPI_Group), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_group_neq

function mpi_info_neq (x, y) result(res)
    type(MPI_Info), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_info_neq

function mpi_op_neq (x, y) result(res)
    type(MPI_Op), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_op_neq

function mpi_request_neq (x, y) result(res)
    type(MPI_Request), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_request_neq

function mpi_win_neq (x, y) result(res)
    type(MPI_Win), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_win_neq

function mpi_message_neq (x, y) result(res)
    type(MPI_Message), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function mpi_message_neq

function mpi_comm_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_comm_f08_ne_f

function mpi_comm_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_comm_f_ne_f08

function mpi_datatype_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_datatype_f08_ne_f

function mpi_datatype_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_datatype_f_ne_f08

function mpi_errhandler_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_errhandler_f08_ne_f

function mpi_errhandler_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_errhandler_f_ne_f08

function mpi_file_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_file_f_ne_f08

function mpi_file_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_file_f08_ne_f

function mpi_group_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_group_f08_ne_f

function mpi_group_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_group_f_ne_f08

function mpi_info_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_info_f08_ne_f

function mpi_info_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_info_f_ne_f08

function mpi_op_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_op_f08_ne_f

function mpi_op_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_op_f_ne_f08

function mpi_request_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_request_f08_ne_f

function mpi_request_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_request_f_ne_f08

function mpi_win_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_win_f08_ne_f

function mpi_win_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_win_f_ne_f08

function mpi_message_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_message_f08_ne_f

function mpi_message_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function mpi_message_f_ne_f08

! 17.2.4 - Conversion between Fortran and C handles

!  MPI_Comm MPI_Comm_f2c(MPI_Fint comm)
function mpi_comm_f2c (comm) bind(c,name="MPI_Comm_f2c") result (res)
    use mpi_c_interface_types, only: c_Comm
    integer,value :: comm
    integer(c_Comm) :: res

    res = comm
end function mpi_comm_f2c

function pmpi_comm_f2c (comm) bind(c,name="PMPI_Comm_f2c") result (res)
    use mpi_c_interface_types, only: c_Comm
    integer,value :: comm
    integer(c_Comm) :: res

    res = comm
end function pmpi_comm_f2c

!  MPI_Fint MPI_Comm_c2f(MPI_Comm comm)
function mpi_comm_c2f (comm) bind(c,name="MPI_Comm_c2f") result (res)
    use mpi_c_interface_types, only: c_Comm
    integer(c_Comm),value :: comm
    integer :: res

    res = comm
end function mpi_comm_c2f

function pmpi_comm_c2f (comm) bind(c,name="PMPI_Comm_c2f") result (res)
    use mpi_c_interface_types, only: c_Comm
    integer(c_Comm),value :: comm
    integer :: res

    res = comm
end function pmpi_comm_c2f

!  MPI_Type MPI_Type_f2c(MPI_Fint type)
function mpi_type_f2c (datatype) bind(c,name="MPI_Type_f2c") result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer,value :: datatype
    integer(c_Datatype) :: res

    res = datatype
end function mpi_type_f2c

function pmpi_type_f2c (datatype) bind(c,name="PMPI_Type_f2c") result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer,value :: datatype
    integer(c_Datatype) :: res

    res = datatype
end function pmpi_type_f2c

!  MPI_Fint MPI_Type_c2f(MPI_Type type)
function mpi_type_c2f (datatype) bind(c,name="MPI_Type_c2f") result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer(c_Datatype),value :: datatype
    integer :: res

    res = datatype
end function mpi_type_c2f

function pmpi_type_c2f (datatype) bind(c,name="PMPI_Type_c2f") result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer(c_Datatype),value :: datatype
    integer :: res

    res = datatype
end function pmpi_type_c2f

!  MPI_Group MPI_Group_f2c(MPI_Fint group)
function mpi_group_f2c (group) bind(c,name="MPI_Group_f2c") result (res)
    use mpi_c_interface_types, only: c_Group
    integer,value :: group
    integer(c_Group) :: res

    res = group
end function mpi_group_f2c

function pmpi_group_f2c (group) bind(c,name="PMPI_Group_f2c") result (res)
    use mpi_c_interface_types, only: c_Group
    integer,value :: group
    integer(c_Group) :: res

    res = group
end function pmpi_group_f2c


! MPI_Fint MPI_Group_c2f(MPI_Datatype datatype)
function mpi_group_c2f (group) bind(c,name="MPI_Group_c2f") result (res)
    use mpi_c_interface_types, only: c_Group
    integer(c_Group),value :: group
    integer :: res

    res = group
end function mpi_group_c2f

function pmpi_group_c2f (group) bind(c,name="PMPI_Group_c2f") result (res)
    use mpi_c_interface_types, only: c_Group
    integer(c_Group),value :: group
    integer :: res

    res = group
end function pmpi_group_c2f


! MPI_Request MPI_Request_f2c(MPI_Fint request)
function mpi_request_f2c (request) bind(c,name="MPI_Request_f2c") result (res)
    use mpi_c_interface_types, only: c_Request
    integer,value :: request
    integer(c_Request) :: res

    res = request
end function mpi_request_f2c

function pmpi_request_f2c (request) bind(c,name="PMPI_Request_f2c") result (res)
    use mpi_c_interface_types, only: c_Request
    integer,value :: request
    integer(c_Request) :: res

    res = request
end function pmpi_request_f2c


! MPI_Fint MPI_Request_c2f(MPI_Datatype datatype)
function mpi_request_c2f (request) bind(c,name="MPI_Request_c2f") result (res)
    use mpi_c_interface_types, only: c_Request
    integer(c_Request),value :: request
    integer :: res

    res = request
end function mpi_request_c2f

function pmpi_request_c2f (request) bind(c,name="PMPI_Request_c2f") result (res)
    use mpi_c_interface_types, only: c_Request
    integer(c_Request),value :: request
    integer :: res

    res = request
end function pmpi_request_c2f

! MPI_File MPI_File_f2c(MPI_Fint file)
! The MPI_File_f2c/c2f functions are defined in C

function MPI_File_f2c (file) bind(c,name="MPI_File_f2c") result (res)
    use mpi_c_interface_types, only: c_File
    integer,value :: file
    integer(c_File) :: res
end function MPI_File_f2c

function pmpi_file_f2c (file) bind(c,name="PMPI_File_f2c") result (res)
    use mpi_c_interface_types, only: c_File
    integer,value :: file
    integer(c_File) :: res

    res = file
end function pmpi_file_f2c

! MPI_Fint MPI_File_c2f(MPI_Datatype datatype)
function MPI_File_c2f (file) bind(c,name="MPI_File_c2f") result (res)
    use mpi_c_interface_types, only: c_File
    integer(c_File),value :: file
    integer :: res
end function MPI_File_c2f

function pmpi_file_c2f (file) bind(c,name="PMPI_File_c2f") result (res)
    use mpi_c_interface_types, only: c_File
    integer(c_File),value :: file
    integer :: res

    res = file
end function pmpi_file_c2f

! MPI_Win MPI_Win_f2c(MPI_Fint win)
function mpi_win_f2c (win) bind(c,name="MPI_Win_f2c") result (res)
    use mpi_c_interface_types, only: C_win
    integer,value :: win
    integer(C_win) :: res

    res = win
end function mpi_win_f2c

function pmpi_win_f2c (win) bind(c,name="PMPI_Win_f2c") result (res)
    use mpi_c_interface_types, only: C_win
    integer,value :: win
    integer(C_win) :: res

    res = win
end function pmpi_win_f2c

! MPI_Fint MPI_Win_c2f(MPI_Datatype datatype)
function mpi_win_c2f (win) bind(c,name="MPI_Win_c2f") result (res)
    use mpi_c_interface_types, only: C_win
    integer(C_win),value :: win
    integer :: res

    res = win
end function mpi_win_c2f

function pmpi_win_c2f (win) bind(c,name="PMPI_Win_c2f") result (res)
    use mpi_c_interface_types, only: C_win
    integer(C_win),value :: win
    integer :: res

    res = win
end function pmpi_win_c2f

! MPI_Op MPI_Op_f2c(MPI_Fint op)
function mpi_op_f2c (op) bind(c,name="MPI_Op_f2c") result (res)
    use mpi_c_interface_types, only: c_Op
    integer,value :: op
    integer(c_Op) :: res

    res = op
end function mpi_op_f2c

function pmpi_op_f2c (op) bind(c,name="PMPI_Op_f2c") result (res)
    use mpi_c_interface_types, only: c_Op
    integer,value :: op
    integer(c_Op) :: res

    res = op
end function pmpi_op_f2c

! MPI_Fint MPI_Op_c2f(MPI_Datatype datatype)
function mpi_op_c2f (op) bind(c,name="MPI_Op_c2f") result (res)
    use mpi_c_interface_types, only: c_Op
    integer(c_Op),value :: op
    integer :: res

    res = op
end function mpi_op_c2f

function pmpi_op_c2f (op) bind(c,name="PMPI_Op_c2f") result (res)
    use mpi_c_interface_types, only: c_Op
    integer(c_Op),value :: op
    integer :: res

    res = op
end function pmpi_op_c2f

! MPI_Info MPI_Info_f2c(MPI_Fint info)
function mpi_info_f2c (info) bind(c,name="MPI_Info_f2c") result (res)
    use mpi_c_interface_types, only: c_Info
    integer,value :: info
    integer(c_Info) :: res

    res = info
end function mpi_info_f2c

function pmpi_info_f2c (info) bind(c,name="PMPI_Info_f2c") result (res)
    use mpi_c_interface_types, only: c_Info
    integer,value :: info
    integer(c_Info) :: res

    res = info
end function pmpi_info_f2c

! MPI_Fint MPI_Info_c2f(MPI_Datatype datatype)
function mpi_info_c2f (info) bind(c,name="MPI_Info_c2f") result (res)
    use mpi_c_interface_types, only: c_Info
    integer(c_Info),value :: info
    integer :: res

    res = info
end function mpi_info_c2f

function pmpi_info_c2f (info) bind(c,name="PMPI_Info_c2f") result (res)
    use mpi_c_interface_types, only: c_Info
    integer(c_Info),value :: info
    integer :: res

    res = info
end function pmpi_info_c2f

! MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler)
function mpi_errhandler_f2c (errhandler) bind(c,name="MPI_Errhandler_f2c") result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer,value :: errhandler
    integer(c_Errhandler) :: res

    res = errhandler
end function mpi_errhandler_f2c

function pmpi_errhandler_f2c (errhandler) bind(c,name="PMPI_Errhandler_f2c") result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer,value :: errhandler
    integer(c_Errhandler) :: res

    res = errhandler
end function pmpi_errhandler_f2c

! MPI_Fint MPI_Errhandler_c2f(MPI_Datatype datatype)
function mpi_errhandler_c2f (errhandler) bind(c,name="MPI_Errhandler_c2f") result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer(c_Errhandler),value :: errhandler
    integer :: res

    res = errhandler
end function mpi_errhandler_c2f

function pmpi_errhandler_c2f (errhandler) bind(c,name="PMPI_Errhandler_c2f") result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer(c_Errhandler),value :: errhandler
    integer :: res

    res = errhandler
end function pmpi_errhandler_c2f

! MPI_Message MPI_Message_f2c(MPI_Fint message)
function mpi_message_f2c (message) bind(c,name="MPI_Message_f2c") result (res)
    use mpi_c_interface_types, only: c_Message
    integer,value :: message
    integer(c_Message) :: res

    res = message
end function mpi_message_f2c

function pmpi_message_f2c (message) bind(c,name="PMPI_Message_f2c") result (res)
    use mpi_c_interface_types, only: c_Message
    integer,value :: message
    integer(c_Message) :: res

    res = message
end function pmpi_message_f2c

! MPI_Fint MPI_Message_c2f(MPI_Datatype datatype)
function mpi_message_c2f (message) bind(c,name="MPI_Message_c2f") result (res)
    use mpi_c_interface_types, only: c_Message
    integer(c_Message),value :: message
    integer :: res

    res = message
end function mpi_message_c2f

function pmpi_message_c2f (message) bind(c,name="PMPI_Message_c2f") result (res)
    use mpi_c_interface_types, only: c_Message
    integer(c_Message),value :: message
    integer :: res

    res = message
end function pmpi_message_c2f

end module mpi_f08_types
