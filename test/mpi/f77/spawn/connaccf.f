C -*- Mode: Fortran; -*- 
C
C  (C) 2003 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer size, rank, ierr, errs, eclass
      integer color, comm, intercomm
      integer s1, s2
      character*(MPI_MAX_PORT_NAME) portname

      errs = 0
      call mtest_init( ierr )

      call mpi_comm_size( MPI_COMM_WORLD, size, ierr )
      call mpi_comm_rank( MPI_COMM_WORLD, rank, ierr )
      if (size .lt. 2) then
         print *, 'This example must have at least 2 processes'
         call mpi_abort( MPI_COMM_WORLD, 1, ierr )
      endif
C
C The server (accept) side is rank < size/2 and the client (connect)
C side is rank >= size/2
      color = 0
      if (rank .ge. size/2) color = 1
      call mpi_comm_split( MPI_COMM_WORLD, color, rank, comm, ierr )
C
      if (rank .lt. size/2) then
C        Server
         if (rank .eq. 0) then
             call mpi_open_port( MPI_INFO_NULL, portname, ierr )
             call mpi_publish_name( "fservtest", MPI_INFO_NULL, 
     &            portname, ierr )
         endif
         call mpi_barrier( MPI_COMM_WORLD, ierr )
         call mpi_comm_accept( portname, MPI_INFO_NULL, 0, comm, 
     &                         intercomm, ierr )

         call mpi_close_port( portname, ierr )
      else
C        Client
         call mpi_comm_set_errhandler( MPI_COMM_WORLD,MPI_ERRORS_RETURN, 
     &                                 ierr )
         ierr = MPI_SUCCESS
         call mpi_lookup_name( "fservtest", MPI_INFO_NULL, 
     &                         portname, ierr )
         ierr = MPI_ERR_NAME
         if (ierr .eq. MPI_SUCCESS) then
            errs = errs + 1
            print *, 'lookup name returned a value before published'
         else
            call mpi_error_class( ierr, eclass, ierr )
            if (eclass .ne. MPI_ERR_NAME) then
               errs = errs + 1
               print *, ' Wrong error class, is ', eclass, ' must be ',
     &          MPI_ERR_NAME
C              See the MPI-2 Standard, 5.4.4
            endif
         endif
         call mpi_comm_set_errhandler( MPI_COMM_WORLD, 
     &            MPI_ERRORS_ARE_FATAL, ierr )
         call mpi_barrier( MPI_COMM_WORLD, ierr )
         call mpi_lookup_name( "fservtest", MPI_INFO_NULL, 
     &                         portname, ierr )
         call mpi_comm_connect( portname, MPI_INFO_NULL, 0, comm, 
     &                          intercomm, ierr )
      endif
C
C Check that this is an acceptable intercomm
      call mpi_comm_size( intercomm, s1, ierr )
      call mpi_comm_remote_size( intercomm, s2, ierr )
      if (s1 + s2 .ne. size) then
         errs = errs + 1
         print *, ' Wrong size for intercomm = ', s1+s2
      endif

      call mpi_comm_free(comm, ierr)
C Everyone can now abandon the new intercomm      
      call mpi_comm_disconnect( intercomm, ierr )

      call mtest_finalize( errs )
      call mpi_finalize( ierr )

      end
