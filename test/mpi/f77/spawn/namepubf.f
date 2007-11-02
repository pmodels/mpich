C -*- Mode: Fortran; -*- 
C
C (C) 2003 by Argonne National Laboratory.
C     See COPYRIGHT in top-level directory.
C
      program main
      implicit none
      include 'mpif.h'
      integer errs
      character*(MPI_MAX_PORT_NAME) port_name
      character*(MPI_MAX_PORT_NAME) port_name_out
      character*(256) serv_name
      integer merr, mclass
      character*(MPI_MAX_ERROR_STRING) errmsg
      integer msglen, rank
      integer ierr

      errs = 0
      call MTest_Init( ierr )
      call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr )

C       Note that according to the MPI standard, port_name must
C       have been created by MPI_Open_port.  For current testing
C       purposes, we'll use a fake name.  This test should eventually use
C       a valid name from Open_port 

      port_name = 'otherhost:122'
      serv_name = 'MyTest'
      
      call MPI_Comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN,
     $     ierr )

      if (rank .eq. 0) then
         merr = -1
         call MPI_Publish_name( serv_name, MPI_INFO_NULL, port_name,
     $        merr )
         if (merr .ne. MPI_SUCCESS) then
            errs = errs + 1
            call MPI_Error_string( merr, errmsg, msglen, ierr )
            print *, "Error in Publish_name ", errmsg(1:msglen)
         endif

         call MPI_Barrier(MPI_COMM_WORLD, ierr )
         call MPI_Barrier(MPI_COMM_WORLD, ierr )
        
         merr = -1
         call MPI_Unpublish_name( serv_name, MPI_INFO_NULL, port_name,
     $        merr)
         if (merr .ne. MPI_SUCCESS) then
            errs = errs + 1
            call MPI_Error_string( merr, errmsg, msglen, ierr )
            print *,  "Error in Unpublish name ", errmsg(1:msglen)
         endif
      else
         call MPI_Barrier(MPI_COMM_WORLD, ierr )
         merr = -1
         call MPI_Lookup_name( serv_name, MPI_INFO_NULL, port_name_out,
     $        merr)
         if (merr .ne. MPI_SUCCESS) then
            errs = errs + 1
            call MPI_Error_string( merr, errmsg, msglen, ierr )
            print *, "Error in Lookup name", errmsg(1:msglen)
         else 
            if (port_name .ne. port_name_out) then
                errs = errs + 1
                print *, "Lookup name returned the wrong value (",
     $               port_name_out, "), expected (", port_name, ")" 
             endif
          endif

        call MPI_Barrier(MPI_COMM_WORLD, ierr )
      endif

      call MPI_Barrier(MPI_COMM_WORLD, ierr )
    
      merr = -1
      call MPI_Lookup_name( serv_name, MPI_INFO_NULL, port_name_out,
     $     merr )
      if (merr .eq. MPI_SUCCESS) then
         errs = errs + 1
         print *, "Lookup name returned name after it was unpublished"
      else
C       Must be class MPI_ERR_NAME 
         call MPI_Error_class( merr, mclass, ierr )
        if (mclass .ne. MPI_ERR_NAME) then
            errs = errs + 1
            call MPI_Error_string( merr, errmsg, msglen, ierr )
            print *,    "Lookup name returned the wrong error class
     $           (",mclass,"), msg ", errmsg

         endif
      endif
      
      call MTest_Finalize( errs )
      call MPI_Finalize( ierr )
      end
