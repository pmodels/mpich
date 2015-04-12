!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_get_group_f08(fh, group, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_File, MPI_Group
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File, c_Group
    use :: mpi_c_interface, only : MPIR_File_get_group_c

    implicit none

    type(MPI_File), intent(in) :: fh
    type(MPI_Group), intent(out) :: group
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(c_Group) :: group_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    if (c_int == kind(0)) then
        ierror_c = MPIR_File_get_group_c(fh_c, group%MPI_VAL)
    else
        ierror_c = MPIR_File_get_group_c(fh_c, group_c)
        group%MPI_VAL = group_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_get_group_f08
