!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_create_f08(comm, group, newcomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_Group
    use :: mpi_c_interface, only : c_Group, c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_create_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    type(MPI_Group), intent(in) :: group
    type(MPI_Comm), intent(out) :: newcomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_Group) :: group_c
    integer(c_Comm) :: newcomm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_create_c(comm%MPI_VAL, group%MPI_VAL, newcomm%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        group_c = group%MPI_VAL
        ierror_c = MPIR_Comm_create_c(comm_c, group_c, newcomm_c)
        newcomm%MPI_VAL = newcomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_create_f08
