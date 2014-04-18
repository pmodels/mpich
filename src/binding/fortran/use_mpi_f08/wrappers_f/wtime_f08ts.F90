!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
function MPI_Wtime_f08( ) Result(time)
    use,intrinsic :: iso_c_binding, only: c_double
    use :: mpi_c_interface_nobuf, only: MPIR_Wtime_c
    double precision :: time
    real(c_double)   :: time_c

    time_c = MPIR_Wtime_c()
    time = time_c
end function MPI_Wtime_f08
