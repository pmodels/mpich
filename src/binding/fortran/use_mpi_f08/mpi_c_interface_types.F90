!   -*- Mode: Fortran; -*-
!
!   (C) 2014 by Argonne National Laboratory.
!   See COPYRIGHT in top-level directory.
!
! mpich3 integer sizes for handles and status type
!
! Module mpi_c_interface_types is a helper module used by the modules
! specifying c interfaces:
!
! mpi_c_interface_cdesc - interfaces for MPI functions with a descriptor argument
! mpi_c_interface_nobuf - interfaces for MPI functions with no choice argument
!
! It is not defined in the spec and not intended for end users.
!--------------------------------------------------------------

module mpi_c_interface_types

!=========================
! Kinds for native C types
!=========================
 use,intrinsic :: iso_c_binding, only: c_int, c_long, c_long_long, &
                                       c_ptr, c_double, C_NULL_PTR, &
                                       c_char, c_intptr_t , c_sizeof
implicit none

!=============================================================
! Kinds for integer types in C prototypes; should match  mpi.h
!=============================================================

! C types (with MPI_ => c_ for the internal names here)
! A.1.2 p 676

!Extracted from 'grep typedef mpi.h'
!typedef int MPI_Datatype;
!typedef int MPI_Comm;
!typedef int MPI_Group;
!typedef int MPI_Win;
!typedef int MPI_Op;
!typedef int MPI_Errhandler;
!typedef int MPI_Request;
!typedef int MPI_Message;
!typedef int MPI_Info;
!typedef long MPI_Aint;
!typedef int MPI_Fint;   ! replaced above with compiler-flag dependent value
!typedef long long MPI_Count;
!typedef long long MPI_Offset;

integer,parameter :: c_Fint       = KIND(0)

! TODO: need to automatically set these three parameters as in mpi.h.in
integer,parameter :: c_Aint       = c_long
integer,parameter :: c_Count      = c_long_long
integer,parameter :: c_Offset     = c_long_long

integer,parameter :: c_Datatype   = c_int
integer,parameter :: c_Comm       = c_int
integer,parameter :: c_Group      = c_int
integer,parameter :: c_Win        = c_int
integer,parameter :: c_Op         = c_int
integer,parameter :: c_Info       = c_int
integer,parameter :: c_Errhandler = c_int
integer,parameter :: c_Request    = c_int
integer,parameter :: c_Message    = c_int
integer,parameter :: c_file       = c_intptr_t

!================================================
! C struct for STATUS values; should match  mpi.h
!================================================

! MPI_Status, with name changed to c_Status for internal disambiguation
! A.1.2 p 676

type, bind(c) :: c_Status
   integer(c_int)  :: count_lo
   integer(c_int)  :: count_hi_and_cancelled
   integer(c_int)  :: MPI_SOURCE
   integer(c_int)  :: MPI_TAG
   integer(c_int)  :: MPI_ERROR
end type c_Status

end module mpi_c_interface_types

