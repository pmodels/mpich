!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_sync_f08(fh, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_File
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File
    use :: mpi_c_interface, only : MPIR_File_sync_c

    implicit none

    type(MPI_File), intent(in) :: fh
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    ierror_c = MPIR_File_sync_c(fh_c)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_sync_f08
