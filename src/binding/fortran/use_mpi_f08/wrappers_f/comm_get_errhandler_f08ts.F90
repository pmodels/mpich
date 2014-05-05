!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_get_errhandler_f08(comm, errhandler, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_Errhandler
    use :: mpi_c_interface, only : c_Comm, c_Errhandler
    use :: mpi_c_interface, only : MPIR_Comm_get_errhandler_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    type(MPI_Errhandler), intent(out) :: errhandler
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_Errhandler) :: errhandler_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_get_errhandler_c(comm%MPI_VAL, errhandler%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_get_errhandler_c(comm_c, errhandler_c)
        errhandler%MPI_VAL = errhandler_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_get_errhandler_f08
