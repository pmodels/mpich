! This file created from test/mpi/f77/coll/allredopttf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2007 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer*8 inbuf, outbuf
      double complex zinbuf, zoutbuf
      integer wsize
      integer errs, ierr

      errs = 0

      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
!
! A simple test of allreduce for the optional integer*8 type

      inbuf = 1
      outbuf = 0
      call mpi_allreduce(inbuf, outbuf, 1, MPI_INTEGER8, MPI_SUM,  &
      &                   MPI_COMM_WORLD, ierr)
      if (outbuf .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with integer*8 = got ", outbuf,  &
      & " but should have ", wsize
      endif
      zinbuf = (1,1)
      zoutbuf = (0,0)
      call mpi_allreduce(zinbuf, zoutbuf, 1, MPI_DOUBLE_COMPLEX,  &
      &                   MPI_SUM,  MPI_COMM_WORLD, ierr)
      if (dreal(zoutbuf) .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with double complex = got ",  &
      & outbuf, " but should have ", wsize
      endif
      if (dimag(zoutbuf) .ne. wsize ) then
         errs = errs + 1
         print *, "result wrong for sum with double complex = got ",  &
      & outbuf, " but should have ", wsize
      endif
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
