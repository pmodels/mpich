! This file created from test/mpi/f77/attr/typeattr2f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!  This is a modified version of typeattrf.f that uses two of the
!  default functions
!
      program main
      use mpi_f08
      integer errs, ierr
      integer (kind=MPI_ADDRESS_KIND) extrastate, valin, valout, val

      TYPE(MPI_Datatype) type1, type2
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
      type1 = MPI_INTEGER
!
      extrastate = 1001
      call mpi_type_create_keyval( MPI_TYPE_DUP_FN,  &
      &                             MPI_TYPE_NULL_DELETE_FN, keyval,  &
      &                             extrastate, ierr )
      flag = .true.
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' get attr returned true when no attr set'
      endif

      valin = 2003
      call mpi_type_set_attr( type1, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (valout .ne. 2003) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2003)', valout,  &
      &            ' from attr'
      endif

      valin = 2001
      call mpi_type_set_attr( type1, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (valout .ne. 2001) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2001)', valout,  &
      &            ' from attr'
      endif

!
! Test the copy function
      valin = 5001
      call mpi_type_set_attr( type1, keyval, valin, ierr )
      call mpi_type_dup( type1, type2, ierr )
      flag = .false.
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (valout .ne. 5001) then
         errs = errs + 1
         print *, 'Unexpected output value in type ', valout
      endif
      flag = .false.
      call mpi_type_get_attr( type2, keyval, valout, flag, ierr )
      if (valout .ne. 5001) then
         errs = errs + 1
         print *, 'Unexpected output value in type2 ', valout
      endif
! Test the delete function
      call mpi_type_free( type2, ierr )
!
! Test the attr delete function
      call mpi_type_dup( type1, type2, ierr )
      valin      = 6001
      extrastate = 1001
      call mpi_type_set_attr( type2, keyval, valin, ierr )
      call mpi_type_delete_attr( type2, keyval, ierr )
      flag = .true.
      call mpi_type_get_attr( type2, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif
      call mpi_type_free( type2, ierr )
!
      ierr = -1
      call mpi_type_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
