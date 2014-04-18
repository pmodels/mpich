!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_delete_f08(filename, info, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_File_delete_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: filename
    type(MPI_Info), intent(in) :: info
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: filename_c(len_trim(filename)+1)
    integer(c_Info) :: info_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(filename, filename_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_File_delete_c(filename_c, info%MPI_VAL)
    else
        info_c = info%MPI_VAL
        ierror_c = MPIR_File_delete_c(filename_c, info_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_delete_f08
