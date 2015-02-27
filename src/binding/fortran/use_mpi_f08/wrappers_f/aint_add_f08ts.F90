!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
function MPI_Aint_add_f08(base, disp) result(res)
    use :: mpi_f08_compile_constants, only : MPI_ADDRESS_KIND
    use :: mpi_c_interface_nobuf, only : MPIR_Aint_add_c
    implicit none
    integer(MPI_ADDRESS_KIND), intent(in) :: base, disp
    integer(MPI_ADDRESS_KIND) :: res

    res = MPIR_Aint_add_c(base, disp)
end function MPI_Aint_add_f08