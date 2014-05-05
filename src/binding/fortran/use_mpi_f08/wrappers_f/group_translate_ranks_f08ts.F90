!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Group_translate_ranks_f08(group1,n, ranks1,group2,ranks2,ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Group
    use :: mpi_c_interface, only : c_Group
    use :: mpi_c_interface, only : MPIR_Group_translate_ranks_c

    implicit none

    type(MPI_Group), intent(in) :: group1
    type(MPI_Group), intent(in) :: group2
    integer, intent(in) :: n
    integer, intent(in) :: ranks1(n)
    integer, intent(out) :: ranks2(n)
    integer, optional, intent(out) :: ierror

    integer(c_Group) :: group1_c
    integer(c_Group) :: group2_c
    integer(c_int) :: n_c
    integer(c_int) :: ranks1_c(n)
    integer(c_int) :: ranks2_c(n)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Group_translate_ranks_c(group1%MPI_VAL, n, ranks1, group2%MPI_VAL, ranks2)
    else
        group1_c = group1%MPI_VAL
        n_c = n
        ranks1_c = ranks1
        group2_c = group2%MPI_VAL
        ierror_c = MPIR_Group_translate_ranks_c(group1_c, n_c, ranks1_c, group2_c, ranks2_c)
        ranks2 = ranks2_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Group_translate_ranks_f08
