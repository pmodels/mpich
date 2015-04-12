!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_get_errhandler_f08(file, errhandler, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_File, MPI_Errhandler
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File, c_Errhandler
    use :: mpi_c_interface, only : MPIR_File_get_errhandler_c

    implicit none

    type(MPI_File), intent(in) :: file
    type(MPI_Errhandler), intent(out) :: errhandler
    integer, optional, intent(out) :: ierror

    integer(c_File) :: file_c
    integer(c_Errhandler) :: errhandler_c
    integer(c_int) :: ierror_c

    file_c = MPI_File_f2c(file%MPI_VAL)
    if (c_int == kind(0)) then
        ierror_c = MPIR_File_get_errhandler_c(file_c, errhandler%MPI_VAL)
    else
        ierror_c = MPIR_File_get_errhandler_c(file_c, errhandler_c)
        errhandler%MPI_VAL = errhandler_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_get_errhandler_f08
