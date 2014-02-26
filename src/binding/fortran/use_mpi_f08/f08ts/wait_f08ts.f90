
subroutine MPI_Wait_f08(request, status, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Request, MPI_Status, assignment(=)
    use :: mpi_c_interface, only: c_Request, MPIR_Wait_c, c_Status

    type(MPI_Request), intent(inout) :: request
    type(MPI_Status)                 :: status
    integer, optional, intent(out)   :: ierror

    integer(C_Request) :: request_c
    integer(c_int)     :: res
    type(c_Status) :: status_c

    request_c = request%MPI_VAL
    res = MPIR_Wait_c (request_c, status_c)
    request%MPI_VAL = request_c
    status = status_c
    if (present(ierror)) ierror = res

end subroutine MPI_Wait_f08

