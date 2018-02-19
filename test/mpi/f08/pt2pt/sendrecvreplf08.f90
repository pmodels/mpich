! This file created from test/mpi/f77/pt2pt/sendrecvreplf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program is based on the allpair.f test from the MPICH-1 test
! (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
! fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program sendrecvrepl
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
         call test_pair_sendrecvrepl( comm, errs )
         call mtestFreeComm( comm )
      enddo
!
      call MTest_Finalize( errs )
!
      end
!
      subroutine test_pair_sendrecvrepl( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Sendrecv replace'
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
      tag = 4456
      count = TEST_SIZE / 3

      if (rank .eq. 0) then
!
         call init_test_data(recv_buf, TEST_SIZE)
!
         do 11 i = count+1,TEST_SIZE
            recv_buf(i) = 0.0
 11      continue
!
         call MPI_Sendrecv_replace(recv_buf, count, MPI_REAL, &
      &                             next, tag, next, tag, &
      &                             comm, status, ierr)

         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'sendrecvreplace', errs )

      else if (prev .eq. 0) then

         call clear_test_data(recv_buf,TEST_SIZE)

         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'recv/send for replace', errs )

         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, &
      &                 comm, ierr)
      end if
!
      end
