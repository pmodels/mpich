!
! -*- Mode: Fortran; -*-
!
!  (C) 2012 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!
!
! In MPI 2.2, the behavior of attributes set in one language and retrieved
! from another was defined.  There are three types of attribute values:
!  1. pointer (void *): "The C type"
!  2. INTEGER         : "The MPI-1 Fortran type"
!  3. INTEGER (KIND=MPI_ADDRESS_KIND) : "The MPI-2 Fortran type"
!
! All three apply to Communicator attributes, with case 2 using the
! deprecated MPI_ATTR_GET and MPI_ATTR_PUT routines. For Datatype and
! RMA Window attributes, cases 1 and 3 apply.
!
! Note, just to make this more complex, there are some reasons why an MPI
! implementation may choose to make MPI_Aint (and the corresponding
! Fortran MPI_ADDRESS_KIND) larger than a void pointer.  Specifically,
! make it as large as MPI_Offset, which simplifies certain operations
! with datatypes.
!
! There are 9 cases:
! 1. C sets, C gets
! 2. C sets, Fortran INTEGER gets
! 3. C sets, Fortran ADDRESS_KIND gets
! 4. Fortran INTEGER sets, C gets
! 5. Fortran INTEGER sets, Fortran INTEGER gets
! 6. Fortran INTEGER sets, Fortran ADDRESS_KIND gets
! 7. Fortran ADDRESS_KIND sets, C gets
! 8. Fortran ADDRESS_KIND sets, Fortran INTEGER gets
! 9. Fortran ADDRESS_KIND sets, Fortran ADDRESS_KIND gets
!
! These are the basic cases.  They are complicated by the fact that
! the attribute values have 3 types: void * (C interface), MPI_Fint
! (Fortran INTEGER), and MPI_Aint (Fortran ADDRESS_KIND).  These
! have the following size relationships:
!
! sizeof(void *) <= sizeof(MPI_Aint)
!     (For some systems, MPI_Aint is set to the same size as
!      MPI_Offset, and may be larger than a void *.)
! sizeof(MPI_Fint) <= sizeof(MPI_Aint)
!     (Not strictly defined, but all reasonable implementations will
!      have this property)
!
! When a value is stored, the full value is stored (this may be fewer
! bytes than the maximum-sized attribute, in which case the high
! bytes are stored as zero).  When a value is retrieved, if the
! destination location is smaller, the low bytes (in value) are
! saved; this is the same as trunction.  If the destination location
! is longer, then then value is sign-extended (See MPI-3, 17.2.7).
!
! Specifically, if the value was set from Fortran, C will return a
! pointer to the appropriate-sized integer.
! When the value is set from C but accessed from Fortran, the value
! is converted to an integer of the appropriate length, possibly truncated.
!
! FIXME: The above different-length attribute case is not yet handled
!  in this code.
!
! In addition to setting and getting attributes, they are accessed
! through duplication (COMM_DUP and TYPE_DUP), and on deletion of the
! object to which they are attached, when the copy functions will be
! well-defined.
!
! This code was inspired by a code written by Jeff Squyres to test these
! nine cases. This code, however, is different.
!
! So, for each of the same->same tests:
!    Store largest positive and negative attributes.  Dup them,
!    retrieve them, delete them.  All bytes should remain value, and
!    no other.  Use keys created in all three languages for set/get;
!    use language under test for dup.
!
! For X->Y tests:
!    Using X, store into key created in all three.
!    Using Y, retrive all attributes.  See above for handling of
!     truncated or sign-extended
!
! Use Fortran to drive tests (Fortran main program).  Call C for
! C routines and to check data with different sizes (to ensure that
! the proper bytes are used in the value).
!
! Use the same keyval for attributes used in both C and Fortran (both
! modes).  This found an error in MPICH, where the type of the
! attribute (e.g., pointer, integer, or address-sized integer) needs
! to be saved.
!
! Module containing the keys and other information, including
! interfaces to the C routines
      module keyvals
        use mpi_f08
        logical fverbose, useintSize
        integer ptrSize, intSize, aintSize
        integer fcomm1_key, fcomm1_extra
        integer fcomm2_key, ftype2_key, fwin2_key
        integer ccomm1_key
        integer ccomm2_key, ctype2_key, cwin2_key
        TYPE(MPI_Win) win
        integer (kind=MPI_ADDRESS_KIND) fcomm2_extra, ftype2_extra,&
             & fwin2_extra
        interface
           pure function bigint()
             integer bigint
           end function bigint
           pure function bigaint()
             use mpi_f08, only : MPI_ADDRESS_KIND
             integer (kind=MPI_ADDRESS_KIND) bigaint
           end function bigaint
           ! Could use bind(c) once we require that level of Fortran support.
           subroutine csetmpi( fcomm, fkey, val, errs )
             use mpi_f08
             TYPE(MPI_Comm), INTENT(IN) :: fcomm
             integer, INTENT(IN) :: fkey, val
             integer errs
           end subroutine csetmpi
           subroutine csetmpi2( fcomm, fkey, val, errs )
             use mpi_f08
             TYPE(MPI_Comm), INTENT(IN) :: fcomm
             integer, INTENT(IN) :: fkey
             integer  errs
             integer (KIND=MPI_ADDRESS_KIND) val
           end subroutine csetmpi2
           subroutine cattrinit( fv )
             integer fv
           end subroutine cattrinit
           subroutine cgetsizes( ps, is, as )
             integer, INTENT(OUT) :: ps, is, as
           end subroutine cgetsizes
           subroutine ccreatekeys( k1, k2, k3, k4 )
             integer, INTENT(OUT) :: k1, k2, k3, k4
           end subroutine ccreatekeys
           subroutine cfreekeys()
           end subroutine cfreekeys
           subroutine ctoctest( errs )
             integer errs
           end subroutine ctoctest
        end interface
      end module keyvals
