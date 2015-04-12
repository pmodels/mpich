!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Query_thread_f08(provided, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Query_thread_c

    implicit none

    integer, intent(out) :: provided
    integer, optional, intent(out) :: ierror

    integer(c_int) :: provided_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Query_thread_c(provided)
    else
        ierror_c = MPIR_Query_thread_c(provided_c)
        provided = provided_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Query_thread_f08
