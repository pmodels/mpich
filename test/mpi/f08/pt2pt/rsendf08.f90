! This file created from test/mpi/f77/pt2pt/rsendf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! This program is based on the allpair.f test from the MPICH-1 test
! (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
! fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program rsend
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
         call test_pair_rsend( comm, errs )
         call mtestFreeComm( comm )
      enddo
!
      call MTest_Finalize( errs )
!
      end
!
      subroutine test_pair_rsend( comm, errs )
      use mpi
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(1)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Rsend and recv'
      endif
!
!
      call mpi_comm_rank( comm, rank, ierr )
      call mpi_comm_size( comm, size, ierr )
      next = rank + 1
      if (next .ge. size) next = 0
!
      prev = rank - 1
      if (prev .lt. 0) prev = size - 1
!
      tag = 1456
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      if (rank .eq. 0) then
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Recv( MPI_BOTTOM, 0, MPI_INTEGER, next, tag, &
      &                  comm, status, ierr )
!
         call MPI_Rsend(send_buf, count, MPI_REAL, next, tag, &
      &                  comm, ierr)
!
         call MPI_Probe(MPI_ANY_SOURCE, tag, comm, status, ierr)
!
         if (status(MPI_SOURCE) .ne. next) then
            print *, 'Rsend: Incorrect source, expected', next, &
      &               ', got', status(MPI_SOURCE)
            errs = errs + 1
         end if
!
         if (status(MPI_TAG) .ne. tag) then
            print *, 'Rsend: Incorrect tag, expected', tag, &
      &               ', got', status(MPI_TAG)
            errs = errs + 1
         end if
!
         call MPI_Get_count(status, MPI_REAL, i, ierr)
!
         if (i .ne. count) then
            print *, 'Rsend: Incorrect count, expected', count, &
      &               ', got', i
            errs = errs + 1
         end if
!
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 status, ierr)
!
         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE, &
      &                   'rsend and recv', errs )
!
      else if (prev .eq. 0) then
!
         call MPI_Irecv(recv_buf, TEST_SIZE, MPI_REAL, &
      &                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                 requests(1), ierr)
         call MPI_Send( MPI_BOTTOM, 0, MPI_INTEGER, prev, tag, &
      &                  comm, ierr )
         call MPI_Wait( requests(1), status, ierr )
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE, &
      &                   'rsend and recv', errs )
!
         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, &
      &                  comm, ierr)
      end if
!
      end
