!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Free_mem_f08(base, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface, only : MPIR_Free_mem_c

    implicit none

    type(*), dimension(..), intent(in), asynchronous :: base
    integer, optional, intent(out) :: ierror

    integer(c_int) :: ierror_c

    ierror_c = MPIR_Free_mem_c(base)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Free_mem_f08
