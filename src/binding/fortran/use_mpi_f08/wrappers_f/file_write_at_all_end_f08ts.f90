!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_write_at_all_end_f08ts(fh, buf, status, ierror)
    use, intrinsic :: iso_c_binding, only : c_loc, c_associated
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08, only : MPI_File, MPI_Status
    use :: mpi_f08, only : MPI_STATUS_IGNORE, MPIR_C_MPI_STATUS_IGNORE, assignment(=)
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File
    use :: mpi_c_interface, only : c_Status
    use :: mpi_c_interface, only : MPIR_File_write_at_all_end_cdesc

    implicit none

    type(MPI_File), intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    type(MPI_Status), target :: status
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    type(c_Status), target :: status_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    if (c_associated(c_loc(status), c_loc(MPI_STATUS_IGNORE))) then
        ierror_c = MPIR_File_write_at_all_end_cdesc(fh_c, buf, MPIR_C_MPI_STATUS_IGNORE)
    else
        ierror_c = MPIR_File_write_at_all_end_cdesc(fh_c, buf, c_loc(status_c))
        status = status_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_write_at_all_end_f08ts
