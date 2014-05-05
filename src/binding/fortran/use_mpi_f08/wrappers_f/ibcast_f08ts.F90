!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Ibcast_f08ts(buffer, count, datatype, root, comm, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Request
    use :: mpi_c_interface, only : MPIR_Ibcast_cdesc

    implicit none

    type(*), dimension(..), asynchronous :: buffer
    integer, intent(in) :: count
    integer, intent(in) :: root
    type(MPI_Datatype), intent(in) :: datatype
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: root_c
    integer(c_Datatype) :: datatype_c
    integer(c_Comm) :: comm_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Ibcast_cdesc(buffer, count, datatype%MPI_VAL, root, comm%MPI_VAL, request%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        root_c = root
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Ibcast_cdesc(buffer, count_c, datatype_c, root_c, comm_c, request_c)
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Ibcast_f08ts
