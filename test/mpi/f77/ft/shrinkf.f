C -*- Mode: Fortran; -*- 

C
C This test ensures that shrink works correctly
C
      program main
      include "mpif.h"
      integer rank, size, newsize, rc, errclass, errs, ierr
      integer newcomm
      
      errs = 0

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
      call MPI_Comm_size(MPI_COMM_WORLD, size, ierr)
      call MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     &                             ierr)

      if (size .lt. 4) then
         write(*,*) "Must run with at least 4 processes"
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      if (2 .eq. rank) error stop "FORTRAN STOP"

      call MPIX_Comm_shrink(MPI_COMM_WORLD, newcomm, rc)
      if (rc .ne. 0) then
         call MPI_Error_class(rc, errclass, ierr)
         write(*,*) "Expected MPI_SUCCESS from MPIX_Comm_shrink. "
     &           // "Received:", errclass
         errs = errs + 1
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_size(newcomm, newsize, ierr)
      if (newsize .ne. size - 1) errs = errs + 1

      call MPI_Barrier(newcomm, rc)
      if (rc .ne. 0) then
         call MPI_Error_class(rc, errclass, ierr)
         write(*,*) "Expected MPI_SUCCESS from MPI_BARRIER. "
     &           // "Received:", errclass
         errs = errs + 1
         call MPI_Abort(MPI_COMM_WORLD, 1, ierr)
      end if

      call MPI_Comm_free(newcomm, ierr)

      if (0 .eq. rank) write(*,*) " No Errors"

      call MPI_Finalize(ierr)

      end program
