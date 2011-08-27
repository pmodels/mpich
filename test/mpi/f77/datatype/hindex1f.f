C -*- Mode: Fortran; -*- 
C
C
C  (C) 2011 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, ierr, intsize
      integer i, displs(10), counts(10), dtype
      integer bufsize
      parameter (bufsize=100)
      integer inbuf(bufsize), outbuf(bufsize), packbuf(bufsize)
      integer position, len, psize
C
C     Test for hindexed; 
C     
      errs = 0
      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      
      do i=1, 10
         displs(i) = (10-i)*intsize
         counts(i) = 1
      enddo
      call mpi_type_hindexed( 10, counts, displs, MPI_INTEGER, dtype,
     &     ierr ) 
      call mpi_type_commit( dtype, ierr )
C
      call mpi_pack_size( 1, dtype, MPI_COMM_WORLD, psize, ierr )
      if (psize .gt. bufsize*intsize) then
         errs = errs + 1
      else
         do i=1,10
            inbuf(i)  = i
            outbuf(i) = -i
         enddo
         position = 0
         call mpi_pack( inbuf, 1, dtype, packbuf, psize, position,
     $        MPI_COMM_WORLD, ierr )
C
         len      = position
         position = 0
         call mpi_unpack( packbuf, len, position, outbuf, 10,
     $        MPI_INTEGER, MPI_COMM_WORLD, ierr )
C     
         do i=1, 10
            if (outbuf(i) .ne. 11-i) then
               errs = errs + 1
               print *, 'outbuf(',i,')=',outbuf(i),', expected ', 10-i
            endif
         enddo
      endif
C
      call mpi_type_free( dtype, ierr )
C
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
