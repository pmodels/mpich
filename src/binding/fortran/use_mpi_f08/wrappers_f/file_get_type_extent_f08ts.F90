!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_get_type_extent_f08(fh, datatype, extent, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_File, MPI_Datatype
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_File, c_Datatype
    use :: mpi_c_interface, only : MPIR_File_get_type_extent_c

    implicit none

    type(MPI_File), intent(in) :: fh
    type(MPI_Datatype), intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), intent(out) :: extent
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(c_Datatype) :: datatype_c
    integer(MPI_ADDRESS_KIND) :: extent_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    if (c_int == kind(0)) then
        ierror_c = MPIR_File_get_type_extent_c(fh_c, datatype%MPI_VAL, extent)
    else
        datatype_c = datatype%MPI_VAL
        ierror_c = MPIR_File_get_type_extent_c(fh_c, datatype_c, extent_c)
        extent = extent_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_get_type_extent_f08
