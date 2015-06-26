! -*- Mode: Fortran; -*- 
!
!  (C) 2013 by Argonne National Laboratory.
!      See COPYRIGHT in top-level directory.
!

! Based on a test written by Jim Hoekstra on behalf of Cray, Inc.
! see ticket #884 https://trac.mpich.org/projects/mpich/ticket/884

PROGRAM get_elem_u

  USE mpi 
  IMPLICIT NONE 
  INTEGER    RANK, SIZE, IERR, COMM, errs 
  INTEGER    MAX, I, K, dest
  INTEGER   STATUS(MPI_STATUS_SIZE)

  INTEGER, PARAMETER :: nb=2
  INTEGER :: blklen(nb)=(/1,1/)
  INTEGER :: types(nb)
  INTEGER(kind=MPI_ADDRESS_KIND) :: disp(nb)=(/0,8/)

  INTEGER, PARAMETER :: amax=200
  INTEGER :: type1, type2, extent
  REAL    :: a(amax)

  errs = 0 
  CALL MPI_Init( ierr ) 
  COMM = MPI_COMM_WORLD 
  types(1) = MPI_DOUBLE_PRECISION
  types(2) = MPI_CHAR
  CALL MPI_Comm_rank(COMM,RANK,IERR) 
  CALL MPI_Comm_size(COMM,SIZE,IERR) 
  dest=size-1

  CALL MPI_Type_create_struct(nb, blklen, disp, types, type1, ierr)
  CALL MPI_Type_commit(type1, ierr)
  CALL MPI_Type_extent(type1, extent, ierr)

  CALL MPI_Type_contiguous(4, Type1, Type2, ierr) 
  CALL MPI_Type_commit(Type2, ierr) 
  CALL MPI_Type_extent(Type2, extent, ierr)

  DO k=1,17

     IF(rank .EQ. 0) THEN 

        !       send k copies of datatype Type1
        CALL MPI_Send(a, k, Type1, dest, 0, comm, ierr) 

     ELSE IF (rank == dest) THEN

        CALL MPI_Recv(a, 200, Type2, 0, 0, comm, status, ierr) 
        CALL MPI_Get_elements(status, Type2, i, ierr)
        IF (i .NE. 2*k) THEN
           errs = errs+1
           PRINT *, "k=",k,"  MPI_Get_elements returns", i, ", but it should be", 2*k
        END IF

     ELSE
        !       thix rank does not particupate
     END IF
  enddo

  CALL MPI_Type_free(type1, ierr)
  CALL MPI_Type_free(type2, ierr)

  CALL MPI_Finalize( ierr )

  IF(rank .EQ. 0 .AND. errs .EQ. 0) THEN
     PRINT *, " No Errors"
  END IF

END PROGRAM get_elem_u
