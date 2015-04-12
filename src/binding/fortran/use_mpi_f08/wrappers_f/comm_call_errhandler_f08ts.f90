!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_call_errhandler_f08(comm, errorcode, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_call_errhandler_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: errorcode
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: errorcode_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_call_errhandler_c(comm%MPI_VAL, errorcode)
    else
        comm_c = comm%MPI_VAL
        errorcode_c = errorcode
        ierror_c = MPIR_Comm_call_errhandler_c(comm_c, errorcode_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_call_errhandler_f08
