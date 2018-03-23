! This file created from test/mpi/f77/attr/baseattr2f.f with f77tof90
! -*- Mode: Fortran; -*-
!
!
! (C) 2001 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
        program main
        use mpi_f08
        integer ierr, errs
        logical flag
        integer value, commsize, commrank

        errs = 0
        call mtest_init( ierr )

        call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )
        call mpi_comm_rank( MPI_COMM_WORLD, commrank, ierr )

        call mpi_attr_get( MPI_COMM_WORLD, MPI_TAG_UB, value, flag, ierr &
      &       )
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get TAG_UB"
        else
           if (value .lt. 32767) then
              errs = errs + 1
              print *, "Got too-small value (", value, ") for TAG_UB"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_HOST, value, flag, ierr )
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get HOST"
        else
           if ((value .lt. 0 .or. value .ge. commsize) .and. value .ne. &
      &          MPI_PROC_NULL) then
              errs = errs + 1
              print *, "Got invalid value ", value, " for HOST"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_IO, value, flag, ierr )
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get IO"
        else
           if ((value .lt. 0 .or. value .ge. commsize) .and. value .ne. &
      &          MPI_ANY_SOURCE .and. value .ne. MPI_PROC_NULL) then
              errs = errs + 1
              print *, "Got invalid value ", value, " for IO"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL, value, &
      &       flag, ierr )
        if (flag) then
!          Wtime need not be set
           if (value .lt.  0 .or. value .gt. 1) then
              errs = errs + 1
              print *, "Invalid value for WTIME_IS_GLOBAL (got ", value, &
      &             ")"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_APPNUM, value, flag, ierr &
      &       )
!     appnum need not be set
        if (flag) then
           if (value .lt. 0) then
              errs = errs + 1
              print *, "MPI_APPNUM is defined as ", value, &
      &             " but must be nonnegative"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, value, &
      &       flag, ierr )
!     MPI_UNIVERSE_SIZE need not be set
        if (flag) then
           if (value .lt. commsize) then
              errs = errs + 1
              print *, "MPI_UNIVERSE_SIZE = ", value, &
      &             ", less than comm world (", commsize, ")"
           endif
        endif

        call mpi_attr_get( MPI_COMM_WORLD, MPI_LASTUSEDCODE, value, flag &
      &       , ierr )
! Last used code must be defined and >= MPI_ERR_LASTCODE
        if (flag) then
           if (value .lt. MPI_ERR_LASTCODE) then
            errs = errs + 1
            print *, "MPI_LASTUSEDCODE points to an integer (", &
      &           MPI_ERR_LASTCODE, ") smaller than MPI_ERR_LASTCODE (", &
      &           value, ")"
            endif
         else
            errs = errs + 1
            print *, "MPI_LASTUSECODE is not defined"
         endif

      call MTest_Finalize( errs )

      end
