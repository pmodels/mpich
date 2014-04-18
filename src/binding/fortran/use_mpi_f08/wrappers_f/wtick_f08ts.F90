!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
function MPI_Wtick_f08() result(res)
    use,intrinsic :: iso_c_binding, only: c_double
    use :: mpi_c_interface_nobuf, only: MPIR_Wtick_c
    double precision :: res
    real(c_double)   :: tick_c

    res_c = MPIR_Wtick_c()
    res = res_c
end function MPI_Wtick_f08
