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
          integer err
          double precision time1

          call mpi_init(err)

          time1 = mpi_wtime()
          time1 = time1 + mpi_wtick()
          time1 = time1 + pmpi_wtime()
          time1 = time1 + pmpi_wtick()
! Add a test on time1 to ensure that the compiler does not remove the calls
! (The compiler should call them anyway because they aren't pure, but
! including these operations ensures that a buggy compiler doesn't 
! pass this test by mistake).
          if (time1 .lt. 0.0d0) then
             print *, ' Negative time result'
          else
                print *, ' No Errors'
          endif
          
          call mpi_finalize(err)
        end
