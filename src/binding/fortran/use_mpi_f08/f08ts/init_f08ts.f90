
subroutine MPI_Init_f08(ierror)
     use,intrinsic :: iso_c_binding, only: c_int
     use mpi_c_interface, only: C_NULL_PTR, MPIR_Init_c
     integer, optional,   intent(out) :: ierror
     integer(c_int) :: res

     res = MPIR_Init_c(C_NULL_PTR, C_NULL_PTR)
     if (present(ierror)) ierror = res

end subroutine MPI_Init_f08
