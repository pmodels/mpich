C -*- Mode: Fortran; -*- 
C
C  (C) 2004 by Argonne National Laboratory.
C      See COPYRIGHT in top-level directory.
C
      program main
      include 'mpif.h'
      integer atype, ierr
C
      call mtest_init(ierr)
      call mpi_comm_set_errhandler( MPI_COMM_WORLD, MPI_ERRORS_RETURN, 
     *                              ierr )
C
C     Check that all Ctypes are available in Fortran (MPI 2.1, p 483, line 46)
C
       call checkdtype( MPI_CHAR, "MPI_CHAR", ierr )
       call checkdtype( MPI_SIGNED_CHAR, "MPI_SIGNED_CHAR", ierr )
       call checkdtype( MPI_UNSIGNED_CHAR, "MPI_UNSIGNED_CHAR", ierr )
       call checkdtype( MPI_BYTE, "MPI_BYTE", ierr )
       call checkdtype( MPI_WCHAR, "MPI_WCHAR", ierr )
       call checkdtype( MPI_SHORT, "MPI_SHORT", ierr )
       call checkdtype( MPI_UNSIGNED_SHORT, "MPI_UNSIGNED_SHORT", ierr )
       call checkdtype( MPI_INT, "MPI_INT", ierr )
       call checkdtype( MPI_UNSIGNED, "MPI_UNSIGNED", ierr )
       call checkdtype( MPI_LONG, "MPI_LONG", ierr )
       call checkdtype( MPI_UNSIGNED_LONG, "MPI_UNSIGNED_LONG", ierr )
       call checkdtype( MPI_FLOAT, "MPI_FLOAT", ierr )
       call checkdtype( MPI_DOUBLE, "MPI_DOUBLE", ierr )
       if (MPI_LONG_DOUBLE .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_LONG_DOUBLE, "MPI_LONG_DOUBLE", ierr )
       endif
       if (MPI_LONG_LONG_INT .ne. MPI_DATATYPE_NULL) then
         call checkdtype2( MPI_LONG_LONG_INT, "MPI_LONG_LONG_INT", 
     *                     "MPI_LONG_LONG", ierr )
       endif
       if (MPI_UNSIGNED_LONG_LONG .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_UNSIGNED_LONG_LONG, 
     *                    "MPI_UNSIGNED_LONG_LONG", ierr )
       endif
       if (MPI_LONG_LONG .ne. MPI_DATATYPE_NULL) then
         call checkdtype2( MPI_LONG_LONG, "MPI_LONG_LONG", 
     *                     "MPI_LONG_LONG_INT", ierr )
       endif
       call checkdtype( MPI_PACKED, "MPI_PACKED", ierr )
       call checkdtype( MPI_LB, "MPI_LB", ierr )
       call checkdtype( MPI_UB, "MPI_UB", ierr )
       call checkdtype( MPI_FLOAT_INT, "MPI_FLOAT_INT", ierr )
       call checkdtype( MPI_DOUBLE_INT, "MPI_DOUBLE_INT", ierr )
       call checkdtype( MPI_LONG_INT, "MPI_LONG_INT", ierr )
       call checkdtype( MPI_SHORT_INT, "MPI_SHORT_INT", ierr )
       call checkdtype( MPI_2INT, "MPI_2INT", ierr )
       if (MPI_LONG_DOUBLE_INT .ne. MPI_DATATYPE_NULL) then
         call checkdtype( MPI_LONG_DOUBLE_INT, "MPI_LONG_DOUBLE_INT",
     *                    ierr)
       endif
C
C     Check that all Ctypes are available in Fortran (MPI 2.2)
C     Note that because of implicit declarations in Fortran, this
C     code should compile even with pre MPI 2.2 implementations.
C
       if (MPI_VERSION .gt. 2 .or. (MPI_VERSION .eq. 2 .and. 
     *      MPI_SUBVERSION .ge. 2)) then
          call checkdtype( MPI_INT8_T, "MPI_INT8_T", ierr )
          call checkdtype( MPI_INT16_T, "MPI_INT16_T", ierr )
          call checkdtype( MPI_INT32_T, "MPI_INT32_T", ierr )
          call checkdtype( MPI_INT64_T, "MPI_INT64_T", ierr )
          call checkdtype( MPI_UINT8_T, "MPI_UINT8_T", ierr )
          call checkdtype( MPI_UINT16_T, "MPI_UINT16_T", ierr )
          call checkdtype( MPI_UINT32_T, "MPI_UINT32_T", ierr )
          call checkdtype( MPI_UINT64_T, "MPI_UINT64_T", ierr )
C other C99 types
          call checkdtype( MPI_C_BOOL, "MPI_C_BOOL", ierr )
          call checkdtype( MPI_C_FLOAT_COMPLEX, "MPI_C_FLOAT_COMPLEX",
     *                     ierr)
          call checkdtype2( MPI_C_COMPLEX, "MPI_C_COMPLEX", 
     *                      "MPI_C_FLOAT_COMPLEX", ierr )
          call checkdtype( MPI_C_DOUBLE_COMPLEX, "MPI_C_DOUBLE_COMPLEX", 
     *                     ierr )
          if (MPI_C_LONG_DOUBLE_COMPLEX .ne. MPI_DATATYPE_NULL) then
            call checkdtype( MPI_C_LONG_DOUBLE_COMPLEX, 
     *                       "MPI_C_LONG_DOUBLE_COMPLEX", ierr )
          endif
C address/offset types 
          call checkdtype( MPI_AINT, "MPI_AINT", ierr )
          call checkdtype( MPI_OFFSET, "MPI_OFFSET", ierr )
       endif
C
       call mtest_finalize( ierr )
       call MPI_Finalize( ierr )
       end
C
C Check name of datatype
      subroutine CheckDtype( intype, name, ierr )
      include 'mpif.h'
      integer intype, ierr
      character *(*) name
      integer ir, rlen
      character *(MPI_MAX_OBJECT_NAME) outname
C     
      outname = ""
      call MPI_TYPE_GET_NAME( intype, outname, rlen, ir )
      if (ir .ne. MPI_SUCCESS) then
         print *, " Datatype ", name, " not available in Fortran"
         ierr = ierr + 1
      else
         if (outname .ne. name) then
            print *, " For datatype ", name, " found name ",
     *           outname(1:rlen)
            ierr = ierr + 1
         endif
      endif
      
      return
      end
C
C Check name of datatype (allows alias)
      subroutine CheckDtype2( intype, name, name2, ierr )
      include 'mpif.h'
      integer intype, ierr
      character *(*) name, name2
      integer ir, rlen
      character *(MPI_MAX_OBJECT_NAME) outname
C     
      outname = ""
      call MPI_TYPE_GET_NAME( intype, outname, rlen, ir )
      if (ir .ne. MPI_SUCCESS) then
         print *, " Datatype ", name, " not available in Fortran"
         ierr = ierr + 1
      else
         if (outname .ne. name .and. outname .ne. name2) then
            print *, " For datatype ", name, " found name ",
     *           outname(1:rlen)
            ierr = ierr + 1
         endif
      endif
      
      return
      end
