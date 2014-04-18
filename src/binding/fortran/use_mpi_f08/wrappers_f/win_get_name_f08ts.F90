!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Win_get_name_f08(win, win_name, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Win
    use :: mpi_f08, only : MPI_MAX_OBJECT_NAME
    use :: mpi_c_interface, only : c_Win
    use :: mpi_c_interface, only : MPIR_Win_get_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    type(MPI_Win), intent(in) :: win
    character(len=MPI_MAX_OBJECT_NAME), intent(out) :: win_name
    integer, intent(out) :: resultlen
    integer, optional, intent(out) :: ierror

    integer(c_Win) :: win_c
    character(kind=c_char) :: win_name_c(MPI_MAX_OBJECT_NAME+1)
    integer(c_int) :: resultlen_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Win_get_name_c(win%MPI_VAL, win_name_c, resultlen)
    else
        win_c = win%MPI_VAL
        ierror_c = MPIR_Win_get_name_c(win_c, win_name_c, resultlen_c)
        resultlen = resultlen_c
    end if

    call MPIR_Fortran_string_c2f(win_name_c, win_name)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Win_get_name_f08
