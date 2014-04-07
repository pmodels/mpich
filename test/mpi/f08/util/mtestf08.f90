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
