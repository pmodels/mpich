C -*- Mode: Fortran; -*-
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This program is based on the allpair.f test from the MPICH-1 test
C (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
C fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program rsend
      implicit none
      include 'mpif.h'
      integer ierr, errs, comm
      logical mtestGetIntraComm
      logical verbose
      common /flags/ verbose

      errs = 0
      verbose = .false.
C      verbose = .true.
      call MTest_Init( ierr )

      do while ( mtestGetIntraComm( comm, 2, .false. ) )
         call test_pair_rsend( comm, errs )
         call mtestFreeComm( comm )
      enddo
C
      call MTest_Finalize( errs )
C
      end
C
      subroutine test_pair_rsend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(1)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Rsend and recv'
      endif
C
C
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
C
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
C
      tag = 1456
      count = TEST_SIZE / 3
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      if (rank .eq. 0) then
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Recv( MPI_BOTTOM, 0, MPI_INTEGER, next, tag,
     .                  comm, status, ierr )
C
         call MPI_Rsend(send_buf, count, MPI_REAL, next, tag,
     .                  comm, ierr)
C
         call MPI_Probe(MPI_ANY_SOURCE, tag, comm, status, ierr)
C
         if (status(MPI_SOURCE) .ne. next) then
            print *, 'Rsend: Incorrect source, expected', next,
     .               ', got', status(MPI_SOURCE)
            errs = errs + 1
         end if
C
         if (status(MPI_TAG) .ne. tag) then
            print *, 'Rsend: Incorrect tag, expected', tag,
     .               ', got', status(MPI_TAG)
            errs = errs + 1
         end if
C
         call MPI_Get_count(status, MPI_REAL, i, ierr)
C
         if (i .ne. count) then
            print *, 'Rsend: Incorrect count, expected', count,
     .               ', got', i
            errs = errs + 1
         end if
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)
C
         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE,
     .                   'rsend and recv', errs )
C
      else if (prev .eq. 0) then
C
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 requests(1), ierr)
         call MPI_Send( MPI_BOTTOM, 0, MPI_INTEGER, prev, tag,
     .                  comm, ierr )
         call MPI_Wait( requests(1), status, ierr )
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'rsend and recv', errs )
C
         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag,
     .                  comm, ierr)
      end if
C
      end
