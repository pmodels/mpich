!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_test_f08(win, flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win
    use :: mpi_c_interface, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_test_c

    implicit none

    logical, intent(out) :: flag
    type(MPI_Win), intent(in) :: win
    integer, optional, intent(out) :: ierror

    integer(c_int) :: flag_c
    integer(c_Win) :: win_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_test_c(win%MPI_VAL, flag_c)
    else
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_test_c(win_c, flag_c)
    end if

    flag = (flag_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_test_f08
