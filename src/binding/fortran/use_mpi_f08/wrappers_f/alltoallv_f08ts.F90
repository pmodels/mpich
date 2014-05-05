!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Alltoallv_f08ts(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, &
    rdispls, recvtype, comm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm
    use :: mpi_c_interface, only : c_Datatype, c_Comm
    use :: mpi_c_interface, only : MPIR_Alltoallv_cdesc, MPIR_Comm_size_c

    implicit none

    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer, intent(in) :: sendcounts(*)
    integer, intent(in) :: sdispls(*)
    integer, intent(in) :: recvcounts(*)
    integer, intent(in) :: rdispls(*)
    type(MPI_Datatype), intent(in) :: sendtype
    type(MPI_Datatype), intent(in) :: recvtype
    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_int), allocatable :: sendcounts_c(:)
    integer(c_int), allocatable :: sdispls_c(:)
    integer(c_int), allocatable :: recvcounts_c(:)
    integer(c_int), allocatable :: rdispls_c(:)

    integer(c_Datatype) :: sendtype_c
    integer(c_Datatype) :: recvtype_c
    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c
    integer(c_int) :: err, length ! To get length of assumed-size arrays

    if (c_int == kind(0)) then
        ierror_c = MPIR_Alltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype%MPI_VAL, recvbuf, recvcounts, &
                    rdispls, recvtype%MPI_VAL, comm%MPI_VAL)
    else
        sendtype_c = sendtype%MPI_VAL
        recvtype_c = recvtype%MPI_VAL
        comm_c = comm%MPI_VAL
        err = MPIR_Comm_size_c(comm_c, length);
        sendcounts_c = sendcounts(1:length);
        sdispls_c = sdispls(1:length)
        recvcounts_c = recvcounts(1:length)
        rdispls_c = sdispls(1:length)

        ierror_c = MPIR_Alltoallv_cdesc(sendbuf, sendcounts_c, sdispls_c, sendtype_c, recvbuf, recvcounts_c, &
                    rdispls_c, recvtype_c, comm_c)
    endif

    if(present(ierror)) ierror = ierror_c

end subroutine MPI_Alltoallv_f08ts
