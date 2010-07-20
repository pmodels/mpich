dnl
dnl Check for BSD or POSIZ style global symbol lister, nm.
dnl If found, pac_path_NM_G contains the absolute athname of nm + options
dnl pac_path_NM_G_type will be either POSIX or BSD.  NM_G will be
dnl pac_path_NM_G without the absolute path.  Preference is BSD style.
dnl
dnl The test checks if nm accepts the known options and also if nm produces
dnl the expected BSD or POSIX output format.
dnl
AC_DEFUN([PAC_PATH_NM_G],[
AC_MSG_CHECKING([for BSD/POSIX style global symbol lister])
AC_LANG_PUSH(C)
AC_PATH_PROGS_FEATURE_CHECK(NM_G, nm, [
    # Check if nm accepts -g and BSD or POSIX compatible flag.
    # Use the `sed 1q' to avoid HP-UX's unknown option message:
    #   nm: unknown option "B" ignored
    # Tru64's nm complains that /dev/null is an invalid object file
    #
    # AIX's sed does not accept \+, 1) instead of doing 's|a\+||', do 's|aa*||'
    # or 2) instead of 's|A \+B|AB|g', do 's|A  *B|AB|g' 

    # Check if nm accepts -g
    case `${ac_path_NM_G} -g /dev/null 2>&1 | sed '1q'` in
    */dev/null* | *'Invalid file or object type'*)
        ac_path_NM_G="${ac_path_NM_G} -g"
        # Check if nm accepts -B
        case `${ac_path_NM_G} -B /dev/null 2>&1 | sed '1q'` in
        */dev/null* | *'Invalid file or object type'*)
            AC_COMPILE_IFELSE([
                AC_LANG_SOURCE([int iglobal;])
            ],[
                changequote(,)
                case `${ac_path_NM_G} -B conftest.$ac_objext | sed -e 's|[0-9][0-9]*  *[A-Z]  *iglobal|XXXX|g'` in
                *XXXX*)
                    pac_path_NM_G="${ac_path_NM_G} -B"
                    pac_path_NM_G_type="BSD"
                    ;;
                esac
                changequote([,])
            ])
            ;;
        *)
            # Check if nm accepts -P
            case `${ac_path_NM_G} -P /dev/null 2>&1 | sed '1q'` in
            */dev/null* | *'Invalid file or object type'*)
                AC_COMPILE_IFELSE([
                    AC_LANG_SOURCE([int iglobal;])
                ],[
                    changequote(,)
                    case `${ac_path_NM_G} -P conftest.$ac_objext | sed -e 's|iglobal  *[A-Z]  *[0-9][0-9]*|XXXX|g'` in
                    *XXXX*)
                        pac_path_NM_G="${ac_path_NM_G} -P"
                        pac_path_NM_G_type="POSIX"
                        ;;
                    esac
                    changequote([,])
                ])
                ;;
            esac  # Endof case `${ac_path_NM_G} -P
            ;;
        esac   # Endof case `${ac_path_NM_G} -B
        ;;
    esac  # Endof case `${ac_path_NM_G} -g
    if test "X$pac_path_NM_G" != "X" ; then
        AC_MSG_RESULT([$pac_path_NM_G_type style,$pac_path_NM_G])
        NM_G="`echo $pac_path_NM_G | sed -e 's|^.*nm |nm |g'`"
    else
        AC_MSG_RESULT(no)
    fi
    ac_cv_path_NM_G=${ac_path_NM_G}
    ac_path_NM_G_found=:
], [AC_MSG_RESULT(no)],
[$PATH$PATH_SEPARATOR/usr/ccs/bin/elf$PATH_SEPARATOR/usr/ccs/bin$PATH_SEPARATOR/usr/ucb$PATH_SEPARATOR/bin])
AC_LANG_POP(C)
]) dnl Endof AC_DEFUN([PAC_PATH_NM_G]
dnl
dnl
dnl The checks if Multiple __attribute__((alias)) is available
dnl If the multiple __attribute((alias)) support is there,
dnl define(HAVE_C_MULTI_ATTR_ALIAS) and pac_c_multi_attr_alias=yes.
dnl The default is to do a runtime test.  When cross_compiling=yes,
dnl pac_path_NM_G will be used to determine the test result.
dnl If CFLAGS(or CPPFLAGS) contains ATTR_ALIAS_DEBUG, the runtime will print
dnl out addresses of struct(s) for debugging purpose.
dnl
dnl
AC_DEFUN([PAC_C_MULTI_ATTR_ALIAS],[
AC_REQUIRE([PAC_PATH_NM_G])
AC_LANG_PUSH(C)
AC_CHECK_HEADERS([stdio.h])
AC_MSG_CHECKING([for multiple __attribute__((alias)) support])

#Compile the "other" __attribute__ object file.
AC_COMPILE_IFELSE([
    AC_LANG_SOURCE([
#if defined(HAVE_STDIO_H) || defined(STDC_HEADERS)
#include <stdio.h>
#endif

struct mpif_cmblk_t_ { int imember; };
typedef struct mpif_cmblk_t_ mpif_cmblk_t;

/* NOT initialize these structure so it appears in BSS or as COMMON symbols */
mpif_cmblk_t mpifcmb;
mpif_cmblk_t MPIFCMB;

/*
   Do the test in this file instead in the file
   where __attribute__((alias)) is used. 
   This is needed for pgcc since pgcc seems to
   define aliased symbols if they are in the same file.
*/
/*
    We can't do the following comparision in one test:

    ilogical = (( &mpifcmb == ptr && &MPIFCMB == ptr ) ? TRUE : FALSE) ;

    because some compiler, like gcc 4.4.2's -O* optimizes the code
    such that the ilogical expression is FALSE. The likely reason is that
    mpifcmb and MPIFCMB are defined in the same scope in which C optimizer
    may have treated them as different objects (with different addresses),
    &mpifcmb != &MPIFCMB, before actually running the test and hence the
    illogical expression is assumed to be always FALSE.  The solution taken
    here is to prevent the optimizer the opportunity to equate &mpifcmb and
    &MPIFCMB (in same scope), e.g. using 2 separate tests and combine the
    test results in a different scope.
*/
int same_addrs1( void *ptr );
int same_addrs1( void *ptr )
{
#if defined(ATTR_ALIAS_DEBUG)
    printf( "others: addr(mpifcmb)=%p, addr(input ptr)=%p\n", &mpifcmb, ptr );
#endif
    return ( &mpifcmb == ptr ? 1 : 0 );
}

int same_addrs2( void *ptr );
int same_addrs2( void *ptr )
{
#if defined(ATTR_ALIAS_DEBUG)
    printf( "others: addr(MPIFCMB)=%p, addr(input ptr)=%p\n", &MPIFCMB, ptr );
#endif
    return ( &MPIFCMB == ptr ? 1 : 0 );
}

    ])
],[
    rm -f pac_conftest_other.$ac_objext
    _AC_DO(cp conftest.$ac_objext pac_conftest_other.$ac_objext)
    test -s pac_conftest_other.$ac_objext && pac_c_attr_alias_other=yes
dnl     cp conftest.$ac_ext pac_conftest_other.$ac_ext
dnl     echo
dnl     echo "pac_conftest_other.$ac_objext"
dnl     nm -P -g pac_conftest_other.$ac_objext | grep -i "mpifcmb"
],[
    pac_c_attr_alias_other=no
])  dnl Endof AC_COMPILE_IFELSE

pac_c_attr_alias_main=no
if test "$pac_c_attr_alias_other" = "yes" ; then

#   Save LIBS for later restoration.
    saved_LIBS="$LIBS"
    LIBS="pac_conftest_other.$ac_objext $LIBS"

#   Link the "other" __attribute__ object file.
    AC_LINK_IFELSE([
        AC_LANG_PROGRAM([
#if defined(HAVE_STDIO_H) || defined(STDC_HEADERS)
#include <stdio.h>
#endif
 
struct mpif_cmblk_t_ { int imember; };
typedef struct mpif_cmblk_t_ mpif_cmblk_t;

mpif_cmblk_t mpifcmbr = {0};
extern mpif_cmblk_t MPIFCMB __attribute__ ((alias("mpifcmbr")));
extern mpif_cmblk_t mpifcmb __attribute__ ((alias("mpifcmbr")));

extern int same_addrs1( void *ptr );
extern int same_addrs2( void *ptr );

        ],[
    int iaddr = 0;
#if defined(ATTR_ALIAS_DEBUG)
    printf( "main: addr(mpifcmbr) = %p\n", &mpifcmbr );
    printf( "main: addr(mpifcmb) = %p\n", &mpifcmb );
    printf( "main: addr(MPIFCMB) = %p\n", &MPIFCMB );
#endif
    iaddr = same_addrs1( &mpifcmbr ) && same_addrs2( &mpifcmbr );
    FILE *file = fopen( "pac_conftestval", "w" );
    if (!file) return 1;
    fprintf( file, "%d\n", iaddr );
        ])
    ],[
        rm -f pac_conftest_main$ac_exeext
        _AC_DO([cp conftest$ac_exeext pac_conftest_main$ac_exeext])
        test -x pac_conftest_main$ac_exeext && pac_c_attr_alias_main=yes
dnl         cp conftest.$ac_ext pac_conftest_main.$ac_ext
dnl         echo
dnl         echo "pac_conftest_main$ac_exeext"
dnl         nm -P -g pac_conftest_main$ac_exeext | grep -i "mpifcmb"
    ],[
        pac_c_attr_alias_main=no
dnl         cp conftest.$ac_ext pac_conftest_main.$ac_ext
    ])  dnl Endof AC_LINK_IFELSE

# Restore the previously saved LIBS
    LIBS="$saved_LIBS"
    rm -f pac_conftest_other.$ac_objext
fi dnl Endof if test "$pac_c_attr_alias_other" = "yes"

if test "$pac_c_attr_alias_main" = "yes" ; then
    if test "$cross_compiling" = "yes" ; then
        changequote(,)
        # echo "PAC CROSS-COMPILING" dnl
        # POSIX NM = nm -P format dnl
        if test "$pac_path_NM_G_type" = "POSIX" ; then
            addrs=`${pac_path_NM_G} ./pac_conftest_main$ac_exeext | grep -i mpifcmb | sed -e 's% *[a-zA-Z][a-zA-Z]*  *[a-zA-Z]  *\([0-9abcdef][0-9abcdef]*\).*%\1%g'`
        fi

        # BSD NM = nm -B format dnl
        if test "$pac_path_NM_G_type" = "BSD" ; then
            addrs=`${pac_path_NM_G} -g ./pac_conftest_main$ac_exeext | grep -i mpifcmb | sed -e "s% *\([0-9abcdef][0-9abcdef]*\)  *[a-zA-Z]  *[a-zA-Z][a-zA-A]*.*%\1%g"`
        fi
        changequote([,])

        cmp_addr=""
        diff_addrs=no
        for addr in ${addrs} ; do
            if test "X${cmp_addr}" != "X" ; then
                if test "${cmp_addr}" != "${addr}" ; then
                    diff_addrs=yes
                    break
                fi
            else
                cmp_addr=${addr}
            fi
        done
        
        if test "$diff_addrs" != "yes" ; then
            dnl echo "Same addresses. Multiple aliases support"
            AC_MSG_RESULT([${NM_G} says yes])
            AC_DEFINE(HAVE_C_MULTI_ATTR_ALIAS, 1,
                      [Define if multiple __attribute__((alias)) are supported])
            pac_c_multi_attr_alias=yes
        else
            dnl echo "Different addresses. No multiple aliases support."
            AC_MSG_RESULT([${NM_G} says no])
            pac_c_multi_attr_alias=no
        fi

    else # if test "$cross_compiling" != "yes"
        rm -f pac_conftestval
        ac_try="./pac_conftest_main$ac_exeext"
        if AC_TRY_EVAL(ac_try) ; then
            pac_c_attr_alias_val=0
            if test -s pac_conftestval ; then
                eval pac_c_attr_alias_val=`cat pac_conftestval`
            fi
            if test "$pac_c_attr_alias_val" -eq 1 ; then
                AC_MSG_RESULT(yes)
                AC_DEFINE(HAVE_C_MULTI_ATTR_ALIAS, 1,
                          [Define if multiple __attribute__((alias)) are supported])
                pac_c_multi_attr_alias=yes
            else
                AC_MSG_RESULT(no)
                pac_c_multi_attr_alias=no
            fi
            rm -f pac_conftestval
        fi
    fi
    dnl Endof if test "$cross_compiling" = "yes"
    rm -f pac_conftest_main$ac_exeext
else
    AC_MSG_RESULT(no! link failure)
    pac_c_multi_attr_alias=no
fi dnl Endof if test "$pac_c_attr_alias_main" = "yes"

AC_LANG_POP(C)

]) dnl  Endof AC_DEFUN([PAC_C_MULTI_ATTR_ALIAS]
dnl
dnl Check if __attribute__((aligned)) is supported.
dnl If so, define HAVE_C_ATTR_ALIGNED and set pac_c_attr_aligned=yes.
dnl
dnl Do a link test instead of compile test to check if the linker
dnl would emit an error.
dnl
AC_DEFUN([PAC_C_ATTR_ALIGNED],[
AC_LANG_PUSH(C)
AC_MSG_CHECKING([for __attribute__((aligned)) support])
#Link the __attribute__ object file.
AC_LINK_IFELSE([
    AC_LANG_PROGRAM([
struct mpif_cmblk_t_ { int imembers[5]; };
typedef struct mpif_cmblk_t_ mpif_cmblk_t;
mpif_cmblk_t mpifcmbr __attribute__((aligned)) = {0};
    ],[])
],[pac_c_attr_aligned=yes], [pac_c_attr_aligned=no])
if test "$pac_c_attr_aligned" = "yes" ; then
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_C_ATTR_ALIGNED,1,
              [Define if __attribute__((aligned)) is supported.])
else
    AC_MSG_RESULT(no)
fi
AC_LANG_POP(C)
])
