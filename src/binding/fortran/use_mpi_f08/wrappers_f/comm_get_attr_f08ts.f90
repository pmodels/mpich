!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_get_attr_f08(comm, comm_keyval, attribute_val, flag, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_get_attr_c
    use :: mpi_c_interface, only : MPIR_ATTR_AINT

    implicit none

    type(MPI_Comm), intent(in) :: comm
    integer, intent(in) :: comm_keyval
    integer(MPI_ADDRESS_KIND), intent(out) :: attribute_val
    logical, intent(out) :: flag
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: comm_c
    integer(c_int) :: comm_keyval_c
    integer(c_int) :: flag_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_get_attr_c(comm%MPI_VAL, comm_keyval, attribute_val, flag_c, MPIR_ATTR_AINT)
    else
        comm_c = comm%MPI_VAL
        comm_keyval_c = comm_keyval
        ierror_c = MPIR_Comm_get_attr_c(comm_c, comm_keyval_c, attribute_val, flag_c, MPIR_ATTR_AINT)
    end if

    flag = (flag_c /= 0)
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_get_attr_f08
