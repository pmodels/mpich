C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
C This tests spawn_mult by using the same executable and no command-line
C options.  The attribute MPI_APPNUM is used to determine which
C executable is running.
C
       program main
       implicit none
       include 'mpif.h'
       include 'type1aint.h'
       integer errs, err
       integer rank, size, rsize, wsize, i
       integer np(2)
       integer infos(2)
       integer errcodes(2)
       integer parentcomm, intercomm
       integer status(MPI_STATUS_SIZE)
       character*(30) cmds(2)
       integer appnum
       logical flag
       integer ierr
       integer can_spawn

       errs = 0

       call MTest_Init( ierr )

       call MTestSpawnPossible( can_spawn, errs )
        if ( can_spawn .eq. 0 ) then
            call MTest_Finalize( errs )
            goto 300
        endif

       call MPI_Comm_get_parent( parentcomm, ierr )

       if (parentcomm .eq. MPI_COMM_NULL) then
C       Create 2 more processes 
           cmds(1) = "./spawnmult2f"
           cmds(2) = "./spawnmult2f"
           np(1)   = 1
           np(2)   = 1
           infos(1)= MPI_INFO_NULL
           infos(2)= MPI_INFO_NULL
           call MPI_Comm_spawn_multiple( 2, cmds, MPI_ARGVS_NULL,            &
     &             np, infos, 0,                                             &
     &             MPI_COMM_WORLD, intercomm, errcodes, ierr )  
        else 
           intercomm = parentcomm
        endif

C       We now have a valid intercomm

        call MPI_Comm_remote_size( intercomm, rsize, ierr )
        call MPI_Comm_size( intercomm, size, ierr )
        call MPI_Comm_rank( intercomm, rank, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
C           Master 
            if (rsize .ne. np(1) + np(2)) then
                errs = errs + 1
                print *, "Did not create ", np(1)+np(2),                    &
     &          " processes (got ", rsize, ")" 
            endif
C Allow a multi-process parent
            if (rank .eq. 0) then
               do i=0, rsize-1
                  call MPI_Send( i, 1, MPI_INTEGER, i, 0, intercomm,        &
     &                 ierr ) 
               enddo
C       We could use intercomm reduce to get the errors from the 
C       children, but we'll use a simpler loop to make sure that
C       we get valid data 
              do i=0, rsize-1
                 call MPI_Recv( err, 1, MPI_INTEGER, i, 1, intercomm,       &
     &                       MPI_STATUS_IGNORE, ierr )
                 errs = errs + err
              enddo
            endif
         else 
C       Child 
C       FIXME: This assumes that stdout is handled for the children
C       (the error count will still be reported to the parent)
           if (size .ne. 2) then
              errs = errs + 1
              print *, "(Child) Did not create ", 2,                        &
     &             " processes (got ",size, ")"
              call mpi_comm_size( MPI_COMM_WORLD, wsize, ierr )
              if (wsize .eq. 2) then 
                 errs = errs + 1
                 print *, "(Child) world size is 2 but ",                   &
     &          " local intercomm size is not 2"
              endif
           endif
           
         call MPI_Recv( i, 1, MPI_INTEGER, 0, 0, intercomm, status, ierr    &
     &     )
         if (i .ne. rank) then
            errs = errs + 1
            print *, "Unexpected rank on child ", rank, "(",i,")"
         endif
C
C       Check for correct APPNUM
         call mpi_comm_get_attr( MPI_COMM_WORLD, MPI_APPNUM, aint,         &
     &        flag, ierr )
C        My appnum should be my rank in comm world
         if (flag) then
            appnum = aint
            if (appnum .ne. rank) then
                errs = errs + 1
                print *, "appnum is ", appnum, " but should be ", rank
             endif     
         else
             errs = errs + 1
             print *, "appnum was not set"
         endif

C       Send the errs back to the master process 
        call MPI_Ssend( errs, 1, MPI_INTEGER, 0, 1, intercomm, ierr )
        endif

C       It isn't necessary to free the intercomm, but it should not hurt
        call MPI_Comm_free( intercomm, ierr )

C       Note that the MTest_Finalize get errs only over COMM_WORLD 
        if (parentcomm .eq. MPI_COMM_NULL) then
            call MTest_Finalize( errs )
        endif

 300    continue
        call MPI_Finalize( ierr )
        end
