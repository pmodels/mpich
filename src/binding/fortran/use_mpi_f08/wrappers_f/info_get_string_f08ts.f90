!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

subroutine MPI_Info_get_string_f08(info, key, buflen, value, flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Info_get_string_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Info), intent(in) :: info
    character(len=*), intent(in) :: key
    integer, intent(inout) :: buflen
    character(len=*), intent(out) :: value
    logical, intent(out) :: flag
    integer, optional, intent(out) :: ierror

    integer(c_Info) :: info_c
    character(kind=c_char) :: key_c(len_trim(key)+1)
    integer(c_int) :: buflen_c
    character(kind=c_char) :: value_c(buflen+1)
    integer(c_int) :: flag_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(key, key_c)

    info_c = info%MPI_VAL
    if (buflen > 0) then
        buflen_c = buflen + 1
    else
        buflen_c = 0
    endif
    ierror_c = MPIR_Info_get_string_c(info_c, key_c, buflen_c, value_c, flag_c)

    flag = (flag_c /= 0)

    if (flag) then  ! value is unchanged when flag is false
        if (buflen > 0) then
            call MPIR_Fortran_string_c2f(value_c, value)
        endif
        buflen = buflen_c - 1
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Info_get_string_f08
