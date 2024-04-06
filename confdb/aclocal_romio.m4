dnl
dnl This files contains additional macros for using autoconf to
dnl build configure scripts.
dnl
dnl Almost all of this file is taken from the aclocal.m4 of MPICH
dnl
dnl Get the format of Fortran names.  Uses F77, FFLAGS, and sets WDEF.
dnl If the test fails, sets NOF77 to 1, HAVE_FORTRAN to 0.
dnl
dnl
AC_DEFUN([PAC_GET_FORTNAMES],[
   rm -f confftest.f confftest.$OBJEXT
   cat > confftest.f <<EOF
       subroutine mpir_init_fop( a )
       integer a
       a = 1
       return
       end
EOF
   $F77 $FFLAGS -c confftest.f > /dev/null 2>&1
   if test ! -s confftest.$OBJEXT ; then
	AC_MSG_WARN([Unable to test Fortran compiler.  Compiling a test
program failed to produce an object file])
	NOF77=1
   elif test -z "$FORTRANNAMES" ; then
     # MAC OS X (and probably FreeBSD need strings - (not strings -a)
     # Cray doesn't accept -a ...
     allstrings="-a"
     if test $arch_CRAY ; then
	allstrings=""
     elif strings - confftest.$OBJEXT < /dev/null >/dev/null 2>&1 ; then
         allstrings="-"
     elif strings -a confftest.$OBJEXT < /dev/null >/dev/null 2>&1 ; then
         allstrings="-a"
     fi

     nameform1=`strings $allstrings confftest.$OBJEXT | grep mpir_init_fop_  | head -1`
     nameform2=`strings $allstrings confftest.$OBJEXT | grep MPIR_INIT_FOP   | head -1`
     nameform3=`strings $allstrings confftest.$OBJEXT | grep mpir_init_fop   | head -1`
     nameform4=`strings $allstrings confftest.$OBJEXT | grep mpir_init_fop__ | head -1`
    rm -f confftest.f confftest.$OBJEXT
    if test -n "$nameform4" ; then
	echo "Fortran externals are lower case and have two trailing underscores"
	FORTRANNAMES="FORTRANDOUBLEUNDERSCORE"
    elif test -n "$nameform1" ; then
        # We don't set this in CFLAGS; it is a default case
        echo "Fortran externals have a trailing underscore and are lowercase"
	FORTRANNAMES="FORTRANUNDERSCORE"
    elif test -n "$nameform2" ; then
	echo "Fortran externals are uppercase"
	FORTRANNAMES="FORTRANCAPS"
    elif test -n "$nameform3" ; then
	echo "Fortran externals are lower case"
	FORTRANNAMES="FORTRANNOUNDERSCORE"
    else
	AC_MSG_WARN([Unable to determine the form of Fortran external names.
Make sure that the compiler $F77 can be run on this system.
Turning off Fortran (-nof77 being assumed)])
	NOF77=1
    fi
    fi
    if test -n "$FORTRANNAMES" ; then
        WDEF="-D$FORTRANNAMES"
    fi
    # Delete confftest files with any extension.  This catches the case
    # where auxiliary files, such as coverage files, are removed.
    rm -f confftest.*
    ])dnl
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
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_LINK_IFELSE([AC_LANG_SOURCE([[
      #include <mpi.h>
      int main(int argc, char **argv)
      {
          MPI_Init(&argc,&argv);
          MPI_Finalize();
          return 0;
      }]])],
      [AC_MSG_RESULT(yes)],
      [AC_MSG_ERROR([Unable to compile a simple MPI program.
Use environment variables to provide the location of MPI libraries and
include directories])])
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
dnl
dnl
define(PAC_NEEDS_FINT,[
   saved_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   AC_CHECK_TYPE([MPI_Fint],
                 [NEEDS_MPI_FINT=""],
                 [NEEDS_MPI_FINT="#define NEEDS_MPI_FINT"
                  CPPFLAGS="$CPPFLAGS -DNEEDS_MPI_FINT"],
                 [[#include <mpi.h>]])
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
define(PAC_MPI_LONG_LONG_INT,[
   saved_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   AC_CHECK_DECL([MPI_LONG_LONG_INT],
                 [AC_DEFINE(HAVE_MPI_LONG_LONG_INT, 1,[Define if mpi has long long it])],
                 [], [[#include <mpi.h>]])
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
dnl PAC_LONG_LONG_64: check if there is a 64-bit long long
dnl
define(PAC_LONG_LONG_64,[
   AC_TYPE_LONG_LONG_INT
   AC_CHECK_SIZEOF(long long)
   if test "x$ac_cv_type_long_long_int" = xyes ; then
      if test "x$c_cv_sizeof_long_long" = x8 ; then
         AC_MSG_NOTICE([defining MPI_Offset as long long in C and integer*8 in Fortran])
         AC_DEFINE(HAVE_LONG_LONG_64, 1, [Define if long long is 64 bits])
         DEFINE_MPI_OFFSET="typedef long long MPI_Offset;"
         FORTRAN_MPI_OFFSET="integer*8"
         LL="\%lld"
      elif test "x$c_cv_sizeof_long_long" = x4 ; then # a hack to set MPI_Offset as int
         AC_MSG_NOTICE([defining MPI_Offset as int in C and integer in Fortran])
         DEFINE_MPI_OFFSET="typedef int MPI_Offset;"
         FORTRAN_MPI_OFFSET="integer"
         AC_DEFINE(MPI_OFFSET_IS_INT, 1, [Define if MPI_Offset is int])
         LL="\%d"
      else
         AC_MSG_NOTICE([defining MPI_Offset as long in C and integer in Fortran])
         DEFINE_MPI_OFFSET="typedef long MPI_Offset;"
         FORTRAN_MPI_OFFSET="integer"
         LL="\%ld"
      fi
   else
      AC_MSG_NOTICE([assuming long long is not available; use '-longlongsize' to indicate otherwise])
      AC_MSG_NOTICE([defining MPI_Offset as long in C and integer in Fortran])
      DEFINE_MPI_OFFSET="typedef long MPI_Offset;"
      FORTRAN_MPI_OFFSET="integer"
      LL="\%ld"
   fi
fi
])dnl
dnl
dnl
define(PAC_MPI_INFO,[
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_SEARCH_LIBS([MPI_Info_create], [mpi])
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
   if test "x$ac_cv_search_MPI_Info_create" = "xnone required" ; then
      AC_DEFINE(HAVE_MPI_INFO,1,[Define if MPI_Info available])
      HAVE_MPI_INFO="#define HAVE_MPI_INFO"
      MPI_FINFO1="!"
      MPI_FINFO2="!"
      MPI_FINFO3="!"
      MPI_FINFO4="!"
   else
      BUILD_MPI_INFO=1
      MPI_FINFO1="      INTEGER MPI_MAX_INFO_KEY, MPI_MAX_INFO_VAL"
      MPI_FINFO2="      PARAMETER (MPI_MAX_INFO_KEY=255, MPI_MAX_INFO_VAL=1024)"
      MPI_FINFO3="      INTEGER MPI_INFO_NULL"
      MPI_FINFO4="      PARAMETER (MPI_INFO_NULL=0)"
  fi
])dnl
dnl
dnl
define(PAC_MPI_DARRAY_SUBARRAY,[
   AC_MSG_CHECKING(if darray and subarray constructors are defined in the MPI implementation)
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_LINK_IFELSE([AC_LANG_SOURCE([[
      #include <mpi.h>
      int main(int argc, char **argv)
      {
          int i = MPI_DISTRIBUTE_CYCLIC;
          MPI_Datatype t;
          MPI_Init(&argc,&argv);
          MPI_Type_create_darray(i, i, i, &i, &i, &i, &i, i, MPI_INT, &t);
          MPI_Type_create_subarray(i, &i, &i, &i, i, MPI_INT, &t);
          MPI_Finalize();
          return 0;
      }]])],
      [ac_mpi_DARRAY_SUBARRAY=yes], [ac_mpi_DARRAY_SUBARRAY=no]
   )
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
   AC_MSG_RESULT([$ac_mpi_DARRAY_SUBARRAY])
   if test "x$ac_mpi_DARRAY_SUBARRAY" = xyes ; then
      AC_DEFINE(HAVE_MPI_DARRAY_SUBARRAY,,[Define if MPI Darray available])
      HAVE_MPI_DARRAY_SUBARRAY="#define HAVE_MPI_DARRAY_SUBARRAY"
      MPI_FARRAY1="!"
      MPI_FARRAY2="!"
      MPI_FARRAY3="!"
      MPI_FARRAY4="!"
      MPI_FARRAY5="!"
      MPI_FARRAY6="!"
      MPI_FARRAY7="!"
  else
      BUILD_MPI_ARRAY=1
      MPI_FARRAY1="      INTEGER MPI_ORDER_C, MPI_ORDER_FORTRAN"
      MPI_FARRAY2="      PARAMETER (MPI_ORDER_C=56, MPI_ORDER_FORTRAN=57)"
      MPI_FARRAY3="      INTEGER MPI_DISTRIBUTE_BLOCK, MPI_DISTRIBUTE_CYCLIC"
      MPI_FARRAY4="      INTEGER MPI_DISTRIBUTE_NONE, MPI_DISTRIBUTE_DFLT_DARG"
      MPI_FARRAY5="      PARAMETER (MPI_DISTRIBUTE_BLOCK=121, MPI_DISTRIBUTE_CYCLIC=122)"
      MPI_FARRAY6="      PARAMETER (MPI_DISTRIBUTE_NONE=123)"
      MPI_FARRAY7="      PARAMETER (MPI_DISTRIBUTE_DFLT_DARG=-49767)"
  fi
])dnl
dnl
dnl
define(PAC_CHECK_MPI_SGI_INFO_NULL,[
   saved_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   AC_CHECK_DECL([MPI_INFO_NULL],
                 [cp adio/sgi/mpi3.1/*.h include],
                 [], [[#include <mpi.h>]])
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
dnl
dnl
define(PAC_CHECK_MPIOF_H,[
   AC_MSG_CHECKING(if mpiof.h is included in mpif.h)
   AC_LANG_PUSH([Fortran 77])
   AC_COMPILE_IFELSE(
      [AC_LANG_SOURCE([
       program main
       implicit none
       include 'mpif.h'
       integer i
       i = MPI_MODE_RDWR
       stop
       end
   ])],
   [AC_MSG_RESULT([yes])
    MPIOF_H_INCLUDED=1],
   [AC_MSG_RESULT([no])])
   AC_LANG_POP([Fortran 77])
])dnl
dnl
dnl
dnl check if pread64 is defined in IRIX. needed on IRIX 6.5
dnl
define(PAC_HAVE_PREAD64,[
   # autoconf default includes unistd.h
   AC_CHECK_FUNC([pread64],
                 [AC_DEFINE(HAVE_PREAD64, 1, [Define if pread64 available])])
])dnl
dnl
dnl
define(PAC_TEST_MPI_SGI_type_is_contig,[
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_SEARCH_LIBS([MPI_SGI_type_is_contig], [mpi], [],
                [AC_DEFINE(NO_MPI_SGI_type_is_contig, 1, [Define if no MPI type is contig])])
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
dnl
dnl
define(PAC_TEST_MPI_COMBINERS,[
   saved_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   AC_CHECK_DECL([MPI_COMBINER_STRUCT],
                 [AC_DEFINE(HAVE_MPI_COMBINERS, 1, [Define if MPI combiners available])],
                 [], [[#include <mpi.h>]])
   CPPFLAGS=$saved_CPPFLAGS
])dnl
dnl
dnl
dnl PAC_MPI_OFFSET_KIND()
dnl
dnl tries to determine the Fortran 90 kind parameter for 8-byte integers
dnl
define(PAC_MPI_OFFSET_KIND,
[
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
# Determine the extension for Fortran 90 files (not all compilers accept
# .f and not all accept .f90)
if test -z "$ac_f90ext" ; then
    if test -z "$FC" ; then
       # This list should correspond to the list in aclocal_fc.m4
       AC_CHECK_PROGS(FC,ifort pgf90 pathf90 pathf95 xlf90 xlf95 f90 epcf90 \
       f95 fort lf95 gfortran g95 ifc efc)
    fi
    AC_MSG_CHECKING([for extension for Fortran 90 programs])
    ac_f90ext="f90"
    ac_f90compile='${FC-f90} -c $FCFLAGS conftest.$ac_f90ext 1>&AS_MESSAGE_LOG_FD'
    cat > conftest.$ac_f90ext <<EOF
      program conftest
      end
EOF
    if AC_TRY_EVAL(ac_f90compile) ; then
        AC_MSG_RESULT([f90])
    else
	# This is needed for Mac OSX 10.5
	rm -rf conftest.dSYM
        rm -f conftest*
        ac_f90ext="f"
        cat > conftest.$ac_f90ext <<EOF
      program conftest
      end
EOF
        if AC_TRY_EVAL(ac_f90compile) ; then
            AC_MSG_RESULT([f])
        else
            AC_MSG_RESULT([unknown!])
        fi
    fi
fi
AC_MSG_CHECKING([for Fortran 90 KIND parameter for 8-byte integers])
cat <<EOF > conftest.$ac_f90ext
      program main
      integer i
      i = selected_int_kind(16)
      open(8, file="conftest.out", form="formatted")
      write (8,*) i
      close(8)
      stop
      end
EOF
if test -z "$FC" ; then
   FC=f90
fi
KINDVAL=""
if $FC -o conftest$EXEEXT conftest.$ac_f90ext $FCFLAGS >/dev/null 2>&1 ; then
    ./conftest$EXEEXT >/dev/null 2>&1
    if test -s conftest.out ; then
        KINDVAL=`cat conftest.out`
    fi
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
if test -n "$KINDVAL" -a "$KINDVAL" != "-1" ; then
   AC_MSG_RESULT($KINDVAL)
   MPI_OFFSET_KIND1="      INTEGER MPI_OFFSET_KIND"
   MPI_OFFSET_KIND2="      PARAMETER (MPI_OFFSET_KIND=$KINDVAL)"
else
    AC_MSG_RESULT(unavailable)
fi
])dnl
dnl
dnl
define(PAC_TEST_MPI_HAVE_OFFSET_KIND,[
  AC_MSG_CHECKING(if MPI_OFFSET_KIND is defined in mpif.h)
  rm -f mpitest.f
  cat > mpitest.f <<EOF
      program main
      implicit none
      include 'mpif.h'
      integer i
      i = MPI_OFFSET_KIND
      stop
      end
EOF
  rm -f conftest$EXEEXT
  $F77 $FFLAGS $MPI_CPPFLAGS -o conftest$EXEEXT mpitest.f $MPI_LIB > /dev/null 2>&1
  if test -x conftest$EXEEXT ; then
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
  fi
  rm -f conftest$EXEEXT mpitest.f
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
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
      #include <sys/param.h>
      #include <sys/mount.h>]], [[int i=MOUNT_NFS;]])],
      [ac_have_MOUNT_NFS=yes], [ac_have_MOUNT_NFS=no]
   )
   AC_MSG_RESULT([$ac_have_MOUNT_NFS])
   if test "x$ac_have_MOUNT_NFS" = xyes ; then
      ROMIO_HAVE_MOUNT_NFS=1
      AC_DEFINE(HAVE_MOUNT_NFS, 1, [Define if MOUNT_NFS defined])
   else
      ROMIO_HAVE_MOUNT_NFS=0
   fi
])dnl
dnl
dnl
dnl PAC_MPI_OFFSET_KIND_4BYTE()
dnl
dnl tries to determine the Fortran 90 kind parameter for 4-byte integers
dnl
define(PAC_MPI_OFFSET_KIND_4BYTE,
[AC_MSG_CHECKING([for Fortran 90 KIND parameter for 4-byte integers])
rm -f kind.f kind.$OBJEXT kind$EXEEXT
cat <<EOF > kind.f
      program main
      integer i
      i = selected_int_kind(8)
      open(8, file="k.out", form="formatted")
      write (8,*) i
      close(8)
      stop
      end
EOF
if test -z "$FC" ; then
   FC=f90
fi
KINDVAL=""
if $FC -o kind$EXEEXT kind.f $FCFLAGS >/dev/null 2>&1 ; then
    ./kind >/dev/null 2>&1
    if test -s k.out ; then
        KINDVAL=`cat k.out`
    fi
fi
rm -f kind$EXEEXT k.out kind.f kind.$OBJEXT
if test -n "$KINDVAL" -a "$KINDVAL" != "-1" ; then
   AC_MSG_RESULT($KINDVAL)
   MPI_OFFSET_KIND1="      INTEGER MPI_OFFSET_KIND"
   MPI_OFFSET_KIND2="      PARAMETER (MPI_OFFSET_KIND=$KINDVAL)"
else
    AC_MSG_RESULT(unavailable)
fi
])dnl
dnl
dnl
define(PAC_FUNC_STRERROR,[
   AC_MSG_CHECKING([for strerror])
   AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <string.h>]],
                                   [[char *s = strerror(5);]])],
                  [ac_have_STRERROR=yes], [ac_have_STRERROR=no]
   )
   AC_MSG_RESULT([$ac_have_STRERROR])
   if test "x$ac_have_STRERROR" = xyes ; then
      AC_DEFINE(HAVE_STRERROR, 1, [Define if strerror available])
   else
      AC_MSG_CHECKING([for sys_errlist])
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdio.h>]], [[
         extern char *sys_errlist[];
	 printf("%s\n", sys_errlist[34]);]])],
         [ac_have_SYS_ERRLIST=yes], [ac_have_SYS_ERRLIST=no]
      )
      AC_MSG_RESULT([$ac_have_SYS_ERRLIST])
      if test "x$ac_have_SYS_ERRLIST" = xyes ; then
         AC_DEFINE(HAVE_SYSERRLIST, 1, [Define if syserrlist available])
      fi
   fi
])dnl

define(PAC_TEST_MPIR_STATUS_SET_BYTES,[
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_SEARCH_LIBS([MPIR_Status_set_bytes], [mpi],
                [AC_DEFINE(HAVE_STATUS_SET_BYTES,1,[Define if status set bytes available])])
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
])dnl

define(PAC_TEST_MPI_GREQUEST,[
   saved_CPPFLAGS=$CPPFLAGS
   saved_LDFLAGS=$LDFLAGS
   saved_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   LDFLAGS="$LDFLAGS $MPI_LDFLAGS"
   LIBS="$LIBS $MPI_LIB"
   AC_SEARCH_LIBS([MPI_Grequest_start], [mpi],
                [AC_DEFINE(HAVE_MPI_GREQUEST,1,[Define if generalized requests available])
                 DEFINE_HAVE_MPI_GREQUEST="#define HAVE_MPI_GREQUEST 1"])
   LIBS=$saved_LIBS
   LDFLAGS=$saved_LDFLAGS
   CPPFLAGS=$saved_CPPFLAGS
])dnl

define(PAC_TEST_MPI_GREQUEST_EXTENSIONS,[
   saved_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS $MPI_CPPFLAGS"
   AC_CHECK_TYPE([MPIX_Grequest_class],
                 [AC_DEFINE(HAVE_MPI_GREQUEST_EXTENTIONS,1,[Define if non-standard generalized requests extensions available])
                  DEFINE_HAVE_MPI_GREQUEST_EXTENSIONS="#define HAVE_MPI_GREQUEST_EXTENSIONS 1"],
                 [], [[#include <mpi.h>]])
   CPPFLAGS=$saved_CPPFLAGS
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
