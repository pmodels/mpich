dnl
dnl record top-level directory (this one)
dnl A problem.  Some systems use an NFS automounter.  This can generate
dnl paths of the form /tmp_mnt/... . On SOME systems, that path is
dnl not recognized, and you need to strip off the /tmp_mnt. On others, 
dnl it IS recognized, so you need to leave it in.  Grumble.
dnl The real problem is that OTHER nodes on the same NFS system may not
dnl be able to find a directory based on a /tmp_mnt/... name.
dnl
dnl It is WRONG to use $PWD, since that is maintained only by the C shell,
dnl and if we use it, we may find the 'wrong' directory.  To test this, we
dnl try writing a file to the directory and then looking for it in the 
dnl current directory.  Life would be so much easier if the NFS automounter
dnl worked correctly.
dnl
dnl PAC_GETWD(varname [, filename ] )
dnl 
dnl Set varname to current directory.  Use filename (relative to current
dnl directory) if provided to double check.
dnl
dnl Need a way to use "automounter fix" for this.
dnl
define(PAC_GETWD,[
AC_MSG_CHECKING(for current directory name)
$1=$PWD
if test "${$1}" != "" -a -d "${$1}" ; then 
    if test -r ${$1}/.foo$$ ; then
        rm -f ${$1}/.foo$$
        rm -f .foo$$
    fi
    if test -r ${$1}/.foo$$ -o -r .foo$$ ; then
        $1=
    else
        echo "test" > ${$1}/.foo$$
        if test ! -r .foo$$ ; then
            rm -f ${$1}/.foo$$
            $1=
        else
            rm -f ${$1}/.foo$$
        fi
    fi
fi
if test "${$1}" = "" ; then
    $1=`pwd | sed -e 's%/tmp_mnt/%/%g'`
fi
dnl
dnl First, test the PWD is sensible
ifelse($2,,,
if test ! -r ${$1}/$2 ; then
    dnl PWD must be messed up
    $1=`pwd`
    if test ! -r ${$1}/$2 ; then
        AC_MSG_ERROR([Cannot determine the root directory!])
    fi
    $1=`pwd | sed -e 's%/tmp_mnt/%/%g'`
    if test ! -d ${$1} ; then 
        AC_MSG_WARN([Warning: your default path uses the automounter; this may
cause some problems if you use other NFS-connected systems.])
        $1=`pwd`
    fi
fi)
if test -z "${$1}" ; then
    $1=`pwd | sed -e 's%/tmp_mnt/%/%g'`
    if test ! -d ${$1} ; then 
        AC_MSG_WARN([Warning: your default path uses the automounter; this may
cause some problems if you use other NFS-connected systems.])
        $1=`pwd`
    fi
fi
AC_MSG_RESULT(${$1})
])dnl
dnl
dnl This version compiles an entire function; used to check for
dnl things like varargs done correctly
dnl
dnl PAC_COMPILE_CHECK_FUNC(msg,function,if_true,if_false)
dnl
define(PAC_COMPILE_CHECK_FUNC,
[AC_PROVIDE([$0])dnl
ifelse([$1], , , [AC_MSG_CHECKING(for $1)]
)dnl
if test ! -f confdefs.h ; then
    AC_MSG_RESULT("!! SHELL ERROR !!")
    echo "The file confdefs.h created by configure has been removed"
    echo "This may be a problem with your shell; some versions of LINUX"
    echo "have this problem.  See the Installation guide for more"
    echo "information."
    exit 1
fi
cat > conftest.c <<EOF
#include "confdefs.h"
[$2]
EOF
dnl Don't try to run the program, which would prevent cross-configuring.
if { (eval echo configure:__oline__: \"$ac_compile\") 1>&5; (eval $ac_compile) 2>&5; }; then
  ifelse([$1], , , [AC_MSG_RESULT(yes)])
  ifelse([$3], , :, [rm -rf conftest*
  $3
])
ifelse([$4], , , [else
  rm -rf conftest*
  $4
])dnl
   ifelse([$1], , , ifelse([$4], ,else) [AC_MSG_RESULT(no)])
fi
rm -f conftest*]
)dnl
dnl
dnl PAC_OUTPUT_EXEC(files[,mode]) - takes files (as shell script or others),
dnl and applies configure to the them.  Basically, this is what AC_OUTPUT
dnl should do, but without adding a comment line at the top.
dnl Must be used ONLY after AC_OUTPUT (it needs config.status, which 
dnl AC_OUTPUT creates).
dnl Optionally, set the mode (+x, a+x, etc)
dnl
define(PAC_OUTPUT_EXEC,[
CONFIG_FILES="$1"
export CONFIG_FILES
./config.status
CONFIG_FILES=""
for pac_file in $1 ; do 
    rm -f .pactmp
    sed -e '1d' $pac_file > .pactmp
    rm -f $pac_file
    mv .pactmp $pac_file
    ifelse($2,,,chmod $2 $pac_file)
done
])dnl
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
dnlD*/
AC_DEFUN(PAC_PROG_CC,[
AC_PROVIDE([AC_PROG_CC])
AC_CHECK_PROGS(CC, cc xlC xlc pgcc icc gcc )
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
dnl
dnl This is a replacement that checks that FAILURES are signaled as well
dnl (later configure macros look for the .o file, not just success from the
dnl compiler, but they should not HAVE to
dnl
dnl --- insert 2.52 compatibility here ---
dnl 2.52+ does not have AC_PROG_CC_GNU
ifdef([AC_PROG_CC_GNU],,[AC_DEFUN([AC_PROG_CC_GNU],)])
dnl 2.52 does not have AC_PROG_CC_WORKS
ifdef([AC_PROG_CC_WORKS],,[AC_DEFUN([AC_PROG_CC_WORKS],)])
dnl
dnl
AC_DEFUN(PAC_PROG_CC_WORKS,
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
dnl ***TAKEN FROM sowing/confdb/aclocal_cc.m4 IF YOU FIX THIS, FIX THAT
dnl VERSION AS WELL
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
dnl NOT TESTED
AC_DEFUN(PAC_PROG_C_BROKEN_COMMON,[
AC_MSG_CHECKING([whether global variables handled properly])
AC_REQUIRE([AC_PROG_RANLIB])
ac_cv_prog_cc_globals_work=no
echo 'extern int a; int a;' > conftest1.c
echo 'extern int a; int main( ){ return a; }' > conftest2.c
if ${CC-cc} $CFLAGS -c conftest1.c >conftest.out 2>&1 ; then
    if ${AR-ar} cr libconftest.a conftest1.o >/dev/null 2>&1 ; then
        if ${RANLIB-:} libconftest.a >/dev/null 2>&1 ; then
            if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a >> conftest.out 2>&1 ; then
                # Success!  C works
                ac_cv_prog_cc_globals_work=yes
            else
                # Failure!  Do we need -fno-common?
                ${CC-cc} $CFLAGS -fno-common -c conftest1.c >> conftest.out 2>&1
                rm -f libconftest.a
                ${AR-ar} cr libconftest.a conftest1.o >>conftest.out 2>&1
                ${RANLIB-:} libconftest.a >>conftest.out 2>&1
                if ${CC-cc} $CFLAGS -o conftest conftest2.c $LDFLAGS libconftest.a >> conftest.out 2>&1 ; then
                    ac_cv_prog_cc_globals_work="needs -fno-common"
                    CFLAGS="$CFLAGS -fno-common"
                fi
            fi
        fi
    fi
fi
rm -f conftest* libconftest*
AC_MSG_RESULT($ac_cv_prog_cc_globals_work)
])dnl
dnl
dnl PAC_MSG_ERROR($enable_softerr,ErrorMsg) -
dnl return AC_MSG_ERROR(ErrorMsg) if "$enable_softerr" = "yes"
dnl return AC_MSG_WARN(ErrorMsg) + exit 0 otherwise
dnl
define(PAC_MSG_ERROR,[
if test "$1" = "yes" ; then
    AC_MSG_WARN([ $2 ])
    exit 0
else
    AC_MSG_ERROR([ $2 ])
fi
])dnl
dnl
dnl
dnl Use the value of enable-strict to update CFLAGS
dnl
dnl -std=c89 is used to select the C89 version of the ANSI/ISO C standard.
dnl As of this writing, many C compilers still accepted only this version,
dnl not the later C99 version.  When all compilers accept C99, this
dnl should be changed to the appropriate standard level.  Note that we've
dnl had trouble with gcc 2.95.3 accepting -std=c89 but then trying to
dnl compile program with a invalid set of options
dnl (-D __STRICT_ANSI__-trigraphs)
dnl
AC_DEFUN(PAC_CHECK_GCC_STD_C89,[
AC_MSG_CHECKING([whether $1 accepts -std=c89])
# We must know the compiler type, assumed used in PAC_GET_GCC_STRICT_FLAGS.
dnl if test -z "CC" ; then
dnl    AC_CHECK_PROGS(CC,gcc)
dnl fi
# See if we can add -std=c89
savedCFLAGS="[$]$1"
$1="[$]$1 -std=c89"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,[int a;])],
      stdc_ok=yes,
      stdc_ok=no)
AC_MSG_RESULT($stdc_ok)
if test "$stdc_ok" != yes ; then
   $1="$savedCFLAGS"
fi
])dnl
dnl
dnl Modified from mpich2/confdb/aclocal_cc.m4's PAC_CC_STRICT,
dnl remove all reference to enable_strict_done.  Also, make it
dnl more flexible by appending the content of $1 with the
dnl --enable-strict flags.
dnl
dnl PAC_GET_GCC_STRICT_CFLAGS([COPTIONS])
dnl COPTIONS    - returned variable with --enable-strict flags appended.
dnl
dnl Use the value of enable-strict to update input COPTIONS.
dnl be sure no space is inserted after "(" and before ")", otherwise invalid
dnl /bin/sh shell statement like 'COPTIONS  ="$COPTIONS ..."' will be resulted.
dnl
dnl AC_PROG_CC should be called before this macro function.
dnl
AC_DEFUN(PAC_GET_GCC_STRICT_FLAGS,[
# We must know the compiler type
if test -z "CC" ; then
    AC_CHECK_PROGS(CC,gcc)
fi
case "$enable_strict" in
    yes)
        AC_MSG_CHECKING( [whether $1 accepts strict compiler flags] )
        if test "$ac_cv_prog_gcc" = "yes" ; then
            $1="[$]$1 -Wall -O2 -Wstrict-prototypes -Wmissing-prototypes -Wundef -Wpointer-arith -Wbad-function-cast -ansi -DGCC_WALL"
            AC_MSG_RESULT([yes])
            PAC_CHECK_GCC_STD_C89($1)
        else
            AC_MSG_WARN([no, strict support for gcc only!])
        fi
        ;;
    all)
        AC_MSG_CHECKING( [whether $1 accepts strict compiler flags] )
        if test "$ac_cv_prog_gcc" = "yes" ; then
            $1="[$]$1 -Wall -O -Wstrict-prototypes -Wmissing-prototypes -Wundef -Wpointer-arith -Wbad-function-cast -ansi -DGCC_WALL -Wunused -Wshadow -Wmissing-declarations -Wno-long-long"
            AC_MSG_RESULT([yes, all possible flags.])
            PAC_CHECK_GCC_STD_C89($1)
        else
            AC_MSG_WARN([no, strict support for gcc only!])
        fi
        ;;
    posix)
        AC_MSG_CHECKING( [whether $1 accepts strict compiler flags] )
        if test "$ac_cv_prog_gcc" = "yes" ; then
            $1="[$]$1 -Wall -O2 -Wstrict-prototypes -Wmissing-prototypes -Wundef -Wpointer-arith -Wbad-function-cast -ansi -DGCC_WALL -D_POSIX_C_SOURCE=199506L"
            AC_MSG_RESULT([yes, POSIX flavored flags.])
            PAC_CHECK_GCC_STD_C89($1)
        else
            AC_MSG_WARN([no, strict support for gcc only!])
        fi
        ;;
    noopt)
        AC_MSG_CHECKING( [whether $1 accepts strict compiler flags] )
        if test "$ac_cv_prog_gcc" = "yes" ; then
            $1="[$]$1 -Wall -Wstrict-prototypes -Wmissing-prototypes -Wundef -Wpointer-arith -Wbad-function-cast -ansi -DGCC_WALL"
            AC_MSG_RESULT([yes, non-optimized flags.])
            PAC_CHECK_GCC_STD_C89($1)
        else
            AC_MSG_WARN([no, strict support for gcc only!])
        fi
        ;;
    no)
        # Accept and ignore this value
        :
        ;;
    *)
        if test -n "$enable_strict" ; then
            AC_MSG_WARN([Unrecognized value for enable-strict:$enable_strict])
        fi
        ;;
