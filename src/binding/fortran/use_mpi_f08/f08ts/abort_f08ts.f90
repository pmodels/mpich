
subroutine MPI_Abort_f08(comm, errorcode, ierror)
     use,intrinsic :: iso_c_binding, only: c_int
     use :: mpi_f08,          only: MPI_Comm
     use :: mpi_c_interface      ,  only: c_Comm, MPIR_Abort_c
     type(MPI_Comm),      intent(in)  :: comm
     integer,             intent(in)  :: errorcode
     integer, optional,   intent(out) :: ierror
     integer(c_Comm) :: comm_c
     integer(c_int)  :: errorcode_c
     integer(c_int)  :: res

     comm_c = comm%MPI_VAL
     errorcode_c = errorcode
     res = MPIR_Abort_c (comm_c, errorcode_c)
     if (present(ierror)) ierror = res

end subroutine MPI_Abort_f08
