! This file created from test/mpi/f77/rma/c2f2cwinf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
! Test just MPI-RMA
!
      program main
      use mpi_f08
      integer errs, toterrs, ierr
      integer wrank, wsize
      integer wgroup, info, req
      TYPE(MPI_Win) win
      integer result
      integer c2fwin
! The integer asize must be of ADDRESS_KIND size
      integer (kind=MPI_ADDRESS_KIND) asize

      errs = 0

      call mpi_init( ierr )

!
! Test passing a Fortran MPI object to C
      call mpi_comm_rank( MPI_COMM_WORLD, wrank, ierr )
      asize = 0
      call mpi_win_create( 0, asize, 1, MPI_INFO_NULL,  &
      &     MPI_COMM_WORLD, win, ierr )
      errs = errs + c2fwin( win )
      call mpi_win_free( win, ierr )

!
! Test using a C routine to provide the Fortran handle
      call f2cwin( win )
!     no info, in comm world, created with no memory (base address 0,
!     displacement unit 1
      call mpi_win_free( win, ierr )

!
! Summarize the errors
!
      call mpi_allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, &
      &     MPI_COMM_WORLD, ierr )
      if (wrank .eq. 0) then
         if (toterrs .eq. 0) then
            print *, ' No Errors'
         else
            print *, ' Found ', toterrs, ' errors'
         endif
      endif

      call mpi_finalize( ierr )
      end

