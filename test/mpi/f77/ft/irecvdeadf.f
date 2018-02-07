C -*- Mode: Fortran; -*- 

C
C  This test attempts MPI_Irecv with the source being a dead process.
C  It should fail and return an error at completion. If we are testing
C  sufficiently new MPICH, we look for the MPIX_ERR_PROC_FAILED error
C  code. These should be converted to look for the standarized error
C  code once it is finalized.
C  
      program main
      include "mpif.h"
      integer rank, size, err, errclass, ierr
      integer request
      character*10 buf

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      if (size .lt. 2) then
         write(*,*) "Must run with at least 2 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) then
         call MPI_Comm_set_errhandler(MPI_COMM_WORLD,MPI_ERRORS_RETURN,
     &                                ierr)
         call MPI_Irecv(buf, 1, MPI_CHARACTER, 1, 0, MPI_COMM_WORLD,
     &                  request, err)
         if (err .ne. 0) write(*,*) "MPI_Irecv returned an error"

         call MPI_Wait(request, MPI_STATUS_IGNORE, err)
         call MPI_Error_class(err, errclass, ierr)
         if (errclass .eq. MPIX_ERR_PROC_FAILED) then
            write(*,*) " No Errors"
         else
            write(*,*) "Wrong error code (", errclass, ") returned. "
     &              // "Expected MPIX_ERR_PROC_FAILED", errclass
         end if
      end if

      call MPI_Finalize(ierr)

      end program
