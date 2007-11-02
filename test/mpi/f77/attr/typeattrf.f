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
      integer comm
      integer type1, type2
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
C 
C Attach an attribute to a predefined object
      type1 = MPI_INTEGER
      extrastate = 1001
      call mpi_type_create_keyval( mycopyfn, mydelfn, keyval, 
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
         print *, 'Unexpected value (should be 2003)', valout, 
     &            ' from attr'
      endif
      
      valin = 2001
      call mpi_type_set_attr( type1, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_type_get_attr( type1, keyval, valout, flag, ierr )
      if (valout .ne. 2001) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2001)', valout, 
     &            ' from attr'
      endif
      
C
C Test the copy function
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
      if (valout .ne. 5003) then
         errs = errs + 1
         print *, 'Unexpected output value in type2 ', valout
      endif
C Test the delete function      
      curcount = delcount
      call mpi_type_free( type2, ierr )
      if (delcount .ne. curcount + 1) then
         errs = errs + 1
         print *, ' did not get expected value of delcount ', 
     &          delcount, curcount + 1
      endif
C
C Test the attr delete function
      call mpi_type_dup( type1, type2, ierr )
      valin      = 6001
      extrastate = 1001
      call mpi_type_set_attr( type2, keyval, valin, ierr )
      delcount   = 0
      call mpi_type_delete_attr( type2, keyval, ierr )
      if (delcount .ne. 1) then
         errs = errs + 1
         print *, ' Delete_attr did not call delete function'
      endif
      flag = .true.
      call mpi_type_get_attr( type2, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif
      call mpi_type_free( type2, ierr )

      ierr = -1
      call mpi_type_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
C
      subroutine mycopyfn( oldtype, keyval, extrastate, valin, valout,
     &                     flag, ierr )
      implicit none
      include 'mpif.h'
      integer oldtype, keyval, ierr
      include 'attraints.h'
      logical flag
      integer callcount, delcount
      common /myattr/ callcount, delcount
C increment the attribute by 2
      valout = valin + 2
      callcount = callcount + 1
      if (extrastate .eq. 1001) then
         flag = .true.
         ierr = MPI_SUCCESS
      else
         print *, ' Unexpected value of extrastate = ', extrastate
         flag = .false.
         ierr = MPI_ERR_OTHER
      endif
      end
C
      subroutine mydelfn( type, keyval, val, extrastate, ierr )
      implicit none
      include 'mpif.h'
      integer type, keyval, ierr
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
