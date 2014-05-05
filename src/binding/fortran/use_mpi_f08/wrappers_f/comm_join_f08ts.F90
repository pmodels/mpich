!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_join_f08(fd, intercomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_join_c

    implicit none

    integer, intent(in) :: fd
    type(MPI_Comm), intent(out) :: intercomm
    integer, optional, intent(out) :: ierror

    integer(c_int) :: fd_c
    integer(c_Comm) :: intercomm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_join_c(fd, intercomm%MPI_VAL)
    else
        fd_c = fd
        ierror_c = MPIR_Comm_join_c(fd_c, intercomm_c)
        intercomm%MPI_VAL = intercomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_join_f08
