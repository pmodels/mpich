C -*- Mode: Fortran; -*- 
C
C  (C) 2012 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This program is based on the allpair.f test from the MPICH-1 test
C (test/pt2pt/allpair.f), which in turn was inspired by a bug report from
C fsset@corelli.lerc.nasa.gov (Scott Townsend)

      program allpair
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
         call test_pair_send( comm, errs )
         call test_pair_ssend( comm, errs )
         call test_pair_rsend( comm, errs )
         call test_pair_isend( comm, errs )
         call test_pair_irsend( comm, errs )
         call test_pair_issend( comm, errs )
         call test_pair_psend( comm, errs )
         call test_pair_prsend( comm, errs )
         call test_pair_pssend( comm, errs )
         call test_pair_sendrecv( comm, errs )
         call test_pair_sendrecvrepl( comm, errs )
         call mtestFreeComm( comm )
      enddo
C         
      call MTest_Finalize( errs )
      call MPI_Finalize(ierr)
C
      end
C
      subroutine test_pair_send( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Send and recv'
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
      tag = 1123
      count = TEST_SIZE / 5
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      if (rank .eq. 0) then
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Send(send_buf, count, MPI_REAL, next, tag,
     .        comm, ierr) 
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, status, ierr)
C
         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE,
     .                   'send and recv', errs )
      else if (prev .eq. 0)  then
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm, status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'send and recv', errs )
C
         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag, comm, ierr) 
      end if
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
C
      subroutine test_pair_ssend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Ssend and recv'
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
      tag = 1789
      count = TEST_SIZE / 3
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      if (rank .eq. 0) then
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Iprobe(MPI_ANY_SOURCE, tag,
     .                   comm, flag, status, ierr) 
C
         if (flag) then
            print *, 'Ssend: Iprobe succeeded! source', 
     .               status(MPI_SOURCE),
     .               ', tag', status(MPI_TAG)
            errs = errs + 1
         end if
C
         call MPI_Ssend(send_buf, count, MPI_REAL, next, tag,
     .                  comm, ierr) 
C
         do while (.not. flag)
            call MPI_Iprobe(MPI_ANY_SOURCE, tag,
     .                      comm, flag, status, ierr) 
         end do
C           
         if (status(MPI_SOURCE) .ne. next) then
            print *, 'Ssend: Incorrect source, expected', next,
     .               ', got', status(MPI_SOURCE)
            errs = errs + 1
         end if
C
         if (status(MPI_TAG) .ne. tag) then
            print *, 'Ssend: Incorrect tag, expected', tag,
     .               ', got', status(MPI_TAG)
            errs = errs + 1
         end if
C
         call MPI_Get_count(status, MPI_REAL, i, ierr)
C
         if (i .ne. count) then
            print *, 'Ssend: Incorrect count, expected', count,
     .               ', got', i
            errs = errs + 1
         end if
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)
C
         call msg_check( recv_buf, next, tag, count, status,
     .        TEST_SIZE, 'ssend and recv', errs ) 
C
      else if (prev .eq. 0) then
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)
C
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'ssend and recv', errs )
C
         call MPI_Ssend(recv_buf, count, MPI_REAL, prev, tag,
     .                  comm, ierr) 
      end if
C
      end
C
      subroutine test_pair_isend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' isend and irecv'
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
      tag = 2123
      count = TEST_SIZE / 5
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
         call MPI_Isend(send_buf, count, MPI_REAL, next, tag,
     .                  comm, requests(2), ierr) 
C
         call MPI_Waitall(2, requests, statuses, ierr)
C
         call rq_check( requests, 2, 'isend and irecv' )
C
         call msg_check( recv_buf, next, tag, count, statuses(1,1),
     .        TEST_SIZE, 'isend and irecv', errs )
C
      else if (prev .eq. 0) then
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)
C
         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'isend and irecv', errs )
C
         call MPI_Isend(recv_buf, count, MPI_REAL, prev, tag,
     .                  comm, requests(1), ierr) 
C
         call MPI_Wait(requests(1), status, ierr)
C
         call rq_check( requests(1), 1, 'isend and irecv' )
C
      end if
C
      end
