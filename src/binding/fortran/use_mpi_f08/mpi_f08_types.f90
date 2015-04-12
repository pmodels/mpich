! MPI 3 data types for Fortran.
! From A.1.2, pp 676-677
! Note added MPI_Message type not in the spec by mistake
!
!--------------------------------------------------------------

module MPI_f08_types
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

! MPI_File_f2c/c2f are implemented in C, so we do direct binding.
interface
    function MPI_File_f2c(file) bind(C, name="MPI_File_f2c") result (res)
        use mpi_c_interface_types, only: c_File
        integer, value :: file
        integer(c_File) :: res
    end function MPI_File_f2c

    function MPI_File_c2f(file) bind(C, name="MPI_File_c2f") result (res)
        use mpi_c_interface_types, only: c_File
        integer(c_File), value :: file
        integer :: res
    end function MPI_File_c2f
end interface

! Fortran 2008 struct for status. Must be consistent with MPI.h, MPIf.h
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
    module procedure MPI_Status_f08_assgn_c
    module procedure MPI_Status_c_assgn_f08
end interface

private :: MPI_Status_f08_assgn_c
private :: MPI_Status_c_assgn_f08
private :: MPI_Status_f_assgn_c
private :: MPI_Status_c_assgn_f

! Required operator overloads for == and /= for opaque handles
! 2.5.1 pp 12-13

interface operator(==)
    module procedure MPI_Comm_eq
    module procedure MPI_Datatype_eq
    module procedure MPI_Errhandler_eq
    module procedure MPI_File_eq
    module procedure MPI_Group_eq
    module procedure MPI_Info_eq
    module procedure MPI_Op_eq
    module procedure MPI_Request_eq
    module procedure MPI_Win_eq
    module procedure MPI_Message_eq

    module procedure MPI_Comm_f08_eq_f
    module procedure MPI_Comm_f_eq_f08
    module procedure MPI_Datatype_f08_eq_f
    module procedure MPI_Datatype_f_eq_f08
    module procedure MPI_Errhandler_f08_eq_f
    module procedure MPI_Errhandler_f_eq_f08
    module procedure MPI_File_f08_eq_f
    module procedure MPI_File_f_eq_f08
    module procedure MPI_Group_f08_eq_f
    module procedure MPI_Group_f_eq_f08
    module procedure MPI_Info_f08_eq_f
    module procedure MPI_Info_f_eq_f08
    module procedure MPI_Op_f08_eq_f
    module procedure MPI_Op_f_eq_f08
    module procedure MPI_Request_f08_eq_f
    module procedure MPI_Request_f_eq_f08
    module procedure MPI_Win_f08_eq_f
    module procedure MPI_Win_f_eq_f08
    module procedure MPI_Message_f08_eq_f
    module procedure MPI_Message_f_eq_f08
end interface

private :: MPI_Comm_eq
private :: MPI_Datatype_eq
private :: MPI_Errhandler_eq
private :: MPI_File_eq
private :: MPI_Group_eq
private :: MPI_Info_eq
private :: MPI_Op_eq
private :: MPI_Request_eq
private :: MPI_Win_eq
private :: MPI_Message_eq

private :: MPI_Comm_f08_eq_f
private :: MPI_Comm_f_eq_f08
private :: MPI_Datatype_f08_eq_f
private :: MPI_Datatype_f_eq_f08
private :: MPI_Errhandler_f08_eq_f
private :: MPI_Errhandler_f_eq_f08
private :: MPI_File_f08_eq_f
private :: MPI_File_f_eq_f08
private :: MPI_Group_f08_eq_f
private :: MPI_Group_f_eq_f08
private :: MPI_Info_f08_eq_f
private :: MPI_Info_f_eq_f08
private :: MPI_Op_f08_eq_f
private :: MPI_Op_f_eq_f08
private :: MPI_Request_f08_eq_f
private :: MPI_Request_f_eq_f08
private :: MPI_Win_f08_eq_f
private :: MPI_Win_f_eq_f08
private :: MPI_Message_f08_eq_f
private :: MPI_Message_f_eq_f08

