!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
subroutine MPI_Comm_create_keyval_f08(comm_copy_attr_fn, comm_delete_attr_fn, comm_keyval, &
    extra_state, ierror)
    use, intrinsic :: iso_c_binding, only : c_funloc
    use, intrinsic :: iso_c_binding, only : c_int, c_funptr
    use :: mpi_f08, only : MPI_Comm_copy_attr_function
    use :: mpi_f08, only : MPI_Comm_delete_attr_function
    use :: mpi_f08, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface, only : MPIR_Comm_create_keyval_c
    use :: mpi_c_interface, only : MPIR_Keyval_set_proxy, MPIR_Comm_copy_attr_f08_proxy, MPIR_Comm_delete_attr_f08_proxy

    implicit none

    procedure(MPI_Comm_copy_attr_function) :: comm_copy_attr_fn
    procedure(MPI_Comm_delete_attr_function) :: comm_delete_attr_fn
    integer, intent(out) :: comm_keyval
    integer(MPI_ADDRESS_KIND), intent(in) :: extra_state
    integer, optional, intent(out) :: ierror

    type(c_funptr) :: comm_copy_attr_fn_c
    type(c_funptr) :: comm_delete_attr_fn_c
    integer(c_int) :: comm_keyval_c
    integer(c_int) :: ierror_c

    comm_copy_attr_fn_c = c_funloc(comm_copy_attr_fn)
    comm_delete_attr_fn_c = c_funloc(comm_delete_attr_fn)

    ierror_c = MPIR_Comm_create_keyval_c(comm_copy_attr_fn_c, comm_delete_attr_fn_c, comm_keyval_c, extra_state)

    call MPIR_Keyval_set_proxy(comm_keyval_c, c_funloc(MPIR_Comm_copy_attr_f08_proxy), c_funloc(MPIR_Comm_delete_attr_f08_proxy))
    comm_keyval = comm_keyval_c
    if (present(ierror)) ierror = ierror_c

end subroutine MPI_Comm_create_keyval_f08