C
      subroutine test_pair_irsend( comm, errs )
      implicit none
      include 'mpif.h'
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
         index = -1
         do while (index .ne. 1)
            call MPI_Waitany(2, requests, index, statuses, ierr)
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
      subroutine test_pair_issend( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, index
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE), requests(2)
      integer statuses(MPI_STATUS_SIZE,2)
      logical flag
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' issend and irecv (testall)'
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
      tag = 2789
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
         call MPI_Issend(send_buf, count, MPI_REAL, next, tag,
     .                   comm, requests(2), ierr) 
C
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testall(2, requests, flag, statuses, ierr)
         end do
C
         call rq_check( requests, 2, 'issend and irecv (testall)' )
C
         call msg_check( recv_buf, next, tag, count, statuses(1,1),
     .           TEST_SIZE, 'issend and recv (testall)', errs )
C
      else if (prev .eq. 0) then
C
         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'issend and recv', errs )

         call MPI_Issend(recv_buf, count, MPI_REAL, prev, tag,
     .                   comm, requests(1), ierr) 
C
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testany(1, requests(1), index, flag,
     .                       statuses(1,1), ierr)
         end do
C
         call rq_check( requests, 1, 'issend and recv (testany)' )
C
      end if
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
C
      subroutine test_pair_pssend( comm, errs )
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
         print *, ' Persistent Ssend and recv'
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
      tag = 3789
      count = TEST_SIZE / 3
C
      call clear_test_data(recv_buf,TEST_SIZE)
C
      call MPI_Recv_init(recv_buf, TEST_SIZE, MPI_REAL,
     .                   MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                   requests(1), ierr)
C
      if (rank .eq. 0) then
C
         call MPI_Ssend_init(send_buf, count, MPI_REAL, next, tag,
     .                       comm, requests(2), ierr) 
C
         call init_test_data(send_buf,TEST_SIZE)
C
         call MPI_Startall(2, requests, ierr)
C
         index = -1
         do while (index .ne. 1)
            call MPI_Testsome(2, requests, outcount,
     .                        indices, statuses, ierr)
            do i = 1,outcount
               if (indices(i) .eq. 1) then
                  call msg_check( recv_buf, next, tag, count,
     .                 statuses(1,i), TEST_SIZE, 'testsome', errs )
                  index = 1
               end if
            end do
         end do
C
         call MPI_Request_free(requests(2), ierr)
C
      else if (prev .eq. 0) then
C
         call MPI_Ssend_init(send_buf, count, MPI_REAL, prev, tag,
     .                       comm, requests(2), ierr) 
C
         call MPI_Start(requests(1), ierr)
C
         flag = .FALSE.
         do while (.not. flag)
            call MPI_Testany(1, requests(1), index, flag,
     .                       statuses(1,1), ierr)
         end do
         call msg_check( recv_buf, prev, tag, count, statuses(1,1),
     .           TEST_SIZE, 'testany', errs )

         do i = 1,count
            send_buf(i) = recv_buf(i)
         end do
C
         call MPI_Start(requests(2), ierr)
         call MPI_Wait(requests(2), status, ierr)
C
         call MPI_Request_free(requests(2), ierr)
C
      end if
C
      call dummyRef( send_buf, count, ierr )
      call MPI_Request_free(requests(1), ierr)
C
      end
C
      subroutine test_pair_sendrecv( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Sendrecv'
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
      tag = 4123
      count = TEST_SIZE / 5

      call clear_test_data(recv_buf,TEST_SIZE)

      if (rank .eq. 0) then

         call init_test_data(send_buf,TEST_SIZE)

         call MPI_Sendrecv(send_buf, count, MPI_REAL, next, tag,
     .                     recv_buf, count, MPI_REAL, next, tag,
     .                     comm, status, ierr) 

         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE,
     .                   'sendrecv', errs )

      else if (prev .eq. 0) then

         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'recv/send', errs )

         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag,
     .                 comm, ierr) 
      end if
C
      end
C
      subroutine test_pair_sendrecvrepl( comm, errs )
      implicit none
      include 'mpif.h'
      integer comm, errs
      integer rank, size, ierr, next, prev, tag, count, i
      integer TEST_SIZE
      parameter (TEST_SIZE=2000)
      integer status(MPI_STATUS_SIZE)
      real send_buf(TEST_SIZE), recv_buf(TEST_SIZE)
      logical verbose
      common /flags/ verbose
C
      if (verbose) then
         print *, ' Sendrecv replace'
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
      tag = 4456
      count = TEST_SIZE / 3

      if (rank .eq. 0) then
