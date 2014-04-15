! This file created from test/mpi/f77/rma/winnamef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer errs, ierr
      integer rlen, ln
      TYPE(MPI_Win) win
      character*(MPI_MAX_OBJECT_NAME) cname
      integer buf(10)
      integer intsize
! Include addsize defines asize as an address-sized integer
      integer (kind=MPI_ADDRESS_KIND) asize

      logical found
!
      errs = 0
      call mtest_init( ierr )
!
! Create a window and get, set the names on it
!
      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      asize = 10
      call mpi_win_create( buf, asize, intsize,  &
      &     MPI_INFO_NULL, MPI_COMM_WORLD, win, ierr )
!
!     Check that there is no name yet
      cname = 'XXXXXX'
      rlen  = -1
      call mpi_win_get_name( win, cname, rlen, ierr )
      if (rlen .ne. 0) then
         errs = errs + 1
         print *, ' Did not get empty name from new window'
      else if (cname(1:6) .ne. 'XXXXXX') then
         found = .false.
         do ln=MPI_MAX_OBJECT_NAME,1,-1
            if (cname(ln:ln) .ne. ' ') then
               found = .true.
            endif
         enddo
         if (found) then
            errs = errs + 1
            print *, ' Found a non-empty name'
         endif
      endif
!
! Now, set a name and check it
      call mpi_win_set_name( win, 'MyName', ierr )
      cname = 'XXXXXX'
      rlen = -1
      call mpi_win_get_name( win, cname, rlen, ierr )
      if (rlen .ne. 6) then
         errs = errs + 1
         print *, ' Expected 6, got ', rlen, ' for rlen'
         if (rlen .gt. 0 .and. rlen .lt. MPI_MAX_OBJECT_NAME) then
            print *, ' Cname = ', cname(1:rlen)
         endif
      else if (cname(1:6) .ne. 'MyName') then
         errs = errs + 1
         print *, ' Expected MyName, got ', cname(1:6)
      else
         found = .false.
         do ln=MPI_MAX_OBJECT_NAME,7,-1
            if (cname(ln:ln) .ne. ' ') then
               found = .true.
            endif
         enddo
         if (found) then
            errs = errs + 1
            print *, ' window name is not blank padded'
         endif
      endif
!
      call mpi_win_free( win, ierr )
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
