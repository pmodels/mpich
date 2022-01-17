!
! Copyright (C) by Argonne National Laboratory
!     See COPYRIGHT in top-level directory
!

! This program is based on the allpair.f test from the MPICH-1 test
! (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
! fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program pssend
      use mpi_f08
      type(MPI_Comm) comm
      integer ierr, errs
      logical mtestGetIntraComm
      logical verbose
      common /flags/ verbose

      errs = 0
      verbose = .false.
!      verbose = .true.
      call MTest_Init( ierr )

      do while ( mtestGetIntraComm( comm, 2, .false. ) )
         call test_pair_pssend( comm, errs )
         call mtestFreeComm( comm )
      enddo
!
      call MTest_Finalize( errs )
!
      end
!
      subroutine test_pair_pssend( comm, errs )
      use mpi_f08
      type(MPI_Comm) comm
      integer errs
      integer rank, size, ierr, next, prev, tag, count, index, i
      integer outcount, indices(2)
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      type(MPI_Status) statuses(2)
      type(MPI_Request) requests(2)
      type(MPI_Status) status
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      integer completed
      common /flags/ verbose
!
      if (verbose) then
         print *, ' Persistent Ssend and recv'
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
      tag = 3789
      count = TEST_SIZE / 3
!
      call clear_test_data(recv_buf,TEST_SIZE)
!
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL, &
      &                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &
      &                   requests(1), ierr)
!
      if (rank .eq. 0) then
!
         call MPI_Ssend_init(send_buf, count, MPI_REAL, next, tag, &
      &                       comm, requests(2), ierr)
!
         call init_test_data(send_buf,TEST_SIZE)
!
         call MPI_Startall(2, requests, ierr)
!
         completed = 0
         do while (completed .lt. 2)
            call MPI_Testsome(2, requests, outcount, &
      &                        indices, statuses, ierr)
            do i = 1,outcount
               if (indices(i) .lt. 1 .or. indices(i) .gt. 2) then
                    print *, "Invalid index in array_of_indices of MPI_Testsome:", indices(i)
               end if
               if (indices(i) .eq. 1) then
                  call msg_check( recv_buf, next, tag, count, &
      &                 statuses(i), TEST_SIZE, 'testsome', errs )
               end if
               completed = completed + 1
            end do
         end do
!
         call MPI_Request_free(requests(2), ierr)
!
      else if (prev .eq. 0) then
!
         call MPI_Ssend_init(send_buf, count, MPI_REAL, prev, tag, &
      &                       comm, requests(2), ierr)
!
         call MPI_Start(requests(1), ierr)
!
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testany(1, requests(1:1), index, flag, &
      &                       statuses(1), ierr)
         end do
         call msg_check( recv_buf, prev, tag, count, statuses(1), &
      &           TEST_SIZE, 'testany', errs )

         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
!
         call MPI_Start(requests(2), ierr)
         call MPI_Wait(requests(2), status, ierr)
!
         call MPI_Request_free(requests(2), ierr)
!
      end if
!
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(1), ierr)
!
      end
