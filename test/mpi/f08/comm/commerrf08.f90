! This file created from test/mpi/f77/comm/commerrf.f with f77tof90
! -*- Mode: Fortran; -*-
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
       program main
       use mpi_f08
       integer errs, ierr, code(2), newerrclass, eclass
       character*(MPI_MAX_ERROR_STRING) errstring
       integer rlen
       TYPE(MPI_Comm) comm
       external myerrhanfunc
       TYPE(MPI_Errhandler) myerrhan, qerr
       integer callcount, codesSeen(3)
       common /myerrhan/ callcount, codesSeen

       errs = 0
       callcount = 0
       call mtest_init( ierr )
!
! Setup some new codes and classes
       call mpi_add_error_class( newerrclass, ierr )
       call mpi_add_error_code( newerrclass, code(1), ierr )
       call mpi_add_error_code( newerrclass, code(2), ierr )
       call mpi_add_error_string( newerrclass, "New Class", ierr )
       call mpi_add_error_string( code(1), "First new code", ierr )
       call mpi_add_error_string( code(2), "Second new code", ierr )
!
!
       call mpi_comm_create_errhandler( myerrhanfunc, myerrhan, ierr )
!
! Create a new communicator so that we can leave the default errors-abort
! on MPI_COMM_WORLD
       call mpi_comm_dup( MPI_COMM_WORLD, comm, ierr )
!
       call mpi_comm_set_errhandler( comm, myerrhan, ierr )

       call mpi_comm_get_errhandler( comm, qerr, ierr )
       if (qerr .ne. myerrhan) then
          errs = errs + 1
          print *, ' Did not get expected error handler'
       endif
       call mpi_errhandler_free( qerr, ierr )
! We can free our error handler now
       call mpi_errhandler_free( myerrhan, ierr )

       call mpi_comm_call_errhandler( comm, newerrclass, ierr )
       call mpi_comm_call_errhandler( comm, code(1), ierr )
       call mpi_comm_call_errhandler( comm, code(2), ierr )

       if (callcount .ne. 3) then
          errs = errs + 1
          print *, ' Expected 3 calls to error handler, found ',  &
      &             callcount
       else
          if (codesSeen(1) .ne. newerrclass) then
             errs = errs + 1
             print *, 'Expected class ', newerrclass, ' got ',  &
      &                codesSeen(1)
          endif
          if (codesSeen(2) .ne. code(1)) then
             errs = errs + 1
             print *, 'Expected code ', code(1), ' got ',  &
      &                codesSeen(2)
          endif
          if (codesSeen(3) .ne. code(2)) then
             errs = errs + 1
             print *, 'Expected code ', code(2), ' got ',  &
      &                codesSeen(3)
          endif
       endif

       call mpi_comm_free( comm, ierr )
!
! Check error strings while here...
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
       call mpi_finalize( ierr )

       end
!
       subroutine myerrhanfunc( comm, errcode )
       use mpi_f08
       integer errcode
       TYPE(MPI_Comm) comm
       integer rlen, ierr
       integer callcount, codesSeen(3)
       character*(MPI_MAX_ERROR_STRING) errstring
       common /myerrhan/ callcount, codesSeen

       callcount = callcount + 1
! Remember the code we've seen
       if (callcount .le. 3) then
          codesSeen(callcount) = errcode
       endif
       call mpi_error_string( errcode, errstring, rlen, ierr )
       if (ierr .ne. MPI_SUCCESS) then
          print *, ' Panic! could not get error string'
          call mpi_abort( MPI_COMM_WORLD, 1, ierr )
       endif
       end
