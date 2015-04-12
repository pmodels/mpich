!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Allreduce_f08ts(sendbuf, recvbuf, count, datatype, op, comm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Op, MPI_Comm
    use :: mpi_c_interface, only : c_Datatype, c_Op, c_Comm
    use :: mpi_c_interface, only : MPIR_Allreduce_cdesc

    implicit none

    type(*), dimension(..), intent(in) :: sendbuf
    type(*), dimension(..) :: recvbuf
    integer, intent(in) :: count
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Op), intent(in) :: op
    type(MPI_Comm), intent(in) :: comm
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_Datatype) :: datatype_c
    integer(c_Op) :: op_c
    integer(c_Comm) :: comm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Allreduce_cdesc(sendbuf, recvbuf, count, datatype%MPI_VAL, op%MPI_VAL, comm%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        op_c = op%MPI_VAL
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Allreduce_cdesc(sendbuf, recvbuf, count_c, datatype_c, op_c, comm_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Allreduce_f08ts