esac
])dnl
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
dnl D*/
AC_DEFUN(PAC_FUNC_NEEDS_DECL,[
AC_CACHE_CHECK([whether $2 needs a declaration],
pac_cv_func_decl_$2,[
AC_TRY_COMPILE([$1],[int a=$2(1.0,27,1.0,"foo");],
pac_cv_func_decl_$2=yes,pac_cv_func_decl_$2=no)])
if test "$pac_cv_func_decl_$2" = "yes" ; then
changequote(<<,>>)dnl
define(<<PAC_FUNC_NAME>>, translit(NEEDS_$2_DECL, [a-z *], [A-Z__]))dnl
changequote([, ])dnl
    AC_DEFINE_UNQUOTED(PAC_FUNC_NAME,1,[Define if $2 needs a declaration])
undefine([PAC_FUNC_NAME])
fi
])dnl
dnl/*D
dnl PAC_PROG_CHECK_INSTALL_WORKS - Check whether the install program in INSTALL
dnl works.
dnl
dnl Synopsis:
dnl PAC_PROG_CHECK_INSTALL_WORKS
dnl
dnl Output Effect:
dnl   Sets the variable 'INSTALL' to the value of 'ac_sh_install' if 
dnl   a file cannot be installed into a local directory with the 'INSTALL'
dnl   program
dnl
dnl Notes:
dnl   The 'AC_PROG_INSTALL' scripts tries to avoid broken versions of 
dnl   install by avoiding directories such as '/usr/sbin' where some 
dnl   systems are known to have bad versions of 'install'.  Unfortunately, 
dnl   this is exactly the sort of test-on-name instead of test-on-capability
dnl   that 'autoconf' is meant to eliminate.  The test in this script
dnl   is very simple but has been adequate for working around problems 
dnl   on Solaris, where the '/usr/sbin/install' program (known by 
dnl   autoconf to be bad because it is in /usr/sbin) is also reached by a 
dnl   soft link through /bin, which autoconf believes is good.
dnl
dnl   No variables are cached to ensure that we do not make a mistake in 
dnl   our choice of install program.
dnl
dnl   The Solaris configure requires the directory name to immediately
dnl   follow the '-c' argument, rather than the more common 
dnl.vb
dnl      args sourcefiles destination-dir
dnl.ve
dnl D*/
AC_DEFUN([PAC_PROG_CHECK_INSTALL_WORKS],[
if test -z "$INSTALL" ; then
    AC_MSG_RESULT([No install program available])
else
    # Check that this install really works
    rm -f conftest
    echo "Test file" > conftest
    if test ! -d .conftest ; then mkdir .conftest ; fi
    AC_MSG_CHECKING([whether install works])
    if $INSTALL conftest .conftest >/dev/null 2>&1 ; then
        installOk=yes
    else
        installOk=no
    fi
    rm -rf .conftest conftest
    AC_MSG_RESULT($installOk)
    if test "$installOk" = no ; then
        if test -n "$ac_install_sh" ; then
            INSTALL=$ac_install_sh
        else
            AC_MSG_ERROR([Unable to find working install])
        fi
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
rm -f libconftest* conftest*
echo 'int foo(int);int foo(int a){return a;}' > conftest1.c
echo 'extern int foo(int); int main( int argc, char **argv){ return foo(0); }' > conftest2.c
if ${CC-cc} $CFLAGS -c conftest1.c >conftest.out 2>&1 ; then
    if ${AR-ar} cr libconftest.a conftest1.o >/dev/null 2>&1 ; then
        if ${RANLIB-:} libconftest.a >/dev/null 2>&1 ; then
            # Anything less than sleep 10, and Mac OS/X (Darwin)
            # will claim that install works because ranlib won't complain
            sleep 10
            libinstall="$INSTALL"
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
rm -f conftest* libconftest*])

