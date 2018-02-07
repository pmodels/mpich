C -*- Mode: Fortran; -*- 

      program main
      include "mpif.h"

      integer rank, size, ierr

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)

      if (rank .eq. 1) error stop "FORTRAN STOP"

      if (rank .eq. 0) write(*,*) " No Errors"

      call MPI_Finalize(ierr)

      end program
