!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Test_cancelled_f08(status, flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Status
    use :: mpi_f08, only : assignment(=)
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Test_cancelled_c

    implicit none

    type(MPI_Status), intent(in), target :: status
    logical, intent(out) :: flag
    integer, optional, intent(out) :: ierror

    type(c_Status), target :: status_c
    integer(c_int) :: flag_c
    integer(c_int) :: ierror_c

    ierror_c = MPIR_Test_cancelled_c(c_loc(status), flag_c)

    flag = (flag_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Test_cancelled_f08
