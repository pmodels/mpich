!--> MPIR_Error_string

subroutine MPIR_Error_string_f08 (errorcode, string, resultlen, ierror)
    use, intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_MAX_ERROR_STRING
    use :: mpi_c_interface, only: c_char, MPIR_Error_string_c

    integer,                              intent(in) :: errorcode
    CHARACTER(LEN=MPI_MAX_ERROR_STRING), intent(out) :: string
    integer,                             intent(out) :: resultlen
    integer, optional,                   intent(out) :: ierror
    integer(c_int) :: errorcode_c
    integer(c_int) :: resultlen_c
    integer(c_int) :: res

    errorcode_c = errorcode
    res = MPIR_Error_string_C (errorcode_c, string, resultlen_c)
    resultlen = resultlen_c
    if (present(ierror)) ierror = res

end subroutine MPIR_Error_string_f08
