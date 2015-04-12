!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_connect_f08(port_name, info, root, comm, newcomm, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info, MPI_Comm
    use :: mpi_c_interface, only : c_Info, c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_connect_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_f2c

    implicit none

    character(len=*), intent(in) :: port_name
    type(MPI_Info), intent(in) :: info
    integer, intent(in) :: root
    type(MPI_Comm), intent(in) :: comm
    type(MPI_Comm), intent(out) :: newcomm
    integer, optional, intent(out) :: ierror

    character(kind=c_char) :: port_name_c(len_trim(port_name)+1)
    integer(c_Info) :: info_c
    integer(c_int) :: root_c
    integer(c_Comm) :: comm_c
    integer(c_Comm) :: newcomm_c
    integer(c_int) :: ierror_c

    call MPIR_Fortran_string_f2c(port_name, port_name_c)

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_connect_c(port_name_c, info%MPI_VAL, root, comm%MPI_VAL, newcomm%MPI_VAL)
    else
        info_c = info%MPI_VAL
        root_c = root
        comm_c = comm%MPI_VAL
        ierror_c = MPIR_Comm_connect_c(port_name_c, info_c, root_c, comm_c, newcomm_c)
        newcomm%MPI_VAL = newcomm_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_connect_f08
