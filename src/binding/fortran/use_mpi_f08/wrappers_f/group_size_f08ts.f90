!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Group_size_f08(group, size, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group
    use :: mpi_c_interface, only : c_Group
    use :: mpi_c_interface, only : MPIR_Group_size_c

    implicit none

    type(MPI_Group), intent(in) :: group
    integer, intent(out) :: size
    integer, optional, intent(out) :: ierror

    integer(c_Group) :: group_c
    integer(c_int) :: size_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Group_size_c(group%MPI_VAL, size)
    else
        group_c = group%MPI_VAL
        ierror_c = MPIR_Group_size_c(group_c, size_c)
        size = size_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Group_size_f08
