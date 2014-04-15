! This file created from test/mpi/f77/pt2pt/mprobef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer ierr, rank, size, count
      integer sendbuf(8), recvbuf(8)
      TYPE(MPI_Status) s1, s2
      integer errs
      TYPE(MPI_Message) msg
      TYPE(MPI_Request) rreq
      logical found, flag

      ierr = -1
      errs = 0
      call mpi_init( ierr )
      if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          print *, ' Unexpected return from MPI_INIT', ierr
      endif

      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      if (size .lt. 2) then
          errs = errs + 1
          print *, ' This test requires at least 2 processes'
!         Abort now - do not continue in this case.
          call mpi_abort( MPI_COMM_WORLD, 1, ierr )
      endif
      if (size .gt. 2) then
          print *, ' This test is running with ', size, ' processes,'
          print *, ' only 2 processes are used.'
      endif

! Test 0: simple Send and Mprobe+Mrecv.
      if (rank .eq. 0) then
          sendbuf(1) = 1735928559
          sendbuf(2) = 1277009102
          call MPI_Send(sendbuf, 2, MPI_INTEGER, &
      &                  1, 5, MPI_COMM_WORLD, ierr)
      else
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Mprobe(0, 5, MPI_COMM_WORLD, msg, s1, ierr)
          if (s1%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's1%MPI_SOURCE != 0 at T0 Mprobe().'
          endif
          if (s1%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's1%MPI_TAG != 5 at T0 Mprobe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T0 Mprobe().'
          endif
          if (msg .eq. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg == MPI_MESSAGE_NULL at T0 Mprobe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 2) then
              errs = errs + 1
              print *, 'probed buffer does not have 2 MPI_INTEGERs.'
          endif

          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Mrecv(recvbuf, count, MPI_INTEGER, msg, s2, ierr)
          if (recvbuf(1) .ne. 1735928559) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T0 Mrecv().'
          endif
          if (recvbuf(2) .ne. 1277009102) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T0 Mrecv().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T0 Mrecv().'
          endif
          if (s2%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's2%MPI_TAG != 5 at T0 Mrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T0 Mrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T0 Mrecv().'
          endif
      endif

! Test 1: simple Send and Mprobe+Imrecv.
      if (rank .eq. 0) then
          sendbuf(1) = 1735928559
          sendbuf(2) = 1277009102
          call MPI_Send(sendbuf, 2, MPI_INTEGER, &
      &                  1, 5, MPI_COMM_WORLD, ierr)
      else
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Mprobe(0, 5, MPI_COMM_WORLD, msg, s1, ierr)
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T1 Mprobe().'
          endif
          if (s1%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's1%MPI_TAG != 5 at T1 Mprobe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T1 Mprobe().'
          endif
          if (msg .eq. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg == MPI_MESSAGE_NULL at T1 Mprobe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 2) then
              errs = errs + 1
              print *, 'probed buffer does not have 2 MPI_INTEGERs.'
          endif

          rreq = MPI_REQUEST_NULL
          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Imrecv(recvbuf, count, MPI_INTEGER, msg, rreq, ierr)
          if (rreq .eq. MPI_REQUEST_NULL) then
              errs = errs + 1
              print *, 'rreq is unmodified at T1 Imrecv().'
          endif
          call MPI_Wait(rreq, s2, ierr)
          if (recvbuf(1) .ne. 1735928559) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T1 Imrecv().'
          endif
          if (recvbuf(2) .ne. 1277009102) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T1 Imrecv().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T1 Imrecv().'
          endif
          if (s2%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's2%MPI_TAG != 5 at T1 Imrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T1 Imrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T1 Imrecv().'
          endif
      endif

! Test 2: simple Send and Improbe+Mrecv.
      if (rank .eq. 0) then
          sendbuf(1) = 1735928559
          sendbuf(2) = 1277009102
          call MPI_Send(sendbuf, 2, MPI_INTEGER, &
      &                  1, 5, MPI_COMM_WORLD, ierr)
      else
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Improbe(0, 5, MPI_COMM_WORLD, found, msg, s1, ierr)
          do while (.not. found)
              call MPI_Improbe(0, 5, MPI_COMM_WORLD, &
      &                          found, msg, s1, ierr)
          enddo
          if (msg .eq. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg == MPI_MESSAGE_NULL at T2 Improbe().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T2 Improbe().'
          endif
          if (s1%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's1%MPI_TAG != 5 at T2 Improbe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T2 Improbe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 2) then
              errs = errs + 1
              print *, 'probed buffer does not have 2 MPI_INTEGERs.'
          endif

          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Mrecv(recvbuf, count, MPI_INTEGER, msg, s2, ierr)
          if (recvbuf(1) .ne. 1735928559) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T2 Mrecv().'
          endif
          if (recvbuf(2) .ne. 1277009102) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T2 Mrecv().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T2 Mrecv().'
          endif
          if (s2%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's2%MPI_TAG != 5 at T2 Mrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T2 Mrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T2 Mrecv().'
          endif
      endif

! Test 3: simple Send and Improbe+Imrecv.
      if (rank .eq. 0) then
          sendbuf(1) = 1735928559
          sendbuf(2) = 1277009102
          call MPI_Send(sendbuf, 2, MPI_INTEGER, &
      &                  1, 5, MPI_COMM_WORLD, ierr)
      else
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Improbe(0, 5, MPI_COMM_WORLD, found, msg, s1, ierr)
          do while (.not. found)
              call MPI_Improbe(0, 5, MPI_COMM_WORLD, &
      &                          found, msg, s1, ierr)
          enddo
          if (msg .eq. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg == MPI_MESSAGE_NULL at T3 Improbe().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T3 Improbe().'
          endif
          if (s1%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's1%MPI_TAG != 5 at T3 Improbe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T3 Improbe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 2) then
              errs = errs + 1
              print *, 'probed buffer does not have 2 MPI_INTEGERs.'
          endif

          rreq = MPI_REQUEST_NULL
          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Imrecv(recvbuf, count, MPI_INTEGER, msg, rreq, ierr)
          if (rreq .eq. MPI_REQUEST_NULL) then
              errs = errs + 1
              print *, 'rreq is unmodified at T3 Imrecv().'
          endif
          call MPI_Wait(rreq, s2, ierr)
          if (recvbuf(1) .ne. 1735928559) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T3 Imrecv().'
          endif
          if (recvbuf(2) .ne. 1277009102) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T3 Imrecv().'
          endif
          if (s2%MPI_SOURCE .ne. 0) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != 0 at T3 Imrecv().'
          endif
          if (s2%MPI_TAG .ne. 5) then
              errs = errs + 1
              print *, 's2%MPI_TAG != 5 at T3 Imrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T3 Imrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T3 Imrecv().'
          endif
      endif

! Test 4: Mprobe+Mrecv with MPI_PROC_NULL
      if (.true.) then
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Mprobe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &
      &                     msg, s1, ierr)
          if (s1%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's1%MPI_SOURCE != MPI_PROC_NULL at T4 Mprobe().'
          endif
          if (s1%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's1%MPI_TAG != MPI_ANY_TAG at T4 Mprobe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T4 Mprobe().'
          endif
          if (msg .ne. MPI_MESSAGE_NO_PROC) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NO_PROC at T4 Mprobe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'probed buffer does not have 0 MPI_INTEGER.'
          endif

          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Mrecv(recvbuf, count, MPI_INTEGER, msg, s2, ierr)
!         recvbuf() should remain unmodified
          if (recvbuf(1) .ne. 19088743) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T4 Mrecv().'
          endif
          if (recvbuf(2) .ne. 1309737967) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T4 Mrecv().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T4 Mrecv().'
          endif
          if (s2%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's2%MPI_TAG != MPI_ANY_TAG at T4 Mrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T4 Mrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T4 Mrecv().'
          endif

          count = -1
          call MPI_Get_count(s2, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'recv buffer does not have 0 MPI_INTEGER.'
          endif
      endif

! Test 5: Mprobe+Imrecv with MPI_PROC_NULL
      if (.true.) then
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          msg = MPI_MESSAGE_NULL
          call MPI_Mprobe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &
      &                     msg, s1, ierr)
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T5 Mprobe().'
          endif
          if (s1%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's1%MPI_TAG != MPI_ANY_TAG at T5 Mprobe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T5 Mprobe().'
          endif
          if (msg .ne. MPI_MESSAGE_NO_PROC) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NO_PROC at T5 Mprobe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'probed buffer does not have 0 MPI_INTEGER.'
          endif

          rreq = MPI_REQUEST_NULL
          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Imrecv(recvbuf, count, MPI_INTEGER, msg, rreq, ierr)
          if (rreq .eq. MPI_REQUEST_NULL) then
              errs = errs + 1
              print *, 'rreq == MPI_REQUEST_NULL at T5 Imrecv().'
          endif
          flag = .false.
          call MPI_Test(rreq, flag, s2, ierr)
          if (.not. flag) then
              errs = errs + 1
              print *, 'flag is false at T5 Imrecv().'
          endif
!         recvbuf() should remain unmodified
          if (recvbuf(1) .ne. 19088743) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T5 Imrecv().'
          endif
          if (recvbuf(2) .ne. 1309737967) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T5 Imrecv().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T5 Imrecv().'
          endif
          if (s2%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's2%MPI_TAG != MPI_ANY_TAG at T5 Imrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T5 Imrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T5 Imrecv().'
          endif

          count = -1
          call MPI_Get_count(s2, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'recv buffer does not have 0 MPI_INTEGER.'
          endif
      endif

! Test 6: Improbe+Mrecv with MPI_PROC_NULL
      if (.true.) then
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          found = .false.
          msg = MPI_MESSAGE_NULL
          call MPI_Improbe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &
      &                      found, msg, s1, ierr)
          if (.not. found) then
              errs = errs + 1
              print *, 'found is false at T6 Improbe().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T6 Improbe()'
          endif
          if (s1%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's1%MPI_TAG != MPI_ANY_TAG at T6 Improbe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T6 Improbe().'
          endif
          if (msg .ne. MPI_MESSAGE_NO_PROC) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NO_PROC at T6 Improbe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'probed buffer does not have 0 MPI_INTEGER.'
          endif

          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Mrecv(recvbuf, count, MPI_INTEGER, msg, s2, ierr)
!         recvbuf() should remain unmodified
          if (recvbuf(1) .ne. 19088743) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T6 Mrecv().'
          endif
          if (recvbuf(2) .ne. 1309737967) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T6 Mrecv().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T6 Mrecv().'
          endif
          if (s2%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's2%MPI_TAG != MPI_ANY_TAG at T6 Mrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T6 Mrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T6 Mrecv().'
          endif

          count = -1
          call MPI_Get_count(s2, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'recv buffer does not have 0 MPI_INTEGER.'
          endif
      endif

! Test 7: Improbe+Imrecv with MPI_PROC_NULL
      if (.true.) then
!         the error fields are initialized for modification check.
          s1%MPI_ERROR = MPI_ERR_DIMS
          s2%MPI_ERROR = MPI_ERR_OTHER

          found = .false.
          msg = MPI_MESSAGE_NULL
          call MPI_Improbe(MPI_PROC_NULL, 5, MPI_COMM_WORLD, &
      &                      found, msg, s1, ierr)
          if (.not. found) then
              errs = errs + 1
              print *, 'found is false at T7 Improbe().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T7 Improbe()'
          endif
          if (s1%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's1%MPI_TAG != MPI_ANY_TAG at T7 Improbe().'
          endif
          if (s1%MPI_ERROR .ne. MPI_ERR_DIMS) then
              errs = errs + 1
              print *, 's1%MPI_ERROR != MPI_ERR_DIMS at T7 Improbe().'
          endif
          if (msg .ne. MPI_MESSAGE_NO_PROC) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NO_PROC at T7 Improbe().'
          endif

          count = -1
          call MPI_Get_count(s1, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'probed buffer does not have 0 MPI_INTEGER.'
          endif

          rreq = MPI_REQUEST_NULL
          recvbuf(1) = 19088743
          recvbuf(2) = 1309737967
          call MPI_Imrecv(recvbuf, count, MPI_INTEGER, msg, rreq, ierr)
          if (rreq .eq. MPI_REQUEST_NULL) then
              errs = errs + 1
              print *, 'rreq == MPI_REQUEST_NULL at T7 Imrecv().'
          endif
          flag = .false.
          call MPI_Test(rreq, flag, s2, ierr)
          if (.not. flag) then
              errs = errs + 1
              print *, 'flag is false at T7 Imrecv().'
          endif
!         recvbuf() should remain unmodified
          if (recvbuf(1) .ne. 19088743) then
              errs = errs + 1
              print *, 'recvbuf(1) is corrupted at T7 Imrecv().'
          endif
          if (recvbuf(2) .ne. 1309737967) then
              errs = errs + 1
              print *, 'recvbuf(2) is corrupted at T7 Imrecv().'
          endif
          if (s2%MPI_SOURCE .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, 's2%MPI_SOURCE != MPI_PROC_NULL at T7 Imrecv().'
          endif
          if (s2%MPI_TAG .ne. MPI_ANY_TAG) then
              errs = errs + 1
              print *, 's2%MPI_TAG != MPI_ANY_TAG at T7 Imrecv().'
          endif
          if (s2%MPI_ERROR .ne. MPI_ERR_OTHER) then
              errs = errs + 1
              print *, 's2%MPI_ERROR != MPI_ERR_OTHER at T7 Imrecv().'
          endif
          if (msg .ne. MPI_MESSAGE_NULL) then
              errs = errs + 1
              print *, 'msg != MPI_MESSAGE_NULL at T7 Imrecv().'
          endif

          count = -1
          call MPI_Get_count(s2, MPI_INTEGER, count, ierr)
          if (count .ne. 0) then
              errs = errs + 1
              print *, 'recv buffer does not have 0 MPI_INTEGER.'
          endif
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
