!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
module mpi_c_interface_cdesc

implicit none

interface

function MPIR_Bsend_cdesc(buf, count, datatype, dest, tag, comm) &
    bind(C, name="MPIR_Bsend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Bsend_cdesc

function MPIR_Bsend_init_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Bsend_init_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Bsend_init_cdesc

function MPIR_Buffer_attach_cdesc(buffer, size) &
    bind(C, name="MPIR_Buffer_attach_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    type(*), dimension(..) :: buffer
    integer(c_int), value, intent(in) :: size
    integer(c_int) :: ierror
end function MPIR_Buffer_attach_cdesc

function MPIR_Ibsend_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Ibsend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ibsend_cdesc

function MPIR_Irecv_cdesc(buf, count, datatype, source, tag, comm, request) &
    bind(C, name="MPIR_Irecv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, source, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Irecv_cdesc

function MPIR_Irsend_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Irsend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Irsend_cdesc

function MPIR_Isend_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Isend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Isend_cdesc

function MPIR_Issend_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Issend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Issend_cdesc

function MPIR_Recv_cdesc(buf, count, datatype, source, tag, comm, status) &
    bind(C, name="MPIR_Recv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, source, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Recv_cdesc

function MPIR_Recv_init_cdesc(buf, count, datatype, source, tag, comm, request) &
    bind(C, name="MPIR_Recv_init_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, source, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Recv_init_cdesc

function MPIR_Rsend_cdesc(buf, count, datatype, dest, tag, comm) &
    bind(C, name="MPIR_Rsend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Rsend_cdesc

function MPIR_Rsend_init_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Rsend_init_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Rsend_init_cdesc

function MPIR_Send_cdesc(buf, count, datatype, dest, tag, comm) &
    bind(C, name="MPIR_Send_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Send_cdesc

function MPIR_Sendrecv_cdesc(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, &
           recvcount, recvtype, source, recvtag, comm, status) &
    bind(C, name="MPIR_Sendrecv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, dest, sendtag, recvcount, source, recvtag
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Sendrecv_cdesc

function MPIR_Sendrecv_replace_cdesc(buf, count, datatype, dest, sendtag, source, recvtag, &
           comm, status) &
    bind(C, name="MPIR_Sendrecv_replace_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, dest, sendtag, source, recvtag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Sendrecv_replace_cdesc

function MPIR_Send_init_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Send_init_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Send_init_cdesc

function MPIR_Ssend_cdesc(buf, count, datatype, dest, tag, comm) &
    bind(C, name="MPIR_Ssend_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Ssend_cdesc

function MPIR_Ssend_init_cdesc(buf, count, datatype, dest, tag, comm, request) &
    bind(C, name="MPIR_Ssend_init_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count, dest, tag
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ssend_init_cdesc

function MPIR_Get_address_cdesc(location, address) &
    bind(C, name="MPIR_Get_address_cdesc") result(ierror)
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    type(*), dimension(..) :: location
    integer(MPI_ADDRESS_KIND), intent(out) :: address
    integer(c_int) :: ierror
end function MPIR_Get_address_cdesc

function MPIR_Pack_cdesc(inbuf, incount, datatype, outbuf, outsize, position, comm) &
    bind(C, name="MPIR_Pack_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer(c_int), value, intent(in) :: incount, outsize
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), intent(inout) :: position
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Pack_cdesc

function MPIR_Pack_external_cdesc(datarep, inbuf, incount, datatype, outbuf, outsize, position) &
    bind(C, name="MPIR_Pack_external_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    character(kind=c_char), intent(in) :: datarep(*)
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer(c_int), value, intent(in) :: incount
    integer(c_Datatype), value, intent(in) :: datatype
    integer(MPI_ADDRESS_KIND), value, intent(in) :: outsize
    integer(MPI_ADDRESS_KIND), intent(inout) :: position
    integer(c_int) :: ierror
end function MPIR_Pack_external_cdesc

function MPIR_Unpack_cdesc(inbuf, insize, position, outbuf, outcount, datatype, comm) &
    bind(C, name="MPIR_Unpack_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer(c_int), value, intent(in) :: insize, outcount
    integer(c_int), intent(inout) :: position
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Unpack_cdesc

function MPIR_Unpack_external_cdesc(datarep, inbuf, insize, position, outbuf, outcount, datatype) &
    bind(C, name="MPIR_Unpack_external_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype
    implicit none
    character(kind=c_char), intent(in) :: datarep(*)
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: outbuf
    integer(MPI_ADDRESS_KIND), value, intent(in) :: insize
    integer(MPI_ADDRESS_KIND), intent(inout) :: position
    integer(c_int), value, intent(in) :: outcount
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_Unpack_external_cdesc

function MPIR_Allgather_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm) &
    bind(C, name="MPIR_Allgather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Allgather_cdesc

function MPIR_Iallgather_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request) &
    bind(C, name="MPIR_Iallgather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iallgather_cdesc

function MPIR_Allgatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm) &
    bind(C, name="MPIR_Allgatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Allgatherv_cdesc

function MPIR_Iallgatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, &
           recvtype, comm, request) &
    bind(C, name="MPIR_Iallgatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iallgatherv_cdesc

function MPIR_Allreduce_cdesc(sendbuf, recvbuf, count, datatype, op, comm) &
    bind(C, name="MPIR_Allreduce_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Allreduce_cdesc

function MPIR_Iallreduce_cdesc(sendbuf, recvbuf, count, datatype, op, comm, request) &
    bind(C, name="MPIR_Iallreduce_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iallreduce_cdesc

function MPIR_Alltoall_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm) &
    bind(C, name="MPIR_Alltoall_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Alltoall_cdesc

function MPIR_Ialltoall_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request) &
    bind(C, name="MPIR_Ialltoall_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ialltoall_cdesc

function MPIR_Alltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, &
           rdispls, recvtype, comm) &
    bind(C, name="MPIR_Alltoallv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Alltoallv_cdesc

function MPIR_Ialltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype, recvbuf, recvcounts, &
           rdispls, recvtype, comm, request) &
    bind(C, name="MPIR_Ialltoallv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ialltoallv_cdesc

function MPIR_Alltoallw_cdesc(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, &
           rdispls, recvtypes, comm) &
    bind(C, name="MPIR_Alltoallw_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), intent(in) :: sendtypes(*), recvtypes(*)
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Alltoallw_cdesc

function MPIR_Ialltoallw_cdesc(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, recvcounts, &
           rdispls, recvtypes, comm, request) &
    bind(C, name="MPIR_Ialltoallw_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), intent(in) :: sendtypes(*), recvtypes(*)
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ialltoallw_cdesc

function MPIR_Bcast_cdesc(buffer, count, datatype, root, comm) &
    bind(C, name="MPIR_Bcast_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..) :: buffer
    integer(c_int), value, intent(in) :: count, root
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Bcast_cdesc

function MPIR_Ibcast_cdesc(buffer, count, datatype, root, comm, request) &
    bind(C, name="MPIR_Ibcast_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..) :: buffer
    integer(c_int), value, intent(in) :: count, root
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ibcast_cdesc

function MPIR_Exscan_cdesc(sendbuf, recvbuf, count, datatype, op, comm) &
    bind(C, name="MPIR_Exscan_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Exscan_cdesc

function MPIR_Iexscan_cdesc(sendbuf, recvbuf, count, datatype, op, comm, request) &
    bind(C, name="MPIR_Iexscan_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iexscan_cdesc

function MPIR_Gather_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm) &
    bind(C, name="MPIR_Gather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount, root
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Gather_cdesc

function MPIR_Igather_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, &
            root, comm, request) &
    bind(C, name="MPIR_Igather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount, root
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Igather_cdesc

function MPIR_Gatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, &
           root, comm) &
    bind(C, name="MPIR_Gatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, root
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Gatherv_cdesc

function MPIR_Igatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, &
           recvtype, root, comm, request) &
    bind(C, name="MPIR_Igatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, root
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Igatherv_cdesc

function MPIR_Reduce_cdesc(sendbuf, recvbuf, count, datatype, op, root, comm) &
    bind(C, name="MPIR_Reduce_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count, root
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Reduce_cdesc

function MPIR_Ireduce_cdesc(sendbuf, recvbuf, count, datatype, op, root, comm, request) &
    bind(C, name="MPIR_Ireduce_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count, root
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ireduce_cdesc

function MPIR_Reduce_local_cdesc(inbuf, inoutbuf, count, datatype, op) &
    bind(C, name="MPIR_Reduce_local_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op
    implicit none
    type(*), dimension(..), intent(in) :: inbuf
    type(*), dimension(..) :: inoutbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_int) :: ierror
end function MPIR_Reduce_local_cdesc

function MPIR_Reduce_scatter_cdesc(sendbuf, recvbuf, recvcounts, datatype, op, comm) &
    bind(C, name="MPIR_Reduce_scatter_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: recvcounts(*)
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Reduce_scatter_cdesc

function MPIR_Ireduce_scatter_cdesc(sendbuf, recvbuf, recvcounts, datatype, op, comm, request) &
    bind(C, name="MPIR_Ireduce_scatter_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: recvcounts(*)
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ireduce_scatter_cdesc

function MPIR_Reduce_scatter_block_cdesc(sendbuf, recvbuf, recvcount, datatype, op, comm) &
    bind(C, name="MPIR_Reduce_scatter_block_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: recvcount
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Reduce_scatter_block_cdesc

function MPIR_Ireduce_scatter_block_cdesc(sendbuf, recvbuf, recvcount, datatype, op, comm, request) &
    bind(C, name="MPIR_Ireduce_scatter_block_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: recvcount
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ireduce_scatter_block_cdesc

function MPIR_Scan_cdesc(sendbuf, recvbuf, count, datatype, op, comm) &
    bind(C, name="MPIR_Scan_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Scan_cdesc

function MPIR_Iscan_cdesc(sendbuf, recvbuf, count, datatype, op, comm, request) &
    bind(C, name="MPIR_Iscan_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iscan_cdesc

function MPIR_Scatter_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm) &
    bind(C, name="MPIR_Scatter_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount, root
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Scatter_cdesc

function MPIR_Iscatter_cdesc(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request) &
    bind(C, name="MPIR_Iscatter_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount, root
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iscatter_cdesc

function MPIR_Scatterv_cdesc(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm) &
    bind(C, name="MPIR_Scatterv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: recvcount, root
    integer(c_int), intent(in) :: sendcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Scatterv_cdesc

function MPIR_Iscatterv_cdesc(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, &
           recvtype, root, comm, request) &
    bind(C, name="MPIR_Iscatterv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: recvcount, root
    integer(c_int), intent(in) :: sendcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Iscatterv_cdesc

function MPIR_Accumulate_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
           target_disp, target_count, target_datatype, op, win) &
    bind(C, name="MPIR_Accumulate_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Win
    implicit none
    type(*), dimension(..), intent(in) :: origin_addr
    integer(c_int), value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype
    integer(MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Datatype), value, intent(in) :: target_datatype
    integer(c_Op), value, intent(in) :: op
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Accumulate_cdesc

function MPIR_Compare_and_swap_cdesc(origin_addr, compare_addr, result_addr, datatype, &
        target_rank, target_disp, win) &
    bind(C, name="MPIR_Compare_and_swap_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Win
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    type(*), dimension(..), intent(in), asynchronous :: compare_addr
    type(*), dimension(..), asynchronous :: result_addr
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: target_rank
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Compare_and_swap_cdesc

function MPIR_Fetch_and_op_cdesc(origin_addr, result_addr, datatype, target_rank, &
        target_disp, op, win) &
    bind(C, name="MPIR_Fetch_and_op_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    type(*), dimension(..), asynchronous :: result_addr
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int), value, intent(in) :: target_rank
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Op), value, intent(in) :: op
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Fetch_and_op_cdesc

function MPIR_Get_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
           target_disp, target_count, target_datatype, win) &
    bind(C, name="MPIR_Get_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Win
    implicit none
    type(*), dimension(..) :: origin_addr
    integer(c_int), value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype
    integer(MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Datatype), value, intent(in) :: target_datatype
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Get_cdesc

function MPIR_Get_accumulate_cdesc(origin_addr, origin_count, origin_datatype, result_addr, &
        result_count, result_datatype, target_rank, target_disp, &
        target_count, target_datatype, op, win) &
    bind(C, name="MPIR_Get_accumulate_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    type(*), dimension(..), asynchronous :: result_addr
    integer(c_int), value, intent(in) :: origin_count, result_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype, target_datatype, result_datatype
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Op), value, intent(in) :: op
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Get_accumulate_cdesc

function MPIR_Put_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
           target_disp, target_count, target_datatype, win) &
    bind(C, name="MPIR_Put_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Win
    implicit none
    type(*), dimension(..), intent(in) :: origin_addr
    integer(c_int), value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype
    integer(MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Datatype), value, intent(in) :: target_datatype
    integer(c_Win), value, intent(in) :: win
    integer(c_int) :: ierror
end function MPIR_Put_cdesc

function MPIR_Raccumulate_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
        target_disp, target_count, target_datatype, op, win, request) &
    bind(C, name="MPIR_Raccumulate_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Win, c_Request
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    integer, value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype, target_datatype
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Op), value, intent(in) :: op
    integer(c_Win), value, intent(in) :: win
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Raccumulate_cdesc

function MPIR_Rget_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
        target_disp, target_count, target_datatype, win, request) &
    bind(C, name="MPIR_Rget_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Win, c_Request
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), asynchronous :: origin_addr
    integer, value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype, target_datatype
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Win), value, intent(in) :: win
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Rget_cdesc

function MPIR_Rget_accumulate_cdesc(origin_addr, origin_count, origin_datatype, &
        result_addr, result_count, result_datatype, target_rank, &
        target_disp, target_count, target_datatype, op, win, request) &
    bind(C, name="MPIR_Rget_accumulate_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Op, c_Win, c_Request
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    type(*), dimension(..), asynchronous :: result_addr
    integer, value, intent(in) :: origin_count, result_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype, target_datatype, result_datatype
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Op), value, intent(in) :: op
    integer(c_Win), value, intent(in) :: win
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Rget_accumulate_cdesc

function MPIR_Rput_cdesc(origin_addr, origin_count, origin_datatype, target_rank, &
        target_disp, target_count, target_datatype, win, request) &
    bind(C, name="MPIR_Rput_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Win, c_Request
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: origin_addr
    integer, value, intent(in) :: origin_count, target_rank, target_count
    integer(c_Datatype), value, intent(in) :: origin_datatype, target_datatype
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: target_disp
    integer(c_Win), value, intent(in) :: win
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Rput_cdesc

function MPIR_Win_attach_cdesc(win, base, size) &
    bind(C, name="MPIR_Win_attach_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    implicit none
    integer(c_Win), value, intent(in) :: win
    type(*), dimension(..), asynchronous :: base
    integer(kind=MPI_ADDRESS_KIND), value, intent(in) :: size
    integer(c_int) :: ierror
end function MPIR_Win_attach_cdesc

function MPIR_Win_create_cdesc(base, size, disp_unit, info, comm, win) &
    bind(C, name="MPIR_Win_create_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Info, c_Comm, c_Win
    implicit none
    type(*), dimension(..) :: base
    integer(MPI_ADDRESS_KIND), value, intent(in) :: size
    integer(c_int), value, intent(in) :: disp_unit
    integer(c_Info), value, intent(in) :: info
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Win), intent(out) :: win
    integer(c_int) :: ierror
end function MPIR_Win_create_cdesc

function MPIR_Win_detach_cdesc(win, base) &
    bind(C, name="MPIR_Win_detach_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Win
    implicit none
    integer(c_Win), value, intent(in) :: win
    type(*), dimension(..), asynchronous :: base
    integer(c_int) :: ierror
end function MPIR_Win_detach_cdesc

function MPIR_File_iread_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iread_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iread_cdesc

function MPIR_File_iread_at_cdesc(fh, offset, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iread_at_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iread_at_cdesc

function MPIR_File_iread_shared_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iread_shared_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iread_shared_cdesc

function MPIR_File_iwrite_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iwrite_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iwrite_cdesc

function MPIR_File_iwrite_at_cdesc(fh, offset, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iwrite_at_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iwrite_at_cdesc

function MPIR_File_iwrite_shared_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iwrite_shared_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: buf
    integer(c_File), value, intent(in) :: fh
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iwrite_shared_cdesc

function MPIR_File_read_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_cdesc

function MPIR_File_read_all_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_all_cdesc

function MPIR_File_read_all_begin_cdesc(fh, buf, count, datatype) &
    bind(C, name="MPIR_File_read_all_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_read_all_begin_cdesc

function MPIR_File_read_all_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_read_all_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_all_end_cdesc

function MPIR_File_read_at_cdesc(fh, offset, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_at_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_at_cdesc

function MPIR_File_read_at_all_cdesc(fh, offset, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_at_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_at_all_cdesc

function MPIR_File_read_at_all_begin_cdesc(fh, offset, buf, count, datatype) &
    bind(C, name="MPIR_File_read_at_all_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_read_at_all_begin_cdesc

function MPIR_File_read_at_all_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_read_at_all_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_at_all_end_cdesc

function MPIR_File_read_ordered_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_ordered_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_ordered_cdesc

function MPIR_File_read_ordered_begin_cdesc(fh, buf, count, datatype) &
    bind(C, name="MPIR_File_read_ordered_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_read_ordered_begin_cdesc

function MPIR_File_read_ordered_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_read_ordered_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_ordered_end_cdesc

function MPIR_File_read_shared_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_read_shared_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_read_shared_cdesc

function MPIR_File_write_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_cdesc

function MPIR_File_write_all_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_all_cdesc

function MPIR_File_write_all_begin_cdesc(fh, buf, count, datatype) &
    bind(C, name="MPIR_File_write_all_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_write_all_begin_cdesc

function MPIR_File_write_all_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_write_all_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_all_end_cdesc

function MPIR_File_write_at_cdesc(fh, offset, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_at_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_at_cdesc

function MPIR_File_write_at_all_cdesc(fh, offset, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_at_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_at_all_cdesc

function MPIR_File_write_at_all_begin_cdesc(fh, offset, buf, count, datatype) &
    bind(C, name="MPIR_File_write_at_all_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_write_at_all_begin_cdesc

function MPIR_File_write_at_all_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_write_at_all_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_at_all_end_cdesc

function MPIR_File_write_ordered_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_ordered_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_ordered_cdesc

function MPIR_File_write_ordered_begin_cdesc(fh, buf, count, datatype) &
    bind(C, name="MPIR_File_write_ordered_begin_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_int) :: ierror
end function MPIR_File_write_ordered_begin_cdesc

function MPIR_File_write_ordered_end_cdesc(fh, buf, status) &
    bind(C, name="MPIR_File_write_ordered_end_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_ordered_end_cdesc

function MPIR_File_write_shared_cdesc(fh, buf, count, datatype, status) &
    bind(C, name="MPIR_File_write_shared_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_File_write_shared_cdesc

function MPIR_Free_mem_c(base) &
    BIND(C, name="MPIR_Free_mem_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    type(*), dimension(..), intent(in), asynchronous :: base
    integer(c_int) :: ierror
end function MPIR_Free_mem_c

function MPIR_F_sync_reg_cdesc(buf) &
    bind(C, name="MPIR_F_sync_reg_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int) :: ierror
end function MPIR_F_sync_reg_cdesc

function MPIR_Imrecv_cdesc(buf, count, datatype, message, request) &
    bind(C, name="MPIR_Imrecv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Message, c_Request
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Message), intent(inout) :: message
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Imrecv_cdesc

function MPIR_Mrecv_cdesc(buf, count, datatype, message, status) &
    bind(C, name="MPIR_Mrecv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_Datatype, c_Message
    implicit none
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Message), intent(inout) :: message
    type(c_ptr), value, intent(in) :: status
    integer(c_int) :: ierror
end function MPIR_Mrecv_cdesc

function MPIR_Neighbor_allgather_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcount, recvtype, comm) &
    bind(C, name="MPIR_Neighbor_allgather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Neighbor_allgather_cdesc

function MPIR_Ineighbor_allgather_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcount, recvtype, comm, request) &
    bind(C, name="MPIR_Ineighbor_allgather_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ineighbor_allgather_cdesc

function MPIR_Neighbor_allgatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcounts, displs, recvtype, comm) &
    bind(C, name="MPIR_Neighbor_allgatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Neighbor_allgatherv_cdesc

function MPIR_Ineighbor_allgatherv_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcounts, displs, recvtype, comm, request) &
    bind(C, name="MPIR_Ineighbor_allgatherv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount
    integer(c_int), intent(in) :: recvcounts(*), displs(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ineighbor_allgatherv_cdesc

function MPIR_Neighbor_alltoall_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcount, recvtype, comm) &
    bind(C, name="MPIR_Neighbor_alltoall_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Neighbor_alltoall_cdesc

function MPIR_Ineighbor_alltoall_cdesc(sendbuf, sendcount, sendtype, recvbuf, &
           recvcount, recvtype, comm, request) &
    bind(C, name="MPIR_Ineighbor_alltoall_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), value, intent(in) :: sendcount, recvcount
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_Ineighbor_alltoall_cdesc

function MPIR_Neighbor_alltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype, recvbuf, &
           recvcounts, rdispls, recvtype, comm) &
    bind(C, name="MPIR_Neighbor_alltoallv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Neighbor_alltoallv_cdesc

function MPIR_Ineighbor_alltoallv_cdesc(sendbuf, sendcounts, sdispls, sendtype, recvbuf, &
           recvcounts, rdispls, recvtype, comm, request) &
    bind(C, name="MPIR_Ineighbor_alltoallv_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), sdispls(*), recvcounts(*), rdispls(*)
    integer(c_Datatype), value, intent(in) :: sendtype, recvtype
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), value, intent(in) :: request
    integer(c_int) :: ierror
end function MPIR_Ineighbor_alltoallv_cdesc

function MPIR_Neighbor_alltoallw_cdesc(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, &
           recvcounts, rdispls, recvtypes, comm) &
    bind(C, name="MPIR_Neighbor_alltoallw_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), recvcounts(*)
    integer(MPI_ADDRESS_KIND), intent(in) :: sdispls(*), rdispls(*)
    integer(c_Datatype), intent(in) :: sendtypes(*), recvtypes(*)
    integer(c_Comm), value, intent(in) :: comm
    integer(c_int) :: ierror
end function MPIR_Neighbor_alltoallw_cdesc

function MPIR_Ineighbor_alltoallw_cdesc(sendbuf, sendcounts, sdispls, sendtypes, recvbuf, &
           recvcounts, rdispls, recvtypes, comm, request) &
    bind(C, name="MPIR_Ineighbor_alltoallw_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_types, only : c_Datatype, c_Comm, c_Request
    implicit none
    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer(c_int), intent(in) :: sendcounts(*), recvcounts(*)
    integer(MPI_ADDRESS_KIND), intent(in) :: sdispls(*), rdispls(*)
    integer(c_Datatype), intent(in) :: sendtypes(*), recvtypes(*)
    integer(c_Comm), value, intent(in) :: comm
    integer(c_Request), value, intent(in) :: request
    integer(c_int) :: ierror
end function MPIR_Ineighbor_alltoallw_cdesc

function MPIR_File_iread_all_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iread_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iread_all_cdesc

function MPIR_File_iwrite_all_cdesc(fh, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iwrite_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_ptr
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    type(*), dimension(..), intent(in) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iwrite_all_cdesc

function MPIR_File_iread_at_all_cdesc(fh, offset, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iread_at_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iread_at_all_cdesc

function MPIR_File_iwrite_at_all_cdesc(fh, offset, buf, count, datatype, request) &
    bind(C, name="MPIR_File_iwrite_at_all_cdesc") result(ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08_compile_constants, only : MPI_OFFSET_KIND
    use :: mpi_c_interface_types, only : c_File, c_Datatype, c_Request
    implicit none
    integer(c_File), value, intent(in) :: fh
    integer(MPI_OFFSET_KIND), value, intent(in) :: offset
    type(*), dimension(..) :: buf
    integer(c_int), value, intent(in) :: count
    integer(c_Datatype), value, intent(in) :: datatype
    integer(c_Request), intent(out) :: request
    integer(c_int) :: ierror
end function MPIR_File_iwrite_at_all_cdesc

end interface

end module mpi_c_interface_cdesc
