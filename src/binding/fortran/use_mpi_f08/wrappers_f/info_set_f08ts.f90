!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Info_set_f08(info, key, value, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Info_set_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Info), intent(in) :: info
    character(len=*), intent(in) :: key
    character(len=*), intent(in) :: value
    integer, optional, intent(out) :: ierror

    integer(c_Info) :: info_c
    character(kind=c_char) :: key_c(len_trim(key)+1)
    character(kind=c_char) :: value_c(len_trim(value)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(key, key_c)

    call MPIR_Fortran_string_f2c(value, value_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Info_set_c(info%MPI_VAL, key_c, value_c)
    else
        info_c = info%MPI_VAL
        ierror_c = MPIR_Info_set_c(info_c, key_c, value_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Info_set_f08
