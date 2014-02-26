
subroutine MPI_Bsend_init_f08ts(buf, count, datatype, dest, tag, comm, ierror, request)
     use,intrinsic :: iso_c_binding, only: c_int
     use :: mpi_f08,           only: MPI_Datatype, MPI_Comm, MPI_Request
     use :: mpi_c_interface,        only: c_Datatype, c_Comm, c_Request, MPIR_Bsend_init_cdesc
     type(*), DIMENSION(..), intent(in) :: buf
     integer, intent(in) :: count, dest, tag
     type(MPI_Datatype), intent(in) :: datatype
     type(MPI_Comm), intent(in) :: comm
     type(MPI_Request), intent(out) :: request
     integer, optional, intent(out) :: ierror
     integer(c_int)          :: count_c
     integer(c_Datatype)     :: datatype_c
     integer(c_int)          :: dest_c
     integer(c_int)          :: tag_c
     integer(c_comm)         :: comm_c
     integer(c_Request)      :: request_c
     integer(c_int)          :: res

     count_c = count
     datatype_c = datatype%MPI_VAL
     dest_c = dest
     tag_c = tag
     comm_c = comm%MPI_VAL
     res = MPIR_Bsend_init_cdesc (buf, count_c, datatype_c, dest_c, tag_c, comm_c, request_c)
     request%MPI_VAL = request_c
     if (present(ierror)) ierror = res

end subroutine MPI_Bsend_init_f08ts

