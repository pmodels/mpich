! This file created from test/mpi/f77/pt2pt/irsendf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program is based on the allpair.f test from the MPICH-1 test
! (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
! fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program isend
      use mpi
      integer ierr, errs, comm
      logical mtestGetIntraComm
      logical verbose
      common /flags/ verbose

      errs = 0
      verbose = .false.
!      verbose = .true.
      call MTest_Init( ierr )

      do while ( mtestGetIntraComm( comm, 2, .false. ) )
         call test_pair_irsend( comm, errs )
         call mtestFreeComm( comm )
      enddo
!
      call MTest_Finalize( errs )
!
      end
!
      subroutine test_pair_irsend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer TEST_SIZE
      integer dupcom
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Irsend and irecv'
      endif
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      call mpi_comm_dup( comm, dupcom, ierr )
!
      tag = 2456
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, next, 0, &
      &                      MPI_BOTTOM, 0, MPI_INTEGER, next, 0, &
      &                      dupcom, status, ierr )
!
         call MPI_Irsend(send_buf, count, MPI_REAL, next, tag, &
      &                   comm, requests(2), ierr)
!
         index = -1
         do while (index .ne. 1)
            call MPI_Waitany(2, requests, index, statuses, ierr)
         end do
!
         call rq_check( requests(1), 1, 'irsend and irecv' )
!
         call msg_check( recv_buf, next, tag, count, statuses, &
      &           TEST_SIZE, 'irsend and irecv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                  MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                  requests(1), ierr)
!
         call MPI_Sendrecv( MPI_BOTTOM, 0, MPI_INTEGER, prev, 0, &
      &                      MPI_BOTTOM, 0, MPI_INTEGER, prev, 0, &
      &                      dupcom, status, ierr )
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Test(requests(1), flag, status, ierr)
         end do
!
         call rq_check( requests, 1, 'irsend and irecv (test)' )
!
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'irsend and irecv', errs )
!
         call MPI_Irsend(recv_buf, count, MPI_REAL, prev, tag, &
      &                   comm, requests(1), ierr)
!
         call MPI_Waitall(1, requests, statuses, ierr)
!
         call rq_check( requests, 1, 'irsend and irecv' )
!
      end if
!
      call mpi_comm_free( dupcom, ierr )
!
      end
!
