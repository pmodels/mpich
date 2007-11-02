C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      include 'attraints.h'
      logical flag
      integer ierr, errs
      integer base(1024)
      integer disp
      integer win
      integer commsize
C Include addsize defines asize as an address-sized integer
      include 'addsize.h'

      errs = 0
      
      call mtest_init( ierr )
      call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )

C Create a window; then extract the values 
      asize    = 1024
      disp = 4
      call MPI_Win_create( base, asize, disp, MPI_INFO_NULL, 
     &  MPI_COMM_WORLD, win, ierr )
C
C In order to check the base, we need an address-of function.
C We use MPI_Get_address, even though that isn't strictly correct
      call MPI_Win_get_attr( win, MPI_WIN_BASE, valout, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_BASE"
C
C There is no easy way to get the actual value of base to compare 
C against.  MPI_Address gives a value relative to MPI_BOTTOM, which 
C is different from 0 in Fortran (unless you can define MPI_BOTTOM
C as something like %pointer(0)).
C      else
C
CC For this Fortran 77 version, we use the older MPI_Address function
C         call MPI_Address( base, baseadd, ierr )
C         if (valout .ne. baseadd) then
C           errs = errs + 1
C           print *, "Got incorrect value for WIN_BASE (", valout, 
C     &             ", should be ", baseadd, ")"
C         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_SIZE, valout, flag, ierr )
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_SIZE"
      else
        if (valout .ne. asize) then
            errs = errs + 1
            print *, "Got incorrect value for WIN_SIZE (", valout, 
     &        ", should be ", asize, ")"
         endif
      endif

      call MPI_Win_get_attr( win, MPI_WIN_DISP_UNIT, valout, flag, ierr)
      if (.not. flag) then
         errs = errs + 1
         print *, "Could not get WIN_DISP_UNIT"
      else
         if (valout .ne. disp) then
            errs = errs + 1
            print *, "Got wrong value for WIN_DISP_UNIT (", valout, 
     &               ", should be ", disp, ")"
         endif
      endif

      call MPI_Win_free( win, ierr )

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )

      end
