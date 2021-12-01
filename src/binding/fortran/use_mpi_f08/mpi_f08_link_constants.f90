!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

module mpi_f08_link_constants
use, intrinsic :: iso_c_binding, only: c_ptr, c_int, c_char, c_loc
use :: mpi_f08_types, only : MPI_Status
use :: mpi_c_interface_types, only : c_Status

implicit none

! Named link time constants

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
! MPI_STATUS_IGNORE, MPI_STATUSES_IGNORE
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

! In MPICH C code, MPI_STATUS_IGNORE, MPI_STATUSES_IGNORE are pointers of bad address

! Fortran programmers use these MPI_Status variables for INOUT status arguments
type(MPI_Status), bind(C, name="MPIR_F08_MPI_STATUS_IGNORE_OBJ"), target :: MPI_STATUS_IGNORE
type(MPI_Status), dimension(1), bind(C, name="MPIR_F08_MPI_STATUSES_IGNORE_OBJ"), target :: MPI_STATUSES_IGNORE


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  MPI_ARGV_NULL, MPI_ARGVS_NULL
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! In MPICH C code, MPI_ARGV_NULL and MPI_ARGVS_NULL are NULL pointers
character(len=1), dimension(1), target :: MPI_ARGV_NULL
character(len=1), dimension(1,1), target :: MPI_ARGVS_NULL


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  MPI_ERRCODES_IGNORE
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
integer, dimension(1), target :: MPI_ERRCODES_IGNORE


!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  MPI_UNWEIGHTED, MPI_WEIGHTS_EMPTY
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Type of MPI_UNWEIGHTED and MPI_WEIGHTS_EMPTY is not integer(c_int) since we only care their address

integer, dimension(1), target :: MPI_UNWEIGHTED
integer, dimension(1), target :: MPI_WEIGHTS_EMPTY

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  MPI_IN_PLACE
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
integer(c_int), bind(C, name="MPIR_F08_MPI_IN_PLACE"), target :: MPI_IN_PLACE

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!
!  MPI_BOTTOM
!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! Buffer Address Constants
! A.1.1 p. 663
integer(c_int), bind(C, name="MPIR_F08_MPI_BOTTOM"), target :: MPI_BOTTOM

INTERFACE

FUNCTION MPIR_F08_get_MPI_STATUS_IGNORE_c() &
    bind (C, name="MPIR_F08_get_MPI_STATUS_IGNORE") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_STATUSES_IGNORE_c() &
    bind (C, name="MPIR_F08_get_MPI_STATUSES_IGNORE") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_ARGV_NULL_c() &
    bind (C, name="MPIR_F08_get_MPI_ARGV_NULL") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_ARGVS_NULL_c() &
    bind (C, name="MPIR_F08_get_MPI_ARGVS_NULL") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_ERRCODES_IGNORE_c() &
    bind (C, name="MPIR_F08_get_MPI_ERRCODES_IGNORE") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_UNWEIGHTED_c() &
    bind (C, name="MPIR_F08_get_MPI_UNWEIGHTED") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

FUNCTION MPIR_F08_get_MPI_WEIGHTS_EMPTY_c() &
    bind (C, name="MPIR_F08_get_MPI_WEIGHTS_EMPTY") result(p)
    USE, intrinsic :: iso_c_binding, ONLY : c_ptr
    IMPLICIT NONE
    TYPE(c_ptr) :: p
END FUNCTION

END INTERFACE

end module mpi_f08_link_constants
