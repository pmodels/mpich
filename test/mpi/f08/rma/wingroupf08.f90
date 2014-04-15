! This file created from test/mpi/f77/rma/wingroupf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer ierr, errs
      integer buf(10)
      integer result, intsize
      TYPE(MPI_Win) win
      TYPE(MPI_Group) group1, group2
      TYPE(MPI_Comm) comm
      logical mtestGetIntraComm
      integer (kind=MPI_ADDRESS_KIND) asize


      errs = 0
      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, intsize, ierr )
      do while( mtestGetIntraComm( comm, 2, .false. ) )
         asize = 10
         call mpi_win_create( buf, asize, intsize,  &
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
!
      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
