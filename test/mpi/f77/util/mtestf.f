C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
        subroutine MTest_Init( ierr )
C       Place the include first so that we can automatically create a
C       Fortran 90 version that uses the mpi module instead.  If
C       the module is in a different place, the compiler can complain
C       about out-of-order statements
        implicit none
        include 'mpif.h'
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
C
        subroutine MTest_Finalize( errs )
        implicit none
        include 'mpif.h'
        integer errs
        integer rank, toterrs, ierr
        
        call MPI_Comm_rank( MPI_COMM_WORLD, rank, ierr )

        call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM, 
     *        MPI_COMM_WORLD, ierr ) 
        
        if (rank .eq. 0) then
           if (toterrs .gt. 0) then 
                print *, " Found ", toterrs, " errors"
           else
                print *, " No Errors"
           endif
        endif
        end
C
C A simple get intracomm for now
        logical function MTestGetIntracomm( comm, min_size, qsmaller )
        implicit none
        include 'mpif.h'
        integer ierr
        integer comm, min_size, size, rank
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
           call mpi_comm_split( MPI_COMM_WORLD, 0, size - rank, comm, 
     &                                 ierr )
        else
           if (min_size .eq. 1 .and. myindex .eq. 3) then
              comm = MPI_COMM_SELF
           endif
        endif
        myindex = mod( myindex, 4 ) + 1
        MTestGetIntracomm = comm .ne. MPI_COMM_NULL
        end
C
        subroutine MTestFreeComm( comm )
        implicit none
        include 'mpif.h'
        integer comm, ierr
        if (comm .ne. MPI_COMM_WORLD .and.
     &      comm .ne. MPI_COMM_SELF  .and.
     &      comm .ne. MPI_COMM_NULL) then
           call mpi_comm_free( comm, ierr )
        endif
        end
C
        subroutine MTestPrintError( errcode )
        implicit none
        include 'mpif.h'
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, "Error class ", errclass, "(", string(1:slen), ")"
        end
C
        subroutine MTestPrintErrorMsg( msg, errcode )
        implicit none
        include 'mpif.h'
        character*(*) msg
        integer errcode
        integer errclass, slen, ierr
        character*(MPI_MAX_ERROR_STRING) string

        call MPI_Error_class( errcode, errclass, ierr )
        call MPI_Error_string( errcode, string, slen, ierr )
        print *, msg, ": Error class ", errclass, "
     $       (", string(1:slen), ")" 
        end

        subroutine MTestSpawnPossible( can_spawn, errs )
        implicit none
        include 'mpif.h'
        integer can_spawn
        integer errs
        integer(kind=MPI_ADDRESS_KIND) val
        integer ierror
        logical flag
        integer comm_size

        call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_UNIVERSE_SIZE, val,
     &                          flag, ierror )
        if ( ierror .ne. MPI_SUCCESS ) then
C       MPI_UNIVERSE_SIZE keyval missing from MPI_COMM_WORLD attributes
            can_spawn = -1
            errs = errs + 1
        else
            if ( flag ) then
                comm_size = -1

                call mpi_comm_size( MPI_COMM_WORLD, comm_size, ierror )
                if ( ierror .ne. MPI_SUCCESS ) then
C       MPI_COMM_SIZE failed for MPI_COMM_WORLD
                    can_spawn = -1
                    errs = errs + 1
                    return
                endif

                if ( val .le. comm_size ) then
C       no additional processes can be spawned
                    can_spawn = 0
                else
                    can_spawn = 1
                endif
            else
C       No attribute associated with key MPI_UNIVERSE_SIZE of MPI_COMM_WORLD
                can_spawn = -1
            endif
        endif
        end
