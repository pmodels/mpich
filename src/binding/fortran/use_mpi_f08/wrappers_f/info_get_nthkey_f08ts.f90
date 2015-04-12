!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Info_get_nthkey_f08(info, n, key, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Info_get_nthkey_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    type(MPI_Info), intent(in) :: info
    integer, intent(in) :: n
    character(len=*), intent(out) :: key
    integer, optional, intent(out) :: ierror

    integer(c_Info) :: info_c
    integer(c_int) :: n_c
    character(kind=c_char) :: key_c(len(key)+1)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Info_get_nthkey_c(info%MPI_VAL, n, key_c)
    else
        info_c = info%MPI_VAL
        n_c = n
        ierror_c = MPIR_Info_get_nthkey_c(info_c, n_c, key_c)
    end if

    call MPIR_Fortran_string_c2f(key_c, key)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Info_get_nthkey_f08
