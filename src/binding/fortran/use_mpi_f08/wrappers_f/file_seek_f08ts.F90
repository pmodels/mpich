!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_seek_f08(fh, offset, whence, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_File
    use :: mpi_f08, only : MPI_OFFSET_KIND
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File
    use :: mpi_c_interface, only : MPIR_File_seek_c

    implicit none

    type(MPI_File), intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(in) :: offset
    integer, intent(in) :: whence
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(MPI_OFFSET_KIND) :: offset_c
    integer(c_int) :: whence_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    if (c_int == kind(0)) then
        ierror_c = MPIR_File_seek_c(fh_c, offset, whence)
    else
        offset_c = offset
        whence_c = whence
        ierror_c = MPIR_File_seek_c(fh_c, offset_c, whence_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_seek_f08
