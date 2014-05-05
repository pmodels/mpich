!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_open_f08(comm, filename, amode, info, fh, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Comm, MPI_Info, MPI_File
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_Comm, c_Info, c_File
    use :: mpi_c_interface, only : MPIR_File_open_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    character(len=*), intent(in) :: filename
    integer, intent(in) :: amode
    type(MPI_Info), intent(in) :: info
    type(MPI_File), intent(out) :: fh
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    character(kind=c_char) :: filename_c(len_trim(filename)+1)
    integer(c_int) :: amode_c
    integer(c_Info) :: info_c
    integer(c_File) :: fh_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(filename, filename_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_File_open_c(comm%MPI_VAL, filename_c, amode, info%MPI_VAL, fh_c)
    else
        comm_c = comm%MPI_VAL
        amode_c = amode
        info_c = info%MPI_VAL
        ierror_c = MPIR_File_open_c(comm_c, filename_c, amode_c, info_c, fh_c)
    end if

    fh%MPI_VAL = MPI_File_c2f(fh_c)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_open_f08
