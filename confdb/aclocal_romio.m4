dnl
dnl This files contains additional macros for using autoconf to
dnl build configure scripts.
dnl
dnl Almost all of this file is taken from the aclocal.m4 of MPICH
dnl
dnl
define(PAC_GET_SPECIAL_SYSTEM_INFO,[
#
if test -n "$arch_IRIX"; then
   AC_MSG_CHECKING(for IRIX OS version)
   dnl This block of code replaces a generic "IRIX" arch value with
   dnl  IRIX_<version>_<chip>
   dnl  For example
   dnl  IRIX_5_4400 (IRIX 5.x, using MIPS 4400)
   osversion=`uname -r | sed 's/\..*//'`
   dnl Note that we need to allow brackets here, so we briefly turn off
   dnl the macro quotes
   changequote(,)dnl
   dnl Get the second field (looking for 6.1)
   osvminor=`uname -r | sed 's/[0-9]\.\([0-9]*\)\..*/\1/'`
   changequote([,])dnl
   AC_MSG_RESULT($osversion)
   dnl Get SGI processor count by quick hack
   AC_MSG_CHECKING(for IRIX cpucount)
   changequote(,)dnl
   cpucount=`hinv | grep '[0-9]* [0-9]* MHZ IP[0-9]* Proc' | cut -f 1 -d' '`
   if test "$cpucount" = "" ; then
     cpucount=`hinv | grep 'Processor [0-9]*:' | wc -l | sed -e 's/ //g'`
   fi
   changequote([,])dnl
   if test "$cpucount" = "" ; then
     AC_MSG_RESULT([Could not determine cpucount.  Please send])
     hinv
     AC_MSG_ERROR([to romio-maint@mcs.anl.gov])
   fi
   AC_MSG_RESULT($cpucount)
   dnl
   AC_MSG_CHECKING(for IRIX cpumodel)
   dnl The tail -1 is necessary for multiple processor SGI boxes
   dnl We might use this to detect SGI multiprocessors and recommend
   dnl -comm=shared
   cputype=`hinv -t cpu | tail -1 | cut -f 3 -d' '`
   if test -z "$cputype" ; then
        AC_MSG_RESULT([Could not get cputype from hinv -t cpu command. Please send])
        hinv -t cpu 2>&1
        hinv -t cpu | cut -f 3 -d' ' 2>&1
	AC_MSG_ERROR([to romio-maint@mcs.anl.gov])
   fi
   AC_MSG_RESULT($cputype)
   dnl echo "checking for osversion and cputype"
   dnl cputype may contain R4400, R2000A/R3000, or something else.
   dnl We may eventually need to look at it.
   if test -z "$osversion" ; then
        AC_MSG_RESULT([Could not determine OS version.  Please send])
        uname -a
        AC_MSG_ERROR([to romio-maint@mcs.anl.gov])
   elif test $osversion = 4 ; then
        true
   elif test $osversion = 5 ; then
        true
   elif test $osversion = 6 ; then
        true
   else
       AC_MSG_RESULT([Could not recognize the version of IRIX (got $osversion).
ROMIO knows about versions 4, 5 and 6; the version being returned from
uname -r is $osversion.  Please send])
       uname -a 2>&1
       hinv 2>&1
       AC_MSG_ERROR([to romio-maint@mcs.anl.gov])
   fi
   AC_MSG_CHECKING(for cputype)
   OLD_ARCH=IRIX
   IRIXARCH="$ARCH_$osversion"
   dnl Now, handle the chip set
   changequote(,)dnl
   cputype=`echo $cputype | sed -e 's%.*/%%' -e 's/R//' | tr -d "[A-Z]"`
   changequote([,])dnl
   case $cputype in
        3000) ;;
        4000) ;;
        4400) ;;
        4600) ;;
        5000) ;;
        8000) ;;
        10000);;
	12000);;
        *)
	AC_MSG_WARN([Unexpected IRIX/MIPS chipset $cputype.  Please send the output])
        uname -a 2>&1
        hinv 2>&1
        AC_MSG_WARN([to romio-maint@mcs.anl.gov
ROMIO will continue and assume that the cputype is
compatible with a MIPS 4400 processor.])
        cputype=4400
        ;;
   esac
   AC_MSG_RESULT($cputype)
   IRIXARCH="$IRIXARCH_$cputype"
   echo "IRIX-specific architecture is $IRIXARCH"
