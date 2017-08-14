!   -*- Mode: Fortran; -*-
!
!   See COPYRIGHT in top-level directory.
!
subroutine MPIX_Comm_revoke_f08(comm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_revoke_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_revoke_c(comm%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_revoke_c(comm_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPIX_Comm_revoke_f08
