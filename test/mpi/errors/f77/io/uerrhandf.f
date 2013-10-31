C -*- Mode: Fortran; -*- 
C
C  (C) 2013 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      include 'addsize.h'
      include 'iooffset.h'
      integer ierr, rank, i
      integer errs
      external comm_errh_fn, win_errh_fn, file_errh_fn
      integer comm_errh, win_errh, file_errh
      integer winbuf(2), winh, wdup, wdsize, sizeofint, id
      integer fh, status(MPI_STATUS_SIZE)
      common /ec/ iseen
      integer iseen(3)
      save /ec/

      iseen(1) = 0
      iseen(2) = 0
      iseen(3) = 0
      ierr = -1
      errs = 0
      call mtest_init( ierr )

      call mpi_type_size( MPI_INTEGER, sizeofint, ierr )

      call mpi_comm_create_errhandler( comm_errh_fn, comm_errh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         call mtestprinterrormsg( "Comm_create_errhandler:", ierr )
         errs = errs + 1
      endif
      call mpi_win_create_errhandler( win_errh_fn, win_errh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         call mtestprinterrormsg( "Win_create_errhandler:", ierr )
         errs = errs + 1
      endif
      call mpi_file_create_errhandler( file_errh_fn, file_errh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         call mtestprinterrormsg( "File_create_errhandler:", ierr )
         errs = errs + 1
      endif
C
      call mpi_comm_dup( MPI_COMM_WORLD, wdup, ierr )
      call mpi_comm_set_errhandler( wdup, comm_errh, ierr )
      call mpi_comm_size( wdup, wdsize, ierr )
      call mpi_send( id, 1, MPI_INTEGER, wdsize, -37, wdup, ierr )
      if (ierr .eq. MPI_SUCCESS) then
         print *, ' Failed to detect error in use of MPI_SEND'
         errs = errs + 1
      else
         if (iseen(1) .ne. 1) then
            errs = errs + 1
            print *, ' Failed to increment comm error counter'
         endif
      endif

      asize = 2*sizeofint
      call mpi_win_create( winbuf, asize, sizeofint, MPI_INFO_NULL
     $     , wdup, winh, ierr ) 
      if (ierr .ne. MPI_SUCCESS) then
         call mtestprinterrormsg( "Win_create:", ierr )
         errs = errs + 1
      endif
      call mpi_win_set_errhandler( winh, win_errh, ierr )
      asize = 0
      call mpi_put( winbuf, 1, MPI_INT, wdsize, asize, 1, MPI_INT, winh,
     $     ierr )
      if (ierr .eq. MPI_SUCCESS) then
         print *, ' Failed to detect error in use of MPI_PUT'
         errs = errs + 1
      else
         if (iseen(3) .ne. 1) then
            errs = errs + 1
            print *, ' Failed to increment win error counter'
         endif
      endif

      call mpi_file_open( MPI_COMM_SELF, 'ftest', MPI_MODE_CREATE +
     $     MPI_MODE_RDWR + MPI_MODE_DELETE_ON_CLOSE, MPI_INFO_NULL, fh,
     $     ierr ) 
      if (ierr .ne. MPI_SUCCESS) then
         call mtestprinterrormsg( "File_open:", ierr )
         errs = errs + 1
      endif
      call mpi_file_set_errhandler( fh, file_errh, ierr )
      offset = -100
      call mpi_file_read_at( fh, offset, winbuf, 1, MPI_INTEGER, status,
     $     ierr )
      if (ierr .eq. MPI_SUCCESS) then
         print *, ' Failed to detect error in use of MPI_PUT'
         errs = errs + 1
      else
         if (iseen(2) .ne. 1) then
            errs = errs + 1
            print *, ' Failed to increment file error counter'
         endif
      endif

      call mpi_comm_free( wdup, ierr )
      call mpi_win_free( winh, ierr )
      call mpi_file_close( fh, ierr )
      
      call mpi_errhandler_free( win_errh, ierr )
      call mpi_errhandler_free( comm_errh, ierr )
      call mpi_errhandler_free( file_errh, ierr )
      
      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
C
      subroutine comm_errh_fn( comm, ec )
      integer comm, ec
      common /ec/ iseen
      integer iseen(3)
      save /ec/
C
      iseen(1) = iseen(1) + 1
C
      end
C
      subroutine win_errh_fn( win, ec )
      integer win, ec
      common /ec/ iseen
      integer iseen(3)
      save /ec/
C
      iseen(3) = iseen(3) + 1
C
      end
      subroutine file_errh_fn( fh, ec )
      integer fh, ec
      common /ec/ iseen
      integer iseen(3)
      save /ec/
C
      iseen(2) = iseen(2) + 1
C
      end
