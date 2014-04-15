! This file created from test/mpi/f77/rma/winattr2f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!  This is a modified version of winattrf.f that uses two of the
!  default functions
!
      program main
      use mpi_f08
      integer errs, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      integer buf(10)
      TYPE(MPI_Comm) comm
      TYPE(MPI_Win) win
      integer keyval
      logical flag
!
! The only difference between the MPI-2 and MPI-1 attribute caching
! routines in Fortran is that the take an address-sized integer
! instead of a simple integer.  These still are not pointers,
! so the values are still just integers.
!
      errs      = 0
      call mtest_init( ierr )
      call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
! Create a new window; use val for an address-sized int
      val = 10
      call mpi_win_create( buf, val, 1, &
      &                        MPI_INFO_NULL, comm, win, ierr )
!
      extrastate = 1001
      call mpi_win_create_keyval( MPI_WIN_DUP_FN,  &
      &                            MPI_WIN_NULL_DELETE_FN, keyval,  &
      &                             extrastate, ierr )
      flag = .true.
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' get attr returned true when no attr set'
      endif

      valin = 2003
      call mpi_win_set_attr( win, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (valout .ne. 2003) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2003)', valout,  &
      &            ' from attr'
      endif

      valin = 2001
      call mpi_win_set_attr( win, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (valout .ne. 2001) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2001)', valout,  &
      &            ' from attr'
      endif
!
! Test the attr delete function
      call mpi_win_delete_attr( win, keyval, ierr )
      flag = .true.
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif

! Test the delete function on window free
      valin = 2001
      call mpi_win_set_attr( win, keyval, valin, ierr )
      call mpi_win_free( win, ierr )
      call mpi_comm_free( comm, ierr )
      ierr = -1
      call mpi_win_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
