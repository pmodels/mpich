C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C  This is a modified version of commattrf.f that uses two of the
C  default functions
C
      program main
      implicit none
      include 'mpif.h'
      integer errs, ierr
      include 'attraints.h'
      integer comm1, comm2
      integer keyval
      logical flag
C
C The only difference between the MPI-2 and MPI-1 attribute caching
C routines in Fortran is that the take an address-sized integer
C instead of a simple integer.  These still are not pointers,
C so the values are still just integers. 
C
      errs      = 0
      call mtest_init( ierr )
      call mpi_comm_dup( MPI_COMM_WORLD, comm1, ierr )
C 
      extrastate = 1001
      call mpi_comm_create_keyval( MPI_COMM_DUP_FN, 
     &                             MPI_COMM_NULL_DELETE_FN, keyval, 
     &                             extrastate, ierr )
      flag = .true.
      call mpi_comm_get_attr( comm1, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' get attr returned true when no attr set'
      endif

      valin = 2003
      call mpi_comm_set_attr( comm1, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_comm_get_attr( comm1, keyval, valout, flag, ierr )
      if (valout .ne. 2003) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2003)', valout, 
     &            ' from attr'
      endif
      
      valin = 2001
      call mpi_comm_set_attr( comm1, keyval, valin, ierr )
      flag = .false.
      valout = -1
      call mpi_comm_get_attr( comm1, keyval, valout, flag, ierr )
      if (valout .ne. 2001) then
         errs = errs + 1
         print *, 'Unexpected value (should be 2001)', valout, 
     &            ' from attr'
      endif
      
C
C Test the copy function
      valin = 5001
      call mpi_comm_set_attr( comm1, keyval, valin, ierr )
      call mpi_comm_dup( comm1, comm2, ierr )
      flag = .false.
      call mpi_comm_get_attr( comm1, keyval, valout, flag, ierr )
      if (valout .ne. 5001) then
         errs = errs + 1
         print *, 'Unexpected output value in comm ', valout
      endif
      flag = .false.
      call mpi_comm_get_attr( comm2, keyval, valout, flag, ierr )
      if (valout .ne. 5001) then
         errs = errs + 1
         print *, 'Unexpected output value in comm2 ', valout
      endif
C Test the delete function      
      call mpi_comm_free( comm2, ierr )
C
C Test the attr delete function
      call mpi_comm_dup( comm1, comm2, ierr )
      valin      = 6001
      extrastate = 1001
      call mpi_comm_set_attr( comm2, keyval, valin, ierr )
      call mpi_comm_delete_attr( comm2, keyval, ierr )
      flag = .true.
      call mpi_comm_get_attr( comm2, keyval, valout, flag, ierr )
      if (flag) then
         errs = errs + 1
         print *, ' Delete_attr did not delete attribute'
      endif
      call mpi_comm_free( comm2, ierr )
C
      ierr = -1
      call mpi_comm_free_keyval( keyval, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call mtestprinterror( ierr )
      endif
      call mpi_comm_free( comm1, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
