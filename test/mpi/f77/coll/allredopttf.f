C -*- Mode: Fortran; -*- 
C
C  (C) 2007 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer*8 inbuf, outbuf
      double complex zinbuf, zoutbuf
      integer wsize
      integer errs, ierr

      errs = 0
      
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
C
C A simple test of allreduce for the optional integer*8 type

      inbuf = 1
      outbuf = 0
      call mpi_allreduce(inbuf, outbuf, 1, MPI_INTEGER8, MPI_SUM, 
     &                   MPI_COMM_WORLD, ierr)
      if (outbuf .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with integer*8 = got ", outbuf, 
     & " but should have ", wsize
      endif
      zinbuf = (1,1)
      zoutbuf = (0,0)
      call mpi_allreduce(zinbuf, zoutbuf, 1, MPI_DOUBLE_COMPLEX, 
     &                   MPI_SUM,  MPI_COMM_WORLD, ierr)
      if (dreal(zoutbuf) .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with double complex = got ", 
     & outbuf, " but should have ", wsize
      endif
      if (dimag(zoutbuf) .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with double complex = got ", 
     & outbuf, " but should have ", wsize
      endif
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
