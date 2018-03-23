C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
       program main
       implicit none
       include 'mpif.h'
       integer errs, ierr, code(2), newerrclass, eclass
       character*(MPI_MAX_ERROR_STRING) errstring
       integer comm, rlen
       integer buf(10)
       integer file
       external myerrhanfunc
CF90   INTERFACE 
CF90   SUBROUTINE myerrhanfunc(vv0,vv1)
CF90   INTEGER vv0,vv1
CF90   END SUBROUTINE
CF90   END INTERFACE
       integer myerrhan, qerr
       integer callcount, codesSeen(3)
       common /myerrhan/ callcount, codesSeen

       errs = 0
       callcount = 0
       call mtest_init( ierr )
C
C Setup some new codes and classes
       call mpi_add_error_class( newerrclass, ierr )
       call mpi_add_error_code( newerrclass, code(1), ierr )
       call mpi_add_error_code( newerrclass, code(2), ierr )
       call mpi_add_error_string( newerrclass, "New Class", ierr )
       call mpi_add_error_string( code(1), "First new code", ierr )
       call mpi_add_error_string( code(2), "Second new code", ierr )
C
       call mpi_file_create_errhandler( myerrhanfunc, myerrhan, ierr )
C
C Create a new communicator so that we can leave the default errors-abort
C on MPI_COMM_WORLD.  Use this comm for file_open, just to leave a little
C more separation from comm_world
C
       call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
       call mpi_file_open( comm, "testfile.txt", MPI_MODE_RDWR +           &
     &        MPI_MODE_CREATE, MPI_INFO_NULL, file, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif
C
       call mpi_file_set_errhandler( file, myerrhan, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif

       call mpi_file_get_errhandler( file, qerr, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif
       if (qerr .ne. myerrhan) then
          errs = errs + 1
          print *, ' Did not get expected error handler'
       endif
       call mpi_errhandler_free( qerr, ierr )
C We can free our error handler now
       call mpi_errhandler_free( myerrhan, ierr )

       call mpi_file_call_errhandler( file, newerrclass, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif
       call mpi_file_call_errhandler( file, code(1), ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif
       call mpi_file_call_errhandler( file, code(2), ierr )
       if (ierr .ne. MPI_SUCCESS) then
          errs = errs + 1
          call MTestPrintError( ierr )
       endif
       
       if (callcount .ne. 3) then
          errs = errs + 1
          print *, ' Expected 3 calls to error handler, found ', 
     &             callcount
       else
          if (codesSeen(1) .ne. newerrclass) then
             errs = errs + 1
             print *, 'Expected class ', newerrclass, ' got ', 
     &                codesSeen(1)
          endif
          if (codesSeen(2) .ne. code(1)) then
             errs = errs + 1
             print *, 'Expected code ', code(1), ' got ', 
     &                codesSeen(2)
          endif
          if (codesSeen(3) .ne. code(2)) then
             errs = errs + 1
             print *, 'Expected code ', code(2), ' got ', 
     &                codesSeen(3)
          endif
       endif

       call mpi_file_close( file, ierr )
       call mpi_comm_free( comm, ierr )
       call mpi_file_delete( "testfile.txt", MPI_INFO_NULL, ierr )
C
C Check error strings while here here...
       call mpi_error_string( newerrclass, errstring, rlen, ierr )
       if (errstring(1:rlen) .ne. "New Class") then
          errs = errs + 1
          print *, ' Wrong string for error class: ', errstring(1:rlen)
       endif
       call mpi_error_class( code(1), eclass, ierr )
       if (eclass .ne. newerrclass) then
          errs = errs + 1
          print *, ' Class for new code is not correct'
       endif
       call mpi_error_string( code(1), errstring, rlen, ierr )
       if (errstring(1:rlen) .ne. "First new code") then
          errs = errs + 1
          print *, ' Wrong string for error code: ', errstring(1:rlen)
       endif
       call mpi_error_class( code(2), eclass, ierr )
       if (eclass .ne. newerrclass) then
          errs = errs + 1
          print *, ' Class for new code is not correct'
       endif
       call mpi_error_string( code(2), errstring, rlen, ierr )
       if (errstring(1:rlen) .ne. "Second new code") then
          errs = errs + 1
          print *, ' Wrong string for error code: ', errstring(1:rlen)
       endif

       call mtest_finalize( errs )

       end
C
       subroutine myerrhanfunc( file, errcode )
       implicit none
       include 'mpif.h'
       integer file, errcode
       integer rlen, ierr
       integer callcount, codesSeen(3)
       character*(MPI_MAX_ERROR_STRING) errstring
       common /myerrhan/ callcount, codesSeen

       callcount = callcount + 1
C Remember the code we've seen
       if (callcount .le. 3) then
          codesSeen(callcount) = errcode
       endif
       call mpi_error_string( errcode, errstring, rlen, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          print *, ' Panic! could not get error string'
          call mpi_abort( MPI_COMM_WORLD, 1, ierr )
       endif
       end
