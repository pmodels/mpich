!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_free_keyval_f08(win_keyval, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Win_free_keyval_c

    implicit none

    integer, intent(inout) :: win_keyval
    integer, optional, intent(out) :: ierror

    integer(c_int) :: win_keyval_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_free_keyval_c(win_keyval)
    else
        win_keyval_c = win_keyval
        ierror_c = MPIR_Win_free_keyval_c(win_keyval_c)
        win_keyval = win_keyval_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_free_keyval_f08
