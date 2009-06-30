dnl
dnl This is a replacement for AC_PROG_CC that does not prefer gcc and
dnl that does not mess with CFLAGS.  See acspecific.m4 for the original defn.
dnl
dnl/*D
dnl PAC_PROG_CC - Find a working C compiler
dnl
dnl Synopsis:
dnl PAC_PROG_CC
dnl
dnl Output Effect:
dnl   Sets the variable CC if it is not already set
dnl
dnl Notes:
dnl   Unlike AC_PROG_CC, this does not prefer gcc and does not set CFLAGS.
dnl   It does check that the compiler can compile a simple C program.
dnl   It also sets the variable GCC to yes if the compiler is gcc.  It does
dnl   not yet check for some special options needed in particular for 
dnl   parallel computers, such as -Tcray-t3e, or special options to get
dnl   full ANSI/ISO C, such as -Aa for HP.
dnl
dnl D*/
dnl 2.52 doesn't have AC_PROG_CC_GNU
ifdef([AC_PROG_CC_GNU],,[AC_DEFUN([AC_PROG_CC_GNU],)])
AC_DEFUN([PAC_PROG_CC],[
AC_PROVIDE([AC_PROG_CC])
AC_CHECK_PROGS(CC, cc xlC xlc pgcc icc pathcc gcc )
test -z "$CC" && AC_MSG_ERROR([no acceptable cc found in \$PATH])
PAC_PROG_CC_WORKS
AC_PROG_CC_GNU
if test "$ac_cv_prog_gcc" = yes; then
  GCC=yes
else
  GCC=
fi
])
dnl
dnl/*D
dnl PAC_C_CHECK_COMPILER_OPTION - Check that a compiler option is accepted
dnl without warning messages
dnl
dnl Synopsis:
dnl PAC_C_CHECK_COMPILER_OPTION(optionname,action-if-ok,action-if-fail)
dnl
dnl Output Effects:
dnl
dnl If no actions are specified, a working value is added to 'COPTIONS'
dnl
dnl Notes:
dnl This is now careful to check that the output is different, since 
dnl some compilers are noisy.
dnl 
dnl We are extra careful to prototype the functions in case compiler options
dnl that complain about poor code are in effect.
dnl
dnl Because this is a long script, we have ensured that you can pass a 
dnl variable containing the option name as the first argument.
dnl D*/
AC_DEFUN([PAC_C_CHECK_COMPILER_OPTION],[
AC_MSG_CHECKING([whether C compiler accepts option $1])
save_CFLAGS="$CFLAGS"
CFLAGS="$1 $CFLAGS"
rm -f conftest.out
echo 'int foo(void);int foo(void){return 0;}' > conftest2.c
echo 'int main(void);int main(void){return 0;}' > conftest.c
if ${CC-cc} $save_CFLAGS $CPPFLAGS -o conftest conftest.c $LDFLAGS >conftest.bas 2>&1 ; then
   if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest.c $LDFLAGS >conftest.out 2>&1 ; then
      if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
         AC_MSG_RESULT(yes)
         AC_MSG_CHECKING([whether routines compiled with $1 can be linked with ones compiled without $1])       
         rm -f conftest.out
         rm -f conftest.bas
         if ${CC-cc} -c $save_CFLAGS $CPPFLAGS conftest2.c >conftest2.out 2>&1 ; then
            if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest2.o conftest.c $LDFLAGS >conftest.bas 2>&1 ; then
               if ${CC-cc} $CFLAGS $CPPFLAGS -o conftest conftest2.o conftest.c $LDFLAGS >conftest.out 2>&1 ; then
                  if diff -b conftest.out conftest.bas >/dev/null 2>&1 ; then
	             AC_MSG_RESULT(yes)	  
		     CFLAGS="$save_CFLAGS"
                     ifelse($2,,COPTIONS="$COPTIONS $1",$2)
                  elif test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	             AC_MSG_RESULT(no)
                     CFLAGS="$save_CFLAGS"
	             $3
                  else
                     AC_MSG_RESULT(no)
                     CFLAGS="$save_CFLAGS"
	             $3
                  fi  
               else
	          if test -s conftest.out ; then
	             cat conftest.out >&AC_FD_CC
	          fi
                  AC_MSG_RESULT(no)
                  CFLAGS="$save_CFLAGS"
                  $3
               fi
	    else
               # Could not link with the option!
               AC_MSG_RESULT(no)
            fi
         else
            if test -s conftest2.out ; then
               cat conftest2.out >&AC_FD_CC
            fi
	    AC_MSG_RESULT(no)
            CFLAGS="$save_CFLAGS"
	    $3
         fi
      else
         cat conftest.out >&AC_FD_CC
         AC_MSG_RESULT(no)
         $3
         CFLAGS="$save_CFLAGS"         
      fi
   else
      AC_MSG_RESULT(no)
      $3
      if test -s conftest.out ; then cat conftest.out >&AC_FD_CC ; fi    
      CFLAGS="$save_CFLAGS"
   fi
else
    # Could not compile without the option!
    AC_MSG_RESULT(no)
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
])
dnl
dnl/*D
dnl PAC_C_OPTIMIZATION - Determine C options for producing optimized code
dnl
dnl Synopsis
dnl PAC_C_OPTIMIZATION([action if found])
dnl
dnl Output Effect:
dnl Adds options to 'COPTIONS' if no other action is specified
dnl 
dnl Notes:
dnl This is a temporary standin for compiler optimization.
dnl It should try to match known systems to known compilers (checking, of
dnl course), and then falling back to some common defaults.
dnl Note that many compilers will complain about -g and aggressive
dnl optimization.  
dnl D*/
AC_DEFUN([PAC_C_OPTIMIZATION],[
    for copt in "-O4 -Ofast" "-Ofast" "-fast" "-O3" "-xO3" "-O" ; do
        PAC_C_CHECK_COMPILER_OPTION($copt,found_opt=yes,found_opt=no)
        if test "$found_opt" = "yes" ; then
	    ifelse($1,,COPTIONS="$COPTIONS $copt",$1)
	    break
        fi
    done
    if test "$ac_cv_prog_gcc" = "yes" ; then
	for copt in "-fomit-frame-pointer" "-finline-functions" \
		 "-funroll-loops" ; do
	    PAC_C_CHECK_COMPILER_OPTION($copt,found_opt=yes,found_opt=no)
	    if test $found_opt = "yes" ; then
	        ifelse($1,,COPTIONS="$COPTIONS $copt",$1)
	        # no break because we're trying to add them all
	    fi
	done
	# We could also look for architecture-specific gcc options
    fi

])
dnl
dnl/*D
dnl PAC_C_DEPENDS - Determine how to use the C compiler to generate 
dnl dependency information
dnl
dnl Synopsis:
dnl PAC_C_DEPENDS
dnl
dnl Output Effects:
dnl Sets the following shell variables and call AC_SUBST for them:
dnl+ C_DEPEND_OPT - Compiler options needed to create dependencies
dnl. C_DEPEND_OUT - Shell redirection for dependency file (may be empty)
dnl. C_DEPEND_PREFIX - Empty (null) or true; this is used to handle
dnl  systems that do not provide dependency information
dnl- C_DEPEND_MV - Command to move created dependency file
dnl Also creates a Depends file in the top directory (!).
dnl
dnl In addition, the variable 'C_DEPEND_DIR' must be set to indicate the
dnl directory in which the dependency files should live.  
dnl
dnl Notes:
dnl A typical Make rule that exploits this macro is
dnl.vb
dnl #
dnl # Dependency processing
dnl .SUFFIXES: .dep
dnl DEP_SOURCES = ${SOURCES:%.c=.dep/%.dep}
dnl C_DEPEND_DIR = .dep
dnl Depends: ${DEP_SOURCES}
dnl         @-rm -f Depends
dnl         cat .dep/*.dep >Depends
dnl .dep/%.dep:%.c
dnl	    @if [ ! -d .dep ] ; then mkdir .dep ; fi
dnl         @@C_DEPEND_PREFIX@ ${C_COMPILE} @C_DEPEND_OPT@ $< @C_DEPEND_OUT@
dnl         @@C_DEPEND_MV@
dnl
dnl depends-clean:
dnl         @-rm -f *.dep ${srcdir}/*.dep Depends ${srcdir}/Depends
dnl         @-touch Depends
dnl.ve
dnl
dnl For each file 'foo.c', this creates a file 'foo.dep' and creates a file
dnl 'Depends' that contains all of the '*.dep' files.
dnl
dnl For your convenience, the autoconf variable 'C_DO_DEPENDS' names a file 
dnl that may contain this code (you must have `dependsrule` or 
dnl `dependsrule.in` in the same directory as the other auxillery configure 
dnl scripts (set with dnl 'AC_CONFIG_AUX_DIR').  If you use `dependsrule.in`,
dnl you must have `dependsrule` in 'AC_OUTPUT' before this `Makefile`.
dnl 
dnl D*/
dnl 
dnl Eventually, we can add an option to the C_DEPEND_MV to strip system
dnl includes, such as /usr/xxxx and /opt/xxxx
dnl
AC_DEFUN([PAC_C_DEPENDS],[
AC_SUBST(C_DEPEND_OPT)AM_IGNORE(C_DEPEND_OPT)
AC_SUBST(C_DEPEND_OUT)AM_IGNORE(C_DEPEND_OUT)
AC_SUBST(C_DEPEND_MV)AM_IGNORE(C_DEPEND_MV)
AC_SUBST(C_DEPEND_PREFIX)AM_IGNORE(C_DEPEND_PREFIX)
AC_SUBST_FILE(C_DO_DEPENDS) 
dnl set the value of the variable to a 
dnl file that contains the dependency code, such as
dnl ${top_srcdir}/maint/dependrule 
if test -n "$ac_cv_c_depend_opt" ; then
    AC_MSG_RESULT([Option $ac_cv_c_depend_opt creates dependencies (cached)])
    C_DEPEND_OUT="$ac_cv_c_depend_out"
    C_DEPEND_MV="$ac_cv_c_depend_mv"
    C_DEPEND_OPT="$ac_cv_c_depend_opt"
    C_DEPEND_PREFIX="$ac_cv_c_depend_prefix"
    C_DO_DEPENDS="$ac_cv_c_do_depends"
else
   # Determine the values
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
dnl
dnl Some systems (/usr/ucb/cc on Solaris) do not generate a dependency for
dnl an include that doesn't begin in column 1
dnl
cat >conftest.c <<EOF
    #include "confdefs.h"
    int f(void) { return 0; }
EOF
dnl -xM1 is Solaris C compiler (no /usr/include files)
dnl -MM is gcc (no /usr/include files)
dnl -MMD is gcc to .d
dnl .u is xlC (AIX) output
for copt in "-xM1" "-c -xM1" "-xM" "-c -xM" "-MM" "-M" "-c -M"; do
    AC_MSG_CHECKING([whether $copt option generates dependencies])
    rm -f conftest.o conftest.u conftest.d conftest.err conftest.out
    dnl also need to check that error output is empty
    if $CC $CFLAGS $copt conftest.c >conftest.out 2>conftest.err && \
	test ! -s conftest.err ; then
        dnl Check for dependency info in conftest.out
        if test -s conftest.u ; then 
	    C_DEPEND_OUT=""
	    C_DEPEND_MV='mv $[*].u ${C_DEPEND_DIR}/$[*].dep'
            pac_dep_file=conftest.u 
        elif test -s conftest.d ; then
	    C_DEPEND_OUT=""
	    C_DEPEND_MV='mv $[*].d ${C_DEPEND_DIR}/$[*].dep'
            pac_dep_file=conftest.d 
        else
	    dnl C_DEPEND_OUT='>${C_DEPEND_DIR}/$[*].dep'
	    dnl This for is needed for VPATH.  Perhaps the others should match.
	    C_DEPEND_OUT='>$@'
	    C_DEPEND_MV=:
            pac_dep_file=conftest.out
        fi
        if grep 'confdefs.h' $pac_dep_file >/dev/null 2>&1 ; then
            AC_MSG_RESULT(yes)
	    C_DEPEND_OPT="$copt"
	    AC_MSG_CHECKING([whether .o file created with dependency file])
	    if test -s conftest.o ; then
	        AC_MSG_RESULT(yes)
	    else
                AC_MSG_RESULT(no)
		echo "Output of $copt option was" >&AC_FD_CC
		cat $pac_dep_file >&AC_FD_CC
            fi
	    break
        else
	    AC_MSG_RESULT(no)
        fi
    else
	echo "Error in compiling program with flags $copt" >&AC_FD_CC
	cat conftest.out >&AC_FD_CC
	if test -s conftest.err ; then cat conftest.err >&AC_FD_CC ; fi
	AC_MSG_RESULT(no)
    fi
    copt=""
done
    if test -f $CONFIG_AUX_DIR/dependsrule -o \
	    -f $CONFIG_AUX_DIR/dependsrule.in; then
	C_DO_DEPENDS="$CONFIG_AUX_DIR/dependsrule"
    else 
	C_DO_DEPENDS="/dev/null"
    fi
    if test "X$copt" = "X" ; then
        C_DEPEND_PREFIX="true"
    else
        C_DEPEND_PREFIX=""
    fi
    ac_cv_c_depend_out="$C_DEPEND_OUT"
    ac_cv_c_depend_mv="$C_DEPEND_MV"
    ac_cv_c_depend_opt="$C_DEPEND_OPT"
    ac_cv_c_depend_prefix="$C_DEPEND_PREFIX"
    ac_cv_c_do_depends="$C_DO_DEPENDS"
fi
])
dnl
dnl/*D 
dnl PAC_C_PROTOTYPES - Check that the compiler accepts ANSI prototypes.  
dnl
dnl Synopsis:
dnl PAC_C_PROTOTYPES([action if true],[action if false])
dnl
dnl D*/
AC_DEFUN([PAC_C_PROTOTYPES],[
AC_CACHE_CHECK([whether $CC supports function prototypes],
pac_cv_c_prototypes,[
AC_TRY_COMPILE([int f(double a){return 0;}],[return 0];,
pac_cv_c_prototypes="yes",pac_cv_c_prototypes="no")])
if test "$pac_cv_c_prototypes" = "yes" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])dnl
dnl
dnl/*D
dnl PAC_FUNC_SEMCTL - Check for semctl and its argument types
dnl
dnl Synopsis:
dnl PAC_FUNC_SEMCTL
dnl
dnl Output Effects:
dnl Sets 'HAVE_SEMCTL' if semctl is available.
dnl Sets 'HAVE_UNION_SEMUN' if 'union semun' is available.
dnl Sets 'SEMCTL_NEEDS_SEMUN' if a 'union semun' type must be passed as the
dnl fourth argument to 'semctl'.
dnl D*/ 
dnl Check for semctl and arguments
AC_DEFUN([PAC_FUNC_SEMCTL],[
AC_CHECK_FUNC(semctl)
if test "$ac_cv_func_semctl" = "yes" ; then
    AC_CACHE_CHECK([for union semun],
    pac_cv_type_union_semun,[
    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>],[union semun arg;arg.val=0;],
    pac_cv_type_union_semun="yes",pac_cv_type_union_semun="no")])
    if test "$pac_cv_type_union_semun" = "yes" ; then
        AC_DEFINE(HAVE_UNION_SEMUN,1,[Has union semun])
        #
        # See if we can use an int in semctl or if we need the union
        AC_CACHE_CHECK([whether semctl needs union semun],
        pac_cv_func_semctl_needs_semun,[
        AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>],[
int arg = 0; semctl( 1, 1, SETVAL, arg );],
        pac_cv_func_semctl_needs_semun="yes",
        pac_cv_func_semctl_needs_semun="no")
        ])
        if test "$pac_cv_func_semctl_needs_semun" = "yes" ; then
            AC_DEFINE(SEMCTL_NEEDS_SEMUN,1,[Needs an explicit definition of semun])
        fi
    fi
