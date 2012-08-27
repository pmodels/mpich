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
C Part of this test is to ensure that lookups cleanly fail when
C a name is not present.  This code is used to ensure that the
C name is not in use  before the test.
C The MPI Standard (10.4.4 Name Publishing) requires that a process that
C has published a name unpublish it before it exits.  
C This code attempts to lookup the name that we want to use as the
C service name for this example.  If it is found (it should not be, but
C might if an MPI program with this service name exits without unpublishing
C the servicename, and the runtime that provides the name publishing
C leaves the servicename in use.  This block of code should not be necessary
C in a robust MPI implementation, but should not cause problems for a correct.
C
      call mpi_barrier( MPI_COMM_WORLD, ierr )
      call mpi_comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN, 
     &     ierr )
      call mpi_lookup_name( "fservtest", MPI_INFO_NULL, portname, ierr )
      if (ierr .eq. MPI_SUCCESS) then
          call mpi_unpublish_name( "fservtest", MPI_INFO_NULL, portname, 
     &        ierr )
      endif
      call mpi_barrier( MPI_COMM_WORLD, ierr )
C Ignore errors from unpublish_name (such as name-not-found)      
      call mpi_comm_set_errhandler( MPI_COMM_WORLD, 
     &     MPI_ERRORS_ARE_FATAL, ierr )
C
C The server (accept) side is rank < size/2 and the client (connect)
C side is rank >= size/2
      color = 0
      if (rank .ge. size/2) color = 1
      call mpi_comm_split( MPI_COMM_WORLD, color, rank, comm, ierr )
C
      if (rank .lt. size/2) then
C        Server
         call mpi_barrier( MPI_COMM_WORLD, ierr )
         if (rank .eq. 0) then
             call mpi_open_port( MPI_INFO_NULL, portname, ierr )
             call mpi_publish_name( "fservtest", MPI_INFO_NULL, 
     &            portname, ierr )
         endif
         call mpi_barrier( MPI_COMM_WORLD, ierr )
         call mpi_comm_accept( portname, MPI_INFO_NULL, 0, comm, 
     &                         intercomm, ierr )

         if (rank .eq. 0) then 
            call mpi_close_port( portname, ierr )
            call mpi_unpublish_name( "fservtest", MPI_INFO_NULL,
     &            portname, ierr )
         endif

      else
C        Client
         call mpi_comm_set_errhandler( MPI_COMM_WORLD,MPI_ERRORS_RETURN, 
     &                                 ierr )
         ierr = MPI_SUCCESS
         call mpi_lookup_name( "fservtest", MPI_INFO_NULL, 
     &                         portname, ierr )
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
         call mpi_barrier( MPI_COMM_WORLD, ierr )
         call mpi_lookup_name( "fservtest", MPI_INFO_NULL, 
     &                         portname, ierr )
C        This should not happen (ERRORS_ARE_FATAL), but just in case...
         if (ierr .ne. MPI_SUCCESS) then
            errs = errs + 1
            print *, ' Major error: errors_are_fatal set but returned'
            print *, ' non MPI_SUCCESS value.  Details:'
            call MTestPrintErrorMsg( ' Unable to lookup fservtest port', 
     &                               ierr )
C           Unable to continue without a valid port
            call mpi_abort( MPI_COMM_WORLD, 1, ierr )
         endif
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