fi
])dnl
dnl
dnl
define(PAC_TEST_MPI,[
  AC_MSG_CHECKING(if a simple MPI program compiles and links)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         MPI_Init(&argc,&argv);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  cmd="$CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB"
  echo "$as_me:$LINENO: $cmd" >&5
  $cmd >&5 2>&5
  if test ! -x conftest$EXEEXT ; then
      echo "$as_me:$LINENO: failed program was:" >&5
      sed 's/^/| /' mpitest.c >&5
      rm -f conftest$EXEEXT mpitest.c
      AC_MSG_ERROR([Unable to compile a simple MPI program.
Use environment variables to provide the location of MPI libraries and
include directories])
  else
      rm -f conftest$EXEEXT mpitest.c
  fi
AC_MSG_RESULT(yes)
])dnl
dnl
dnl
dnl
define(PAC_NEEDS_FINT,[
  AC_MSG_CHECKING(if MPI_Fint is defined in the MPI implementation)
  cat > mpitest1.c <<EOF
#include "mpi.h"
     main()
     {
         MPI_Fint i;
         i = 0;
     }
EOF
  rm -f mpitest1.$OBJEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -c mpitest1.c > /dev/null 2>&1
  if test ! -s mpitest1.$OBJEXT ; then
      NEEDS_MPI_FINT="#define NEEDS_MPI_FINT"
      CFLAGS="$CFLAGS -DNEEDS_MPI_FINT"
      AC_MSG_RESULT(no)
      rm -f mpitest1.$OBJEXT mpitest1.c
  else
      NEEDS_MPI_FINT=""
      AC_MSG_RESULT(yes)
      rm -f mpitest1.$OBJEXT mpitest1.c
  fi
])dnl
dnl
define(PAC_MPI_LONG_LONG_INT,[
  AC_MSG_CHECKING(if MPI_LONG_LONG_INT is defined in mpi.h)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         long long i;
         MPI_Init(&argc,&argv);
         MPI_Send(&i, 1, MPI_LONG_LONG_INT, 0, 0, MPI_COMM_WORLD);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_MPI_LONG_LONG_INT,,[Define if mpi has long long it])
  else
      AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
define(PAC_MPI_INFO,[
  AC_MSG_CHECKING(if MPI_Info functions are defined in the MPI implementation)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         MPI_Info info;
         MPI_Init(&argc,&argv);
         MPI_Info_create(&info);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_MPI_INFO,1,[Define if MPI_Info available])
      HAVE_MPI_INFO="#define HAVE_MPI_INFO"
  else
      AC_MSG_RESULT(no)
      BUILD_MPI_INFO=1
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
define(PAC_MPI_DARRAY_SUBARRAY,[
  AC_MSG_CHECKING(if darray and subarray constructors are defined in the MPI implementation)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         int i=MPI_DISTRIBUTE_CYCLIC;
         MPI_Datatype t;
         MPI_Init(&argc,&argv);
         MPI_Type_create_darray(i, i, i, &i, &i, &i, &i, i, MPI_INT, &t);
         MPI_Type_create_subarray(i, &i, &i, &i, i, MPI_INT, &t);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_MPI_DARRAY_SUBARRAY,,[Define if MPI Darray available])
      HAVE_MPI_DARRAY_SUBARRAY="#define HAVE_MPI_DARRAY_SUBARRAY"
  else
      AC_MSG_RESULT(no)
      BUILD_MPI_ARRAY=1
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
define(PAC_CHECK_MPI_SGI_INFO_NULL,[
  AC_MSG_CHECKING([if MPI_INFO_NULL is defined in mpi.h])
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
	int i;
	i = MPI_INFO_NULL;
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
      AC_MSG_RESULT(yes)
      cp adio/sgi/mpi3.1/*.h include
  else
      AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
dnl check if pread64 is defined in IRIX. needed on IRIX 6.5
dnl
define(PAC_HAVE_PREAD64,[
  AC_MSG_CHECKING(if pread64 is defined)
  rm -f conftest.c
  cat > conftest.c <<EOF
#include <unistd.h>
     main()
     {
         int fd=0, buf=0, i=0;
         off64_t off=0;
         pread64(fd, &buf, i, off);
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -o conftest$EXEEXT conftest.c > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_PREAD64,,[Define if pread64 available])
  else
      AC_MSG_RESULT(no)
  fi
rm -f conftest$EXEEXT conftest.c
])dnl
dnl
dnl
define(PAC_TEST_MPI_SGI_type_is_contig,[
  AC_MSG_CHECKING(if MPI_SGI_type_is_contig is defined)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         MPI_Datatype type;
         int i;

         MPI_Init(&argc,&argv);
         i = MPI_SGI_type_is_contig(type);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
     AC_DEFINE(NO_MPI_SGI_type_is_contig,,[Define if no MPI type is contig])
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
dnl
define(PAC_TEST_MPI_COMBINERS,[
  AC_MSG_CHECKING(if MPI-2 combiners are defined in mpi.h)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
         int i;

         MPI_Init(&argc,&argv);
         i = MPI_COMBINER_STRUCT;
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_MPI_COMBINERS,,[Define if MPI combiners available])
  else
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
dnl
dnl
dnl PAC_GET_XFS_MEMALIGN
dnl
dnl
define(PAC_GET_XFS_MEMALIGN,
[AC_MSG_CHECKING([for memory alignment needed for direct I/O])
rm -f memalignval
rm -f /tmp/romio_tmp.bin
AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
main() {
  struct dioattr st;
  int fd = open("/tmp/romio_tmp.bin", O_RDWR | O_CREAT, 0644);
  FILE *f=fopen("memalignval","w");
  if (fd == -1) exit(1);
  if (!f) exit(1);
  fcntl(fd, F_DIOINFO, &st);
  fprintf( f, "%u\n", st.d_mem);
  exit(0);
}]])],[Pac_CV_NAME=`cat memalignval`],[Pac_CV_NAME=""],[])
rm -f memalignval
rm -f /tmp/romio_tmp.bin
if test -n "$Pac_CV_NAME" -a "$Pac_CV_NAME" != 0 ; then
    AC_MSG_RESULT($Pac_CV_NAME)
    CFLAGS="$CFLAGS -DXFS_MEMALIGN=$Pac_CV_NAME"
else
    AC_MSG_RESULT(unavailable, assuming 128)
    CFLAGS="$CFLAGS -DXFS_MEMALIGN=128"
fi
])dnl
dnl

define(PAC_HAVE_MOUNT_NFS,[
  AC_MSG_CHECKING([if MOUNT_NFS is defined in the include files])
  rm -f conftest.c
  cat > conftest.c <<EOF
#include <sys/param.h>
#include <sys/mount.h>
     main()
     {
         int i=MOUNT_NFS;
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -o conftest$EXEEXT conftest.c > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     ROMIO_HAVE_MOUNT_NFS=1
     AC_DEFINE(HAVE_MOUNT_NFS,,[Define if MOUNT_NFS defined])
  else
     ROMIO_HAVE_MOUNT_NFS=0
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT conftest.c
])dnl
dnl
dnl
define(PAC_FUNC_STRERROR,[
  AC_MSG_CHECKING([for strerror()])
  rm -f conftest.c
  cat > conftest.c <<EOF
#include <string.h>
     main()
     {
        char *s = strerror(5);
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -o conftest$EXEXT conftest.c >> config.log 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_STRERROR,,[Define if strerror available])
  else
     AC_MSG_RESULT(no)
     AC_MSG_CHECKING([for sys_errlist])
     rm -f conftest.c
changequote(,)
     cat > conftest.c <<EOF
#include <stdio.h>
        main()
        {
           extern char *sys_errlist[];
	   printf("%s\n", sys_errlist[34]);
        }
EOF
changequote([,])
     rm -f conftest$EXEEXT
     $CC $USER_CFLAGS -o conftest$EXEEXT conftest.c > config.log 2>&1
     if test -x conftest$EXEEXT ; then
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_SYSERRLIST,,[Define if syserrlist available])
     else
        AC_MSG_RESULT(no)
     fi
  fi
  rm -f conftest$EXEEXT conftest.c
])dnl
dnl
define(PAC_TEST_MPIR_STATUS_SET_BYTES,[
  AC_MSG_CHECKING(if MPIR_Status_set_bytes is defined)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
     main(int argc, char **argv)
     {
     	 MPI_Status status;
         MPI_Datatype type;
	 int err;

         MPI_Init(&argc,&argv);
         MPIR_Status_set_bytes(status,type,err);
         MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_STATUS_SET_BYTES,,[Define if status set bytes available])
  else
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl
define(PAC_TEST_MPI_GREQUEST,[
  AC_MSG_CHECKING(support for generalized requests)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
#include "stdio.h"
    main(int argc, char **argv)
    {
       MPI_Request request;
       MPI_Init(&argc, &argv);
       MPI_Grequest_start(NULL, NULL, NULL, NULL, &request);
       MPI_Finalize();
     }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_MPI_GREQUEST,1,[Define if generalized requests available])
     DEFINE_HAVE_MPI_GREQUEST="#define HAVE_MPI_GREQUEST 1"
  else
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl

define(PAC_TEST_MPI_GREQUEST_EXTENSIONS,[
  AC_MSG_CHECKING(support for non-standard extended generalized requests)
  rm -f mpitest.c
  cat > mpitest.c <<EOF
#include "mpi.h"
#include "stdio.h"
    main(int argc, char **argv)
    {
       MPIX_Grequest_class classtest
    }
EOF
  rm -f conftest$EXEEXT
  $CC $USER_CFLAGS -I$MPI_INCLUDE_DIR -o conftest$EXEEXT mpitest.c $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_MPI_GREQUEST_EXTENTIONS,1,[Define if non-standard generalized requests extensions available])
     DEFINE_HAVE_MPI_GREQUEST_EXTENSIONS="#define HAVE_MPI_GREQUEST_EXTENSIONS 1"
  else
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.c
])dnl

define(PAC_TEST_NEEDS_CONST,[
   AC_MSG_CHECKING([const declarations needed in MPI routines])
   AC_COMPILE_IFELSE([AC_LANG_SOURCE(
   [ #include <mpi.h>
     int MPI_File_delete(char *filename, MPI_Info info) { return (0); }
   ] )],
   [
    AC_MSG_RESULT(no)
   ],[
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_MPIIO_CONST, const, [Define if MPI-IO routines need a const qualifier])
   ])
   ])
