C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
        program main
        implicit none
        include 'mpif.h'
        integer errs, err
        integer rank, size, rsize, i
        integer np
        integer errcodes(2)
        integer parentcomm, intercomm
        integer status(MPI_STATUS_SIZE)
        integer ierr
        integer can_spawn

        errs = 0
        np   = 2

        call MTest_Init( ierr )

        call MTestSpawnPossible( can_spawn, errs )
        if ( can_spawn .eq. 0 ) then
            call MTest_Finalize( errs )
            goto 300
        endif

        call MPI_Comm_get_parent( parentcomm, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
C          Create 2 more processes 
           call MPI_Comm_spawn( "./spawnf", MPI_ARGV_NULL, np,
     $          MPI_INFO_NULL, 0, MPI_COMM_WORLD, intercomm, errcodes
     $          ,ierr ) 
        else
           intercomm = parentcomm
        endif

C   We now have a valid intercomm 

        call MPI_Comm_remote_size( intercomm, rsize, ierr )
        call MPI_Comm_size( intercomm, size, ierr )
        call MPI_Comm_rank( intercomm, rank, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
C           Master
           if (rsize .ne. np) then
              errs = errs + 1
              print *, "Did not create ", np, " processes (got ", rsize,
     $             ")" 
           endif
           if (rank .eq. 0) then
              do i=0,rsize-1
                 call MPI_Send( i, 1, MPI_INTEGER, i, 0, intercomm, ierr
     $                ) 
              enddo
C       We could use intercomm reduce to get the errors from the 
C       children, but we'll use a simpler loop to make sure that
C       we get valid data 
              do i=0, rsize-1
                 call MPI_Recv( err, 1, MPI_INTEGER, i, 1, intercomm,
     $                MPI_STATUS_IGNORE,  ierr ) 
                errs = errs + err
             enddo
          endif
        else 
C             Child 
           if (size .ne. np) then
              errs = errs + 1
              print *, "(Child) Did not create ", np, " processes (got "
     $             ,size, ")" 
           endif

           call MPI_Recv( i, 1, MPI_INTEGER, 0, 0, intercomm, status,
     $          ierr ) 

        if (i .ne. rank) then
            errs = errs + 1
            print *, "Unexpected rank on child ", rank, "(",i,")"
         endif

C       Send the errs back to the master process 
         call MPI_Ssend( errs, 1, MPI_INTEGER, 0, 1, intercomm, ierr )
        endif

C       It isn't necessary to free the intercomm, but it should not hurt
        call MPI_Comm_free( intercomm, ierr )

C       Note that the MTest_Finalize get errs only over COMM_WORLD 
C       Note also that both the parent and child will generate "No
C       Errors" if both call MTest_Finalize 
        if (parentcomm .eq. MPI_COMM_NULL) then
           call MTest_Finalize( errs )
        else
           call MPI_Finalize( ierr )
        endif

 300    continue
        end
