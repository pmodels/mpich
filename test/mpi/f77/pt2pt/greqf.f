C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      subroutine query_fn( extrastate, status, ierr )
      implicit none
      include 'mpif.h'
      integer status(MPI_STATUS_SIZE), ierr
      include 'attr1aints.h'
C
C    set a default status
      status(MPI_SOURCE) = MPI_UNDEFINED
      status(MPI_TAG)    = MPI_UNDEFINED
      call mpi_status_set_cancelled( status, 0, ierr)
      call mpi_status_set_elements( status, MPI_BYTE, 0, ierr )
      ierr = MPI_SUCCESS
      end
C
      subroutine free_fn( extrastate, ierr )
      implicit none
      include 'mpif.h'
      integer value, ierr
      include 'attr1aints.h'
      integer freefncall
      common /fnccalls/ freefncall
C
C   For testing purposes, the following print can be used to check whether
C   the free_fn is called
C      print *, 'Free_fn called'
C
      extrastate = extrastate - 1
C   The value returned by the free function is the error code
C   returned by the wait/test function 
      ierr = MPI_SUCCESS
      end
C
      subroutine cancel_fn( extrastate, complete, ierr )
      implicit none
      include 'mpif.h'
      integer ierr
      logical complete
      include 'attr1aints.h'

      ierr = MPI_SUCCESS
      end
C
C
C This is a very simple test of generalized requests.  Normally, the
C MPI_Grequest_complete function would be called from another routine,
C often running in a separate thread.  This simple code allows us to
C check that requests can be created, tested, and waited on in the
C case where the request is complete before the wait is called.  
C
C Note that MPI did *not* define a routine that can be called within
C test or wait to advance the state of a generalized request.  
C Most uses of generalized requests will need to use a separate thread.
C
       program main
       implicit none
       include 'mpif.h'
       integer errs, ierr
       logical flag
       integer status(MPI_STATUS_SIZE)
       integer request
       external query_fn, free_fn, cancel_fn
       include 'attr1aints.h'
       integer freefncall
       common /fnccalls/ freefncall

       errs = 0
       freefncall = 0
       
       call MTest_Init( ierr )

       extrastate = 0
       call mpi_grequest_start( query_fn, free_fn, cancel_fn, 
     &            extrastate, request, ierr )
       call mpi_test( request, flag, status, ierr )
       if (flag) then
          errs = errs + 1
          print *, 'Generalized request marked as complete'
       endif
       
       call mpi_grequest_complete( request, ierr )

       call MPI_Wait( request, status, ierr )

       extrastate = 1
       call mpi_grequest_start( query_fn, free_fn, cancel_fn, 
     &                          extrastate, request, ierr )
       call mpi_grequest_complete( request, ierr )
       call mpi_wait( request, MPI_STATUS_IGNORE, ierr )
C       
C      The following routine may prevent an optimizing compiler from 
C      just remembering that extrastate was set in grequest_start
       call dummyupdate(extrastate)
       if (extrastate .ne. 0) then
          errs = errs + 1
          if (freefncall .eq. 0) then
              print *, 'Free routine not called'
          else 
              print *, 'Free routine did not update extra_data'
              print *, 'extrastate = ', extrastate
          endif
       endif
C
       call MTest_Finalize( errs )
       call mpi_finalize( ierr )
       end
C