!
      program main
      use mpi_f08
      use keyvals
      implicit none
      integer ierr
      integer errs, tv, rank
      integer(MPI_ADDRESS_KIND) tmp

      errs = 0
      call MPI_INIT( ierr )
      call MPI_COMM_RANK( MPI_COMM_WORLD, rank, ierr )
!
!     Let the C routines know about debugging
      call cgetenvbool( "MPITEST_VERBOSE", tv )
      if (tv .eq. 1) then
         fverbose = .true.
         call cattrinit( 1 )
      else
         fverbose = .false.
         call cattrinit( 0 )
      endif
!
! If this value is true, define an "big MPI_Aint" value that fits in
! an MPI_Fint (see function bigaint)
      call cgetenvbool( "MPITEST_ATTR_INTFORAINT", tv )
      if (tv .eq. 1) then
         useintSize = .true.
      else
         useintSize = .false.
      endif
!
!     Get the sizes of the three types of attributes
      call cgetsizes( ptrSize, intSize, aintSize )
      if (fverbose) then
         print *, 'sizeof(ptr)=',ptrSize, ' sizeof(int)=', intSize, ' &
              &sizeof(aint)=', aintSize
      endif
!
!     Create the keyvals
!
!     Create the attribute values to use.  We want these to use the full
!     available width, which depends on both the type and the test,
!     since when switching between types of different sizes, we need to
!     check only the "low" bits (those shared in types of each size).
!
!
      if (fverbose) then
         print *, "Creating Fortran attribute keys"
      endif

      call fCreateKeys()
      if (fverbose) then
         print *, "Creating C attribute keys"
      endif
      call ccreatekeys( ccomm1_key, ccomm2_key, ctype2_key, cwin2_key&
           & )
!
!     Create a window to use with the attribute tests in Fortran
      tmp = 0
      call MPI_WIN_CREATE( MPI_BOTTOM, tmp, 1, MPI_INFO_NULL,&
           & MPI_COMM_WORLD, win, ierr )
!
      if (fverbose) then
         print *, "Case 1: C sets and C gets"
      endif
      call ctoctest( errs )
      if (fverbose) then
         print *, "Case 2: C sets and Fortran (MPI1) gets"
      endif
      call ctof1test( errs )
      if (fverbose) then
         print *, "Case 3: C sets and Fortran (MPI2) gets"
      endif
      call ctof2test( errs )
      if (fverbose) then
         print *, "Case 4: Fortran (MPI1) sets and C gets"
      endif
      call f1toctest( errs )
      if (fverbose) then
         print *, "Case 5: Fortran (MPI1) sets and gets"
      endif
      call f1tof1test( errs )
      if (fverbose) then
         print *, "Case 6: Fortran (MPI1) sets and Fortran (MPI2) gets"
      endif
      call f1tof2test( errs )
      if (fverbose) then
         print *, "Case 7: Fortran (MPI2) sets and C gets"
      endif
      call f2toctest( errs )
      if (fverbose) then
         print *, "Case 8: Fortran (MPI2) sets and Fortran (MPI1) gets"
      endif
      call f2tof1test( errs )
      if (fverbose) then
         print *, "Case 9: Fortran (MPI2) sets and gets"
      endif
      call f2tof2test( errs )

