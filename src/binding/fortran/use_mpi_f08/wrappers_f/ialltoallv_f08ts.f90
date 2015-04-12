!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Ialltoallv_f08ts(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, &
    rdispls, recvtype, comm, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Ialltoallv_cdesc, MPIR_Comm_size_c

    implicit none

    type(*), dimension(..), intent(in), asynchronous :: sendbuf
    type(*), dimension(..), asynchronous :: recvbuf
    integer, intent(in)  :: sendcounts(*)
    integer, intent(in)  :: sdispls(*)
    integer, intent(in)  :: recvcounts(*)
    integer, intent(in)  :: rdispls(*)
    type(MPI_Datatype), intent(in)  :: sendtype
    type(MPI_Datatype), intent(in)  :: recvtype
    type(MPI_Comm), intent(in)  :: comm
    type(MPI_Request), intent(out)  :: request
    integer, optional, intent(out)  :: ierror

    integer(c_int), allocatable :: sendcounts_c(:)
    integer(c_int), allocatable :: sdispls_c(:)
    integer(c_int), allocatable :: recvcounts_c(:)
    integer(c_int), allocatable :: rdispls_c(:)
    integer(c_Datatype) :: sendtype_c
    integer(c_Datatype) :: recvtype_c
    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c
    integer(c_int) :: err, length ! To get length of assumed-size arrays

    if (c_int == kind(0)) then
        ierror_c = MPIR_Ialltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype%MPI_VAL, recvbuf, recvcounts, &
            rdispls, recvtype%MPI_VAL, comm%MPI_VAL, request%MPI_VAL)
    else
        sendtype_c = sendtype%MPI_VAL
        recvtype_c = recvtype%MPI_VAL
        comm_c = comm%MPI_VAL
        err = MPIR_Comm_size_c(comm_c, length)
        sendcounts_c = sendcounts(1:length)
        sdispls_c = sdispls(1:length)
        recvcounts_c = recvcounts(1:length)
        rdispls_c = rdispls(1:length)

        ierror_c = MPIR_Ialltoallv_cdesc(sendbuf, sendcounts_c, sdispls_c, sendtype_c, recvbuf, recvcounts_c, rdispls_c, &
            recvtype_c, comm_c, request_c)

        request%MPI_VAL = request_c
    end if

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Ialltoallv_f08ts
