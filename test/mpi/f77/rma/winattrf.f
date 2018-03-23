C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, ierr
      include 'attraints.h'
      integer comm, win, buf(10)
      integer curcount, keyval
      logical flag
      external mycopyfn, mydelfn
      integer callcount, delcount
      common /myattr/ callcount, delcount
C
C The only difference between the MPI-2 and MPI-1 attribute caching
C routines in Fortran is that the take an address-sized integer
C instead of a simple integer.  These still are not pointers,
C so the values are still just integers. 
C
      errs      = 0
      callcount = 0
      delcount  = 0
      call mtest_init( ierr )
      call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
C Create a new window; use val for an address-sized int
      val = 10
      call mpi_win_create( buf, val, 1,
     &                        MPI_INFO_NULL, comm, win, ierr )
C 
      extrastate = 1001
      call mpi_win_create_keyval( mycopyfn, mydelfn, keyval, 
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
         print *, 'Unexpected value (should be 2003)', valout, 
     &            ' from attr'
      endif
      
      valin = 2001
      call mpi_win_set_attr( win, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (valout .ne. 2001) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2001)', valout, 
     &            ' from attr'
      endif
C
C Test the attr delete function
      delcount   = 0
      call mpi_win_delete_attr( win, keyval, ierr )
      if (delcount .ne. 1) then
         errs = errs + 1
         print *, ' Delete_attr did not call delete function'
      endif
      flag = .true.
      call mpi_win_get_attr( win, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif
      
C Test the delete function on window free
      valin = 2001
      call mpi_win_set_attr( win, keyval, valin, ierr )
      curcount = delcount
      call mpi_win_free( win, ierr )
      if (delcount .ne. curcount + 1) then
         errs = errs + 1
         print *, ' did not get expected value of delcount ', 
     &          delcount, curcount + 1
      endif

      ierr = -1
      call mpi_win_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif
C
C The MPI standard defines null copy and duplicate functions.
C However, are only used when an object is duplicated.  Since
C MPI_Win objects cannot be duplicated, so under normal circumstances,
C these will not be called.  Since they are defined, they should behave
C as defined.  To test them, we simply call them here
      flag   = .false.
      valin  = 7001
      valout = -1
      ierr   = -1
      call MPI_WIN_DUP_FN( win, keyval, extrastate, valin, valout,
     $     flag, ierr ) 
      if (.not. flag) then
         errs = errs + 1
         print *, " Flag was false after MPI_WIN_DUP_FN"
      else if (valout .ne. 7001) then
         errs = errs + 1
         if (valout .eq. -1 ) then
          print *, " output attr value was not copied in MPI_WIN_DUP_FN" 
         endif
         print *, " value was ", valout, " but expected 7001"
      else if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         print *, " MPI_WIN_DUP_FN did not return MPI_SUCCESS"
      endif

      flag   = .true.
      valin  = 7001
      valout = -1
      ierr   = -1
      call MPI_WIN_NULL_COPY_FN( win, keyval, extrastate, valin, valout
     $     ,flag, ierr ) 
      if (flag) then
         errs = errs + 1
         print *, " Flag was true after MPI_WIN_NULL_COPY_FN"
      else if (valout .ne. -1) then
         errs = errs + 1
         print *,
     $        " output attr value was copied in MPI_WIN_NULL_COPY_FN" 
      else if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         print *, " MPI_WIN_NULL_COPY_FN did not return MPI_SUCCESS"
      endif
C
      call mpi_comm_free( comm, ierr )
      call mtest_finalize( errs )
      end
C
C Note that the copyfn is unused for MPI windows, since there is
C (and because of alias rules, can be) no MPI_Win_dup function
      subroutine mycopyfn( oldwin, keyval, extrastate, valin, valout,
     &                     flag, ierr )
      implicit none
      include 'mpif.h'
      integer oldwin, keyval, ierr
      include 'attraints.h'
      logical flag
      integer callcount, delcount
      common /myattr/ callcount, delcount
C increment the attribute by 2
      valout = valin + 2
      callcount = callcount + 1
C
C Since we should *never* call this, indicate an error
      print *, ' Unexpected use of mycopyfn'
      flag = .false.
      ierr = MPI_ERR_OTHER
      end
C
      subroutine mydelfn( win, keyval, val, extrastate, ierr )
      implicit none
      include 'mpif.h'
      integer win, keyval, ierr
      include 'attraints.h'
      integer callcount, delcount
      common /myattr/ callcount, delcount
      delcount = delcount + 1
      if (extrastate .eq. 1001) then
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         ierr = MPI_ERR_OTHER
      endif
      end
