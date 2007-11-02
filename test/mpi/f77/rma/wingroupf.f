C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer ierr, errs
      integer buf(10)
      integer comm, group1, group2, result, win, intsize
      logical mtestGetIntraComm
      include 'addsize.h'

      errs = 0
      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      do while( mtestGetIntraComm( comm, 2, .false. ) ) 
         asize = 10
         call mpi_win_create( buf, asize, intsize, 
     &                        MPI_INFO_NULL, comm, win, ierr )
         
         call mpi_comm_group( comm, group1, ierr )
         call mpi_win_get_group( win, group2, ierr )
         call mpi_group_compare( group1, group2, result, ierr )
         if (result .ne. MPI_IDENT) then
            errs = errs + 1
            print *, ' Did not get the ident groups'
         endif
         call mpi_group_free( group1, ierr )
         call mpi_group_free( group2, ierr )

         call mpi_win_free( win, ierr )
         call mtestFreeComm( comm )
      enddo
C
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
