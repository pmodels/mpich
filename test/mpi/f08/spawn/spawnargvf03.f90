! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!  This test makes use of routines to access the command line added in
!  Fortran 2003
!
        program main
!     declared on the old sparc compilers
        use mpi_f08
        integer errs, err
        integer rank, size, rsize, i
        integer np
        integer errcodes(2)
        type(MPI_Comm) parentcomm, intercomm
        type(MPI_Status) status
        character*(10) inargv(6), outargv(6)
        character*(80)   argv(64)
        integer argc
        data inargv /"a", "b=c", "d e", "-pf", " Ss", " " /
        data outargv /"a", "b=c", "d e", "-pf", " Ss", " " /
        integer ierr
        integer comm_size
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
!       Create 2 more processes
           call MPI_Comm_spawn( "./spawnargvf03", inargv, np, &
      &          MPI_INFO_NULL, 0, MPI_COMM_WORLD, intercomm, errcodes, &
      &          ierr )
        else
           intercomm = parentcomm
        endif

!       We now have a valid intercomm

        call MPI_Comm_remote_size( intercomm, rsize, ierr )
        call MPI_Comm_size( intercomm, size, ierr )
        call MPI_Comm_rank( intercomm, rank, ierr )

        if (parentcomm .eq. MPI_COMM_NULL) then
!           Master
        if (rsize .ne. np) then
            errs = errs + 1
            print *, "Did not create ", np, " processes (got &
      &           ", rsize, ")"
         endif
         do i=0, rsize-1
            call MPI_Send( i, 1, MPI_INTEGER, i, 0, intercomm, ierr )
         enddo
!       We could use intercomm reduce to get the errors from the
!       children, but we'll use a simpler loop to make sure that
!       we get valid data
         do i=0, rsize-1
            call MPI_Recv( err, 1, MPI_INTEGER, i, 1, intercomm, &
      &           MPI_STATUS_IGNORE, ierr )
            errs = errs + err
         enddo
        else
!       Child
!       FIXME: This assumes that stdout is handled for the children
!       (the error count will still be reported to the parent)
           argc = command_argument_count()
           do i=1, argc
              call get_command_argument( i, argv(i) )
           enddo
        if (size .ne. np) then
            errs = errs + 1
            print *, "(Child) Did not create ", np, " processes (got ", &
      &           size, ")"
         endif

         call MPI_Recv( i, 1, MPI_INTEGER, 0, 0, intercomm, status, ierr &
      &        )
        if (i .ne. rank) then
           errs = errs + 1
           print *, "Unexpected rank on child ", rank, "(",i,")"
        endif
!       Check the command line
        do i=1, argc
           if (outargv(i) .eq. " ") then
              errs = errs + 1
              print *, "Wrong number of arguments (", argc, ")"
              goto 200
           endif
           if (argv(i) .ne. outargv(i)) then
              errs = errs + 1
              print *, "Found arg ", argv(i), " but expected ", &
      &             outargv(i)
           endif
        enddo
 200    continue
        if (outargv(i) .ne. " ") then
!       We had too few args in the spawned command
            errs = errs + 1
            print *, "Too few arguments to spawned command"
         endif
!       Send the errs back to the master process
         call MPI_Ssend( errs, 1, MPI_INTEGER, 0, 1, intercomm, ierr )
        endif

!       It isn't necessary to free the intercomm, but it should not hurt
        call MPI_Comm_free( intercomm, ierr )

!       Note that the MTest_Finalize get errs only over COMM_WORLD
        if (parentcomm .eq. MPI_COMM_NULL) then
           call MTest_Finalize( errs )
        endif

 300    continue
        call MPI_Finalize( ierr )
        end
