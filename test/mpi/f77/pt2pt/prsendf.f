C -*- Mode: Fortran; -*-
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This program is based on the allpair.f test from the MPICH-1 test
C (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
C fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program prsend
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
         call test_pair_prsend( comm, errs )
         call mtestFreeComm( comm )
      enddo
C
      call MTest_Finalize( errs )
C
      end
C
      subroutine test_pair_prsend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer outcount, indices(2)
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer statuses(MPI_STATUS_SIZE,2), requests(2)
      integer status(MPI_STATUS_SIZE)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Persistent Rsend and recv'
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
      tag = 3456
      count = TEST_SIZE / 3
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL,
     .                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                   requests(2), ierr)
C
      if (rank .eq. 0) then
C
         call MPI_Rsend_init(send_buf, count, MPI_REAL, next, tag,
     .                       comm, requests(1), ierr)
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Recv( MPI_BOTTOM, 0, MPI_INTEGER, next, tag,
     .                  comm, status, ierr )
C
         call MPI_Startall(2, requests, ierr)
C
         index = -1
C
         do while (index .ne. 2)
            call MPI_Waitsome(2, requests, outcount,
     .                        indices, statuses, ierr)
            do i = 1,outcount
               if (indices(i) .eq. 2) then
                  call msg_check( recv_buf, next, tag, count,
     .                 statuses(1,i), TEST_SIZE, 'waitsome', errs )
                  index = 2
               end if
            end do
         end do
C
         call MPI_Request_free(requests(1), ierr)
      else if (prev .eq. 0) then
C
         call MPI_Rsend_init(send_buf, count, MPI_REAL, prev, tag,
     .                       comm, requests(1), ierr)
C
         call MPI_Start(requests(2), ierr)
C
         call MPI_Send( MPI_BOTTOM, 0, MPI_INTEGER, prev, tag,
     .                  comm, ierr )
C
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Test(requests(2), flag, status, ierr)
         end do
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'test', errs )
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
