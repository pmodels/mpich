
subroutine MPI_Comm_size_f08(comm, size, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only: c_Comm, MPIR_Comm_size_c

    type(MPI_Comm), intent(in)     :: comm
    integer, intent(out)           :: size
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int)  :: size_c
    integer(c_int)  :: res

    comm_c = comm%MPI_VAL
    res = MPIR_Comm_size_c (comm_c, size_c)
    size = size_c
    if (present(ierror)) ierror = res

end subroutine MPI_Comm_size_f08
