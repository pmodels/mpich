C -*- Mode: Fortran; -*-
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This program is based on the allpair.f test from the MPICH-1 test
C (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
C fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program psend
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
         call test_pair_psend( comm, errs )
         call mtestFreeComm( comm )
      enddo
C
      call MTest_Finalize( errs )
C
      end
C
      subroutine test_pair_psend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      integer statuses(MPI_STATUS_SIZE,2), requests(2)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Persistent send and recv'
      endif
C
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
C
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
C
      tag = 3123
      count = TEST_SIZE / 5
C
      call clear_test_data(recv_buf,TEST_SIZE)
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL,
     .                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                   requests(2), ierr)
C
      if (rank .eq. 0) then
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Send_init(send_buf, count, MPI_REAL, next, tag,
     .                      comm, requests(1), ierr)
C
         call MPI_Startall(2, requests, ierr)
         call MPI_Waitall(2, requests, statuses, ierr)
C
         call msg_check( recv_buf, next, tag, count, statuses(1,2),
     .        TEST_SIZE, 'persistent send/recv', errs )
C
         call MPI_Request_free(requests(1), ierr)
C
      else if (prev .eq. 0) then
C
         call MPI_Send_init(send_buf, count, MPI_REAL, prev, tag,
     .                      comm, requests(1), ierr)
         call MPI_Start(requests(2), ierr)
         call MPI_Wait(requests(2), status, ierr)
C
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     *                   'persistent send/recv', errs )
C
         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
C
         call MPI_Start(requests(1), ierr)
         call MPI_Wait(requests(1), status, ierr)
C
         call MPI_Request_free(requests(1), ierr)
      end if
C
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(2), ierr)
C
      end
