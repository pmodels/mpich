! -*- Mode: Fortran; -*-
!
!  (C) 2006 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer*8 inbuf, outbuf
      integer errs, ierr

      errs = 0

      call mtest_init( ierr )
!
! A simple test of allreduce for the optional integer*8 type

      call mpi_allreduce(inbuf, outbuf, 1, MPI_INTEGER8, MPI_SUM,  &
      &                   MPI_COMM_WORLD, ierr)

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
