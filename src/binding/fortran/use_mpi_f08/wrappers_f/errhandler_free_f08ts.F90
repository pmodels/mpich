!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Errhandler_free_f08(errhandler, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Errhandler
    use :: mpi_c_interface, only : c_Errhandler
    use :: mpi_c_interface, only : MPIR_Errhandler_free_c

    implicit none

    type(MPI_Errhandler), intent(inout) :: errhandler
    integer, optional, intent(out) :: ierror

    integer(c_Errhandler) :: errhandler_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Errhandler_free_c(errhandler%MPI_VAL)
    else
        errhandler_c = errhandler%MPI_VAL
        ierror_c = MPIR_Errhandler_free_c(errhandler_c)
        errhandler%MPI_VAL = errhandler_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Errhandler_free_f08
