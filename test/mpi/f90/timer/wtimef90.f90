!  
!  (C) 2004 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
! This is a simple test to check that both the MPI and PMPI versions of the
! timers are available, and that they return double precision values.
! If this code links, there are no problems.
!
          use mpi
          implicit none

          double precision time1

          time1 = mpi_wtime()
          time1 = mpi_wtick()
          time1 = pmpi_wtime()
          time1 = pmpi_wtick()

          print *, ' No Errors'
        end
