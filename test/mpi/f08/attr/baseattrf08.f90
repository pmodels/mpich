! This file created from test/mpi/f77/attr/baseattrf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      integer value, commsize
      logical flag
      integer ierr, errs

      errs = 0
      call mpi_init( ierr )

      call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )
      call mpi_attr_get( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, value, flag &
      &     , ierr)
      ! MPI_UNIVERSE_SIZE need not be set
      if (flag) then
         if (value .lt. commsize) then
            print *, "MPI_UNIVERSE_SIZE is ", value, " less than world " &
      &           , commsize
            errs = errs + 1
         endif
      endif

      call mpi_attr_get( MPI_COMM_WORLD, MPI_LASTUSEDCODE, value, flag, &
      &     ierr )
      ! Last used code must be defined and >= MPI_ERR_LASTCODE
      if (flag) then
         if (value .lt. MPI_ERR_LASTCODE) then
            errs = errs + 1
            print *, "MPI_LASTUSEDCODE points to an integer &
      &           (", value, ") smaller than MPI_ERR_LASTCODE (", &
      &           MPI_ERR_LASTCODE, ")"
         endif
      else
         errs = errs + 1
         print *, "MPI_LASTUSECODE is not defined"
      endif

      call mpi_attr_get( MPI_COMM_WORLD, MPI_APPNUM, value, flag, ierr )
      ! appnum need not be set
      if (flag) then
         if (value .lt. 0) then
            errs = errs + 1
            print *, "MPI_APPNUM is defined as ", value, &
      &           " but must be nonnegative"
         endif
      endif

      ! Check for errors
      if (errs .eq. 0) then
         print *, " No Errors"
      else
         print *, " Found ", errs, " errors"
      endif

      call MPI_Finalize( ierr )

      end

