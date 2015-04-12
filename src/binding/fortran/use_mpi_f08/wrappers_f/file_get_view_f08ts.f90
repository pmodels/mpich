!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_File_get_view_f08(fh, disp, etype, filetype, datarep, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_File, MPI_Datatype
    use :: mpi_f08, only : MPI_OFFSET_KIND
    use :: mpi_f08, only : MPI_File_f2c, MPI_File_c2f
    use :: mpi_c_interface, only : c_File, c_Datatype
    use :: mpi_c_interface, only : MPIR_File_get_view_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    type(MPI_File), intent(in) :: fh
    integer(MPI_OFFSET_KIND), intent(out) :: disp
    type(MPI_Datatype), intent(out) :: etype
    type(MPI_Datatype), intent(out) :: filetype
    character(len=*), intent(out) :: datarep
    integer, optional, intent(out) :: ierror

    integer(c_File) :: fh_c
    integer(MPI_OFFSET_KIND) :: disp_c
    integer(c_Datatype) :: etype_c
    integer(c_Datatype) :: filetype_c
    character(kind=c_char) :: datarep_c(len(datarep)+1)
    integer(c_int) :: ierror_c

    fh_c = MPI_File_f2c(fh%MPI_VAL)
    if (c_int == kind(0)) then
        ierror_c = MPIR_File_get_view_c(fh_c, disp, etype%MPI_VAL, filetype%MPI_VAL, datarep_c)
    else
        ierror_c = MPIR_File_get_view_c(fh_c, disp_c, etype_c, filetype_c, datarep_c)
        disp = disp_c
        etype%MPI_VAL = etype_c
        filetype%MPI_VAL = filetype_c
    end if

    call MPIR_Fortran_string_c2f(datarep_c, datarep)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_File_get_view_f08