! Cleanup
      call ffreekeys()
      call cfreekeys()
      call MPI_WIN_FREE( win, ierr )

      call MPI_REDUCE( MPI_IN_PLACE, errs, 1, MPI_INT, MPI_SUM, 0,&
           & MPI_COMM_WORLD, ierr )

      if (rank .eq. 0) then
         if (errs .eq. 0) then
            print *, " No Errors"
         else
            print *, " Found ", errs, " errors"
         endif
      endif
      call MPI_FINALIZE( ierr )

      end
!
! -------------------------------------------------------------------
! Check attribute set in Fortran (MPI-1) and read from Fortran (MPI-1)
! -------------------------------------------------------------------
      integer function FMPI1checkCommAttr( comm, key, expected, msg )
      use mpi_f08
      integer key, expected
      TYPE(MPI_Comm) comm
      character*(*) msg
      integer value, ierr
      logical flag
!
      FMPI1checkCommAttr = 0
      call MPI_ATTR_GET( comm, key, value, flag, ierr )
      if (.not. flag) then
         print *, "Error: reading Fortran INTEGER attribute: ", msg
         FMPI1checkCommAttr = 1
         return
      endif
      if (value .ne. expected) then
         print *, "Error: Fortran INTEGER attribute: ", msg
         print *, "Expected ", expected, " but got ", value
         FMPI1checkCommAttr = 1
      endif
      return
      end
!
! -------------------------------------------------------------------
! Functions associated with attribute copy/delete.
! -------------------------------------------------------------------
      subroutine FMPI1_COPY_FN( oldcomm, key, extrastate, inval,&
           & outval, flag, ierr )
      use mpi_f08
      use keyvals, only : fverbose
      integer key, extrastate, inval, outval, ierr
      TYPE(MPI_Comm) oldcomm
      logical flag
!
      if (fverbose) then
         print *, 'FMPI1_COPY: Attr in = ', inval, ' extra = ',&
              & extrastate
      endif
      flag = .true.
      outval = inval + 1
      ierr = MPI_SUCCESS
      end
!
      subroutine FMPI1_DELETE_FN( oldcomm, key, extrastate, inval,&
     &     ierr )
      use mpi_f08
      use keyvals, only : fverbose
      integer key, extrastate, inval, ierr
      TYPE(MPI_Comm) oldcomm
      logical flag
!
      if (fverbose) then
         print *, "FMPI1_DELETE: inval = ", inval, " extra = ",&
              & extrastate
      endif
      ierr = MPI_SUCCESS
      end
!
!
      subroutine FMPI2_COPY_FN( oldcomm, key, extrastate, inval, outval,&
     &     flag, ierr )
      use mpi_f08
      use keyvals, only : fverbose
      implicit none
      integer key, ierr
      TYPE(MPI_Comm) oldcomm
      integer (KIND=MPI_ADDRESS_KIND) inval, outval, extrastate
      logical flag
!
      if (fverbose) then
         print *, 'FMPI2_COPY: Attr in = ', inval, ' extra = ',&
              & extrastate
      endif
      flag = .true.
      outval = inval + 1
      ierr = MPI_SUCCESS
      end
!
      subroutine FMPI2_DELETE_FN( oldcomm, key, extrastate, inval,&
     &     ierr )
      use mpi_f08
      use keyvals, only : fverbose
      implicit none
      integer key, ierr
      TYPE(MPI_Comm) oldcomm
      integer (kind=MPI_ADDRESS_KIND) inval, extrastate
!
      if (fverbose) then
         print *, "FMPI2_DELETE: inval = ", inval, " extra = ",&
              & extrastate
      endif
      ierr = MPI_SUCCESS
      end
