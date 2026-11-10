!
!     Copyright (C) by Argonne National Laboratory
!         See COPYRIGHT in top-level directory
!
!     Initialize the common blocks so they are defined as initialized
!     data (.data) instead of uninitialized (.bss). Otherwise, a shared
!     library that includes mpif.h may get its own copy of the common
!     blocks, and the sentinels (e.g. MPI_IN_PLACE) it passes will not
!     be recognized, e.g. when it is loaded with RTLD_DEEPBIND. Only the
!     addresses matter. The values must be nonzero, since zero-initialized
!     data still goes to .bss.
!
!     Keep this separate from mpirinitf (setbotf.f). In the same object,
!     gfortran references the blocks directly instead of through the GOT,
!     and with -flat_namespace (Darwin) dyld cannot bind them to the
!     program's copy. Not built with -flat_namespace, where dyld already
!     binds every image to one copy.
       block data mpirinitf_data
       include 'mpifnoext.h'
       data MPI_UNWEIGHTED /1/
       data MPI_WEIGHTS_EMPTY /1/
       data MPI_BUFFER_AUTOMATIC /1/
       data MPI_BOTTOM /1/
       data MPI_ERRCODES_IGNORE /1/
       data MPI_ARGV_NULL /'X'/
       end
