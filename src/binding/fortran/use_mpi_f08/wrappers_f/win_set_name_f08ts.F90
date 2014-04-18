!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_set_name_f08(win, win_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Win
    use :: mpi_c_interface, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_set_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Win), intent(in) :: win
    character(len=*), intent(in) :: win_name
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    character(kind=c_char) :: win_name_c(len_trim(win_name)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(win_name, win_name_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_set_name_c(win%MPI_VAL, win_name_c)
    else
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_set_name_c(win_c, win_name_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_set_name_f08