fi
])
dnl
dnl/*D
dnl PAC_C_VOLATILE - Check if C supports volatile
dnl
dnl Synopsis:
dnl PAC_C_VOLATILE
dnl
dnl Output Effect:
dnl Defines 'volatile' as empty if volatile is not available.
dnl
dnl D*/
AC_DEFUN([PAC_C_VOLATILE],[
AC_CACHE_CHECK([for volatile],
pac_cv_c_volatile,[
AC_TRY_COMPILE(,[volatile int a;],pac_cv_c_volatile="yes",
pac_cv_c_volatile="no")])
if test "$pac_cv_c_volatile" = "no" ; then
    AC_DEFINE(volatile,,[if C does not support volatile])
fi
])dnl
dnl
dnl/*D
dnl PAC_C_INLINE - Check if C supports inline
dnl
dnl Synopsis:
dnl PAC_C_INLINE
dnl
dnl Output Effect:
dnl Defines 'inline' as empty if inline is not available.
dnl
dnl D*/
AC_DEFUN([PAC_C_INLINE],[
AC_CACHE_CHECK([for inline],
pac_cv_c_inline,[
AC_TRY_COMPILE([inline int a( int b ){return b+1;}],[int a;],
pac_cv_c_inline="yes",pac_cv_c_inline="no")])
if test "$pac_cv_c_inline" = "no" ; then
    AC_DEFINE(inline,,[if C does not support inline])
fi
])dnl
dnl
dnl/*D
dnl PAC_C_CONST - Check if C supports const 
dnl
dnl Synopsis:
dnl PAC_C_CONST
dnl
dnl Output Effect:
dnl AC_MSG_ERROR if const is not supported.
dnl
dnl D*/
dnl AC_DEFUN(PAC_C_CONST,[
dnl AC_CACHE_CHECK([for support of const in C],
dnl pac_cv_c_const,[
dnl AC_LANG_PUSH(C)
dnl AC_TRY_COMPILE(,[const int a = 1; int b; b = a;],
dnl pac_cv_c_const="yes", pac_cv_c_const="no")])
dnl if test "$pac_cv_c_const" = "no" ; then
dnl     AC_MSG_ERROR([C does not support const! Abort...])
dnl fi
dnl AC_LANG_POP(C)
dnl ])dnl
AC_DEFUN([PAC_C_CONST],
[AC_CACHE_CHECK([for an ANSI C-conforming const], pac_cv_c_const,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[/* FIXME: Include the comments suggested by Paul. */
#ifndef __cplusplus
  /* Ultrix mips cc rejects this.  */
  typedef int charset[2];
  const charset cs = {0,0};
  /* SunOS 4.1.1 cc rejects this.  */
  char const *const *pcpcc;
  char **ppc;
  /* NEC SVR4.0.2 mips cc rejects this.  */
  struct point {int x, y;};
  static struct point const zero = {0,0};
  /* AIX XL C 1.02.0.0 rejects this.
     It does not let you subtract one const X* pointer from another in
     an arm of an if-expression whose if-part is not a constant
     expression */
  const char *g = "string";
  pcpcc = &g + (g ? g-g : 0);
  /* HPUX 7.0 cc rejects these. */
  ++pcpcc;
  ppc = (char**) pcpcc;
  pcpcc = (char const *const *) ppc;
  { /* SCO 3.2v4 cc rejects this.  */
    char const *s = 0 ? (char *) 0 : (char const *) 0;
    if (s) return 0;
  }
  { /* Someone thinks the Sun supposedly-ANSI compiler will reject this.  */
    int x[] = {25, 17};
    const int *foo = &x[0];
    ++foo;
  }
  { /* Sun SC1.0 ANSI compiler rejects this -- but not the above. */
    typedef const int *iptr;
    iptr p = 0;
    ++p;
  }
  { /* AIX XL C 1.02.0.0 rejects this saying
       "k.c", line 2.27: 1506-025 (S) Operand must be a modifiable lvalue. */
    struct s { int j; const int *ap[3]; };
    struct s a;
    struct s *b = &a;
    b->j = 5; 
  }
  { /* ULTRIX-32 V3.1 (Rev 9) vcc rejects this */
    const int foo = 10;
    if (!foo) return 0;
  }
  return !cs[0] && !zero.x;
#endif
]])],
                   [pac_cv_c_const=yes],
                   [pac_cv_c_const=no])])
