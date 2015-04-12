!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_free_keyval_f08(comm_keyval, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Comm_free_keyval_c

    implicit none

    integer, intent(inout) :: comm_keyval
    integer, optional, intent(out) :: ierror

    integer(c_int) :: comm_keyval_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_free_keyval_c(comm_keyval)
    else
        comm_keyval_c = comm_keyval
        ierror_c = MPIR_Comm_free_keyval_c(comm_keyval_c)
        comm_keyval = comm_keyval_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_free_keyval_f08
