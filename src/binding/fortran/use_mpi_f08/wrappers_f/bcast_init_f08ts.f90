!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPIX_Bcast_init_f08ts(buf, count, datatype, root, comm, info, request, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Datatype, MPI_Comm, MPI_Info, MPI_Request
    use :: mpi_c_interface, only : c_Datatype, c_Comm, c_Info, c_Request
    use :: mpi_c_interface, only : MPIR_Bcast_init_cdesc

    implicit none

    type(*), dimension(..) :: buf
    integer, intent(in) :: count
    type(MPI_Datatype), intent(in) :: datatype
    integer, intent(in) :: root
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Info), intent(in) :: info
    type(MPI_Request), intent(out) :: request
    integer, optional, intent(out) :: ierror

    integer(c_int) :: count_c
    integer(c_int) :: root_c
    integer(c_Datatype) :: datatype_c
    integer(c_Comm) :: comm_c
    integer(c_Info) :: info_c
    integer(c_Request) :: request_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Bcast_init_cdesc(buf, count, datatype%MPI_VAL, root, comm%MPI_VAL, info%MPI_VAL, request%MPI_VAL)
    else
        count_c = count
        datatype_c = datatype%MPI_VAL
        root_c = root
        comm_c = comm%MPI_VAL
        info_c = info%MPI_VAL
        ierror_c = MPIR_Bcast_init_cdesc(buf, count_c, datatype_c, root_c, comm_c, info_c, request_c)
        request%MPI_VAL = request_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPIX_Bcast_init_f08ts
