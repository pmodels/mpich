C -*- Mode: Fortran; -*-
C
C  (C) 2004 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      include 'iodisp.h'
C tests whether atomicity semantics are satisfied for overlapping accesses
C in atomic mode. The probability of detecting errors is higher if you run 
C it on 8 or more processes. 
C This is a version of the test in romio/test/atomicity.c .
      integer BUFSIZE
      parameter (BUFSIZE=10000)
      integer writebuf(BUFSIZE), readbuf(BUFSIZE)
      integer i, mynod, nprocs, len, ierr, errs, toterrs
      character*50 filename
      integer newtype, fh, info, status(MPI_STATUS_SIZE)

      errs = 0

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, mynod, ierr )
      call MPI_Comm_size(MPI_COMM_WORLD, nprocs, ierr )

C Unlike the C version, we fix the filename because of the difficulties
C in accessing the command line from different Fortran environments      
      filename = "testfile.txt"
C test atomicity of contiguous accesses 

C initialize file to all zeros 
      if (mynod .eq. 0) then
         call MPI_File_delete(filename, MPI_INFO_NULL, ierr )
         call MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE + 
     $        MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr )
         do i=1, BUFSIZE
            writebuf(i) = 0
         enddo
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status,
     $        ierr) 
         call MPI_File_close(fh, ierr )
      endif
      call MPI_Barrier(MPI_COMM_WORLD, ierr )

      do i=1, BUFSIZE
         writebuf(i) = 10
         readbuf(i)  = 20
      enddo

      call MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE + 
     $     MPI_MODE_RDWR, MPI_INFO_NULL, fh, ierr )

C set atomicity to true 
      call MPI_File_set_atomicity(fh, .true., ierr)
      if (ierr .ne. MPI_SUCCESS) then
         print *, "Atomic mode not supported on this file system."
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
      endif

      call MPI_Barrier(MPI_COMM_WORLD, ierr )
    
C process 0 writes and others concurrently read. In atomic mode, 
C the data read must be either all old values or all new values; nothing
C in between. 

      if (mynod .eq. 0) then
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status,
     $        ierr) 
      else 
         call MPI_File_read(fh, readbuf, BUFSIZE, MPI_INTEGER, status,
     $        ierr ) 
         if (ierr .eq. MPI_SUCCESS) then
            if (readbuf(1) .eq. 0) then
C              the rest must also be 0 
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 0) then
                     errs = errs + 1
                     print *, "(contig)Process ", mynod, ": readbuf(", i
     $                    ,") is ", readbuf(i), ", should be 0"
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else if (readbuf(1) .eq. 10) then
C              the rest must also be 10
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 10) then
                     errs = errs + 1
                     print *, "(contig)Process ", mynod, ": readbuf(", i
     $                    ,") is ", readbuf(i), ", should be 10"  
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else 
               errs = errs + 1
               print *, "(contig)Process ", mynod, ": readbuf(1) is ", 
     $              readbuf(1), ", should be either 0 or 10"
            endif
         endif
      endif

      call MPI_File_close( fh, ierr )
        
      call MPI_Barrier( MPI_COMM_WORLD, ierr )


C repeat the same test with a noncontiguous filetype 

      call MPI_Type_vector(BUFSIZE, 1, 2, MPI_INTEGER, newtype, ierr)
      call MPI_Type_commit(newtype, ierr )

      call MPI_Info_create(info, ierr )
C I am setting these info values for testing purposes only. It is
C better to use the default values in practice. */
      call MPI_Info_set(info, "ind_rd_buffer_size", "1209", ierr )
      call MPI_Info_set(info, "ind_wr_buffer_size", "1107", ierr )
    
      if (mynod .eq. 0) then
         call MPI_File_delete(filename, MPI_INFO_NULL, ierr )
         call MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE +
     $        MPI_MODE_RDWR, info, fh, ierr ) 
        do i=1, BUFSIZE
           writebuf(i) = 0
        enddo
        disp = 0
        call MPI_File_set_view(fh, disp, MPI_INTEGER, newtype, "native"
     $       ,info, ierr) 
        call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status,
     $       ierr ) 
        call MPI_File_close( fh, ierr )
      endif
      call MPI_Barrier( MPI_COMM_WORLD, ierr )

      do i=1, BUFSIZE
         writebuf(i) = 10
         readbuf(i)  = 20 
      enddo

      call MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE +
     $     MPI_MODE_RDWR, info, fh, ierr ) 
      call MPI_File_set_atomicity(fh, .true., ierr)
      disp = 0
      call MPI_File_set_view(fh, disp, MPI_INTEGER, newtype, "native",
     $     info, ierr ) 
      call MPI_Barrier(MPI_COMM_WORLD, ierr )
    
      if (mynod .eq. 0) then 
         call MPI_File_write(fh, writebuf, BUFSIZE, MPI_INTEGER, status,
     $        ierr ) 
      else 
         call MPI_File_read(fh, readbuf, BUFSIZE, MPI_INTEGER, status,
     $        ierr ) 
         if (ierr .eq. MPI_SUCCESS) then
            if (readbuf(1) .eq. 0) then
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 0) then
                     errs = errs + 1
                     print *, "(noncontig)Process ", mynod, ": readbuf("
     $                    , i,") is ", readbuf(i), ", should be 0" 
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else if (readbuf(1) .eq. 10) then
               do i=2, BUFSIZE
                  if (readbuf(i) .ne. 10) then
                     errs = errs + 1
                     print *, "(noncontig)Process ", mynod, ": readbuf("
     $                    , i,") is ", readbuf(i), ", should be 10"
                     call MPI_Abort(MPI_COMM_WORLD, 1, ierr )
                  endif
               enddo
            else 
               errs = errs + 1
               print *, "(noncontig)Process ", mynod, ": readbuf(1) is "
     $              ,readbuf(1), ", should be either 0 or 10" 
            endif
         endif
      endif

      call MPI_File_close( fh, ierr )
        
      call MPI_Barrier(MPI_COMM_WORLD, ierr )

      call MPI_Allreduce( errs, toterrs, 1, MPI_INTEGER, MPI_SUM,
     $     MPI_COMM_WORLD, ierr )
      if (mynod .eq. 0) then
         if( toterrs .gt. 0) then
            print *, "Found ", toterrs, " errors"
         else 
            print *, " No Errors"
         endif
      endif

      call MPI_Type_free(newtype, ierr )
      call MPI_Info_free(info, ierr )
      
      call MPI_Finalize(ierr)
      end
