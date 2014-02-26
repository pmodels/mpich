
subroutine MPI_Barrier_f08(comm, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only: c_Comm, MPIR_Barrier_c

    type(MPI_Comm),      intent(in)  :: comm
    integer, optional,   intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int)  :: res

    comm_c = comm%MPI_VAL
    res = MPIR_Barrier_C (comm_c)
    if (present(ierror)) ierror = res

end subroutine MPI_Barrier_f08