interface operator(/=)
    module procedure MPI_Comm_neq
    module procedure MPI_Datatype_neq
    module procedure MPI_Errhandler_neq
    module procedure MPI_File_neq
    module procedure MPI_Group_neq
    module procedure MPI_Info_neq
    module procedure MPI_Op_neq
    module procedure MPI_Request_neq
    module procedure MPI_Win_neq
    module procedure MPI_Message_neq

    module procedure MPI_Comm_f08_ne_f
    module procedure MPI_Comm_f_ne_f08
    module procedure MPI_Datatype_f08_ne_f
    module procedure MPI_Datatype_f_ne_f08
    module procedure MPI_Errhandler_f08_ne_f
    module procedure MPI_Errhandler_f_ne_f08
    module procedure MPI_File_f08_ne_f
    module procedure MPI_File_f_ne_f08
    module procedure MPI_Group_f08_ne_f
    module procedure MPI_Group_f_ne_f08
    module procedure MPI_Info_f08_ne_f
    module procedure MPI_Info_f_ne_f08
    module procedure MPI_Op_f08_ne_f
    module procedure MPI_Op_f_ne_f08
    module procedure MPI_Request_f08_ne_f
    module procedure MPI_Request_f_ne_f08
    module procedure MPI_Win_f08_ne_f
    module procedure MPI_Win_f_ne_f08
    module procedure MPI_Message_f08_ne_f
    module procedure MPI_Message_f_ne_f08
end interface

private :: MPI_Comm_neq
private :: MPI_Datatype_neq
private :: MPI_Errhandler_neq
private :: MPI_File_neq
private :: MPI_Group_neq
private :: MPI_Info_neq
private :: MPI_Op_neq
private :: MPI_Request_neq
private :: MPI_Win_neq
private :: MPI_Message_neq

private :: MPI_Comm_f08_ne_f
private :: MPI_Comm_f_ne_f08
private :: MPI_Datatype_f08_ne_f
private :: MPI_Datatype_f_ne_f08
private :: MPI_Errhandler_f08_ne_f
private :: MPI_Errhandler_f_ne_f08
private :: MPI_File_f08_ne_f
private :: MPI_File_f_ne_f08
private :: MPI_Group_f08_ne_f
private :: MPI_Group_f_ne_f08
private :: MPI_Info_f08_ne_f
private :: MPI_Info_f_ne_f08
private :: MPI_Op_f08_ne_f
private :: MPI_Op_f_ne_f08
private :: MPI_Request_f08_ne_f
private :: MPI_Request_f_ne_f08
private :: MPI_Win_f08_ne_f
private :: MPI_Win_f_ne_f08
private :: MPI_Message_f08_ne_f
private :: MPI_Message_f_ne_f08

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

subroutine MPI_Status_f2f08(f_status, f08_status, ierror)
    integer, intent(in) :: f_status(MPI_STATUS_SIZE)
    type(MPI_Status), intent(out) :: f08_status
    integer, optional,  intent(out) :: ierror
    f08_status%count_lo = f_status(1)
    f08_status%count_hi_and_cancelled = f_status(2)
    f08_status%MPI_SOURCE = f_status(MPI_SOURCE)
    f08_status%MPI_TAG = f_status(MPI_TAG)
    f08_status%MPI_ERROR = f_status(MPI_ERROR)
    if (present(ierror)) ierror = 0
end subroutine

subroutine MPI_Status_f082f(f08_status, f_status, ierror)
    type(MPI_Status), intent(in) :: f08_status
    integer, intent(out) :: f_status(MPI_STATUS_SIZE)
    integer, optional,  intent(out) :: ierror
    f_status(1) = f08_status%count_lo
    f_status(2) = f08_status%count_hi_and_cancelled
    f_status(MPI_SOURCE) = f08_status%MPI_SOURCE
    f_status(MPI_TAG) = f08_status%MPI_TAG
    f_status(MPI_ERROR) = f08_status%MPI_ERROR
    if (present(ierror)) ierror = 0
