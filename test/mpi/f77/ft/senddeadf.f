C -*- Mode: Fortran; -*- 

C
C This test attempts to MPI_Send with the destination being a dead process.
C The communication should succeed or report an error. It must not block
C indefinitely.
C
      program main
      include "mpif.h"

      integer rank, size, err, errclass, ierr
      character*100000 buf

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                                ierr)
         call MPI_Send(buf,100000,MPI_CHARACTER,1,0,MPI_COMM_WORLD,err)
         call MPI_Error_class(err, errclass, ierr)
         if ((err.ne.0) .and. (errclass .ne. MPIX_ERR_PROC_FAILED)) then
            write(*,*) "Wrong error code (", errclass,
     &                 ") returned. Expected MPIX_ERR_PROC_FAILED"
         end if
         call MPI_Send(buf,100000,MPI_CHARACTER,1,0,MPI_COMM_WORLD,err)
         call MPI_Error_class(err, errclass, ierr)
         if ((err.ne.0) .and. (errclass .ne. MPIX_ERR_PROC_FAILED)) then
            write(*,*) "Wrong error code (", errclass,
     &                 ") returned. Expected MPIX_ERR_PROC_FAILED"
         else
            write(*,*) " No Errors"
         end if
      end if

      call MPI_Finalize(ierr)

      end program
