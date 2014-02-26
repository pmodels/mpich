
subroutine MPI_Test_f08 (request, flag, status, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Request, MPI_Status, assignment(=)
    use :: mpi_c_interface, only: C_Request, c_int, c_Status, MPIR_Test_c

    type(MPI_Request), intent(inout) :: request
    LOGICAL,           intent(out)   :: flag
    type(MPI_Status)                 :: status
    integer,optional,  intent(out)   :: ierror
    integer(C_Request) :: request_c
    integer(c_int)     :: flag_c
    type(c_Status) :: status_c
    integer(c_int)     :: res

    request_c = request%MPI_VAL
    status_c = status
    res = MPIR_Test_C (request_c, flag_c, status_c)
    request%MPI_VAL = request_c
    status = status_c
    flag = (flag_c /= 0)
    if (present(ierror)) ierror = res

end subroutine MPI_Test_f08
