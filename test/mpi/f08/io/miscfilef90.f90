! -*- Mode: Fortran; -*-
!
!  (C) 2014 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
! iooffset.h provides a variable "offset" that is of type MPI_Offset
! (in Fortran 90, kind=MPI_OFFSET_KIND)
      integer (kind=MPI_OFFSET_KIND) offset

! iodisp.h declares disp as an MPI_Offset integer
      integer (kind=MPI_OFFSET_KIND) disp

      integer rank, size
      type(MPI_File) fh
      type(MPI_Group) group, worldgroup
      integer i, result

      integer ierr, errs
      integer BUFSIZE
      parameter (BUFSIZE=1024)
      integer buf(BUFSIZE)
      character*(50) filename
      character*(MPI_MAX_DATAREP_STRING) datarep
      integer amode
      logical atomicity
      type(MPI_Datatype) newtype, etype, filetype
      integer integer_size, type_size
!
      errs = 0
      call mtest_init( ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
!
! Create a file that we'll then query properties
      filename = "testfile.txt"
      call mpi_file_open( MPI_COMM_WORLD, filename, MPI_MODE_CREATE +       &
      &     MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr )
      if (ierr .ne. MPI_SUCCESS) then
         print *, "Unable to create file ", filename
         call mpi_abort( MPI_COMM_WORLD, 1, ierr )
      endif
!
! Fill in some data
      do i=1, BUFSIZE
         buf(i) = i
      enddo
      call mpi_file_write( fh, buf, BUFSIZE, MPI_INTEGER,                &
      &     MPI_STATUS_IGNORE, ierr )
      call MPI_File_sync( fh, ierr )
!
! Now, query properties of the file and the file handle
      call MPI_File_get_amode(fh, amode, ierr )
      if (amode .ne. MPI_MODE_CREATE + MPI_MODE_RDWR) then
         errs = errs + 1
         print *, " Amode was different than expected"
      endif
!
      call MPI_File_get_atomicity( fh, atomicity, ierr )
      if (atomicity) then
         errs = errs + 1
         print *, " Atomicity was true but should be false"
      endif
!
      call MPI_File_set_atomicity( fh, .true., ierr )
      call MPI_File_get_atomicity( fh, atomicity, ierr )
      if (.not. atomicity) then
         errs = errs + 1
         print *, " Atomicity was set to true but ",                    &
      &        "get_atomicity returned false"
      endif
      call MPI_File_set_atomicity( fh, .false., ierr )
!
! FIXME: original code use 10,10,20, and the following code
! assumed the original
!
! Create a vector type of 10 elements, each of 20 elements, with a stride of
! 30 elements
      call mpi_type_vector( 10, 20, 30, MPI_INTEGER, newtype, ierr )
      call mpi_type_commit( newtype, ierr )
!
! All processes are getting the same view, with a 1000 byte offset
      offset = 1000
      call mpi_file_set_view( fh, offset, MPI_INTEGER, newtype, "native"  &
      &     , MPI_INFO_NULL, ierr )

      call mpi_file_get_view( fh, offset, etype, filetype, datarep, ierr  &
      &     )
      if (offset .ne. 1000) then
         print *, " displacement was ", offset, ", expected 1000"
         errs = errs + 1
      endif
      if (datarep .ne. "native") then
         print *, " data representation form was ", datarep,              &
      &        ", expected native"
         errs = errs + 1
      endif

! Find the byte offset, given an offset of 20 etypes relative to the
! current view (the same as the blockcount of the filetype, which
! places it at the beginning of the next block, hence a stride
! distance away).
      offset = 20
      call mpi_file_get_byte_offset( fh, offset, disp, ierr )
      call mpi_type_size( MPI_INTEGER, integer_size, ierr )
      if (disp .ne. 1000 + 30 * integer_size) then
         errs = errs + 1
         print *, " (offset20)Byte offset = ", disp, ", should be ",         &
      &            1000+20*integer_size
      endif
!
!     We should also compare file and etypes.  We just look at the
!     sizes and extents for now

      call mpi_type_size( etype, type_size, ierr )
      if (type_size .ne. integer_size) then
         print *, " Etype has size ", type_size, ", but should be ",      &
      &        integer_size
         errs = errs + 1
      endif
      call mpi_type_size( filetype, type_size, ierr )
      if (type_size .ne. 10*20*integer_size) then
         print *, " filetype has size ", type_size, ", but should be ",   &
      &        10*20*integer_size
         errs = errs + 1
      endif
!
! Only free derived type
      call mpi_type_free( filetype, ierr )

      call mpi_file_get_group( fh, group, ierr )
      call mpi_comm_group( MPI_COMM_WORLD, worldgroup, ierr )
      call mpi_group_compare( group, worldgroup, result, ierr )

      if (result .ne. MPI_IDENT) then
         print *, " Group of file does not match group of comm_world"
         errs = errs + 1
      endif
      call mpi_group_free( group, ierr )
      call mpi_group_free( worldgroup, ierr )

      offset = 1000+25*integer_size
      call mpi_file_set_size(fh, offset, ierr )
      call mpi_barrier(MPI_COMM_WORLD, ierr )
      call mpi_file_sync(fh, ierr )
      call mpi_file_get_size( fh, offset, ierr )

      if (offset .ne. 1000+25*integer_size) then
         errs = errs + 1
         print *, " File size is ", offset, ", should be ", 1000 + 25     &
      &        * integer_size
      endif
!
! File size is 1000+25ints.  Seek to end.  Note that the file size
! places the end of the file into the gap in the view, so seeking
! to the end, which is relative to the view, needs to give the end
! of the first block of 20 ints)
      offset = 0
      call mpi_file_seek( fh, offset, MPI_SEEK_END, ierr )
      call mpi_file_get_position( fh, disp, ierr )
      if (disp .ne. 20) then
         errs = errs + 1
         print *, "File pointer position = ", disp, ", should be 20"
         if (disp .eq. 25) then
! See MPI 2.1, section 13.4, page 399, lines 7-8. The disp must be
! relative to the current view, in the etype units of the current view
             print *, " MPI implementation failed to position the "//      &
      &                "displacement within the current file view"
         endif
! Make sure we use the expected position in the next step.
         disp = 20
      endif
      call mpi_file_get_byte_offset(fh, disp, offset, ierr )
      if (offset .ne. 1000+30*integer_size) then
         errs = errs + 1
         print *, " (seek)Byte offset = ", offset, ", should be ", 1000     &
      &        +30*integer_size
      endif

      call mpi_barrier(MPI_COMM_WORLD, ierr )

      offset = -20
      call mpi_file_seek(fh, offset, MPI_SEEK_CUR, ierr )
      call mpi_file_get_position(fh, disp, ierr )
      call mpi_file_get_byte_offset(fh, disp, offset, ierr )
      if (offset .ne. 1000) then
         errs = errs + 1
         print *, " File pointer position in bytes = ", offset,           &
      &        ", should be 1000"
      endif

      offset = 8192
      call mpi_file_preallocate(fh, offset, ierr )
      offset = 0
      call mpi_file_get_size( fh, offset, ierr )
      if (offset .lt. 8192) then
         errs = errs + 1
         print *, " Size after preallocate is ", offset,                  &
      &        ", should be at least 8192"
      endif
      call mpi_file_close( fh, ierr )

      call mpi_barrier(MPI_COMM_WORLD, ierr )

      if (rank .eq. 0) then
         call MPI_File_delete(filename, MPI_INFO_NULL, ierr )
      endif

      call mpi_type_free( newtype, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )
      end
