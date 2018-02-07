C -*- Mode: Fortran; -*- 
      program main
      include 'mpif.h'
      integer    rank, ierr

      call MPI_Init(ierr)
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)

      if (rank .eq. 0) call MPI_Abort(MPI_COMM_WORLD, 1, ierr)

      do while (.true.)
        continue
      end do

      call MPI_Finalize(ierr)

      end program
