! mpi_c_interfaces_nobuf module
! Provides interfaces for C functions in MPI 3 / mpich3 that do not have
! a choice formal parameter
!
! This module is used by the module mpi_c_interfaces, which is later
! used in the Fortran wrapper functions defined in the modules mpi and
! mpi_f08.
!
! This module provides only BIND(C) interfaces for MPI_* and PMPI_*
! routines in the base C library.  Within the interfaces, these
! modules are used to access constants:
!
!  mpi_c_interface_types
!  iso_c_binding
!

module mpi_c_interface_nobuf

implicit none

interface

function MPIR_Init_c (argc, argv) &
           BIND(C, name="MPI_Init") RESULT (res)
    use,intrinsic :: iso_c_binding, only: c_int, c_ptr
    type(c_ptr),value,intent(in) :: argc
    type(c_ptr),value,intent(in) :: argv
    integer(c_int)               :: res
end function MPIR_Init_c

function MPIR_Finalize_c() &
            BIND(C, name="MPI_Finalize") RESULT (res)
    use,intrinsic :: iso_c_binding, only:  c_int
    integer(c_int) :: res
end function MPIR_Finalize_c

function MPIR_Comm_rank_c (comm, rank) &
            BIND(C, name="MPI_Comm_rank") RESULT (res)
    use :: mpi_c_interface_types, only:  C_comm
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_comm),value,intent(in) :: comm
    integer(c_int), intent(out)      :: rank
    integer(c_int)                   :: res
end function MPIR_Comm_rank_c

function MPIR_Comm_size_c (comm, size) &
           BIND(C, name="MPI_Comm_size") RESULT (res)
    use :: mpi_c_interface_types, only:  C_comm
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_comm),value,intent(in) :: comm
    integer(c_int),intent(out)       :: size
    integer(c_int)                   :: res
end function MPIR_Comm_size_c

function MPIR_Buffer_detach_c (buffer_addr, size) &
           BIND(C, name="MPI_Buffer_detach") RESULT (res)
    use,intrinsic :: iso_c_binding, only: c_ptr, c_int
    type(c_ptr),intent(out)    :: buffer_addr
    integer(c_int),intent(out) :: size
    integer(c_int)             :: res
end function MPIR_Buffer_detach_c

function MPIR_cancel_c (request) &
            BIND(C, name="MPI_cancel") RESULT (res)
    use :: mpi_c_interface_types, only: C_Request
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_Request) :: request
    integer(c_int)     :: res
end function MPIR_cancel_c

function MPIR_Abort_c (comm, errorcode) &
           BIND(C, name="MPI_Abort") RESULT (res)
    use :: mpi_c_interface_types, only: C_comm
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_comm),value,intent(in) :: comm
    integer(c_int), value,intent(in) :: errorcode
    integer(c_int)                   :: res
end function MPIR_Abort_c

function MPIR_Barrier_c (comm) &
           BIND(C, name="MPI_Barrier") RESULT (res)
    use :: mpi_c_interface_types, only:  C_comm
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_comm),value,intent(in) :: comm
    integer(c_int)                  :: res
end function MPIR_Barrier_c

function MPIR_Error_string_c (errorcode, string, resultlen) &
          BIND(C, name="MPI_Error_string") RESULT (res)
    use,intrinsic :: iso_c_binding, only:  c_int, c_char
    integer(c_int),value,intent(in)    :: errorcode
    character(kind=c_char),intent(out) :: string(*)
    integer(c_int),intent(out)         :: resultlen
    integer(c_int)                     :: res
end function MPIR_Error_string_c

function MPIR_Test_c( request, flag, status) &
           BIND(C, name="MPI_Test") RESULT (res)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_c_interface_types, only:  C_request, c_Status
    integer(C_request) :: request
    integer(c_int)     :: flag
    type(c_Status) :: status
    integer(c_int)     :: res
end function MPIR_Test_c

function MPIR_Type_create_f90_complex_c (p, r, newtype) &
           BIND(C, name="MPI_Type_create_f90_complex") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype
    use,intrinsic :: iso_c_binding, only: c_int
    integer(c_int),value :: p
    integer(c_int),value :: r
    integer(C_Datatype),intent(out) :: newtype
    integer(c_int)       :: res
end function MPIR_Type_create_f90_complex_c

function MPIR_Type_create_f90_integer_c (r, newtype) &
           BIND(C, name="MPI_Type_create_f90_integer") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype
    use,intrinsic :: iso_c_binding, only: c_int
    integer(c_int),value :: r
    integer(C_Datatype),intent(out) :: newtype
    integer(c_int)       :: res
end function MPIR_Type_create_f90_integer_c

function MPIR_Type_create_f90_real_c (p, r, newtype) &
           BIND(C, name="MPI_Type_create_f90_real") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype
    use,intrinsic :: iso_c_binding, only: c_int
    integer(c_int),value :: p
    integer(c_int),value :: r
    integer(C_Datatype),intent(out) :: newtype
    integer(c_int)       :: res
end function MPIR_Type_create_f90_real_c

function MPIR_Type_match_size_c (typeclass, size, datatype) &
           BIND(C, name="MPI_Type_match_size") RESULT (res)
    use :: mpi_c_interface_types, only: C_Datatype
    use,intrinsic :: iso_c_binding, only: c_int
    integer(c_int),value :: typeclass
    integer(c_int),value :: size
    integer(C_Datatype)  :: datatype
    integer(c_int)       :: res
end function MPIR_Type_match_size_c

function MPIR_Wait_c (request, status) &
           BIND(C, name="MPI_Wait") RESULT(res)
    use :: mpi_c_interface_types, only:  c_Status, C_Request
    use,intrinsic :: iso_c_binding, only: c_int
    integer(C_Request)  :: request
    type(c_Status)  :: status
    integer(c_int)      :: res
end function MPIR_Wait_c

function MPIR_Wtick_c () &
           BIND(C, name="MPI_Wtick") RESULT (res)
    use,intrinsic :: iso_c_binding, only: c_double
    real(c_double) :: res
end function MPIR_Wtick_c


function MPIR_Wtime_c () &
           BIND(C, name="MPI_Wtime") RESULT (res)
    use,intrinsic :: iso_c_binding, only:  c_double
    real(c_double) :: res
end function MPIR_Wtime_c


end interface


end module mpi_c_interface_nobuf

