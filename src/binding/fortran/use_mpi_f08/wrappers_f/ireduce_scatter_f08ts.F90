!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Ireduce_scatter_f08ts(sendbuf, recvbuf, recvcounts, datatype, op, comm, &
    request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Op, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Op, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Ireduce_scatter_cdesc, MPIR_Comm_size_c

    implicit none

    type(*), dimension(..), intent(in), asynchronous  :: sendbuf
    type(*), dimension(..), asynchronous  :: recvbuf
    integer, intent(in)  :: recvcounts(*)
    type(MPI_Datatype), intent(in)  :: datatype
    type(MPI_Op), intent(in)  :: op
    type(MPI_Comm), intent(in)  :: comm
    type(MPI_Request), intent(out)  :: request
    integer, optional, intent(out)  :: ierror

    integer(c_int), allocatable :: recvcounts_c(:)
    integer(c_Datatype) :: datatype_c
    integer(c_Op) :: op_c
    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c
    integer(c_int) :: err, length ! To get length of assumed-size arrays

    if (c_int == kind(0)) then
        ierror_c = MPIR_Ireduce_scatter_cdesc(sendbuf, recvbuf, recvcounts, datatype%MPI_VAL, &
            op%MPI_VAL, comm%MPI_VAL, request%MPI_VAL)
    else
        datatype_c = datatype%MPI_VAL
        op_c = op%MPI_VAL
        comm_c = comm%MPI_VAL
        err = MPIR_Comm_size_c(comm_c, length)
        recvcounts_c = recvcounts(1:length)

        ierror_c = MPIR_Ireduce_scatter_cdesc(sendbuf, recvbuf, recvcounts_c, datatype_c, op_c, comm_c, request_c)
        request%MPI_VAL = request_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Ireduce_scatter_f08ts
