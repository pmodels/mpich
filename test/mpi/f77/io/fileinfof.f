C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer ierr, errs
      integer fh, info1, info2, rank
      logical flag
      character*(50) filename
      character*(MPI_MAX_INFO_KEY) mykey
      character*(MPI_MAX_INFO_VAL) myvalue

      errs = 0
      call mtest_init( ierr )

      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
C
C Open a simple file
      ierr = -1
      filename = "iotest.txt"
      call mpi_file_open( MPI_COMM_WORLD, filename, MPI_MODE_RDWR + 
     &     MPI_MODE_CREATE, MPI_INFO_NULL, fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
C
C Try to set one of the available info hints  
      call mpi_info_create( info1, ierr )
      call mpi_info_set( info1, "access_style", 
     &                   "read_once,write_once", ierr )
      ierr = -1
      call mpi_file_set_info( fh, info1, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
      call mpi_info_free( info1, ierr )
      
      ierr = -1
      call mpi_file_get_info( fh, info2, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         errs = errs + 1
         call MTestPrintError( ierr )
      endif
      call mpi_info_get( info2, "filename", MPI_MAX_INFO_VAL, 
     &                   myvalue, flag, ierr )
C
C An implementation isn't required to provide the filename (though
C a high-quality implementation should)
      if (flag) then
C If we find it, we must have the correct name
         if (myvalue(1:10) .ne. filename(1:10) .or.
     &       myvalue(11:11) .ne. ' ') then
            errs = errs + 1
            print *, ' Returned wrong value for the filename'
         endif
      endif
      call mpi_info_free( info2, ierr )
C
      call mpi_file_close( fh, ierr )
      call mpi_barrier( MPI_COMM_WORLD, ierr )
      if (rank .eq. 0) then
         call mpi_file_delete( filename, MPI_INFO_NULL, ierr )
      endif

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
