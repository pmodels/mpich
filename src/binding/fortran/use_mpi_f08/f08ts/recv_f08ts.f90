
subroutine MPI_Recv_f08ts(buf, count, datatype, source, tag, comm, status, ierror)
    use,intrinsic :: iso_c_binding, only: c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Status, MPI_Comm, assignment(=)
    use :: mpi_c_interface, only: c_Datatype, c_Comm, MPIR_Recv_cdesc, c_Status

    type(*), DIMENSION(..)        :: buf
    integer,           intent(in) :: count, source, tag
    type(MPI_Datatype),intent(in) :: datatype
    type(MPI_Comm),    intent(in) :: comm
    type(MPI_Status), intent(out) :: status
    integer, optional,intent(out) :: ierror

    integer(c_int)      :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_int)      :: source_c
    integer(c_int)      :: tag_c
    integer(c_Comm)     :: comm_c
    type(c_Status)  :: status_c
    integer(c_int)      :: res

    count_c    = count
    datatype_c = datatype%MPI_VAL
    source_c   = source
    tag_c      = tag
    comm_c     = comm%MPI_VAL

    res = MPIR_Recv_cdesc (buf,count_c,datatype_c,source_c,tag_c,comm_c, status_c)
    status = status_c
    if (present(ierror)) ierror = res

end subroutine MPI_Recv_f08ts
