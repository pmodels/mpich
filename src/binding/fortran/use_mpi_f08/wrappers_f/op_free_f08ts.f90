!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Op_free_f08(op, ierror)
    use, intrinsic :: iso_c_binding, only : c_int
    use :: mpi_f08, only : MPI_Op
    use :: mpi_c_interface, only : c_Op
    use :: mpi_c_interface, only : MPIR_Op_free_c

    implicit none

    type(MPI_Op), intent(inout) :: op
    integer, optional, intent(out) :: ierror

    integer(c_Op) :: op_c
    integer(c_int) :: ierror_c

    if (c_int == kind(0)) then
        ierror_c = MPIR_Op_free_c(op%MPI_VAL)
    else
        op_c = op%MPI_VAL
        ierror_c = MPIR_Op_free_c(op_c)
        op%MPI_VAL = op_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Op_free_f08
