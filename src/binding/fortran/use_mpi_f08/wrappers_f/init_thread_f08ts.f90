!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Init_thread_f08(required, provided, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, C_NULL_PTR
    use :: mpi_c_interface, only : MPIR_Init_thread_c

    implicit none

    integer, intent(in) :: required
    integer, intent(out) :: provided
    integer, optional, intent(out) :: ierror

    integer(c_int) :: required_c
    integer(c_int) :: provided_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Init_thread_c(C_NULL_PTR, C_NULL_PTR, required, provided)
    else
        required_c = required
        ierror_c = MPIR_Init_thread_c(C_NULL_PTR, C_NULL_PTR, required_c, provided_c)
        provided = provided_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Init_thread_f08
