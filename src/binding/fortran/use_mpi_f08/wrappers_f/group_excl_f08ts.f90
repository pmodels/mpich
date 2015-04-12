!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Group_excl_f08(group, n,ranks, newgroup, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group
    use :: mpi_c_interface, only : c_Group
    use :: mpi_c_interface, only : MPIR_Group_excl_c

    implicit none

    type(MPI_Group), intent(in) :: group
    integer, intent(in) :: n
    integer, intent(in) :: ranks(n)
    type(MPI_Group), intent(out) :: newgroup
    integer, optional, intent(out) :: ierror

    integer(c_Group) :: group_c
    integer(c_int) :: n_c
    integer(c_int) :: ranks_c(n)
    integer(c_Group) :: newgroup_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Group_excl_c(group%MPI_VAL, n, ranks, newgroup%MPI_VAL)
    else
        group_c = group%MPI_VAL
        n_c = n
        ranks_c = ranks
        ierror_c = MPIR_Group_excl_c(group_c, n_c, ranks_c, newgroup_c)
        newgroup%MPI_VAL = newgroup_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Group_excl_f08
