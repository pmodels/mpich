! -*- Mode: Fortran; -*- 
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi
      integer (kind=MPI_ADDRESS_KIND) val
      integer ierr, errs

      call mpi_init(ierr)
      call mpi_comm_create_keyval( MPI_NULL_COPY_FN, MPI_NULL_DELETE_FN,  &
     &                             keyval, 0, ierr )
      val = 5555
      call mpi_comm_set_attr( MPI_COMM_WORLD, keyval, val, ierr )

      call chkinc( keyval, 5555, errs )

      call mpi_comm_free_keyval( keyval, ierr )

      if (ierr .eq. 0) then
         print *, ' No Errors'
      else
         print *, ' Found ', errs, ' errors'
      endif

      call mpi_finalize(ierr)
      end
