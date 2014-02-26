
subroutine MPI_Irecv_f08ts(buf, count, datatype, source, tag, comm, request, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Request, MPI_Comm
    use :: mpi_c_interface, only: c_Datatype, c_Comm, C_Request, MPIR_Irecv_cdesc

    type(*), DIMENSION(..), ASYNCHRONOUS :: buf
    integer,            intent(in)       :: count, source, tag
    type(MPI_Datatype), intent(in)       :: datatype
    type(MPI_Comm),     intent(in)       :: comm
    type(MPI_Request), intent(out)       :: request
    integer, optional, intent(out)       :: ierror
    integer(c_int)      :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_int)      :: source_c
    integer(c_int)      :: tag_c
    integer(c_Comm)     :: comm_c
    integer(C_Request)  :: request_c
    integer(c_int)      :: res

    count_c    = count
    datatype_c = datatype%MPI_VAL
    source_c   = source
    tag_c      = tag
    comm_c     = comm%MPI_VAL

    res = MPIR_Irecv_cdesc (buf,count_c,datatype_c,source_c,tag_c,comm_c, request_c)
    request%MPI_VAL = request_c
    if (present(ierror)) ierror = res

end subroutine MPI_Irecv_f08ts
