!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Ireduce_scatter_block_f08ts(sendbuf, recvbuf, recvcount, datatype, op, comm, &
    request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Op, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Ireduce_scatter_block_cdesc

    implicit none

    type(*), dimension(..), intent(in), asynchronous :: sendbuf
    type(*), dimension(..), asynchronous :: recvbuf
    integer, intent(in) :: recvcount
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Op), intent(in) :: op
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_int) :: recvcount_c
    integer(c_Datatype) :: datatype_c
    integer(c_Op) :: op_c
    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Ireduce_scatter_block_cdesc(sendbuf, recvbuf, recvcount, datatype%MPI_VAL, op%MPI_VAL, &
            comm%MPI_VAL, request%MPI_VAL)
    else
        recvcount_c = recvcount
        datatype_c = datatype%MPI_VAL
        op_c = op%MPI_VAL
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Ireduce_scatter_block_cdesc(sendbuf, recvbuf, recvcount_c, datatype_c, op_c, &
            comm_c, request_c)
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Ireduce_scatter_block_f08ts