! -------------------------------------------------------------------
!
! Typical check pattern
!
! Set value
! Get value (check set in same form)
! Get value in other modes
! Dup object (updates value)
! Get value in other modes
! Delete dup'ed object; check correct value sent to delete routine
!
      subroutine fcreateKeys( )
        use mpi_f08
        use keyvals
        implicit none
        external FMPI1_COPY_FN, FMPI1_DELETE_FN, FMPI2_COPY_FN,&
           & FMPI2_DELETE_FN
        integer ierr

        fcomm1_extra = 0
        fcomm2_extra = 0
        ftype2_extra = 0
        fwin2_extra  = 0
        call MPI_KEYVAL_CREATE( FMPI1_COPY_FN, FMPI1_DELETE_FN,&
             & fcomm1_key, fcomm1_extra, ierr )
        call MPI_COMM_CREATE_KEYVAL( FMPI2_COPY_FN, FMPI2_DELETE_FN,&
             & fcomm2_key, fcomm2_extra, ierr )
        call MPI_TYPE_CREATE_KEYVAL( FMPI2_COPY_FN, FMPI2_DELETE_FN,&
             & ftype2_key, ftype2_extra, ierr )
        call MPI_WIN_CREATE_KEYVAL( FMPI2_COPY_FN, FMPI2_DELETE_FN,&
             & fwin2_key, fwin2_extra, ierr )

      end subroutine fcreateKeys
!
      subroutine ffreekeys()
        use mpi_f08
        use keyvals
        implicit none
        integer ierr

        call MPI_KEYVAL_FREE( fcomm1_key, ierr )
        call MPI_COMM_FREE_KEYVAL( fcomm2_key, ierr )
        call MPI_TYPE_FREE_KEYVAL( ftype2_key, ierr )
        call MPI_WIN_FREE_KEYVAL( fwin2_key, ierr )

        return
      end subroutine ffreekeys
! -------------------------------------------------------------------
! Set attributes in Fortran (MPI-1) and read them from Fortran (MPI-1)
      subroutine f1tof1test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr, attrval, fcomm1attr
        TYPE(MPI_Comm) fdup
!
        fcomm1attr = bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             &"F to F (check value)", errs )
        call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
        call fmpi1read( fdup, fcomm1_key, attrval + 1, "F to F dup",&
             & errs )
        call MPI_COMM_FREE( fdup, ierr )
!
! Use a negative value
        fcomm1attr = -bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             &"F to F (check neg value)", errs )
        call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
        call fmpi1read( fdup, fcomm1_key, attrval + 1, "F to F dup",&
             & errs )
        call MPI_COMM_FREE( fdup, ierr )

      end subroutine f1tof1test
!
! Set attributes in C and read them from Fortran (MPI-1)
      subroutine ctof1test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr, attrval, fcomm1attr
        TYPE(MPI_Comm) fdup
