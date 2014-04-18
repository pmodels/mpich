!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_set_view_f08(fh, disp, etype, filetype, datarep, info, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_File, MPI_Datatype, MPI_Info
    use :: mpi_f08, only : MPI_OFFSET_KIND
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File, c_Datatype, c_Info
    use :: mpi_c_interface, only : MPIR_File_set_view_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_File), intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(in) :: disp
    type(MPI_Datatype), intent(in) :: etype
    type(MPI_Datatype), intent(in) :: filetype
    character(len=*), intent(in) :: datarep
    type(MPI_Info), intent(in) :: info
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(MPI_OFFSET_KIND) :: disp_c
    integer(c_Datatype) :: etype_c
    integer(c_Datatype) :: filetype_c
    character(kind=c_char) :: datarep_c(len_trim(datarep)+1)
    integer(c_Info) :: info_c
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    call MPIR_Fortran_string_f2c(datarep, datarep_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_File_set_view_c(fh_c, disp, etype%MPI_VAL, filetype%MPI_VAL, datarep_c, info%MPI_VAL)
    else
        disp_c = disp
        etype_c = etype%MPI_VAL
        filetype_c = filetype%MPI_VAL
        info_c = info%MPI_VAL
        ierror_c = MPIR_File_set_view_c(fh_c, disp_c, etype_c, filetype_c, datarep_c, info_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_set_view_f08
