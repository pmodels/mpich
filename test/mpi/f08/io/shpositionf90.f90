! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
        program main
        use mpi_f08
        type(MPI_Comm) comm
        type(MPI_File) fh
        integer r, s, i
        integer fileintsize
        integer errs, err, ierr
        character *(100) filename
        integer (kind=MPI_OFFSET_KIND) offset

        integer (kind=MPI_ADDRESS_KIND) aint


        errs = 0
        call MTest_Init( ierr )

        filename = "iotest.txt"
        comm = MPI_COMM_WORLD
        call mpi_comm_size( comm, s, ierr )
        call mpi_comm_rank( comm, r, ierr )
! Try writing the file, then check it
        call mpi_file_open( comm, filename, MPI_MODE_RDWR +  &
      &                      MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
        if (ierr .ne. MPI_SUCCESS) then
           errs = errs + 1
           if (errs .le. 10) then
              call MTestPrintError( ierr )
           endif
        endif
!
! Get the size of an INTEGER in the file
        call mpi_file_get_type_extent( fh, MPI_INTEGER, aint, ierr )
        fileintsize = aint
!
! We let each process write in turn, getting the position after each
! write
        do i=1, s
           if (i .eq. r + 1) then
              call mpi_file_write_shared( fh, i, 1, MPI_INTEGER,  &
      &            MPI_STATUS_IGNORE, ierr )
           if (ierr .ne. MPI_SUCCESS) then
              errs = errs + 1
              if (errs .le. 10) then
                 call MTestPrintError( ierr )
              endif
           endif
           endif
           call mpi_barrier( comm, ierr )
           call mpi_file_get_position_shared( fh, offset, ierr )
           if (offset .ne. fileintsize * i) then
              errs = errs + 1
              print *, r, ' Shared position is ', offset,' should be ', &
      &                 fileintsize * i
           endif
           call mpi_barrier( comm, ierr )
        enddo
        call mpi_file_close( fh, ierr )
        if (r .eq. 0) then
            call mpi_file_delete( filename, MPI_INFO_NULL, ierr )
        endif
        if (ierr .ne. MPI_SUCCESS) then
           errs = errs + 1
           if (errs .le. 10) then
              call MTestPrintError( ierr )
           endif
        endif
!
        call MTest_Finalize( errs )
        call mpi_finalize( ierr )
        end
