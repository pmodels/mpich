!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_set_name_f08(comm, comm_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_set_name_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    type(MPI_Comm), intent(in) :: comm
    character(len=*), intent(in) :: comm_name
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    character(kind=c_char) :: comm_name_c(len_trim(comm_name)+1)
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(comm_name, comm_name_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_set_name_c(comm%MPI_VAL, comm_name_c)
    else
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_set_name_c(comm_c, comm_name_c)
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_set_name_f08