if test -z "$RANLIB_AFTER_INSTALL" ; then
    RANLIB_AFTER_INSTALL=no
fi
case "$ac_cv_prog_install_breaks_libs" in
        yes)
            RANLIB_AFTER_INSTALL=yes
        ;;
        "no, with -p")
            INSTALL="$INSTALL -p"
        ;;
        *)
        # Do nothing
        :
        ;;
esac
AC_SUBST(RANLIB_AFTER_INSTALL)
])
dnl
dnl
dnl
dnl Fixes to bugs in AC_xxx macros
dnl 
dnl (AC_TRY_COMPILE is missing a newline after the end in the Fortran
dnl branch; that has been fixed in-place)
dnl
dnl (AC_PROG_CC makes many dubious assumptions.  One is that -O is safe
dnl with -g, even with gcc.  This isn't true; gcc will eliminate dead code
dnl when -O is used, even if you added code explicitly for debugging 
dnl purposes.  -O shouldn't do dead code elimination when -g is selected, 
dnl unless a specific option is selected.  Unfortunately, there is no
dnl documented option to turn off dead code elimination.
dnl
dnl
dnl (AC_CHECK_HEADER and AC_CHECK_HEADERS both make the erroneous assumption
dnl that the C-preprocessor and the C (or C++) compilers are the same program
dnl and have the same search paths.  In addition, CHECK_HEADER looks for 
dnl error messages to decide that the file is not available; unfortunately,
dnl it also interprets messages such as "evaluation copy" and warning messages
dnl from broken CPP programs (such as IBM's xlc -E, which often warns about 
dnl "lm not a valid option").  Instead, we try a compilation step with the 
dnl C compiler.
dnl
dnl AC_CONFIG_AUX_DIRS only checks for install-sh, but assumes other
dnl values are present.  Also doesn't provide a way to override the
dnl sources of the various configure scripts.  This replacement
dnl version of AC_CONFIG_AUX_DIRS overcomes this.
dnl Internal subroutine.
dnl Search for the configuration auxiliary files in directory list $1.
dnl We look only for install-sh, so users of AC_PROG_INSTALL
dnl do not automatically need to distribute the other auxiliary files.
dnl AC_CONFIG_AUX_DIRS(DIR ...)
dnl Also note that since AC_CONFIG_AUX_DIR_DEFAULT calls this, there
dnl isn't a easy way to fix it other than replacing it completely.
dnl This fix applies to 2.13
dnl/*D
dnl AC_CONFIG_AUX_DIRS - Find the directory containing auxillery scripts
dnl for configure
dnl
dnl Synopsis:
dnl AC_CONFIG_AUX_DIRS( [ directories to search ] )
dnl
dnl Output Effect:
dnl Sets 'ac_config_guess' to location of 'config.guess', 'ac_config_sub'
dnl to location of 'config.sub', 'ac_install_sh' to the location of
dnl 'install-sh' or 'install.sh', and 'ac_configure' to the location of a
dnl Cygnus-style 'configure'.  Only 'install-sh' is guaranteed to exist,
dnl since the other scripts are needed only by some special macros.
dnl
dnl The environment variable 'CONFIG_AUX_DIR', if set, overrides the
dnl directories listed.  This is an extension to the 'autoconf' version of
dnl this macro. 
dnl D*/
undefine([AC_CONFIG_AUX_DIRS])
AC_DEFUN(AC_CONFIG_AUX_DIRS,
[if test -f $CONFIG_AUX_DIR/install-sh ; then ac_aux_dir=$CONFIG_AUX_DIR 
else
ac_aux_dir=
# We force the test to use the absolute path to ensure that the install
# program can be used if we cd to a different directory before using
# install.
for ac_dir in $1; do
  if test -f $ac_dir/install-sh; then
    ac_aux_dir=$ac_dir
    abs_ac_aux_dir=`(cd $ac_aux_dir && pwd)`
    ac_install_sh="$abs_ac_aux_dir/install-sh -c"
    break
  elif test -f $ac_dir/install.sh; then
    ac_aux_dir=$ac_dir
    abs_ac_aux_dir=`(cd $ac_aux_dir && pwd)`
    ac_install_sh="$abs_ac_aux_dir/install.sh -c"
    break
  fi
done
fi
if test -z "$ac_aux_dir"; then
  AC_MSG_ERROR([can not find install-sh or install.sh in $1])
fi
ac_config_guess=$ac_aux_dir/config.guess
ac_config_sub=$ac_aux_dir/config.sub
ac_configure=$ac_aux_dir/configure # This should be Cygnus configure.
AC_PROVIDE([AC_CONFIG_AUX_DIR_DEFAULT])dnl
])