C
         call init_test_data(recv_buf, TEST_SIZE)
C
         do 11 i = count+1,TEST_SIZE
            recv_buf(i) = 0.0
 11      continue
C
         call MPI_Sendrecv_replace(recv_buf, count, MPI_REAL,
     .                             next, tag, next, tag,
     .                             comm, status, ierr)  

         call msg_check( recv_buf, next, tag, count, status, TEST_SIZE,
     .                   'sendrecvreplace', errs )

      else if (prev .eq. 0) then

         call clear_test_data(recv_buf,TEST_SIZE)

         call MPI_Recv(recv_buf, TEST_SIZE, MPI_REAL,
     .                 MPI_ANY_SOURCE, MPI_ANY_TAG, comm,
     .                 status, ierr)

         call msg_check( recv_buf, prev, tag, count, status, TEST_SIZE,
     .                   'recv/send for replace', errs )

         call MPI_Send(recv_buf, count, MPI_REAL, prev, tag,
     .                 comm, ierr) 
      end if
C
      end
C
c------------------------------------------------------------------------------
c
c  Check for correct source, tag, count, and data in test message.
c
c------------------------------------------------------------------------------
      subroutine msg_check( recv_buf, source, tag, count, status, n, 
     *                      name, errs )
      implicit none
      include 'mpif.h'
      integer n, errs
      real    recv_buf(n)
      integer source, tag, count, rank, status(MPI_STATUS_SIZE)
      character*(*) name
      logical foundError

      integer ierr, recv_src, recv_tag, recv_count

      foundError = .false.
      recv_src = status(MPI_SOURCE)
      recv_tag = status(MPI_TAG)
      call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )
      call MPI_Get_count(status, MPI_REAL, recv_count, ierr)

      if (recv_src .ne. source) then
         print *, '[', rank, '] Unexpected source:', recv_src, 
     *            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_tag .ne. tag) then
         print *, '[', rank, '] Unexpected tag:', recv_tag, ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_count .ne. count) then
         print *, '[', rank, '] Unexpected count:', recv_count,
     *            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if
         
      call verify_test_data(recv_buf, count, n, name, errs )

      end
c------------------------------------------------------------------------------
c
c  Check that requests have been set to null
c
c------------------------------------------------------------------------------
      subroutine rq_check( requests, n, msg )
      include 'mpif.h'
      integer n, requests(n)
      character*(*) msg
      integer i
c
      do 10 i=1, n
         if (requests(i) .ne. MPI_REQUEST_NULL) then
            print *, 'Nonnull request in ', msg
         endif
 10   continue
c      
      end
c------------------------------------------------------------------------------
c
c  Initialize test data buffer with integral sequence.
c
c------------------------------------------------------------------------------
      subroutine init_test_data(buf,n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = REAL(i)
 10    continue
      end

c------------------------------------------------------------------------------
c
c  Clear test data buffer
c
c------------------------------------------------------------------------------
      subroutine clear_test_data(buf, n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = 0.
 10   continue

      end

c------------------------------------------------------------------------------
c
c  Verify test data buffer
c
c------------------------------------------------------------------------------
      subroutine verify_test_data( buf, count, n, name, errs )
      implicit none
      include 'mpif.h'
      integer n, errs
      real buf(n)
      character *(*) name
      integer count, ierr, i
C
      do 10 i = 1, count
         if (buf(i) .ne. REAL(i)) then
            print 100, buf(i), i, count, name
            errs = errs + 1
         endif
 10   continue
C
      do 20 i = count + 1, n
         if (buf(i) .ne. 0.) then
            print 100, buf(i), i, n, name
            errs = errs + 1
         endif
 20   continue
C      
100   format('Invalid data', f6.1, ' at ', i4, ' of ', i4, ' in ', a)
C
      end
C
C    This routine is used to prevent the compiler from deallocating the 
C    array "a", which may happen in some of the tests (see the text in 
C    the MPI standard about why this may be a problem in valid Fortran 
C    codes).  Without this, for example, tests fail with the Cray ftn
C    compiler.
C
      subroutine dummyRef( a, n, ie )
      integer n, ie
      real    a(n)
C This condition will never be true, but the compile won't know that
      if (ie .eq. -1) then
          print *, a(n)
      endif
      return
      end
