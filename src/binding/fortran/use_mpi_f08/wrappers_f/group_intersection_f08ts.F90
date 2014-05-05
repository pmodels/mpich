!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Group_intersection_f08(group1,group2,newgroup, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group
    use :: mpi_c_interface, only : c_Group
    use :: mpi_c_interface, only : MPIR_Group_intersection_c

    implicit none

    type(MPI_Group), intent(in) :: group1
    type(MPI_Group), intent(in) :: group2
    type(MPI_Group), intent(out) :: newgroup
    integer, optional, intent(out) :: ierror

    integer(c_Group) :: group1_c
    integer(c_Group) :: group2_c
    integer(c_Group) :: newgroup_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Group_intersection_c(group1%MPI_VAL, group2%MPI_VAL, newgroup%MPI_VAL)
    else
        group1_c = group1%MPI_VAL
        group2_c = group2%MPI_VAL
        ierror_c = MPIR_Group_intersection_c(group1_c, group2_c, newgroup_c)
        newgroup%MPI_VAL = newgroup_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Group_intersection_f08
