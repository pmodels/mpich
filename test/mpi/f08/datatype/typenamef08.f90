! This file created from test/mpi/f77/datatype/typenamef.f with f77tof90
! -*- Mode: Fortran; -*-
!
!
!  (C) 2003 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
      program main
      use mpi_f08
      character*(MPI_MAX_OBJECT_NAME) name
      integer namelen
      integer ierr, errs

      errs = 0

      call mtest_init( ierr )
!
! Check each Fortran datatype, including the size-specific ones
! See the C version (typename.c) for the relevant MPI sections

      call MPI_Type_get_name( MPI_COMPLEX, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_COMPLEX") then
           errs = errs + 1
           print *, "Expected MPI_COMPLEX but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_DOUBLE_COMPLEX, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_DOUBLE_COMPLEX") then
           errs = errs + 1
           print *, "Expected MPI_DOUBLE_COMPLEX but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_LOGICAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_LOGICAL") then
           errs = errs + 1
           print *, "Expected MPI_LOGICAL but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_REAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_REAL") then
           errs = errs + 1
           print *, "Expected MPI_REAL but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_DOUBLE_PRECISION, name, namelen, ierr)
      if (name(1:namelen) .ne. "MPI_DOUBLE_PRECISION") then
           errs = errs + 1
           print *, "Expected MPI_DOUBLE_PRECISION but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_INTEGER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_INTEGER") then
           errs = errs + 1
           print *, "Expected MPI_INTEGER but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_2INTEGER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_2INTEGER") then
           errs = errs + 1
           print *, "Expected MPI_2INTEGER but got "//name(1:namelen)
      endif

! 2COMPLEX was present only in MPI 1.0
!      call MPI_Type_get_name( MPI_2COMPLEX, name, namelen, ierr )
!      if (name(1:namelen) .ne. "MPI_2COMPLEX") then
!           errs = errs + 1
!           print *, "Expected MPI_2COMPLEX but got "//name(1:namelen)
!      endif
!
      call MPI_Type_get_name(MPI_2DOUBLE_PRECISION, name, namelen, ierr)
      if (name(1:namelen) .ne. "MPI_2DOUBLE_PRECISION") then
           errs = errs + 1
           print *, "Expected MPI_2DOUBLE_PRECISION but got "// &
      &          name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_2REAL, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_2REAL") then
           errs = errs + 1
           print *, "Expected MPI_2REAL but got "//name(1:namelen)
      endif

! 2DOUBLE_COMPLEX isn't in MPI 2.1
!      call MPI_Type_get_name( MPI_2DOUBLE_COMPLEX, name, namelen, ierr )
!      if (name(1:namelen) .ne. "MPI_2DOUBLE_COMPLEX") then
!           errs = errs + 1
!           print *, "Expected MPI_2DOUBLE_COMPLEX but got "//
!     &          name(1:namelen)
!      endif

      call MPI_Type_get_name( MPI_CHARACTER, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_CHARACTER") then
           errs = errs + 1
           print *, "Expected MPI_CHARACTER but got "//name(1:namelen)
      endif

      call MPI_Type_get_name( MPI_BYTE, name, namelen, ierr )
      if (name(1:namelen) .ne. "MPI_BYTE") then
           errs = errs + 1
           print *, "Expected MPI_BYTE but got "//name(1:namelen)
      endif

      if (MPI_REAL4 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL4, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL4") then
               errs = errs + 1
               print *, "Expected MPI_REAL4 but got "//name(1:namelen)
          endif
      endif

      if (MPI_REAL8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL8") then
               errs = errs + 1
               print *, "Expected MPI_REAL8 but got "//name(1:namelen)
          endif
      endif

      if (MPI_REAL16 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_REAL16, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_REAL16") then
               errs = errs + 1
               print *, "Expected MPI_REAL16 but got "//name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX8") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX8 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX16 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX16, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX16") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX16 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_COMPLEX32 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_COMPLEX32, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_COMPLEX32") then
               errs = errs + 1
               print *, "Expected MPI_COMPLEX32 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER1 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER1, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER1") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER1 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER2 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER2, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER2") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER2 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER4 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER4, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER4") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER4 but got "// &
      &              name(1:namelen)
          endif
      endif

      if (MPI_INTEGER8 .ne. MPI_DATATYPE_NULL) then
          call MPI_Type_get_name( MPI_INTEGER8, name, namelen, ierr )
          if (name(1:namelen) .ne. "MPI_INTEGER8") then
               errs = errs + 1
               print *, "Expected MPI_INTEGER8 but got "// &
      &              name(1:namelen)
          endif
      endif

! MPI_INTEGER16 is in MPI 2.1, but it is missing from most tables
! Some MPI implementations may not provide it
!      if (MPI_INTEGER16 .ne. MPI_DATATYPE_NULL) then
!          call MPI_Type_get_name( MPI_INTEGER16, name, namelen, ierr )
!          if (name(1:namelen) .ne. "MPI_INTEGER16") then
!               errs = errs + 1
!               print *, "Expected MPI_INTEGER16 but got "//
!     &              name(1:namelen)
!          endif
!      endif

      call mtest_finalize( errs )
      call MPI_Finalize( ierr )
      end
