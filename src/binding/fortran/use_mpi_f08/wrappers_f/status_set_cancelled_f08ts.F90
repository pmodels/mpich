!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Status_set_cancelled_f08(status, flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_Status_set_cancelled_c

    implicit none

    type(MPI_Status), intent(inout), target :: status
    logical, intent(out) :: flag
    integer, optional, intent(out) :: ierror

    type(c_Status), target :: status_c
    integer(c_int) :: flag_c
    integer(c_int) :: ierror_c

    if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
        ierror_c = MPIR_Status_set_cancelled_c(MPIR_C_MPI_STATUS_IGNORE, flag_c)
    else
        ierror_c = MPIR_Status_set_cancelled_c(c_loc(status_c), flag_c)
        status = status_c
    end if

    flag = (flag_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Status_set_cancelled_f08