end subroutine

elemental subroutine MPI_Status_f08_assgn_c (status_f08, status_c)
    ! Defined status_f08 = status_c
    type(MPI_Status),intent(out) :: status_f08
    type(c_Status),intent(in)    :: status_c

    status_f08%count_lo   = status_c%count_lo
    status_f08%count_hi_and_cancelled  = status_c%count_hi_and_cancelled
    status_f08%MPI_source = status_c%MPI_source
    status_f08%MPI_tag    = status_c%MPI_tag
    status_f08%MPI_error  = status_c%MPI_error
end subroutine MPI_Status_f08_assgn_c

elemental subroutine MPI_Status_c_assgn_f08 (status_c, status_f08)
    ! Defined status_c = status_f08
    type(c_Status),intent(out) :: status_c
    type(MPI_Status),intent(in) :: status_f08

    status_c%count_lo   = status_f08%count_lo
    status_c%count_hi_and_cancelled  = status_f08%count_hi_and_cancelled
    status_c%MPI_source = status_f08%MPI_source
    status_c%MPI_tag    = status_f08%MPI_tag
    status_c%MPI_error  = status_f08%MPI_error
end subroutine MPI_Status_c_assgn_f08

subroutine MPI_Status_f_assgn_c (status_f, status_c)
    ! Defined status_f = status_c
    use,intrinsic :: iso_fortran_env, only: int32
    integer,intent(out) :: status_f(MPI_STATUS_SIZE)
    type (c_Status),intent(in) :: status_c

    status_f(1) = status_c%count_lo
    status_f(2) = status_c%count_hi_and_cancelled
    status_f(MPI_SOURCE) = status_c%MPI_source
    status_f(MPI_TAG)    = status_c%MPI_tag
    status_f(MPI_ERROR)  = status_c%MPI_error
end subroutine MPI_Status_f_assgn_c

subroutine MPI_Status_c_assgn_f (status_c, status_f)
    ! Defined status_c = status_f
    use,intrinsic :: iso_fortran_env, only: int32
    integer,intent(in) :: status_f(MPI_STATUS_SIZE)
    integer(C_count) :: cnt
    type(c_Status),intent(out) :: status_c

    status_c%count_lo   = status_f(1);
    status_c%count_hi_and_cancelled  = status_f(2);
    status_c%MPI_source = status_f(MPI_SOURCE)
    status_c%MPI_tag    = status_f(MPI_TAG)
    status_c%MPI_error  = status_f(MPI_ERROR)
end subroutine MPI_Status_c_assgn_f

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

function MPI_Comm_eq (x, y) result(res)
    type(MPI_Comm), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Comm_eq

function MPI_Datatype_eq (x, y) result(res)
    type(MPI_Datatype), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Datatype_eq

function MPI_Errhandler_eq (x, y) result(res)
    type(MPI_Errhandler), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Errhandler_eq

function MPI_File_eq (x, y) result(res)
    type(MPI_File), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_File_eq

function MPI_Group_eq (x, y) result(res)
    type(MPI_Group), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Group_eq

function MPI_Info_eq (x, y) result(res)
    type(MPI_Info), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Info_eq

function MPI_Op_eq (x, y) result(res)
    type(MPI_Op), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Op_eq

function MPI_Request_eq (x, y) result(res)
    type(MPI_Request), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Request_eq

function MPI_Win_eq (x, y) result(res)
    type(MPI_Win), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Win_eq

function MPI_Message_eq (x, y) result(res)
    type(MPI_Message), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL == y%MPI_VAL)
end function MPI_Message_eq

function MPI_Comm_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Comm_f08_eq_f

function MPI_Comm_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Comm_f_eq_f08

function MPI_Datatype_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Datatype_f08_eq_f

function MPI_Datatype_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Datatype_f_eq_f08

