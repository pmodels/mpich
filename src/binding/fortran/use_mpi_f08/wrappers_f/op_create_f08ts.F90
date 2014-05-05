!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Op_create_f08(user_fn, commute, op, ierror)
    use, intrinsic :: iso_c_binding, only : c_funloc
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08, only : MPI_Op
    use :: mpi_f08, only : MPI_User_function
    use :: mpi_c_interface, only : c_Op
    use :: mpi_c_interface, only : MPIR_Op_create_c

    implicit none

    procedure(MPI_User_function) :: user_fn
    logical, intent(in) :: commute
    type(MPI_Op), intent(out) :: op
    integer, optional, intent(out) :: ierror

    type(c_funptr) :: user_fn_c
    integer(c_int) :: commute_c
    integer(c_Op) :: op_c
    integer(c_int) :: ierror_c

    user_fn_c = c_funloc(user_fn)
    commute_c = merge(1, 0, commute)
    if (c_int == kind(0)) then
        ierror_c = MPIR_Op_create_c(user_fn_c, commute_c, op%MPI_VAL)
    else
        ierror_c = MPIR_Op_create_c(user_fn_c, commute_c, op_c)
        op%MPI_VAL = op_c
    end if

    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Op_create_f08
