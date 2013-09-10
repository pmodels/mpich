C -*- Mode: Fortran; -*-
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
        program main
        implicit none
        include 'mpif.h'
        integer comm, fh, r, s, i
        integer fileintsize
        integer errs, err, ierr
        character *(100) filename
        include 'iooffset.h'
        include 'ioaint.h'

        errs = 0
        call MTest_Init( ierr )

        filename = "iotest.txt"
        comm = MPI_COMM_WORLD
        call mpi_comm_size( comm, s, ierr )
        call mpi_comm_rank( comm, r, ierr )
C Try writing the file, then check it
        call mpi_file_open( comm, filename, MPI_MODE_RDWR + 
     &                      MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
        if (ierr .ne. MPI_SUCCESS) then
           errs = errs + 1
           if (errs .le. 10) then
              call MTestPrintError( ierr )
           endif
        endif
C
C Get the size of an INTEGER in the file
        call mpi_file_get_type_extent( fh, MPI_INTEGER, aint, ierr )
        fileintsize = aint
C
C We let each process write in turn, getting the position after each 
C write
        do i=1, s
           if (i .eq. r + 1) then
              call mpi_file_write_shared( fh, i, 1, MPI_INTEGER, 
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
              print *, r, ' Shared position is ', offset,' should be ',
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
c
        call MTest_Finalize( errs )
        call mpi_finalize( ierr )
        end
