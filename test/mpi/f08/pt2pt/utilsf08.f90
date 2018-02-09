! This file created from test/mpi/f77/pt2pt/utilsf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!

!------------------------------------------------------------------------------
!
!  Check for correct source, tag, count, and data in test message.
!
!------------------------------------------------------------------------------
      subroutine msg_check( recv_buf, source, tag, count, status, n, &
      &                      name, errs )
      use mpi
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
         print *, '[', rank, '] Unexpected source:', recv_src, &
      &            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_tag .ne. tag) then
         print *, '[', rank, '] Unexpected tag:', recv_tag, ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      if (recv_count .ne. count) then
         print *, '[', rank, '] Unexpected count:', recv_count, &
      &            ' in ', name
         errs       = errs + 1
         foundError = .true.
      end if

      call verify_test_data(recv_buf, count, n, name, errs )

      end
!------------------------------------------------------------------------------
!
!  Check that requests have been set to null
!
!------------------------------------------------------------------------------
      subroutine rq_check( requests, n, msg )
      use mpi
      integer n, requests(n)
      character*(*) msg
      integer i
!
      do 10 i=1, n
         if (requests(i) .ne. MPI_REQUEST_NULL) then
            print *, 'Nonnull request in ', msg
         endif
 10   continue
!
      end
!------------------------------------------------------------------------------
!
!  Initialize test data buffer with integral sequence.
!
!------------------------------------------------------------------------------
      subroutine init_test_data(buf,n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = REAL(i)
 10    continue
      end

!------------------------------------------------------------------------------
!
!  Clear test data buffer
!
!------------------------------------------------------------------------------
      subroutine clear_test_data(buf, n)
      integer n
      real buf(n)
      integer i

      do 10 i = 1, n
         buf(i) = 0.
 10   continue

      end

!------------------------------------------------------------------------------
!
!  Verify test data buffer
!
!------------------------------------------------------------------------------
      subroutine verify_test_data( buf, count, n, name, errs )
      use mpi
      integer n, errs
      real buf(n)
      character *(*) name
      integer count, ierr, i
!
      do 10 i = 1, count
         if (buf(i) .ne. REAL(i)) then
            print 100, buf(i), i, count, name
            errs = errs + 1
         endif
 10   continue
!
      do 20 i = count + 1, n
         if (buf(i) .ne. 0.) then
            print 100, buf(i), i, n, name
            errs = errs + 1
         endif
 20   continue
!
100   format('Invalid data', f6.1, ' at ', i4, ' of ', i4, ' in ', a)
!
      end
!
!    This routine is used to prevent the compiler from deallocating the
!    array "a", which may happen in some of the tests (see the text in
!    the MPI standard about why this may be a problem in valid Fortran
!    codes).  Without this, for example, tests fail with the Cray ftn
!    compiler.
!
      subroutine dummyRef( a, n, ie )
      integer n, ie
      real    a(n)
! This condition will never be true, but the compile won't know that
      if (ie .eq. -1) then
          print *, a(n)
      endif
      return
      end
