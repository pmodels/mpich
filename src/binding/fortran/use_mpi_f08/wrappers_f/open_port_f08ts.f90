!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Open_port_f08(info, port_name, ierror)
    use, intrinsic :: iso_c_binding, only : c_int, c_char
    use :: mpi_f08, only : MPI_Info
    use :: mpi_f08, only : MPI_MAX_PORT_NAME
    use :: mpi_c_interface, only : c_Info
    use :: mpi_c_interface, only : MPIR_Open_port_c
    use :: mpi_c_interface, only : MPIR_Fortran_string_c2f

    implicit none

    type(MPI_Info), intent(in) :: info
    character(len=MPI_MAX_PORT_NAME), intent(out) :: port_name
    integer, optional, intent(out) :: ierror

    integer(c_Info) :: info_c
    character(kind=c_char) :: port_name_c(MPI_MAX_PORT_NAME+1)
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Open_port_c(info%MPI_VAL, port_name_c)
    else
        info_c = info%MPI_VAL
        ierror_c = MPIR_Open_port_c(info_c, port_name_c)
    end if

    call MPIR_Fortran_string_c2f(port_name_c, port_name)

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Open_port_f08
