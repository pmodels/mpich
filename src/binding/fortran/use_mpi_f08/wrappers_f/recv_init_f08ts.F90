!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Recv_init_f08ts(buf, count, datatype, source, tag, comm, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Recv_init_cdesc

    implicit none

    type(*), dimension(..) :: buf
    integer, intent(in) :: count
    integer, intent(in) :: source
    integer, intent(in) :: tag
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: source_c
    integer(c_int) :: tag_c
    integer(c_Datatype) :: datatype_c
    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Recv_init_cdesc(buf, count, datatype%MPI_VAL, source, tag, comm%MPI_VAL, request%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        source_c = source
        tag_c = tag
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Recv_init_cdesc(buf, count_c, datatype_c, source_c, tag_c, comm_c, request_c)
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Recv_init_f08ts
