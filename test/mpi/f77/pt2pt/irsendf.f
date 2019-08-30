C -*- Mode: Fortran; -*-
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This program is based on the allpair.f test from the MPICH-1 test
C (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
C fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program isend
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
         call test_pair_irsend( comm, errs )
         call mtestFreeComm( comm )
      enddo
C
      call MTest_Finalize( errs )
C
      end
C
      subroutine test_pair_irsend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, completed, i
      integer TEST_SIZE
      integer dupcom
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Irsend and irecv'
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
      call mpi_comm_dup( comm, dupcom, ierr )
C
      tag = 2456
      count = TEST_SIZE / 3
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      if (rank .eq. 0) then
C
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL,
     .                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                  requests(1), ierr)
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, next, 0,
     .                      MPI_BOTTOM, 0, MPI_INTEGER, next, 0,
     .                      dupcom, status, ierr )
C
         call MPI_Irsend(send_buf, count, MPI_REAL, next, tag,
     .                   comm, requests(2), ierr)
C
         completed = 0
         do while (completed .lt. 2)
            call MPI_Waitany(2, requests, index, statuses, ierr)
            completed = completed + 1
         end do
C
         call rq_check( requests(1), 1, 'irsend and irecv' )
C
         call msg_check( recv_buf, next, tag, count, statuses,
     .           TEST_SIZE, 'irsend and irecv', errs )
C
      else if (prev .eq. 0) then
C
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL,
     .                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                  requests(1), ierr)
C
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, prev, 0,
     .                      MPI_BOTTOM, 0, MPI_INTEGER, prev, 0,
     .                      dupcom, status, ierr )
C
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Test(requests(1), flag, status, ierr)
         end do
C
         call rq_check( requests, 1, 'irsend and irecv (test)' )
C
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'irsend and irecv', errs )
C
         call MPI_Irsend(recv_buf, count, MPI_REAL, prev, tag,
     .                   comm, requests(1), ierr)
C
         call MPI_Waitall(1, requests, statuses, ierr)
C
         call rq_check( requests, 1, 'irsend and irecv' )
C
      end if
C
      call mpi_comm_free( dupcom, ierr )
C
      end
C
