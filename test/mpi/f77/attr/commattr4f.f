C
      program main
C
      include 'mpif.h'

      integer    ierr
      integer    errs
      logical    found
      integer    comm2
      integer    key
      include    'attraints.h'

      errs = 0
C
C  initialize the mpi environment
C
      call mpi_init(ierr)

      call mpi_comm_create_keyval(MPI_COMM_DUP_FN,
     $     MPI_NULL_DELETE_FN,
     $     key,
     $     extrastate,
     $     ierr)
C
C  set a value for the attribute
C
      valin = huge(valin)
C
C  set attr in comm_world
C
      call mpi_comm_set_attr(MPI_COMM_WORLD,
     $     key,
     $     valin,
     $     ierr)
      call mpi_comm_get_attr(MPI_COMM_WORLD,
     $     key,
     $     valout,
     $     found,
     $     ierr)
      if (found .neqv. .true.) then
         print *, "mpi_comm_set_attr reported key, but not found on ",
     $        "mpi_comm_world"
         errs = errs + 1
      else if (valout .ne. valin) then
         print *, "key found, but valin does not match valout"
         print *, valout, " != ", valin
         errs = errs + 1
      end if
C
C  dup the communicator, attribute should follow
C
      call mpi_comm_dup(MPI_COMM_WORLD,
     $     comm2,
     $     ierr)
C
C  get the value for the attribute
C
      call mpi_comm_get_attr(comm2,
     $     key,
     $     valout,
     $     found,
     $     ierr)
      if (found .neqv. .true.) then
         print *, "mpi_comm_set_attr reported key, but not found on ",
     $        "duped comm"
         errs = errs + 1
      else if (valout .ne. valin) then
         print *, "key found, but value does not match that on ",
     $        "mpi_comm_world"
         print *, valout, " != ", valin
         errs = errs + 1
      end if
C
C     free the duped communicator
C
      call mpi_comm_free(comm2, ierr)
C
C     free keyval
C
      call mpi_comm_delete_attr(MPI_COMM_WORLD,
     $     key, ierr)
      call mpi_comm_free_keyval(key,
     $     ierr)
      call mpi_finalize(ierr)

      if (errs .eq. 0) then
         print *, " No Errors"
      end if

      end
