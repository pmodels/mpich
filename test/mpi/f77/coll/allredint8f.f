C -*- Mode: Fortran; -*- 
C
C  (C) 2006 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer*8 inbuf, outbuf
      integer errs, ierr

      errs = 0
      
      call mtest_init( ierr )
C
C A simple test of allreduce for the optional integer*8 type

      call mpi_allreduce(inbuf, outbuf, 1, MPI_INTEGER8, MPI_SUM, 
     &                   MPI_COMM_WORLD, ierr)
      
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