if test $pac_cv_c_const = no; then
  AC_DEFINE(const,,
            [Define to empty if `const' does not conform to ANSI C.])
fi
])dnl
dnl
dnl/*D
dnl PAC_C_CPP_CONCAT - Check whether the C compiler accepts ISO CPP string
dnl   concatenation
dnl
dnl Synopsis:
dnl PAC_C_CPP_CONCAT([true-action],[false-action])
dnl
dnl Output Effects:
dnl Invokes the true or false action
dnl
dnl D*/
AC_DEFUN([PAC_C_CPP_CONCAT],[
pac_pound="#"
AC_CACHE_CHECK([whether the compiler $CC accepts $ac_pound$ac_pound for concatenation in cpp],
pac_cv_c_cpp_concat,[
AC_TRY_COMPILE([
#define concat(a,b) a##b],[int concat(a,b);return ab;],
pac_cv_cpp_concat="yes",pac_cv_cpp_concat="no")])
if test $pac_cv_c_cpp_concat = "yes" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])dnl
dnl
dnl/*D
dnl PAC_FUNC_GETTIMEOFDAY - Check whether gettimeofday takes 1 or 2 arguments
dnl
dnl Synopsis
dnl  PAC_IS_GETTIMEOFDAY_OK(ok_action,failure_action)
dnl
dnl Notes:
dnl One version of Solaris accepted only one argument.
dnl
dnl D*/
AC_DEFUN([PAC_FUNC_GETTIMEOFDAY],[
AC_CACHE_CHECK([whether gettimeofday takes 2 arguments],
pac_cv_func_gettimeofday,[
AC_TRY_COMPILE([#include <sys/time.h>],[struct timeval tp;
gettimeofday(&tp,(void*)0);return 0;],pac_cv_func_gettimeofday="yes",
pac_cv_func_gettimeofday="no")
])
if test "$pac_cv_func_gettimeofday" = "yes" ; then
     ifelse($1,,:,$1)
else
     ifelse($2,,:,$2)
fi
])
dnl
dnl/*D
dnl PAC_C_RESTRICT - Check if C supports restrict
dnl
dnl Synopsis:
dnl PAC_C_RESTRICT
dnl
dnl Output Effect:
dnl Defines 'restrict' if some version of restrict is supported; otherwise
dnl defines 'restrict' as empty.  This allows you to include 'restrict' in 
dnl declarations in the same way that 'AC_C_CONST' allows you to use 'const'
dnl in declarations even when the C compiler does not support 'const'
dnl
dnl Note that some compilers accept restrict only with additional options.
dnl DEC/Compaq/HP Alpha Unix (Tru64 etc.) -accept restrict_keyword
dnl
dnl D*/
AC_DEFUN([PAC_C_RESTRICT],[
AC_CACHE_CHECK([for restrict],
pac_cv_c_restrict,[
AC_TRY_COMPILE(,[int * restrict a;],pac_cv_c_restrict="restrict",
pac_cv_c_restrict="no")
if test "$pac_cv_c_restrict" = "no" ; then
   AC_TRY_COMPILE(,[int * _Restrict a;],pac_cv_c_restrict="_Restrict",
   pac_cv_c_restrict="no")
fi
if test "$pac_cv_c_restrict" = "no" ; then
   AC_TRY_COMPILE(,[int * __restrict a;],pac_cv_c_restrict="__restrict",
   pac_cv_c_restrict="no")
fi
])
if test "$pac_cv_c_restrict" = "no" ; then
  restrict_val=""
elif test "$pac_cv_c_restrict" != "restrict" ; then
  restrict_val=$pac_cv_c_restrict
fi
if test "$restrict_val" != "restrict" ; then 
  AC_DEFINE_UNQUOTED(restrict,$restrict_val,[if C does not support restrict])
fi
])dnl
dnl
dnl/*D
dnl PAC_HEADER_STDARG - Check whether standard args are defined and whether
dnl they are old style or new style
dnl
dnl Synopsis:
dnl PAC_HEADER_STDARG(action if works, action if oldstyle, action if fails)
dnl
dnl Output Effects:
dnl Defines HAVE_STDARG_H if the header exists.
dnl defines 
dnl
dnl Notes:
dnl It isn't enough to check for stdarg.  Even gcc doesn't get it right;
dnl on some systems, the gcc version of stdio.h loads stdarg.h `with the wrong
dnl options` (causing it to choose the `old style` 'va_start' etc).
dnl
dnl The original test tried the two-arg version first; the old-style
dnl va_start took only a single arg.
dnl This turns out to be VERY tricky, because some compilers (e.g., Solaris) 
dnl are quite happy to accept the *wrong* number of arguments to a macro!
dnl Instead, we try to find a clean compile version, using our special
dnl PAC_C_TRY_COMPILE_CLEAN command.
dnl
dnl D*/
AC_DEFUN([PAC_HEADER_STDARG],[
AC_CHECK_HEADER(stdarg.h)
dnl Sets ac_cv_header_stdarg_h
if test "$ac_cv_header_stdarg_h" = "yes" ; then
    dnl results are yes,oldstyle,no.
    AC_CACHE_CHECK([whether stdarg is oldstyle],
    pac_cv_header_stdarg_oldstyle,[
PAC_C_TRY_COMPILE_CLEAN([#include <stdio.h>
#include <stdarg.h>],
[int func( int a, ... ){
int b;
va_list ap;
va_start( ap );
b = va_arg(ap, int);
printf( "%d-%d\n", a, b );
va_end(ap);
fflush(stdout);
return 0;
}
int main() { func( 1, 2 ); return 0;}],pac_check_compile)
case "$pac_check_compile" in 
    0)  pac_cv_header_stdarg_oldstyle="yes"
	;;
    1)  pac_cv_header_stdarg_oldstyle="may be newstyle"
	;;
    2)  pac_cv_header_stdarg_oldstyle="no"   # compile failed
	;;
esac
])
if test "$pac_cv_header_stdarg_oldstyle" = "yes" ; then
    ifelse($2,,:,[$2])
else
    AC_CACHE_CHECK([whether stdarg works],
    pac_cv_header_stdarg_works,[
    PAC_C_TRY_COMPILE_CLEAN([
#include <stdio.h>
#include <stdarg.h>],[
int func( int a, ... ){
int b;
va_list ap;
va_start( ap, a );
b = va_arg(ap, int);
printf( "%d-%d\n", a, b );
va_end(ap);
fflush(stdout);
return 0;
}
int main() { func( 1, 2 ); return 0;}],pac_check_compile)
case "$pac_check_compile" in 
    0)  pac_cv_header_stdarg_works="yes"
	;;
    1)  pac_cv_header_stdarg_works="yes with warnings"
	;;
    2)  pac_cv_header_stdarg_works="no"
	;;
esac
])
fi   # test on oldstyle
if test "$pac_cv_header_stdarg_works" = "no" ; then
    ifelse($3,,:,[$3])
else
    ifelse($1,,:,[$1])
fi
else 
    ifelse($3,,:,[$3])
fi  # test on header
])
dnl/*D
dnl PAC_C_TRY_COMPILE_CLEAN - Try to compile a program, separating success
dnl with no warnings from success with warnings.
dnl
dnl Synopsis:
dnl PAC_C_TRY_COMPILE_CLEAN(header,program,flagvar)
dnl
dnl Output Effect:
dnl The 'flagvar' is set to 0 (clean), 1 (dirty but success ok), or 2
dnl (failed).
dnl
dnl D*/
AC_DEFUN([PAC_C_TRY_COMPILE_CLEAN],[
$3=2
dnl Get the compiler output to test against
if test -z "$pac_TRY_COMPLILE_CLEAN" ; then
    # This is needed for Mac OSX 10.5
    rm -rf conftest.dSYM
    rm -f conftest*
    echo 'int try(void);int try(void){return 0;}' > conftest.c
    if ${CC-cc} $CFLAGS -c conftest.c >conftest.bas 2>&1 ; then
	if test -s conftest.bas ; then 
	    pac_TRY_COMPILE_CLEAN_OUT=`cat conftest.bas`
        fi
        pac_TRY_COMPILE_CLEAN=1
    else
	AC_MSG_WARN([Could not compile simple test program!])
	if test -s conftest.bas ; then 	cat conftest.bas >> config.log ; fi
    fi
fi
dnl
dnl Create the program that we need to test with
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
cat >conftest.c <<EOF
#include "confdefs.h"
[$1]
[$2]
EOF
dnl
dnl Compile it and test
if ${CC-cc} $CFLAGS -c conftest.c >conftest.bas 2>&1 ; then
    dnl Success.  Is the output the same?
    if test "$pac_TRY_COMPILE_CLEAN_OUT" = "`cat conftest.bas`" ; then
	$3=0
    else
        cat conftest.c >>config.log
	if test -s conftest.bas ; then 	cat conftest.bas >> config.log ; fi
        $3=1
    fi
else
    dnl Failure.  Set flag to 2
    cat conftest.c >>config.log
    if test -s conftest.bas ; then cat conftest.bas >> config.log ; fi
    $3=2
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
])
dnl
dnl/*D
dnl PAC_PROG_C_UNALIGNED_DOUBLES - Check that the C compiler allows unaligned
dnl doubles
dnl
dnl Synopsis:
dnl   PAC_PROG_C_UNALIGNED_DOUBLES(action-if-true,action-if-false,
dnl       action-if-unknown)
dnl
dnl Notes:
dnl 'action-if-unknown' is used in the case of cross-compilation.
dnl D*/
AC_DEFUN([PAC_PROG_C_UNALIGNED_DOUBLES],[
AC_CACHE_CHECK([whether C compiler allows unaligned doubles],
pac_cv_prog_c_unaligned_doubles,[
AC_TRY_RUN([
void fetch_double( v )
double *v;
{
*v = 1.0;
}
int main( argc, argv )
int argc;
char **argv;
{
int p[4];
double *p_val;
fetch_double( (double *)&(p[0]) );
p_val = (double *)&(p[0]);
if (*p_val != 1.0) return 1;
fetch_double( (double *)&(p[1]) );
p_val = (double *)&(p[1]);
if (*p_val != 1.0) return 1;
return 0;
}
],pac_cv_prog_c_unaligned_doubles="yes",pac_cv_prog_c_unaligned_doubles="no",
pac_cv_prog_c_unaligned_doubles="unknown")])
ifelse($1,,,if test "X$pac_cv_prog_c_unaligned_doubles" = "yes" ; then 
$1
fi)
ifelse($2,,,if test "X$pac_cv_prog_c_unaligned_doubles" = "no" ; then 
$2
fi)
ifelse($3,,,if test "X$pac_cv_prog_c_unaligned_doubles" = "unknown" ; then 
$3
fi)
])
dnl
dnl/*D 
dnl PAC_PROG_C_WEAK_SYMBOLS - Test whether C supports weak alias symbols.
dnl
dnl Synopsis
dnl PAC_PROG_C_WEAK_SYMBOLS(action-if-true,action-if-false)
dnl
dnl Output Effect:
dnl Defines one of the following if a weak symbol pragma is found:
dnl.vb
dnl    HAVE_PRAGMA_WEAK - #pragma weak
dnl    HAVE_PRAGMA_HP_SEC_DEF - #pragma _HP_SECONDARY_DEF
dnl    HAVE_PRAGMA_CRI_DUP  - #pragma _CRI duplicate x as y
dnl.ve
dnl May also define
dnl.vb
dnl    HAVE_WEAK_ATTRIBUTE
dnl.ve
dnl if functions can be declared as 'int foo(...) __attribute__ ((weak));'
dnl sets the shell variable pac_cv_attr_weak to yes.
dnl Also checks for __attribute__((weak_import)) which is supported by
dnl Apple in Mac OSX (at least in Darwin).  Note that this provides only
dnl weak symbols, not weak aliases
dnl 
dnl D*/
AC_DEFUN([PAC_PROG_C_WEAK_SYMBOLS],[
pragma_extra_message=""
AC_CACHE_CHECK([for type of weak symbol alias support],
pac_cv_prog_c_weak_symbols,[
# Test for weak symbol support...
# We can't put # in the message because it causes autoconf to generate
# incorrect code
AC_TRY_LINK([
extern int PFoo(int);
#pragma weak PFoo = Foo
int Foo(int a) { return a; }
],[return PFoo(1);],has_pragma_weak=yes)
#
# Some systems (Linux ia64 and ecc, for example), support weak symbols
# only within a single object file!  This tests that case.
# Note that there is an extern int PFoo declaration before the
# pragma.  Some compilers require this in order to make the weak symbol
# extenally visible.  
if test "$has_pragma_weak" = yes ; then
    # This is needed for Mac OSX 10.5
    rm -rf conftest.dSYM
    rm -f conftest*
    cat >>conftest1.c <<EOF
extern int PFoo(int);
#pragma weak PFoo = Foo
int Foo(int);
int Foo(int a) { return a; }
EOF
    cat >>conftest2.c <<EOF
extern int PFoo(int);
int main(int argc, char **argv) {
return PFoo(0);}
EOF
    ac_link2='${CC-cc} -o conftest $CFLAGS $CPPFLAGS $LDFLAGS conftest1.c conftest2.c $LIBS >conftest.out 2>&1'
    if eval $ac_link2 ; then
        # The gcc 3.4.x compiler accepts the pragma weak, but does not
        # correctly implement it on systems where the loader doesn't 
        # support weak symbols (e.g., cygwin).  This is a bug in gcc, but it
        # it is one that *we* have to detect.
	# This is needed for Mac OSX 10.5
	rm -rf conftest.dSYM
        rm -f conftest*
        cat >>conftest1.c <<EOF
extern int PFoo(int);
#pragma weak PFoo = Foo
int Foo(int);
int Foo(int a) { return a; }
EOF
    cat >>conftest2.c <<EOF
extern int Foo(int);
int PFoo(int a) { return a+1;}
int main(int argc, char **argv) {
return Foo(0);}
EOF
        if eval $ac_link2 ; then
            pac_cv_prog_c_weak_symbols="pragma weak"
        else 
            echo "$ac_link2" >> config.log
	    echo "Failed program was" >> config.log
            cat conftest1.c >>config.log
            cat conftest2.c >>config.log
            if test -s conftest.out ; then cat conftest.out >> config.log ; fi
            has_pragma_weak=0
            pragma_extra_message="pragma weak accepted but does not work (probably creates two non-weak entries)"
        fi
    else
      echo "$ac_link2" >>config.log
      echo "Failed program was" >>config.log
      cat conftest1.c >>config.log
      cat conftest2.c >>config.log
      if test -s conftest.out ; then cat conftest.out >> config.log ; fi
      has_pragma_weak=0
      pragma_extra_message="pragma weak does not work outside of a file"
    fi
    # This is needed for Mac OSX 10.5
    rm -rf conftest.dSYM
    rm -f conftest*
fi
dnl
if test -z "$pac_cv_prog_c_weak_symbols" ; then 
    AC_TRY_LINK([
extern int PFoo(int);
#pragma _HP_SECONDARY_DEF Foo  PFoo
int Foo(int a) { return a; }
],[return PFoo(1);],pac_cv_prog_c_weak_symbols="pragma _HP_SECONDARY_DEF")
fi
dnl
if test -z "$pac_cv_prog_c_weak_symbols" ; then
    AC_TRY_LINK([
extern int PFoo(int);
#pragma _CRI duplicate PFoo as Foo
int Foo(int a) { return a; }
],[return PFoo(1);],pac_cv_prog_c_weak_symbols="pragma _CRI duplicate x as y")
fi
dnl
if test -z "$pac_cv_prog_c_weak_symbols" ; then
    pac_cv_prog_c_weak_symbols="no"
fi
dnl
dnl If there is an extra explanatory message, echo it now so that it
dnl doesn't interfere with the cache result value
if test -n "$pragma_extra_message" ; then
    echo $pragma_extra_message
fi
dnl
])
if test "$pac_cv_prog_c_weak_symbols" = "no" ; then
    ifelse([$2],,:,[$2])
else
    case "$pac_cv_prog_c_weak_symbols" in
	"pragma weak") AC_DEFINE(HAVE_PRAGMA_WEAK,1,[Supports weak pragma]) 
	;;
	"pragma _HP")  AC_DEFINE(HAVE_PRAGMA_HP_SEC_DEF,1,[HP style weak pragma])
	;;
	"pragma _CRI") AC_DEFINE(HAVE_PRAGMA_CRI_DUP,1,[Cray style weak pragma])
	;;
    esac
    ifelse([$1],,:,[$1])
fi
AC_CACHE_CHECK([whether __attribute__ ((weak)) allowed],
pac_cv_attr_weak,[
AC_TRY_COMPILE([int foo(int) __attribute__ ((weak));],[int a;],
pac_cv_attr_weak=yes,pac_cv_attr_weak=no)])
# Note that being able to compile with weak_import doesn't mean that 
# it works.
AC_CACHE_CHECK([whether __attribute ((weak_import)) allowed],
pac_cv_attr_weak_import,[
AC_TRY_COMPILE([int foo(int) __attribute__ ((weak_import));],[int a;],
pac_cv_attr_weak_import=yes,pac_cv_attr_weak_import=no)])
])

#
# This is a replacement that checks that FAILURES are signaled as well
# (later configure macros look for the .o file, not just success from the
# compiler, but they should not HAVE to
#
dnl --- insert 2.52 compatibility here ---
dnl 2.52 does not have AC_PROG_CC_WORKS
ifdef([AC_PROG_CC_WORKS],,[AC_DEFUN([AC_PROG_CC_WORKS],)])
dnl
AC_DEFUN([PAC_PROG_CC_WORKS],
[AC_PROG_CC_WORKS
AC_MSG_CHECKING([whether the C compiler sets its return status correctly])
AC_LANG_SAVE
AC_LANG_C
AC_TRY_COMPILE(,[int a = bzzzt;],notbroken=no,notbroken=yes)
AC_MSG_RESULT($notbroken)
if test "$notbroken" = "no" ; then
    AC_MSG_ERROR([installation or configuration problem: C compiler does not
correctly set error code when a fatal error occurs])
fi
])
dnl
dnl/*D 
dnl PAC_PROG_C_MULTIPLE_WEAK_SYMBOLS - Test whether C and the
dnl linker allow multiple weak symbols.
dnl
dnl Synopsis
dnl PAC_PROG_C_MULTIPLE_WEAK_SYMBOLS(action-if-true,action-if-false)
dnl
dnl 
dnl D*/
AC_DEFUN([PAC_PROG_C_MULTIPLE_WEAK_SYMBOLS],[
AC_CACHE_CHECK([for multiple weak symbol support],
pac_cv_prog_c_multiple_weak_symbols,[
# Test for multiple weak symbol support...
#
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
cat >>conftest1.c <<EOF
extern int PFoo(int);
extern int PFoo_(int);
extern int pfoo_(int);
#pragma weak PFoo = Foo
#pragma weak PFoo_ = Foo
#pragma weak pfoo_ = Foo
int Foo(int);
int Foo(a) { return a; }
EOF
cat >>conftest2.c <<EOF
extern int PFoo(int), PFoo_(int), pfoo_(int);
int main() {
return PFoo(0) + PFoo_(1) + pfoo_(2);}
EOF
ac_link2='${CC-cc} -o conftest $CFLAGS $CPPFLAGS $LDFLAGS conftest1.c conftest2.c $LIBS >conftest.out 2>&1'
if eval $ac_link2 ; then
    pac_cv_prog_c_multiple_weak_symbols="yes"
else
    echo "$ac_link2" >>config.log
    echo "Failed program was" >>config.log
    cat conftest1.c >>config.log
    cat conftest2.c >>config.log
    if test -s conftest.out ; then cat conftest.out >> config.log ; fi
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest*
dnl
])
if test "$pac_cv_prog_c_multiple_weak_symbols" = "yes" ; then
    ifelse([$1],,:,[$1])
else
    ifelse([$2],,:,[$2])
fi
])
dnl
dnl/*D
dnl PAC_FUNC_CRYPT - Check that the function crypt is defined
dnl
dnl Synopsis:
dnl PAC_FUNC_CRYPT
dnl
dnl Output Effects:
dnl 
dnl In Solaris, the crypt function is not defined in unistd unless 
dnl _XOPEN_SOURCE is defines and _XOPEN_VERSION is 4 or greater.
dnl We test by looking for a missing crypt by defining our own
dnl incompatible one and trying to compile it.
dnl Defines NEED_CRYPT_PROTOTYPE if no prototype is found.
dnl D*/
AC_DEFUN([PAC_FUNC_CRYPT],[
AC_CACHE_CHECK([whether crypt defined in unistd.h],
pac_cv_func_crypt_defined,[
AC_TRY_COMPILE([
#include <unistd.h>
double crypt(double a){return a;}],[return 0];,
pac_cv_func_crypt_defined="no",pac_cv_func_crypt_defined="yes")])
if test "$pac_cv_func_crypt_defined" = "no" ; then
    # check to see if defining _XOPEN_SOURCE helps
    AC_CACHE_CHECK([whether crypt defined in unistd with _XOPEN_SOURCE],
pac_cv_func_crypt_xopen,[
    AC_TRY_COMPILE([
#define _XOPEN_SOURCE    
#include <unistd.h>
double crypt(double a){return a;}],[return 0];,
pac_cv_func_crypt_xopen="no",pac_cv_func_crypt_xopen="yes")])
fi
if test "$pac_cv_func_crypt_xopen" = "yes" ; then
    AC_DEFINE(_XOPEN_SOURCE,1,[if xopen needed for crypt])
elif test "$pac_cv_func_crypt_defined" = "no" ; then
    AC_DEFINE(NEED_CRYPT_PROTOTYPE,1,[if a prototype for crypt is needed])
fi
])dnl

dnl Use the value of enable-strict to update CFLAGS
dnl pac_cc_strict_flags contains the strict flags.
dnl
dnl -std=c89 is used to select the C89 version of the ANSI/ISO C standard.
dnl As of this writing, many C compilers still accepted only this version,
dnl not the later C99 version. When all compilers accept C99, this 
dnl should be changed to the appropriate standard level.  Note that we've
dnl had trouble with gcc 2.95.3 accepting -std=c89 but then trying to 
dnl compile program with a invalid set of options 
dnl (-D __STRICT_ANSI__-trigraphs)
AC_DEFUN([PAC_CC_STRICT],[
export enable_strict_done
if test "$enable_strict_done" != "yes" ; then

    # Some comments on strict warning options.
    # These were added to reduce warnings:
    #   -Wno-missing-field-initializers  -- We want to allow a struct to be 
    #       initialized to zero using "struct x y = {0};" and not require 
    #       each field to be initialized individually.
    #   -Wno-type-limits -- There are places where we compare an unsigned to 
    #	    a constant that happens to be zero e.g., if x is unsigned and 
    #	    MIN_VAL is zero, we'd like to do "MPIU_Assert(x >= MIN_VAL);".
    #       Note this option is not supported by gcc 4.2.
    #   -Wno-unused-parameter -- For portability, some parameters go unused
    #	    when we have different implementations of functions for 
    #	    different platforms
    #   -Wno-unused-label -- We add fn_exit: and fn_fail: on all functions, 
    #	    but fn_fail may not be used if the function doesn't return an 
    #	    error.
    # These were removed to reduce warnings:
    #   -Wcast-qual -- Sometimes we need to cast "volatile char*" to 
    #	    "char*", e.g., for memcpy.
    #   -Wpadded -- We catch struct padding with asserts when we need to
    #   -Wredundant-decls -- Having redundant declarations is benign and the 
    #	    code already has some.

    pac_common_strict_flags="-O2 -Wall -Wextra -Wno-missing-field-initializers -Wno-type-limits -Wstrict-prototypes -Wmissing-prototypes -DGCC_WALL -Wno-unused-parameter -Wno-unused-label -Wshadow -Wmissing-declarations -Wno-long-long -Wfloat-equal -Wdeclaration-after-statement -Wundef -Wno-endif-labels -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Wsign-compare -Waggregate-return -Wold-style-definition -Wmissing-noreturn -Wmissing-format-attribute -Wno-multichar -Wno-deprecated-declarations -Wpacked -Wnested-externs -Winline -Winvalid-pch -Wno-pointer-sign -Wvariadic-macros -std=c89"
    pac_cc_strict_flags=""
    case "$1" in 
        yes|all|posix)
		enable_strict_done="yes"
		pac_cc_strict_flags="$pac_common_strict_flags -D_POSIX_C_SOURCE=199506L"
        ;;

        noposix)
		enable_strict_done="yes"
		pac_cc_strict_flags="$pac_common_strict_flags"
        ;;
        
        no)
		# Accept and ignore this value
		:
        ;;

        *)
		if test -n "$1" ; then
		   AC_MSG_WARN([Unrecognized value for enable-strict:$1])
		fi
        ;;

    esac

    # See if the above options work with the compiler
    accepted_flags=""
    for flag in $pac_cc_strict_flags ; do
    	old_CFLAGS=$CFLAGS
	CFLAGS="$CFLAGS $accepted_flags $flag"
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[int a;])],accepted_flags="$accepted_flags $flag",)
	CFLAGS="$old_CFLAGS"
    done
    pac_cc_strict_flags=$accepted_flags
fi
])

dnl/*D
dnl PAC_ARG_STRICT - Add --enable-strict to configure.  
dnl
dnl Synopsis:
dnl PAC_ARG_STRICT
dnl 
dnl Output effects:
dnl Adds '--enable-strict' to the command line.
dnl
dnl D*/
AC_DEFUN([PAC_ARG_STRICT],[
AC_ARG_ENABLE(strict,
[--enable-strict  - Turn on strict compilation testing when using gcc])
PAC_CC_STRICT($enable_strict)
CFLAGS="$CFLAGS $pac_cc_strict_flags"
export CFLAGS
])

dnl/*D
dnl PAC_ARG_CC_G - Add debugging flags for the C compiler
dnl
dnl Synopsis:
dnl PAC_ARG_CC_G
dnl
dnl Output Effect:
dnl Adds '-g' to 'COPTIONS' and exports 'COPTIONS'.  Sets and exports the 
dnl variable 'enable_g_simple' so that subsidiary 'configure's will not
dnl add another '-g'.
dnl
dnl Notes:
dnl '--enable-g' should be used for all internal debugging modes if possible.
dnl Use the 'enable_val' that 'enable_g' is set to to pass particular values,
dnl and ignore any values that are not recognized (some other 'configure' 
dnl may have used them.  Of course, if you need extra values, you must
dnl add code to extract values from 'enable_g'.
dnl
dnl For example, to look for a particular keyword, you could use
dnl.vb
dnl SaveIFS="$IFS"
dnl IFS=","
dnl for key in $enable_g ; do
dnl     case $key in 
dnl         mem) # add code for memory debugging 
dnl         ;;
dnl         *)   # ignore all other values
dnl         ;;
dnl     esac
dnl done
dnl IFS="$SaveIFS"
dnl.ve
dnl
dnl D*/
AC_DEFUN([PAC_ARG_CC_G],[
AC_ARG_ENABLE(g,
[--enable-g  - Turn on debugging of the package (typically adds -g to COPTIONS)])
export COPTIONS
export enable_g_simple
if test -n "$enable_g" -a "$enable_g" != "no" -a \
   "$enable_g_simple" != "done" ; then
    enable_g_simple="done"
    if test "$enable_g" = "g" -o "$enable_g" = "yes" ; then
        COPTIONS="$COPTIONS -g"
    fi
fi
])
dnl
dnl Simple version for both options
dnl
AC_DEFUN([PAC_ARG_CC_COMMON],[
PAC_ARG_CC_G
PAC_ARG_STRICT
])
dnl
dnl Eventually, this can be used instead of the funky Fortran stuff to 
dnl access the command line from a C routine.
dnl #
dnl # Under IRIX (some version) __Argc and __Argv gave the argc,argv values
dnl #Under linux, __libc_argv and __libc_argc
dnl AC_MSG_CHECKING([for alternative argc,argv names])
dnl AC_TRY_LINK([
dnl extern int __Argc; extern char **__Argv;],[return __Argc;],
dnl alt_argv="__Argv")
dnl if test -z "$alt_argv" ; then
dnl    AC_TRY_LINK([
dnl extern int __libc_argc; extern char **__libc_argv;],[return __lib_argc;],
dnl alt_argv="__lib_argv")
dnl fi
dnl if test -z "$alt_argv" ; then
dnl   AC_MSG_RESULT(none found)) 
dnl else 
dnl   AC_MSG_RESULT($alt_argv) 
dnl fi
dnl 
dnl
dnl Check whether we need -fno-common to correctly compile the source code.
dnl This is necessary if global variables are defined without values in
dnl gcc.  Here is the test
dnl conftest1.c:
dnl extern int a; int a;
dnl conftest2.c:
dnl extern int a; int main(int argc; char *argv[] ){ return a; }
dnl Make a library out of conftest1.c and try to link with it.
dnl If that fails, recompile it with -fno-common and see if that works.
dnl If so, add -fno-common to CFLAGS
dnl An alternative is to use, on some systems, ranlib -c to force 
dnl the system to find common symbols.
dnl
AC_DEFUN([PAC_PROG_C_BROKEN_COMMON],[
AC_CACHE_CHECK([whether global variables handled properly],
ac_cv_prog_cc_globals_work,[
AC_REQUIRE([AC_PROG_RANLIB])
ac_cv_prog_cc_globals_work=no
rm -f libconftest.a
echo 'extern int a; int a;' > conftest1.c
echo 'extern int a; int main( ){ return a; }' > conftest2.c
if ${CC-cc} $CFLAGS -c conftest1.c >conftest.out 2>&1 ; then
    if ${AR-ar} cr libconftest.a conftest1.o >/dev/null 2>&1 ; then
        if ${RANLIB-:} libconftest.a >/dev/null 2>&1 ; then
            if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a >>conftest.out 2>&1 ; then
		# Success!  C works
		ac_cv_prog_cc_globals_work=yes
	    else
		echo "Error linking program with uninitialized global" >&AC_FD_CC
	        echo "Programs were:" >&AC_FD_CC
		echo "conftest1.c:" >&AC_FD_CC
                cat conftest1.c >&AC_FD_CC
		echo "conftest2.c:" >&AC_FD_CC
		cat conftest2.c >&AC_FD_CC
		echo "and link line was:" >&AC_FD_CC
		echo "${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a" >&AC_FD_CC
		echo "with output:" >&AC_FD_CC
		cat conftest.out >&AC_FD_CC

	        # Failure!  Do we need -fno-common?
	        ${CC-cc} $CFLAGS -fno-common -c conftest1.c >> conftest.out 2>&1
		rm -f libconftest.a
		${AR-ar} cr libconftest.a conftest1.o
	        ${RANLIB-:} libconftest.a
	        if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a >> conftest.out 2>&1 ; then
		    ac_cv_prog_cc_globals_work="needs -fno-common"
		    CFLAGS="$CFLAGS -fno-common"
                elif test -n "$RANLIB" ; then 
		    # Try again, with ranlib changed to ranlib -c
                    # (send output to /dev/null incase this ranlib
                    # doesn't know -c)
		    ${RANLIB} -c libconftest.a >/dev/null 2>&1
		    if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a >> conftest.out 2>&1 ; then
			RANLIB="$RANLIB -c"
	 	    #else
		    #	# That didn't work
		    #	:
		    fi
	        #else 
		#    :
		fi
	    fi
        fi
    fi
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest* libconftest*])
if test "$ac_cv_prog_cc_globals_work" = no ; then
    AC_MSG_WARN([Common symbols not supported on this system!])
fi
])
dnl
dnl
dnl Return the structure alignment in pac_cv_c_struct_align
dnl Possible values include
dnl	packed - no padding or alignment, any item may begin on any byte
dnl	largest - extent of a structure is a multiple of the largest item; 
dnl               items are aligned with their size 
dnl	four - structs padded to a multiple of four
dnl     two  - like four, but to a multiple of two
dnl     eight - If objects containing 8-byte items are padded to a multiple
dnl             of eight
dnl     largestor4 - like largest, except that for items of size > 4, align 
dnl                  on 4-byte boundaries.  E.g., align on the 
dnl                  min(4,max(size of items)).
dnl     largestorword - (should be named largestorqword, with qword meaning 
dnl                     quad-word): Like largestor4, but with a special case 
dnl                     for 16-byte items (this is the 16-byte aligned 
dnl                     quad-word-load special case).
dnl
dnl In addition, a "Could not determine alignment" and a 
dnl "Multiple cases:" return is possible.  
dnl
AC_DEFUN([PAC_C_STRUCT_ALIGNMENT],[
AC_CACHE_CHECK([for C struct alignment],pac_cv_c_struct_align,[
AC_TRY_RUN([
#include <stdio.h>
#ifdef DEBUG_STRUCT_ALIGNMENT
#define DBG(a,b,c) printf( "type %s, size = %d, extent = %d\n", a, b, c )
#define CHECK(cond,flag) if (cond) { flag = 0; \
    printf( "Setting %s to false because of %s\n", #flag, #cond ); }
#else
#define DBG(a,b,c)
#define CHECK(cond,flag) if (cond) { flag = 0; }
#endif

int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    int is_largest = 1;
    int is_largestorword = 1;
    int is_largestor4 = 1;
    int numCases;

    /* We've seen PowerPC systems where the alignment may
       be largest for some items but not for double + int */
    struct { char a; int b; } char_int;
    struct { short a; int b; } short_int;
    struct { char a; short b; } char_short;
    struct { char a; long b; } char_long;
    struct { char a; float b; } char_float;
    struct { char a; double b; } char_double;
    struct { char a; int b; char c; } char_int_char;
    struct { char a; short b; char c; } char_short_char;
#ifdef HAVE_LONG_DOUBLE
    struct { char a; long double b; } char_long_double;
#endif
    int size, extent;

    size = sizeof(char) + sizeof(int);
    extent = sizeof(char_int);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(int)) != 0, is_largest); 
    CHECK((extent % sizeof(int)) != 0, is_largestor4); 
    CHECK((extent % sizeof(int)) != 0, is_largestorword); 
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_int",size,extent);

    size = sizeof(short) + sizeof(int);
    extent = sizeof(short_int);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(int)) != 0, is_largest); 
    CHECK((extent % sizeof(int)) != 0, is_largestor4); 
    CHECK((extent % sizeof(int)) != 0, is_largestorword); 
    if ( (extent % 2) != 0) is_two = 0;
    if ( (size == 6) && (extent == 8) ) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("short_int",size,extent);

    size = sizeof(char) + sizeof(short);
    extent = sizeof(char_short);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(short)) != 0,is_largest);
    CHECK((extent % sizeof(short)) != 0,is_largestor4);
    CHECK((extent % sizeof(short)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
    if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_short",size,extent);

    size = sizeof(char) + sizeof(long);
    extent = sizeof(char_long);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(long)) != 0,is_largest);
    CHECK((extent % 4) != 0,is_largestor4);
    CHECK((extent % sizeof(long)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(long) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_long",size,extent);

    size = sizeof(char) + sizeof(float);
    extent = sizeof(char_float);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(float)) != 0,is_largest);
    CHECK((extent % sizeof(float)) != 0,is_largestor4);
    CHECK((extent % sizeof(float)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(float) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_float",size,extent);

    size = sizeof(char) + sizeof(double);
    extent = sizeof(char_double);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(double)) != 0,is_largest);
    CHECK((extent % 4) != 0,is_largestor4);
    CHECK((extent % sizeof(int)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(double) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_double",size,extent);

#ifdef HAVE_LONG_DOUBLE
    size = sizeof(char) + sizeof(long double);
    extent = sizeof(char_long_double);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(long double)) != 0,is_largest);
    CHECK((extent % 4) != 0,is_largestor4);
    CHECK((extent % 16) == 0,is_largestor4);
    CHECK((extent % sizeof(long double)) != 0,is_largestorword);
    /* This case only applies to largestorword if long doubles are 16 bytes */
    if (sizeof(long double) != 16) is_largestorword = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_long-double",size,extent);
#else
    /* The special case of largestorword only applies if long double 
       available */
    is_largestorword=0;
#endif

    /* char int char helps separate largest from 4/8 aligned */
    size = sizeof(char) + sizeof(int) + sizeof(char);
    extent = sizeof(char_int_char);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(int)) != 0,is_largest);
    CHECK((extent % sizeof(int)) != 0,is_largestor4);
    CHECK((extent % sizeof(int)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_int_char",size,extent);

    /* char short char helps separate largest from 4/8 aligned */
    size = sizeof(char) + sizeof(short) + sizeof(char);
    extent = sizeof(char_short_char);
    if (size != extent) is_packed = 0;
    CHECK((extent % sizeof(short)) != 0,is_largest);
    CHECK((extent % sizeof(short)) != 0,is_largestor4);
    CHECK((extent % sizeof(short)) != 0,is_largestorword);
    if ( (extent % 2) != 0) is_two = 0;
    if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
    CHECK((extent == 6) && (size == 4),is_four);
    if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_short_char",size,extent);

    /* If aligned mod 8, it will be aligned mod 4 */
    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* largest superceeds eight */
    if (is_largest) is_eight = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    numCases = is_packed + is_largest + is_largestorword + is_largestor4 +
	is_two + is_four + is_eight;
    if (numCases == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (numCases != 1) {
	    fprintf( cf, "Multiple cases:\n" );
	}
	if (is_packed) fprintf( cf, "packed\n" );
	if (is_largest) fprintf( cf, "largest\n" );
	if (is_largestorword) fprintf( cf, "largestorword\n" );
	if (is_largestor4) fprintf( cf, "largestor4\n" );
	if (is_two) fprintf( cf, "two\n" );
	if (is_four) fprintf( cf, "four\n" );
	if (is_eight) fprintf( cf, "eight\n" );
    }
    fclose( cf );
    return 0;
}],
pac_cv_c_struct_align=`cat ctest.out`
,pac_cv_c_struct_align="unknown",pac_cv_c_struct_align="$CROSS_ALIGN_STRUCT")
rm -f ctest.out
])
if test -z "$pac_cv_c_struct_align" ; then
    pac_cv_c_struct_align="unknown"
fi
])
dnl
dnl
dnl Return the integer structure alignment in pac_cv_c_max_integer_align
dnl Possible values include
dnl	packed
dnl	two
dnl	four
dnl	eight
dnl
dnl In addition, a "Could not determine alignment" and a "error!"
dnl return is possible.  
AC_DEFUN([PAC_C_MAX_INTEGER_ALIGN],[
AC_CACHE_CHECK([for max C struct integer alignment],
pac_cv_c_max_integer_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    struct { char a; int b; } char_int;
    struct { char a; short b; } char_short;
    struct { char a; long b; } char_long;
    struct { char a; int b; char c; } char_int_char;
    struct { char a; short b; char c; } char_short_char;
#ifdef HAVE_LONG_LONG_INT
    struct { long long int a; char b; } lli_c;
    struct { char a; long long int b; } c_lli;
#endif
    int size, extent, extent2;

    /* assume max integer alignment isn't 8 if we don't have
     * an eight-byte value :)
     */
#ifdef HAVE_LONG_LONG_INT
    if (sizeof(int) < 8 && sizeof(long) < 8 && sizeof(long long int) < 8)
	is_eight = 0;
#else
    if (sizeof(int) < 8 && sizeof(long) < 8) is_eight = 0;
#endif

    size = sizeof(char) + sizeof(int);
    extent = sizeof(char_int);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_int",size,extent);

    size = sizeof(char) + sizeof(short);
    extent = sizeof(char_short);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
    if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_short",size,extent);

    size = sizeof(char) + sizeof(long);
    extent = sizeof(char_long);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(long) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_long",size,extent);

#ifdef HAVE_LONG_LONG_INT
    size = sizeof(char) + sizeof(long long int);
    extent = sizeof(lli_c);
    extent2 = sizeof(c_lli);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(long long int) >= 8 && (extent % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
#endif

    size = sizeof(char) + sizeof(int) + sizeof(char);
    extent = sizeof(char_int_char);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if ( (extent % 4) != 0) is_four = 0;
    if (sizeof(int) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_int_char",size,extent);

    size = sizeof(char) + sizeof(short) + sizeof(char);
    extent = sizeof(char_short_char);
    if (size != extent) is_packed = 0;
    if ( (extent % 2) != 0) is_two = 0;
    if (sizeof(short) == 4 && (extent % 4) != 0) is_four = 0;
    if (sizeof(short) == 8 && (extent % 8) != 0) is_eight = 0;
    DBG("char_short_char",size,extent);

    /* If aligned mod 8, it will be aligned mod 4 */
    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    if (is_packed + is_two + is_four + is_eight == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (is_packed + is_two + is_four + is_eight != 1) {
	    fprintf( cf, "error!\n" );
	}
	else {
	    if (is_packed) fprintf( cf, "packed\n" );
	    if (is_two) fprintf( cf, "two\n" );
	    if (is_four) fprintf( cf, "four\n" );
	    if (is_eight) fprintf( cf, "eight\n" );
	}
    }
    fclose( cf );
    return 0;
}],
pac_cv_c_max_integer_align=`cat ctest.out`,
pac_cv_c_max_integer_align="unknown",
pac_cv_c_max_integer_align="$CROSS_ALIGN_STRUCT_INT")
rm -f ctest.out
])
if test -z "$pac_cv_c_max_integer_align" ; then
    pac_cv_c_max_integer_align="unknown"
fi
])
dnl
dnl
dnl Return the floating point structure alignment in
dnl pac_cv_c_max_fp_align.
dnl
dnl Possible values include:
dnl	packed
dnl	two
dnl	four
dnl	eight
dnl     sixteen
dnl
dnl In addition, a "Could not determine alignment" and a "error!"
dnl return is possible.  
AC_DEFUN([PAC_C_MAX_FP_ALIGN],[
AC_CACHE_CHECK([for max C struct floating point alignment],
pac_cv_c_max_fp_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    int is_sixteen = 1;
    struct { char a; float b; } char_float;
    struct { float b; char a; } float_char;
    struct { char a; double b; } char_double;
    struct { double b; char a; } double_char;
#ifdef HAVE_LONG_DOUBLE
    struct { char a; long double b; } char_long_double;
    struct { long double b; char a; } long_double_char;
    struct { long double a; int b; char c; } long_double_int_char;
#endif
    int size, extent1, extent2;

    size = sizeof(char) + sizeof(float);
    extent1 = sizeof(char_float);
    extent2 = sizeof(float_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(float) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_float",size,extent1);

    size = sizeof(char) + sizeof(double);
    extent1 = sizeof(char_double);
    extent2 = sizeof(double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(double) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_double",size,extent1);

#ifdef HAVE_LONG_DOUBLE
    size = sizeof(char) + sizeof(long double);
    extent1 = sizeof(char_long_double);
    extent2 = sizeof(long_double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0
	&& (extent2 % 16) != 0) is_sixteen = 0;
    DBG("char_long-double",size,extent1);

    extent1 = sizeof(long_double_int_char);
    if ( (extent1 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0)	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0) is_sixteen = 0;
#else
    is_sixteen = 0;
#endif

    if (is_sixteen) { is_eight = 0; is_four = 0; is_two = 0; }

    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    if (is_packed + is_two + is_four + is_eight + is_sixteen == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (is_packed + is_two + is_four + is_eight + is_sixteen != 1) {
	    fprintf( cf, "error!\n" );
	}
	else {
	    if (is_packed) fprintf( cf, "packed\n" );
	    if (is_two) fprintf( cf, "two\n" );
	    if (is_four) fprintf( cf, "four\n" );
	    if (is_eight) fprintf( cf, "eight\n" );
	    if (is_sixteen) fprintf( cf, "sixteen\n" );
	}
    }
    fclose( cf );
    return 0;
}],
pac_cv_c_max_fp_align=`cat ctest.out`,
pac_cv_c_max_fp_align="unknown",
pac_cv_c_max_fp_align="$CROSS_ALIGN_STRUCT_FP")
rm -f ctest.out
])
if test -z "$pac_cv_c_max_fp_align" ; then
    pac_cv_c_max_fp_align="unknown"
fi
])
dnl
dnl
dnl Return the floating point structure alignment in
dnl pac_cv_c_max_double_fp_align.
dnl
dnl Possible values include:
dnl	packed
dnl	two
dnl	four
dnl	eight
dnl
dnl In addition, a "Could not determine alignment" and a "error!"
dnl return is possible.  
AC_DEFUN([PAC_C_MAX_DOUBLE_FP_ALIGN],[
AC_CACHE_CHECK([for max C struct alignment of structs with doubles],
pac_cv_c_max_double_fp_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    struct { char a; float b; } char_float;
    struct { float b; char a; } float_char;
    struct { char a; double b; } char_double;
    struct { double b; char a; } double_char;
    int size, extent1, extent2;

    size = sizeof(char) + sizeof(float);
    extent1 = sizeof(char_float);
    extent2 = sizeof(float_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(float) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_float",size,extent1);

    size = sizeof(char) + sizeof(double);
    extent1 = sizeof(char_double);
    extent2 = sizeof(double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(double) == 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    DBG("char_double",size,extent1);

    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    if (is_packed + is_two + is_four + is_eight == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (is_packed + is_two + is_four + is_eight != 1) {
	    fprintf( cf, "error!\n" );
	}
	else {
	    if (is_packed) fprintf( cf, "packed\n" );
	    if (is_two) fprintf( cf, "two\n" );
	    if (is_four) fprintf( cf, "four\n" );
	    if (is_eight) fprintf( cf, "eight\n" );
	}
    }
    fclose( cf );
    return 0;
}],
pac_cv_c_max_double_fp_align=`cat ctest.out`,
pac_cv_c_max_double_fp_align="unknown",
pac_cv_c_max_double_fp_align="$CROSS_ALIGN_STRUCT_DOUBLE_FP")
rm -f ctest.out
])
if test -z "$pac_cv_c_max_double_fp_align" ; then
    pac_cv_c_max_double_fp_align="unknown"
fi
])
AC_DEFUN([PAC_C_MAX_LONGDOUBLE_FP_ALIGN],[
AC_CACHE_CHECK([for max C struct floating point alignment with long doubles],
pac_cv_c_max_longdouble_fp_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int is_packed  = 1;
    int is_two     = 1;
    int is_four    = 1;
    int is_eight   = 1;
    int is_sixteen = 1;
    struct { char a; long double b; } char_long_double;
    struct { long double b; char a; } long_double_char;
    struct { long double a; int b; char c; } long_double_int_char;
    int size, extent1, extent2;

    size = sizeof(char) + sizeof(long double);
    extent1 = sizeof(char_long_double);
    extent2 = sizeof(long_double_char);
    if (size != extent1) is_packed = 0;
    if ( (extent1 % 2) != 0 && (extent2 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0 && (extent2 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0 && (extent2 % 8) != 0)
	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0
	&& (extent2 % 16) != 0) is_sixteen = 0;
    DBG("char_long-double",size,extent1);

    extent1 = sizeof(long_double_int_char);
    if ( (extent1 % 2) != 0) is_two = 0;
    if ( (extent1 % 4) != 0) is_four = 0;
    if (sizeof(long double) >= 8 && (extent1 % 8) != 0)	is_eight = 0;
    if (sizeof(long double) > 8 && (extent1 % 16) != 0) is_sixteen = 0;

    if (is_sixteen) { is_eight = 0; is_four = 0; is_two = 0; }

    if (is_eight) { is_four = 0; is_two = 0; }

    if (is_four) is_two = 0;

    /* Tabulate the results */
    cf = fopen( "ctest.out", "w" );
    if (is_packed + is_two + is_four + is_eight + is_sixteen == 0) {
	fprintf( cf, "Could not determine alignment\n" );
    }
    else {
	if (is_packed + is_two + is_four + is_eight + is_sixteen != 1) {
	    fprintf( cf, "error!\n" );
	}
	else {
	    if (is_packed) fprintf( cf, "packed\n" );
	    if (is_two) fprintf( cf, "two\n" );
	    if (is_four) fprintf( cf, "four\n" );
	    if (is_eight) fprintf( cf, "eight\n" );
	    if (is_sixteen) fprintf( cf, "sixteen\n" );
	}
    }
    fclose( cf );
    return 0;
}],
pac_cv_c_max_longdouble_fp_align=`cat ctest.out`,
pac_cv_c_max_longdouble_fp_align="unknown",
pac_cv_c_max_longdouble_fp_align="$CROSS_ALIGN_STRUCT_LONGDOUBLE_FP")
rm -f ctest.out
])
if test -z "$pac_cv_c_max_longdouble_fp_align" ; then
    pac_cv_c_max_longdouble_fp_align="unknown"
fi
])
dnl
dnl Other tests assume that there is potentially a maximum alignment
dnl and that if there is no maximum alignment, or a type is smaller than
dnl that value, then we align on the size of the value, with the exception
dnl of the "position-based alignment" rules we test for separately.
dnl
dnl It turns out that these assumptions have fallen short in at least one
dnl case, on MacBook Pros, where doubles are aligned on 4-byte boundaries
dnl even when long doubles are aligned on 16-byte boundaries. So this test
dnl is here specifically to handle this case.
dnl
dnl Puts result in pac_cv_c_double_alignment_exception.
dnl
dnl Possible values currently include no and four.
dnl
AC_DEFUN([PAC_C_DOUBLE_ALIGNMENT_EXCEPTION],[
AC_CACHE_CHECK([if double alignment breaks rules, find actual alignment],
pac_cv_c_double_alignment_exception,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    struct { char a; double b; } char_double;
    struct { double b; char a; } double_char;
    int extent1, extent2, align_4 = 0;

    extent1 = sizeof(char_double);
    extent2 = sizeof(double_char);

    /* we're interested in the largest value, will let separate test
     * deal with position-based issues.
     */
    if (extent1 < extent2) extent1 = extent2;
    if ((sizeof(double) == 8) && (extent1 % 8) != 0) {
       if (extent1 % 4 == 0) {
#ifdef HAVE_MAX_FP_ALIGNMENT
          if (HAVE_MAX_FP_ALIGNMENT >= 8) align_4 = 1;
#else
          align_4 = 1;
#endif
       }
    }

    cf = fopen( "ctest.out", "w" );

    if (align_4) fprintf( cf, "four\n" );
    else fprintf( cf, "no\n" );

    fclose( cf );
    return 0;
}],
pac_cv_c_double_alignment_exception=`cat ctest.out`,
pac_cv_c_double_alignment_exception="unknown",
pac_cv_c_double_alignment_exception="$CROSS_ALIGN_DOUBLE_EXCEPTION")
rm -f ctest.out
])
if test -z "$pac_cv_c_double_alignment_exception" ; then
    pac_cv_c_double_alignment_exception="unknown"
fi
])
dnl
dnl
dnl Test for odd struct alignment rule that only applies max.
dnl padding when double value is at front of type.
dnl Puts result in pac_cv_c_double_pos_align.
dnl
dnl Search for "Power alignment mode" for more details.
dnl
dnl Possible values include yes, no, and unknown.
dnl
AC_DEFUN([PAC_C_DOUBLE_POS_ALIGN],[
AC_CACHE_CHECK([if alignment of structs with doubles is based on position],
pac_cv_c_double_pos_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int padding_varies_by_pos = 0;
    struct { char a; double b; } char_double;
    struct { double b; char a; } double_char;
    int extent1, extent2;

    extent1 = sizeof(char_double);
    extent2 = sizeof(double_char);
    if (extent1 != extent2) padding_varies_by_pos = 1;

    cf = fopen( "ctest.out", "w" );
    if (padding_varies_by_pos) fprintf( cf, "yes\n" );
    else fprintf( cf, "no\n" );

    fclose( cf );
    return 0;
}],
pac_cv_c_double_pos_align=`cat ctest.out`,
pac_cv_c_double_pos_align="unknown",
pac_cv_c_double_pos_align="$CROSS_ALIGN_DOUBLE_POS")
rm -f ctest.out
])
if test -z "$pac_cv_c_double_pos_align" ; then
    pac_cv_c_double_pos_align="unknown"
fi
])
dnl
dnl
dnl Test for odd struct alignment rule that only applies max.
dnl padding when long long int value is at front of type.
dnl Puts result in pac_cv_c_llint_pos_align.
dnl
dnl Search for "Power alignment mode" for more details.
dnl
dnl Possible values include yes, no, and unknown.
dnl
AC_DEFUN([PAC_C_LLINT_POS_ALIGN],[
AC_CACHE_CHECK([if alignment of structs with long long ints is based on position],
pac_cv_c_llint_pos_align,[
AC_TRY_RUN([
#include <stdio.h>
#define DBG(a,b,c)
int main( int argc, char *argv[] )
{
    FILE *cf;
    int padding_varies_by_pos = 0;
#ifdef HAVE_LONG_LONG_INT
    struct { char a; long long int b; } char_llint;
    struct { long long int b; char a; } llint_char;
    int extent1, extent2;

    extent1 = sizeof(char_llint);
    extent2 = sizeof(llint_char);
    if (extent1 != extent2) padding_varies_by_pos = 1;
#endif

    cf = fopen( "ctest.out", "w" );
    if (padding_varies_by_pos) fprintf( cf, "yes\n" );
    else fprintf( cf, "no\n" );

    fclose( cf );
    return 0;
}],
pac_cv_c_llint_pos_align=`cat ctest.out`,
pac_cv_c_llint_pos_align="unknown",
pac_cv_c_llint_pos_align="$CROSS_ALIGN_LLINT_POS")
rm -f ctest.out
])
if test -z "$pac_cv_c_llint_pos_align" ; then
    pac_cv_c_llint_pos_align="unknown"
fi
])

dnl
dnl
dnl/*D
dnl PAC_FUNC_NEEDS_DECL - Set NEEDS_<funcname>_DECL if a declaration is needed
dnl
dnl Synopsis:
dnl PAC_FUNC_NEEDS_DECL(headerfiles,funcname)
dnl
dnl Output Effect:
dnl Sets 'NEEDS_<funcname>_DECL' if 'funcname' is not declared by the 
dnl headerfiles.
dnl
dnl Approach:
dnl Try to compile a program with the function, but passed with an incorrect
dnl calling sequence.  If the compilation fails, then the declaration
dnl is provided within the header files.  If the compilation succeeds,
dnl the declaration is required.
dnl
dnl We use a 'double' as the first argument to try and catch varargs
dnl routines that may use an int or pointer as the first argument.
dnl
dnl There is one difficulty - if the compiler has been instructed to
dnl fail on implicitly defined functions, then this test will always
dnl fail.
dnl 
dnl D*/
AC_DEFUN([PAC_FUNC_NEEDS_DECL],[
AC_CACHE_CHECK([whether $2 needs a declaration],
pac_cv_func_decl_$2,[
AC_TRY_COMPILE([$1
int $2(double, int, double, const char *);],[int a=$2(1.0,27,1.0,"foo");],
pac_cv_func_decl_$2=yes,pac_cv_func_decl_$2=no)])
if test "$pac_cv_func_decl_$2" = "yes" ; then
changequote(<<,>>)dnl
define(<<PAC_FUNC_NAME>>, translit(NEEDS_$2_DECL, [a-z *], [A-Z__]))dnl
changequote([, ])dnl
    AC_DEFINE_UNQUOTED(PAC_FUNC_NAME,1,[Define if $2 needs a declaration])
undefine([PAC_FUNC_NAME])
fi
])dnl
dnl
dnl /*D
dnl PAC_CHECK_SIZEOF_DERIVED - Get the size of a user-defined type,
dnl such as a struct
dnl
dnl PAC_CHECK_SIZEOF_DERIVED(shortname,definition,defaultsize)
dnl Like AC_CHECK_SIZEOF, but handles arbitrary types.
dnl Unlike AC_CHECK_SIZEOF, does not define SIZEOF_xxx (because
dnl autoheader can''t handle this case)
dnl D*/
AC_DEFUN([PAC_CHECK_SIZEOF_DERIVED],[
changequote(<<,>>)dnl
define(<<AC_TYPE_NAME>>,translit(sizeof_$1,[a-z *], [A-Z_P]))dnl
define(<<AC_CV_NAME>>,translit(pac_cv_sizeof_$1,[ *], [_p]))dnl
changequote([,])dnl
rm -f conftestval
AC_MSG_CHECKING([for size of $1])
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
main()
{
  $2 a;
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof(a));
  exit(0);
}],AC_CV_NAME=`cat conftestval`,AC_CV_NAME=0,ifelse([$3],,,AC_CV_NAME=$3))])
AC_MSG_RESULT($AC_CV_NAME)
dnl AC_DEFINE_UNQUOTED(AC_TYPE_NAME,$AC_CV_NAME)
undefine([AC_TYPE_NAME])undefine([AC_CV_NAME])
])
dnl
dnl /*D
dnl PAC_CHECK_SIZEOF_2TYPES - Get the size of a pair of types
dnl
dnl PAC_CHECK_SIZEOF_2TYPES(shortname,type1,type2,defaultsize)
dnl Like AC_CHECK_SIZEOF, but handles pairs of types.
dnl Unlike AC_CHECK_SIZEOF, does not define SIZEOF_xxx (because
dnl autoheader can''t handle this case)
dnl D*/
AC_DEFUN([PAC_CHECK_SIZEOF_2TYPES],[
changequote(<<,>>)dnl
define(<<AC_TYPE_NAME>>,translit(sizeof_$1,[a-z *], [A-Z_P]))dnl
define(<<AC_CV_NAME>>,translit(pac_cv_sizeof_$1,[ *], [_p]))dnl
changequote([,])dnl
rm -f conftestval
AC_MSG_CHECKING([for size of $1])
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
main()
{
  $2 a;
  $3 b;
  FILE *f=fopen("conftestval", "w");
  if (!f) return 1; /* avoid exit */
  fprintf(f, "%d\n", (int)(sizeof(a) + sizeof(b)));
  return 0;
}],AC_CV_NAME=`cat conftestval`,AC_CV_NAME=0,ifelse([$4],,,AC_CV_NAME=$4))])
if test "X$AC_CV_NAME" = "X" ; then
    # We have a problem.  The test returned a zero status, but no output,
    # or we're cross-compiling (or think we are) and have no value for 
    # this object
    :
fi
rm -f conftestval
AC_MSG_RESULT($AC_CV_NAME)
dnl AC_DEFINE_UNQUOTED(AC_TYPE_NAME,$AC_CV_NAME)
undefine([AC_TYPE_NAME])undefine([AC_CV_NAME])
])

dnl
dnl PAC_C_GNU_ATTRIBUTE - See if the GCC __attribute__ specifier is allow.
dnl Use the following
dnl #ifndef HAVE_GCC_ATTRIBUTE
dnl #define __attribute__(a)
dnl #endif
dnl If *not*, define __attribute__(a) as null
dnl
dnl We start by requiring Gcc.  Some other compilers accept __attribute__
dnl but generate warning messages, or have different interpretations 
dnl (which seems to make __attribute__ just as bad as #pragma) 
dnl For example, the Intel icc compiler accepts __attribute__ and
dnl __attribute__((pure)) but generates warnings for __attribute__((format...))
dnl
AC_DEFUN([PAC_C_GNU_ATTRIBUTE],[
AC_REQUIRE([AC_PROG_CC_GNU])
if test "$ac_cv_prog_gcc" = "yes" ; then
    AC_CACHE_CHECK([whether __attribute__ allowed],
pac_cv_gnu_attr_pure,[
AC_TRY_COMPILE([int foo(int) __attribute__ ((pure));],[int a;],
pac_cv_gnu_attr_pure=yes,pac_cv_gnu_attr_pure=no)])
AC_CACHE_CHECK([whether __attribute__((format)) allowed],
pac_cv_gnu_attr_format,[
AC_TRY_COMPILE([int foo(char *,...) __attribute__ ((format(printf,1,2)));],[int a;],
pac_cv_gnu_attr_format=yes,pac_cv_gnu_attr_format=no)])
    if test "$pac_cv_gnu_attr_pure" = "yes" -a "$pac_cv_gnu_attr_format" = "yes" ; then
        AC_DEFINE(HAVE_GCC_ATTRIBUTE,1,[Define if GNU __attribute__ is supported])
    fi
fi
])
dnl
dnl Check for a broken install (fails to preserve file modification times,
dnl thus breaking libraries.
dnl
dnl Create a library, install it, and then try to link against it.
AC_DEFUN([PAC_PROG_INSTALL_BREAKS_LIBS],[
AC_CACHE_CHECK([whether install breaks libraries],
ac_cv_prog_install_breaks_libs,[
AC_REQUIRE([AC_PROG_RANLIB])
AC_REQUIRE([AC_PROG_INSTALL])
AC_REQUIRE([AC_PROG_CC])
ac_cv_prog_install_breaks_libs=yes
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f libconftest* conftest*
echo 'int foo(int);int foo(int a){return a;}' > conftest1.c
echo 'extern int foo(int); int main( int argc, char **argv){ return foo(0); }' > conftest2.c
if ${CC-cc} $CFLAGS -c conftest1.c >conftest.out 2>&1 ; then
    if ${AR-ar} cr libconftest.a conftest1.o >/dev/null 2>&1 ; then
        if ${RANLIB-:} libconftest.a >/dev/null 2>&1 ; then
	    # Anything less than sleep 10, and Mac OS/X (Darwin) 
	    # will claim that install works because ranlib won't complain
	    sleep 10
	    libinstall="$INSTALL_DATA"
	    eval "libinstall=\"$libinstall\""
	    if ${libinstall} libconftest.a libconftest1.a  >/dev/null 2>&1 ; then
                if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest1.a >>conftest.out 2>&1 && test -x conftest ; then
		    # Success!  Install works
 	            ac_cv_prog_install_breaks_libs=no
	        else
	            # Failure!  Does install -p work?	
		    rm -f libconftest1.a
		    if ${libinstall} -p libconftest.a libconftest1.a >/dev/null 2>&1 ; then
                        if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest1.a >>conftest.out 2>&1 && test -x conftest ; then
			# Success!  Install works
			    ac_cv_prog_install_breaks_libs="no, with -p"
			fi
		    fi
                fi
            fi
        fi
    fi
fi
# This is needed for Mac OSX 10.5
rm -rf conftest.dSYM
rm -f conftest* libconftest*])

if test -z "$RANLIB_AFTER_INSTALL" ; then
    RANLIB_AFTER_INSTALL=no
fi
case "$ac_cv_prog_install_breaks_libs" in
	yes)
	    RANLIB_AFTER_INSTALL=yes
	;;
	"no, with -p")
	    INSTALL_DATA="$INSTALL_DATA -p"
	;;
	*)
	# Do nothing
	:
	;;
esac
AC_SUBST(RANLIB_AFTER_INSTALL)
])

#
# determine if the compiler defines a symbol containing the function name
# Inspired by checks within the src/mpid/globus/configure.in file in MPICH2
#
# These tests check not only that the compiler defines some symbol, such
# as __FUNCTION__, but that the symbol correctly names the function.
#
# Defines 
#   HAVE__FUNC__      (if __func__ defined)
#   HAVE_CAP__FUNC__  (if __FUNC__ defined)
#   HAVE__FUNCTION__  (if __FUNCTION__ defined)
#
AC_DEFUN([PAC_CC_FUNCTION_NAME_SYMBOL],[
AC_CACHE_CHECK([whether the compiler defines __func__],
pac_cv_have__func__,[
tmp_am_cross=no
AC_RUN_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__func__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have__func__=yes, pac_cv_have__func__=no,tmp_am_cross=yes)
if test "$tmp_am_cross" = yes ; then
    AC_LINK_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__func__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have__func__=yes, pac_cv_have__func__=no)
fi
])

if test "$pac_cv_have__func__" = "yes" ; then
    AC_DEFINE(HAVE__FUNC__,,[define if the compiler defines __func__])
fi

AC_CACHE_CHECK([whether the compiler defines __FUNC__],
pac_cv_have_cap__func__,[
tmp_am_cross=no
AC_RUN_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__FUNC__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have_cap__func__=yes, pac_cv_have_cap__func__=no,tmp_am_cross=yes)
if test "$tmp_am_cross" = yes ; then
    AC_LINK_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__FUNC__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have__func__=yes, pac_cv_have__func__=no)
fi
])

if test "$pac_cv_have_cap__func__" = "yes" ; then
    AC_DEFINE(HAVE_CAP__FUNC__,,[define if the compiler defines __FUNC__])
fi

AC_CACHE_CHECK([whether the compiler sets __FUNCTION__],
pac_cv_have__function__,[
tmp_am_cross=no
AC_RUN_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__FUNCTION__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have__function__=yes, pac_cv_have__function__=no,tmp_am_cross=yes)
if test "$tmp_am_cross" = yes ; then
    AC_LINK_IFELSE([
#include <string.h>
int foo(void);
int foo(void)
{
    return (strcmp(__FUNCTION__, "foo") == 0);
}
int main(int argc, char ** argv)
{
    return (foo() ? 0 : 1);
}
], pac_cv_have__func__=yes, pac_cv_have__func__=no)
fi
])

if test "$pac_cv_have__function__" = "yes" ; then
    AC_DEFINE(HAVE__FUNCTION__,,[define if the compiler defines __FUNCTION__])
fi

])
