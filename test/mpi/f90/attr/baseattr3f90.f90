! -*- Mode: Fortran; -*-
!
!
! (C) 2012 by Argonne National Laboratory.
!     See COPYRIGHT in top-level directory.
!
! This is a MPI-2 version of baseattr2f90.f90 which uses COMM_GET_ATTR 
! instead of ATTR_GET, using an address-sized integer instead of 
! an INTEGER.
        program main
        use mpi
        integer ierr, errs
        logical flag
        integer commsize, commrank
        integer (KIND=MPI_ADDRESS_KIND) value

        errs = 0
        call mpi_init( ierr )

        call mpi_comm_size( MPI_COMM_WORLD, commsize, ierr )
        call mpi_comm_rank( MPI_COMM_WORLD, commrank, ierr )

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_TAG_UB, value,   &
             & flag, ierr ) 
        if (.not. flag) then
           errs = errs + 1
           print *, "Could not get TAG_UB"
        else
           if (value .lt. 32767) then
              errs = errs + 1
              print *, "Got too-small value (", value, ") for TAG_UB" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_HOST, value, flag&
             &, ierr ) 
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

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_IO, value, flag,&
             & ierr ) 
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

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_WTIME_IS_GLOBAL,&
             & value, flag, ierr ) 
        if (flag) then
!          Wtime need not be set
           if (value .lt.  0 .or. value .gt. 1) then 
              errs = errs + 1
              print *, "Invalid value for WTIME_IS_GLOBAL (got ", value, &
      &             ")" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_APPNUM, value,&
             & flag, ierr ) 
!     appnum need not be set
        if (flag) then
           if (value .lt. 0) then
              errs = errs + 1
              print *, "MPI_APPNUM is defined as ", value, &
      &             " but must be nonnegative" 
           endif
        endif

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE,&
             & value, flag, ierr ) 
!     MPI_UNIVERSE_SIZE need not be set
        if (flag) then
           if (value .lt. commsize) then
              errs = errs + 1
              print *, "MPI_UNIVERSE_SIZE = ", value, &
      &             ", less than comm world (", commsize, ")"
           endif
        endif
    
        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_LASTUSEDCODE,&
             & value, flag, ierr ) 
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

!     Check for errors
      if (errs .eq. 0) then
         print *, " No Errors"
      else
         print *, " Found ", errs, " errors"
      endif

      call MPI_Finalize( ierr )

      end