!
        fcomm1attr = bigint()
        attrval    = fcomm1attr
        call csetmpi( MPI_COMM_SELF, fcomm1_key, fcomm1attr, errs )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, "C to F",&
             & errs )
        if (ptrSize .eq. intSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call fmpi1read( fdup, fcomm1_key, attrval + 1, "C to F dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        fcomm1attr = -bigint()
        attrval    = fcomm1attr
        call csetmpi( MPI_COMM_SELF, fcomm1_key, fcomm1attr, errs )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, "C to F",&
             & errs )
        if (ptrSize .eq. intSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call fmpi1read( fdup, fcomm1_key, attrval + 1, "C to F dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif

      end subroutine ctof1test
!
! Set attributes in Fortran (MPI-1) and read in Fortran (MPI-2)
      subroutine f1tof2test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr, attrval, fcomm1attr
        TYPE(MPI_Comm) fdup
        integer (kind=MPI_ADDRESS_KIND) expected
!
        fcomm1attr = bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             & "F to F (check value for F2 test)", errs )
        if (intSize .eq. aintSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           expected = attrval + 1
           call fmpi2read( fdup, fcomm1_key, expected, "F to F2 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        fcomm1attr = -bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             &"F to F (check neg value for F2 test)", errs )
        if (intSize .eq. aintSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           expected = attrval + 1
           call fmpi2read( fdup, fcomm1_key, expected, "F to F2 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif

      end subroutine f1tof2test
!
      subroutine fmpi1read( comm, key, expected, msg, errs )
        use mpi_f08
        implicit none
        integer key, expected, errs
        TYPE(MPI_Comm) comm
        character *(*) msg
        logical flag
        integer ierr, attrval
!
        call MPI_ATTR_GET( comm, key, attrval, flag, ierr )
        if (.not. flag) then
           print *, 'Error: flag false for Attr_get: ', msg
           errs = errs + 1
           return
        endif
        if (attrval .ne. expected) then
           print *, 'Error: expected ', expected, ' but saw ',&
                & attrval, ':', msg
           errs = errs + 1
        endif
        return
      end subroutine fmpi1read

      subroutine fmpi2read( comm, key, expected, msg, errs )
        use mpi_f08
        implicit none
        integer key, errs
        TYPE(MPI_Comm) comm
        integer (kind=MPI_ADDRESS_KIND) expected
        character *(*) msg
        logical flag
        integer ierr
        integer (kind=MPI_ADDRESS_KIND) attrval
!
        call MPI_COMM_GET_ATTR( comm, key, attrval, flag, ierr )
        if (.not. flag) then
           print *, 'Error: flag false for Attr_get: ', msg
           errs = errs + 1
           return
        endif
        if (attrval .ne. expected) then
           print *, 'Error: expected ', expected, ' but saw ',&
                & attrval, ':', msg
           errs = errs + 1
        endif
        return
      end subroutine fmpi2read

      subroutine fmpi2readwin( win, key, expected, msg, errs )
        use mpi_f08
        implicit none
        integer key, errs
        TYPE(MPI_Win) win
        integer (kind=MPI_ADDRESS_KIND) expected
        character *(*) msg
        logical flag
        integer ierr
        integer (kind=MPI_ADDRESS_KIND) attrval
!
        call MPI_WIN_GET_ATTR( win, key, attrval, flag, ierr )
        if (.not. flag) then
           print *, 'Error: flag false for Win_get_attr: ', msg
           errs = errs + 1
           return
        endif
        if (attrval .ne. expected) then
           print *, 'Error: expected ', expected, ' but saw ',&
                & attrval, ':', msg
           errs = errs + 1
        endif
        return
      end subroutine fmpi2readwin

      subroutine fmpi2readtype( dtype, key, expected, msg, errs )
        use mpi_f08
        implicit none
        integer key, errs
        TYPE(MPI_Datatype) dtype
        integer (kind=MPI_ADDRESS_KIND) expected
        character *(*) msg
        logical flag
        integer ierr
        integer (kind=MPI_ADDRESS_KIND) attrval
!
        call MPI_TYPE_GET_ATTR( dtype, key, attrval, flag, ierr )
        if (.not. flag) then
           print *, 'Error: flag false for Type_get_attr: ', msg
           errs = errs + 1
           return
        endif
        if (attrval .ne. expected) then
           print *, 'Error: expected ', expected, ' but saw ',&
                & attrval, ':', msg
           errs = errs + 1
        endif
        return
      end subroutine fmpi2readtype

      subroutine f2tof2test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr
        TYPE(MPI_Comm) fdup
        TYPE(MPI_Datatype) tdup
        integer (kind=MPI_ADDRESS_KIND) fcomm2attr, ftype2attr,&
             & fwin2attr, attrval
!
        fcomm2attr = bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
        call fmpi2read( fdup, fcomm2_key, attrval + 1, "F2 to F2 dup",&
             & errs )
        call MPI_COMM_FREE( fdup, ierr )
!
        ftype2attr = bigaint()-9
        attrval    = ftype2attr
        call MPI_TYPE_SET_ATTR( MPI_INTEGER, ftype2_key, ftype2attr,&
             & ierr )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "F2 type&
             & to F2", errs )
        call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
        call fmpi2readtype( tdup, ftype2_key, attrval + 1, "F2 type to&
             & F dup", errs )
        call MPI_TYPE_FREE( tdup, ierr )

        fwin2attr = bigaint()-9
        attrval    = fwin2attr
        call MPI_WIN_SET_ATTR( win, fwin2_key, fwin2attr,&
             & ierr )
        call fmpi2readwin( win, fwin2_key, attrval, "F2 win to F2",&
             & errs )
!
        fcomm2attr = -bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
        call fmpi2read( fdup, fcomm2_key, attrval + 1, "F2 to F2 dup",&
             & errs )
        call MPI_COMM_FREE( fdup, ierr )
!
        ftype2attr = -(bigaint()-9)
        attrval    = ftype2attr
        call MPI_TYPE_SET_ATTR( MPI_INTEGER, ftype2_key, ftype2attr,&
             & ierr )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "F2 type&
             & to F2", errs )
        call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
        call fmpi2readtype( tdup, ftype2_key, attrval + 1, "F2 type to&
             & F dup", errs )
        call MPI_TYPE_FREE( tdup, ierr )

        fwin2attr = -(bigaint()-9)
        attrval    = fwin2attr
        call MPI_WIN_SET_ATTR( win, fwin2_key, fwin2attr,&
             & ierr )
        call fmpi2readwin( win, fwin2_key, attrval, "F2 win to F2",&
             & errs )

      end subroutine f2tof2test
!
      subroutine f1toctest( errs )
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr, attrval, fcomm1attr
        TYPE(MPI_Comm) fdup
!
        fcomm1attr = bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             & "F to F (check for F to C)", errs )
        if (intSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call cmpif1read( fdup, fcomm1_key, attrval + 1, errs, &
                & "F to F2 dup" )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        fcomm1attr = -bigint()
        attrval    = fcomm1attr
        call MPI_ATTR_PUT( MPI_COMM_SELF, fcomm1_key, fcomm1attr,&
             & ierr )
        call fmpi1read( MPI_COMM_SELF, fcomm1_key, attrval, &
             &"F to F (check neg value for F to C)", errs )
        if (intSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call cmpif1read( fdup, fcomm1_key, attrval + 1, errs, &
                "F to C dup" )
           call MPI_COMM_FREE( fdup, ierr )
        endif

      end subroutine f1toctest

      subroutine f2tof1test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr
        TYPE(MPI_Comm) fdup
        TYPE(MPI_Datatype) tdup
        integer (kind=MPI_ADDRESS_KIND) fcomm2attr, ftype2attr,&
             & fwin2attr, attrval
        integer expected
!
        fcomm2attr = bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        if (aintSize .eq. intSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           expected = attrval + 1
           call fmpi1read( fdup, fcomm2_key, expected, "F2 to F1 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        fcomm2attr = -bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        if (aintSize .eq. intSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           expected = attrval + 1
           call fmpi1read( fdup, fcomm2_key, expected, "F2 to F1 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif

!
      end subroutine f2tof1test
!
      subroutine f2toctest(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr
        TYPE(MPI_Comm) fdup
        TYPE(MPI_Datatype) tdup
        integer (kind=MPI_ADDRESS_KIND) fcomm2attr, ftype2attr,&
             & fwin2attr, attrval
!
        fcomm2attr = bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        if (aintSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call cmpif2read( fdup, fcomm2_key, attrval + 1, errs, "F2 t&
                &o c dup")
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        ftype2attr = bigaint()-9
        attrval    = ftype2attr
        call MPI_TYPE_SET_ATTR( MPI_INTEGER, ftype2_key, ftype2attr,&
             & ierr )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "F2 type&
             & to F2", errs )
        if (aintSize .eq. ptrSize) then
           call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
           call cmpif2readtype( tdup, ftype2_key, attrval + 1, errs, "F2 &
                &type toF dup" )
           call MPI_TYPE_FREE( tdup, ierr )
        endif

        fwin2attr = bigaint()-9
        attrval    = fwin2attr
        call MPI_WIN_SET_ATTR( win, fwin2_key, fwin2attr, ierr )
        call cmpif2readwin( win, fwin2_key, attrval, errs, "F2 win to &
             &c" )
!
        fcomm2attr = -bigaint()
        attrval    = fcomm2attr
        call MPI_COMM_SET_ATTR( MPI_COMM_SELF, fcomm2_key, fcomm2attr,&
             & ierr )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "F2 to F2",&
             & errs )
        if (aintSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call cmpif2read( fdup, fcomm2_key, attrval + 1, errs, "F2 t&
                &o c dup")
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        ftype2attr = -(bigaint()-9)
        attrval    = ftype2attr
        call MPI_TYPE_SET_ATTR( MPI_INTEGER, ftype2_key, ftype2attr,&
             & ierr )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "F2 type&
             & to F2", errs )
        if (aintSize .eq. ptrSize) then
           call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
           call cmpif2readtype( tdup, ftype2_key, attrval + 1, errs, "F2 &
                &type toF dup" )
           call MPI_TYPE_FREE( tdup, ierr )
        endif

        fwin2attr = -(bigaint()-9)
        attrval    = fwin2attr
        call MPI_WIN_SET_ATTR( win, fwin2_key, fwin2attr, ierr )
        call cmpif2readwin( win, fwin2_key, attrval, errs, "F2 win to &
             &c" )

      end subroutine f2toctest
!
      subroutine ctof2test(errs)
        use mpi_f08
        use keyvals
        implicit none
        integer errs
        integer ierr
        TYPE(MPI_Comm) fdup
        TYPE(MPI_Datatype) tdup
        integer (kind=MPI_ADDRESS_KIND) fcomm2attr, ftype2attr,&
             & fwin2attr, attrval
!
        fcomm2attr = bigaint()
        attrval    = fcomm2attr
        call csetmpi2( MPI_COMM_SELF, fcomm2_key, fcomm2attr, errs )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "c to F2",&
             & errs )
        if (aintSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call fmpi2read( fdup, fcomm2_key, attrval + 1, "c to F2 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        ftype2attr = bigaint()-9
        attrval    = ftype2attr
        call csetmpitype( MPI_INTEGER, ftype2_key, ftype2attr, errs )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "c type&
             & to F2", errs )
        if (aintSize .eq. ptrSize) then
           call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
           call fmpi2readtype( tdup, ftype2_key, attrval + 1, "c type to&
                & F2 dup", errs )
           call MPI_TYPE_FREE( tdup, ierr )
        endif

        fwin2attr = bigaint()-9
        attrval    = fwin2attr
        call csetmpiwin( win%MPI_VAL, fwin2_key, fwin2attr, errs )
        call fmpi2readwin( win, fwin2_key, attrval, "c win to F2",&
             & errs )
!
        fcomm2attr = -bigaint()
        attrval    = fcomm2attr
        call csetmpi2( MPI_COMM_SELF, fcomm2_key, fcomm2attr, errs )
        call fmpi2read( MPI_COMM_SELF, fcomm2_key, attrval, "c to F2",&
             & errs )
        if (aintSize .eq. ptrSize) then
           call MPI_COMM_DUP( MPI_COMM_SELF, fdup, ierr )
           call fmpi2read( fdup, fcomm2_key, attrval + 1, "c to F2 dup",&
                & errs )
           call MPI_COMM_FREE( fdup, ierr )
        endif
!
        ftype2attr = -(bigaint()-9)
        attrval    = ftype2attr
        call csetmpitype( MPI_INTEGER, ftype2_key, ftype2attr, errs )
        call fmpi2readtype( MPI_INTEGER, ftype2_key, attrval, "c type&
             & to F2", errs )
        if (aintSize .eq. ptrSize) then
           call MPI_TYPE_DUP( MPI_INTEGER, tdup, ierr )
           call fmpi2readtype( tdup, ftype2_key, attrval + 1, "c type to&
                & F2 dup", errs )
           call MPI_TYPE_FREE( tdup, ierr )
        endif

        fwin2attr = -(bigaint()-9)
        attrval    = fwin2attr
        call csetmpiwin( win, fwin2_key, fwin2attr, errs )
        call fmpi2readwin( win, fwin2_key, attrval, "c win to F2",&
             & errs )

      end subroutine ctof2test

! -------------------------------------------------------------------
! Return an integer value that fills all of the bytes
      pure integer function bigint()
        integer i, v, digits
        digits = range(i)
        v = 0
        do i=1,digits
           v = v * 10 + i
        enddo
        bigint = v
        return
      end function bigint
!
! Return an integer value that fill all of the bytes in an AINT
! The logical "useintsize" allows us to specify that only an int-sized
! result should be returned
      pure function bigaint()
        use mpi_f08, only : MPI_ADDRESS_KIND
        use keyvals, only : useintsize
        implicit none
        integer (kind=MPI_ADDRESS_KIND) bigaint, v
        integer i, digits
        if (useintsize) then
           digits = range(i)
        else
           digits = range(v)
        endif
        v = 0
        do i=1,digits
           v = v * 10 + i
        enddo
        bigaint = v
        return
      end function bigaint
