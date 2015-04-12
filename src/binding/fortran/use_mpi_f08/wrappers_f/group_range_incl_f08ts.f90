!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Group_range_incl_f08(group, n,ranges, newgroup, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group
    use :: mpi_c_interface, only : c_Group
    use :: mpi_c_interface, only : MPIR_Group_range_incl_c

    implicit none

    type(MPI_Group), intent(in)  :: group
    integer, intent(in)  :: n
    integer, intent(in)  :: ranges(3, n)
    type(MPI_Group), intent(out)  :: newgroup
    integer, optional, intent(out)  :: ierror

    integer(c_Group) :: group_c
    integer(c_int) :: n_c
    integer(c_int) :: ranges_c(3, n)
    integer(c_Group) :: newgroup_c
    integer(c_int) :: ierror_c


    if (c_int == kind(0)) then
        ierror_c = MPIR_Group_range_incl_c(group%MPI_VAL, n, ranges, newgroup%MPI_VAL)
    else
        group_c = group%MPI_VAL
        n_c = n
        ranges_c = ranges
        ierror_c = MPIR_Group_range_incl_c(group_c, n_c, ranges_c, newgroup_c)
        newgroup%MPI_VAL = newgroup_c
    endif

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Group_range_incl_f08
