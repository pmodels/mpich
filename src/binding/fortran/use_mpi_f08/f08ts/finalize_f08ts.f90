
subroutine MPI_Finalize_f08(ierror )
     use,intrinsic :: iso_c_binding, only: c_int
     use mpi_c_interface, only: MPIR_Finalize_c
     integer, optional,   intent(out) :: ierror
     integer(c_int) :: res

     res = MPIR_Finalize_c()
     if (present(ierror)) ierror = res

end subroutine MPI_Finalize_f08
