! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        subroutine MTest_Init( ierr )

        use mpi_f08
        integer ierr
        logical flag
        logical dbgflag
        integer wrank
        common /mtest/ dbgflag, wrank

        call MPI_Initialized( flag, ierr )
        if (.not. flag) then
           call MPI_Init( ierr )
        endif

        dbgflag = .false.
        call MPI_Comm_rank( MPI_COMM_WORLD, wrank, ierr )
        end
!
        subroutine MTest_Finalize( errs )
        use mpi
        integer errs
        integer rank, toterrs, ierr

        call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )

        call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM,  &
      &        MPI_COMM_WORLD, ierr )

        if (rank .eq. 0) then
           if (toterrs .gt. 0) then
                print *, " Found ", toterrs, " errors"
           else
                print *, " No Errors"
           endif
        endif
        end
!
! A simple get intracomm for now
        logical function MTestGetIntracomm( comm, min_size, qsmaller )
        use mpi_f08
        integer ierr
        integer min_size, size, rank
        TYPE(MPI_Comm) comm
        logical qsmaller
        integer myindex
        save myindex
        data myindex /0/

        comm = MPI_COMM_NULL
        if (myindex .eq. 0) then
           comm = MPI_COMM_WORLD
        else if (myindex .eq. 1) then
           call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
        else if (myindex .eq. 2) then
           call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
           call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
           call mpi_comm_split( MPI_COMM_WORLD, 0, size - rank, comm,  &
      &                                 ierr )
        else
           if (min_size .eq. 1 .and. myindex .eq. 3) then
              comm = MPI_COMM_SELF
           endif
        endif
        myindex = mod( myindex, 4 ) + 1
        MTestGetIntracomm = comm /= MPI_COMM_NULL
        end
!
        subroutine MTestFreeComm( comm )
        use mpi_f08
        integer ierr
        TYPE(MPI_Comm) comm
        if (comm .ne. MPI_COMM_WORLD .and. &
      &      comm .ne. MPI_COMM_SELF  .and. &
      &      comm .ne. MPI_COMM_NULL) then
           call mpi_comm_free( comm, ierr )
        endif
        end
!
        subroutine MTestPrintError( errcode )
        use mpi_f08
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, "Error class ", errclass, "(", string(1:slen), ")"
        end
!
        subroutine MTestPrintErrorMsg( msg, errcode )
        use mpi_f08
        character*(*) msg
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, msg, ": Error class ", errclass, " &
      &       (", string(1:slen), ")"
        end