function MPI_Errhandler_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Errhandler_f08_eq_f

function MPI_Errhandler_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Errhandler_f_eq_f08

function MPI_File_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_File_f_eq_f08

function MPI_File_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_File_f08_eq_f

function MPI_Group_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Group_f08_eq_f

function MPI_Group_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Group_f_eq_f08

function MPI_Info_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Info_f08_eq_f

function MPI_Info_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Info_f_eq_f08

function MPI_Op_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Op_f08_eq_f

function MPI_Op_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Op_f_eq_f08

function MPI_Request_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Request_f08_eq_f

function MPI_Request_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Request_f_eq_f08

function MPI_Win_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Win_f08_eq_f

function MPI_Win_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Win_f_eq_f08

function MPI_Message_f08_eq_f (f08, f) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Message_f08_eq_f

function MPI_Message_f_eq_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL == f
end function MPI_Message_f_eq_f08

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  Non-equal part
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

function MPI_Comm_neq (x, y) result(res)
    type(MPI_Comm), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Comm_neq

function MPI_Datatype_neq (x, y) result(res)
    type(MPI_Datatype), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Datatype_neq

function MPI_Errhandler_neq (x, y) result(res)
    type(MPI_Errhandler), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Errhandler_neq

function MPI_File_neq (x, y) result(res)
    type(MPI_File), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_File_neq

function MPI_Group_neq (x, y) result(res)
    type(MPI_Group), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Group_neq

function MPI_Info_neq (x, y) result(res)
    type(MPI_Info), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Info_neq

function MPI_Op_neq (x, y) result(res)
    type(MPI_Op), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Op_neq

function MPI_Request_neq (x, y) result(res)
    type(MPI_Request), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Request_neq

function MPI_Win_neq (x, y) result(res)
    type(MPI_Win), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Win_neq

function MPI_Message_neq (x, y) result(res)
    type(MPI_Message), intent(in) :: x, y
    logical :: res
    res = (x%MPI_VAL /= y%MPI_VAL)
end function MPI_Message_neq

function MPI_Comm_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Comm_f08_ne_f

function MPI_Comm_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Comm and integer handles
    type(MPI_Comm),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Comm_f_ne_f08

function MPI_Datatype_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Datatype_f08_ne_f

function MPI_Datatype_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Datatype and integer handles
    type(MPI_Datatype),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Datatype_f_ne_f08

function MPI_Errhandler_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Errhandler_f08_ne_f

function MPI_Errhandler_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Errhandler and integer handles
    type(MPI_Errhandler),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Errhandler_f_ne_f08

function MPI_File_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_File_f_ne_f08

function MPI_File_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_File and integer handles
    type(MPI_File),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_File_f08_ne_f

function MPI_Group_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Group_f08_ne_f

function MPI_Group_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Group and integer handles
    type(MPI_Group),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Group_f_ne_f08

function MPI_Info_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Info_f08_ne_f

function MPI_Info_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Info and integer handles
    type(MPI_Info),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Info_f_ne_f08

function MPI_Op_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Op_f08_ne_f

function MPI_Op_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Op and integer handles
    type(MPI_Op),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Op_f_ne_f08

function MPI_Request_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Request_f08_ne_f

function MPI_Request_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Request and integer handles
    type(MPI_Request),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Request_f_ne_f08

function MPI_Win_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Win_f08_ne_f

function MPI_Win_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Win and integer handles
    type(MPI_Win),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Win_f_ne_f08

function MPI_Message_f08_ne_f (f08, f) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Message_f08_ne_f

function MPI_Message_f_ne_f08 (f, f08) result(res)
    ! Defined comparison for MPI_Message and integer handles
    type(MPI_Message),intent(in) :: f08
    integer,intent(in)        :: f
    logical                   :: res
    res = f08%MPI_VAL /= f
end function MPI_Message_f_ne_f08

