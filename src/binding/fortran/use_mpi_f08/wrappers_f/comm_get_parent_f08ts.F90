!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_get_parent_f08(parent, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Comm
    use :: mpi_c_interface, only : c_Comm
    use :: mpi_c_interface, only : MPIR_Comm_get_parent_c

    implicit none

    type(MPI_Comm), intent(out) :: parent
    integer, optional, intent(out) :: ierror

    integer(c_Comm) :: parent_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Comm_get_parent_c(parent%MPI_VAL)
    else
        ierror_c = MPIR_Comm_get_parent_c(parent_c)
        parent%MPI_VAL = parent_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_get_parent_f08
