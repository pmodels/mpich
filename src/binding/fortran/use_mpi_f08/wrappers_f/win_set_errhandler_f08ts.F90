!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_set_errhandler_f08(win, errhandler, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Win, MPI_Errhandler
    use :: mpi_c_interface, only : c_Win, c_Errhandler
    use :: mpi_c_interface, only : MPIR_Win_set_errhandler_c

    implicit none

    type(MPI_Win), intent(in) :: win
    type(MPI_Errhandler), intent(in) :: errhandler
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    integer(c_Errhandler) :: errhandler_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_set_errhandler_c(win%MPI_VAL, errhandler%MPI_VAL)
    else
        win_c = win%MPI_VAL
        errhandler_c = errhandler%MPI_VAL
        ierror_c = MPIR_Win_set_errhandler_c(win_c, errhandler_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_set_errhandler_f08
