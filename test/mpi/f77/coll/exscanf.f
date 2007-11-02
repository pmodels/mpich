C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer inbuf(2), outbuf(2)
      integer ans, rank, size, comm
      integer errs, ierr

      errs = 0
      
      call mtest_init( ierr )
C
C A simple test of exscan
      comm = MPI_COMM_WORLD

      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )

      inbuf(1) = rank
      inbuf(2) = -rank
      call mpi_exscan( inbuf, outbuf, 2, MPI_INTEGER, MPI_SUM, comm, 
     &                 ierr )
C this process has the sum of i from 0 to rank-1, which is
C (rank)(rank-1)/2 and -i
      ans = (rank * (rank - 1))/2
      if (rank .gt. 0) then
         if (outbuf(1) .ne. ans) then
            errs = errs + 1
            print *, rank, ' Expected ', ans, ' got ', outbuf(1)
         endif
         if (outbuf(2) .ne. -ans) then
            errs = errs + 1
            print *, rank, ' Expected ', -ans, ' got ', outbuf(1)
         endif
      endif
      
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
