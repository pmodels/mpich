!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_split_type_f08(comm, split_type, key, info, newcomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm, MPI_Info
    use :: mpi_c_interface, only : c_Comm, c_Info
    use :: mpi_c_interface, only : MPIR_Comm_split_type_c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: split_type
    integer, intent(in) :: key
    type(MPI_Info), intent(in) :: info
    type(MPI_Comm), intent(out) :: newcomm
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: split_type_c
    integer(c_int) :: key_c
    integer(c_Info) :: info_c
    integer(c_Comm) :: newcomm_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_split_type_c(comm%MPI_VAL, split_type, key, info%MPI_VAL, newcomm%MPI_VAL)
    else
        comm_c = comm%MPI_VAL
        split_type_c = split_type
        key_c = key
        info_c = info%MPI_VAL
        ierror_c = MPIR_Comm_split_type_c(comm_c, split_type_c, key_c, info_c, newcomm_c)
        newcomm%MPI_VAL = newcomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_split_type_f08