! 17.2.4 - Conversion functions between Fortran and C handles, which are only defined in
! the C interface and can be implemented as macros. We extend them to Fortran.
! TODO: Do we need the PMPI version? Probably not since they are not in standard.

function MPI_Comm_f2c (comm) result (res)
    use mpi_c_interface_types, only: c_Comm
    integer,value :: comm
    integer(c_Comm) :: res
    res = comm
end function MPI_Comm_f2c

function MPI_Comm_c2f (comm) result (res)
    use mpi_c_interface_types, only: c_Comm
    integer(c_Comm),value :: comm
    integer :: res
    res = comm
end function MPI_Comm_c2f

function MPI_type_f2c (datatype) result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer,value :: datatype
    integer(c_Datatype) :: res
    res = datatype
end function MPI_type_f2c

function MPI_type_c2f (datatype) result (res)
    use mpi_c_interface_types, only: c_Datatype
    integer(c_Datatype),value :: datatype
    integer :: res
    res = datatype
end function MPI_type_c2f

function MPI_Group_f2c (group) result (res)
    use mpi_c_interface_types, only: c_Group
    integer,value :: group
    integer(c_Group) :: res
    res = group
end function MPI_Group_f2c

function MPI_Group_c2f (group) result (res)
    use mpi_c_interface_types, only: c_Group
    integer(c_Group),value :: group
    integer :: res
    res = group
end function MPI_Group_c2f

function MPI_Request_f2c (request) result (res)
    use mpi_c_interface_types, only: c_Request
    integer,value :: request
    integer(c_Request) :: res
    res = request
end function MPI_Request_f2c

function MPI_Request_c2f (request) result (res)
    use mpi_c_interface_types, only: c_Request
    integer(c_Request),value :: request
    integer :: res
    res = request
end function MPI_Request_c2f

function MPI_Win_f2c (win) result (res)
    use mpi_c_interface_types, only: C_win
    integer,value :: win
    integer(C_win) :: res
    res = win
end function MPI_Win_f2c

function MPI_Win_c2f (win) result (res)
    use mpi_c_interface_types, only: C_win
    integer(C_win),value :: win
    integer :: res
    res = win
end function MPI_Win_c2f

function pMPI_Win_c2f (win) result (res)
    use mpi_c_interface_types, only: C_win
    integer(C_win),value :: win
    integer :: res
    res = win
end function pMPI_Win_c2f


function MPI_Op_f2c (op) result (res)
    use mpi_c_interface_types, only: c_Op
    integer,value :: op
    integer(c_Op) :: res
    res = op
end function MPI_Op_f2c

function MPI_Op_c2f (op) result (res)
    use mpi_c_interface_types, only: c_Op
    integer(c_Op),value :: op
    integer :: res
    res = op
end function MPI_Op_c2f

function MPI_Info_f2c (info) result (res)
    use mpi_c_interface_types, only: c_Info
    integer,value :: info
    integer(c_Info) :: res
    res = info
end function MPI_Info_f2c

function MPI_Info_c2f (info) result (res)
    use mpi_c_interface_types, only: c_Info
    integer(c_Info),value :: info
    integer :: res
    res = info
end function MPI_Info_c2f

function MPI_Errhandler_f2c (errhandler) result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer,value :: errhandler
    integer(c_Errhandler) :: res
    res = errhandler
end function MPI_Errhandler_f2c

function MPI_Errhandler_c2f (errhandler) result (res)
    use mpi_c_interface_types, only: c_Errhandler
    integer(c_Errhandler),value :: errhandler
    integer :: res
    res = errhandler
end function MPI_Errhandler_c2f

function MPI_Message_f2c (message) result (res)
    use mpi_c_interface_types, only: c_Message
    integer,value :: message
    integer(c_Message) :: res
    res = message
end function MPI_Message_f2c

function MPI_Message_c2f (message) result (res)
    use mpi_c_interface_types, only: c_Message
    integer(c_Message),value :: message
    integer :: res
    res = message
end function MPI_Message_c2f

end module MPI_f08_types
