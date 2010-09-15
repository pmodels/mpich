dnl
dnl   (C) 2001 by Argonne National Laboratory
dnl       See COPYRIGHT in top-level directory.
dnl
dnl       @author  Anthony Chan
dnl
dnl-------------------------------------------------------------------------
dnl JAC_TRY_COMPILE - test the compilation of java program
dnl
dnl JAC_TRY_COMPILE( JC, JFLAGS, IMPORTS, PROGRAM-BODY
dnl                  [, ACTION-IF-WORKING [ , ACTION-IF-NOT-WORKING ] ] )
dnl JC            - java compiler
dnl JFLAGS        - java compiler flags, like options: -d and -classpath, ...
dnl IMPORTS       - java import statements, besides top level "class" statement
dnl PROGRAM_BODY  - java program body
dnl
AC_DEFUN(JAC_TRY_COMPILE,[
dnl - set internal JC and JFLAGS variables
jac_JC="$1"
jac_JFLAGS="$2"
dnl - set the testing java program
changequote(,)
    rm -f conftest*
    cat > conftest.java <<EOF
$3
class conftest {
$4
}
EOF
changequote([,])
dnl
    jac_compile='${jac_JC} ${jac_JFLAGS} conftest.java 1>&AC_FD_CC'
    if AC_TRY_EVAL(jac_compile) && test -s conftest.class ; then
        ifelse([$5],,:,[rm -rf conftest* ; $5])
    else
        ifelse([$6],,:,[rm -rf conftest* ; $6])
    fi
])dnl
dnl
dnl JAC_TRY_RMIC - test the rmic program
dnl
dnl JAC_TRY_RMIC( RMIC, JRFLAGS, JC, JFLAGS
dnl               [, ACTION-IF-WORKING [ , ACTION-IF-NOT-WORKING ] ] )
dnl RMIC          - rmic compiler
dnl JRFLAGS       - rmic compiler flags, like options: -d and -classpath, ...
dnl JC            - java compiler
dnl JFLAGS        - java compiler flags, like options: -d and -classpath, ...
dnl
AC_DEFUN(JAC_TRY_RMIC,[
dnl - set internal RMIC and JRFLAGS variables
jac_RMIC="$1"
jac_JRFLAGS="$2"
dnl - set internal JC and JFLAGS variables
jac_JC="$3"
jac_JFLAGS="$4"
dnl - set the testing java program
changequote(,)
    rm -f conftest*
dnl
    cat > conftest_remote.java <<EOF
import java.rmi.*;
public interface conftest_remote extends Remote
{
    public void remote_interface() throws RemoteException;
}
EOF
dnl
    cat > conftest_rmic.java <<EOF
import java.rmi.*;
import java.rmi.server.*;
public class conftest_rmic extends UnicastRemoteObject
                           implements conftest_remote
{
    public conftest_rmic() throws RemoteException
    { super(); }
    public void remote_interface() throws RemoteException
    {}
}
EOF
changequote([,])
dnl
    jac_compile='${jac_JC} ${jac_JFLAGS} conftest_remote.java conftest_rmic.java 1>&AC_FD_CC'
    if AC_TRY_EVAL(jac_compile) && test -s conftest_rmic.class ; then
        jac_rmic='${jac_RMIC} ${jac_JRFLAGS} conftest_rmic 1>&AC_FD_CC'
        if AC_TRY_EVAL(jac_rmic) && test -s conftest_rmic_Stub.class ; then
            ifelse([$5],,:,[rm -rf conftest* ; $5])
        else
            ifelse([$6],,:,[rm -rf conftest* ; $6])
        fi
    else
        ifelse([$6],,:,[rm -rf conftest* ; $6])
    fi
])dnl
dnl
dnl JAC_FIND_PROG_IN_KNOWNS - locate Java program in standard known locations
dnl
dnl JAC_FIND_PROG_IN_KNOWNS( PROG_VAR, PROG-TO-CHECK-FOR
dnl                          [, TEST-ACTION-IF-FOUND] )
dnl
dnl PROG_VAR              - returned variable name of PROG-TO-CHECK-FOR
dnl PROG-TO-CHECK-FOR     - java program to check for, e.g. javac or java
dnl TEST-ACTION-IF-FOUND  - testing program for PROG-TO-CHECK-FOR.
dnl                         if TRUE,  it must return jac_prog_working=yes
dnl                         if FALSE, it must return jac_prog_working=no
dnl
AC_DEFUN([JAC_FIND_PROG_IN_KNOWNS],[
$1=""
# Determine the system type
AC_REQUIRE([AC_CANONICAL_HOST])
if test -d "/software/commmon" ; then
    subdir=/software/common
else
    subdir=""
fi
subdir_pfx=""
case "$host" in
    *irix*)
        subdir_pfx="irix"
        ;;
    *linux*)
        subdir_pfx="linux"
        ;;
    *solaris*)
        subdir_pfx="solaris"
        ;;
    *sun4*)
        subdir_pfx="sun4"
        ;;
    *aix*|*rs6000*)
        subdir_pfx="aix"
        ;;
    *freebsd*)
        subdir_pfx="freebsd"
esac
#
if test -n "$subdir_pfx" ; then
    for dir in /software/${subdir_pfx}* ; do
       test -d "$dir" && subdir=$dir
    done
fi
#
reverse_dirs=""
for dir in \
    /usr \
    /usr/jdk* \
    /usr/j2sdk* \
    /usr/java* \
    /usr/java/jdk* \
    /usr/java/j2sdk* \
    /usr/local \
    /usr/local/java* \
    /usr/local/jdk* \
    /usr/local/j2sdk* \
    /usr/local/diablo-jdk* \
    /usr/share \
    /usr/share/java* \
    /usr/share/jdk* \
    /usr/share/j2sdk* \
    /usr/contrib \
    /usr/contrib/java* \
    /usr/contrib/jdk* \
    /usr/contrib/j2sdk* \
    /usr/lib/jvm/java* \
    /System/Library/Frameworks/JavaVM.framework/Versions/*/Home \
    $HOME/java* \
    $HOME/jdk* \
    $HOME/j2sdk* \
    /opt/jdk* \
    /opt/j2sdk* \
    /opt/java* \
    /opt/local \
    /opt/local/jdk* \
    /opt/local/j2sdk* \
    /opt/local/java* \
    /Tools/jdk* \
    /Tools/j2sdk* \
    $subdir/apps/packages/java* \
    $subdir/apps/packages/jdk* \
    $subdir/apps/packages/j2sdk* \
    $subdir/com/packages/java* \
    $subdir/com/packages/jdk* \
    $subdir/com/packages/j2sdk* \
    /soft/apps/packages/java* \
    /soft/apps/packages/jdk* \
    /soft/apps/packages/j2sdk* \
    /soft/com/packages/java* \
    /soft/com/packages/jdk* \
    /soft/com/packages/j2sdk* \
    /local/encap/java* \
    /local/encap/j2sdk* \
    /local/encap/jdk* ; do
    if test -d $dir ; then
        reverse_dirs="$dir $reverse_dirs"
    fi
done
dnl
is_prog_found=no
for dir in $reverse_dirs ; do
    if test -d $dir ; then
        case "$dir" in
            *java-workshop* )
                if test -d "$dir/JDK/bin" ; then
                    if test -x "$dir/JDK/bin/$2" ; then
                        $1="$dir/JDK/bin/$2"
                    fi
                fi
                ;;
dnl         *java* | *jdk* | *j2sdk* | *Frameworks* )
            *)
                if test -x "$dir/bin/$2" ; then
                    $1="$dir/bin/$2"
                fi
                ;;
        esac
dnl
        # Not all releases work.Try tests defined in the 3rd argument
        if test -n "[$]$1" ; then
            AC_MSG_CHECKING([for $2 in known path])
            AC_MSG_RESULT([found [$]$1])
            is_prog_found=yes
            ifelse([$3],, [$1="" ; break], [
                $3
                if test "$jac_prog_working" = "yes" ; then
                    break
                else
                    $1=""
                fi
            ])
        fi
dnl
    fi
done
if test -z "[$]$1" -a "$is_prog_found" != "yes" ; then
    AC_MSG_CHECKING([for $2 in user's PATH])
    AC_MSG_RESULT(not found)
fi
])dnl
dnl
dnl JAC_FIND_PROG_IN_PATH - locate Java program in user's $PATH
dnl
dnl JAC_FIND_PROG_IN_PATH( PROG_VAR, PROG-TO-CHECK-FOR
dnl                        [, CHECKING-ACTION-IF-FOUND] )
dnl
dnl PROG_VAR              - returned variable name of PROG-TO-CHECK-FOR
dnl PROG-TO-CHECK-FOR     - java program to check for, e.g. javac or java
dnl TEST-ACTION-IF-FOUND  - testing program for PROG-TO-CHECK-FOR.
dnl                         if TRUE,  it must return jac_prog_working=yes
dnl                         if FALSE, it must return jac_prog_working=no
dnl
AC_DEFUN([JAC_FIND_PROG_IN_PATH], [
if test -n "$PATH" ; then
    $1=""
    is_prog_found=no
    dnl It is safer to create jac_PATH than modify IFS, because the potential
    dnl 3rd argument, TEST-ACTION, may contain code in modifying IFS
    jac_PATH=`echo $PATH | sed 's/:/ /g'`
    for dir in ${jac_PATH} ; do
        if test -d "$dir" -a -x "$dir/$2" ; then
            $1="$dir/$2"
            # Not all releases work.Try tests defined in the 3rd argument
            if test -n "[$]$1" ; then
                AC_MSG_CHECKING([for $2 in user's PATH])
                AC_MSG_RESULT([found [$]$1])
                is_prog_found=yes
                ifelse([$3],, [$1="" ; break], [
                    $3
                    if test "$jac_prog_working" = "yes" ; then
                        break
                    else
                        $1=""
                    fi
                ])
            fi
        fi
    done
    if test -z "[$]$1" -a "$is_prog_found" != "yes" ; then
        AC_MSG_CHECKING([for $2 in user's PATH])
        AC_MSG_RESULT(not found)
    fi
fi
])dnl
dnl
dnl JAC_CHECK_USER_PROG - check Java program in user supplied code,
dnl                       i.e. $JRE_TOPDIR/bin
dnl
dnl PROG_VAR              - returned variable name of PROG-TO-CHECK-FOR
dnl PROG-TO-CHECK-FOR     - java program to check for, e.g. javac or java
dnl TEST-ACTION-IF-FOUND  - testing program for PROG-TO-CHECK-FOR.
dnl                         if TRUE,  it must return jac_prog_working=yes
dnl                         if FALSE, it must return jac_prog_working=no
dnl
AC_DEFUN([JAC_CHECK_USER_PROG], [
if test "x$JRE_TOPDIR" != "x" ; then
    $1="$JRE_TOPDIR/bin/$2"
    is_prog_found=no
    if test -n "[$]$1" -a -x "[$]$1" ; then
        AC_MSG_CHECKING([for user supplied $2])
        AC_MSG_RESULT([found [$]$1])
        is_prog_found=yes
        ifelse([$3],,,[
            $3
            if test "$jac_prog_working" != "yes" ; then
                $1=""
            fi
        ])
    fi
    if test -z "[$]$1" -a "$is_prog_found" != "yes" ; then
        AC_MSG_CHECKING([for user supplied $2])
        AC_MSG_RESULT(not found)
    fi
fi
])dnl
dnl
dnl
dnl JAC_PATH_PROG - locate Java program in user's supplied path,
dnl                 user's $PATH and then the known locations.
dnl
dnl JAC_PATH_PROG( PROG_VAR, PROG-TO-CHECK-FOR [, CHECKING-ACTION-IF-FOUND] )
dnl
dnl PROG_VAR              - returned variable name of PROG-TO-CHECK-FOR
dnl PROG-TO-CHECK-FOR     - java program to check for, e.g. javac or java
dnl TEST-ACTION-IF-FOUND  - testing program for PROG-TO-CHECK-FOR.
dnl                         if TRUE,  it must return jac_prog_working=yes
dnl                         if FALSE, it must return jac_prog_working=no
dnl
AC_DEFUN([JAC_PATH_PROG], [
ifelse([$3],,
    [JAC_CHECK_USER_PROG($1, $2)],
    [JAC_CHECK_USER_PROG($1, $2, [$3])]
)
if test "x[$]$1" = "x" ; then
    ifelse([$3],,
        [JAC_FIND_PROG_IN_PATH($1, $2)],
        [JAC_FIND_PROG_IN_PATH($1, $2, [$3])]
    )
fi
if test "x[$]$1" = "x" ; then
    ifelse([$3],,
        [JAC_FIND_PROG_IN_KNOWNS($1, $2)],
        [JAC_FIND_PROG_IN_KNOWNS($1, $2, [$3])]
    )
fi
])
dnl
dnl JAC_JNI_HEADERS - locate Java Native Interface header files
dnl
dnl JAC_JNI_HEADER( JNI_INC [, JDK_TOPDIR] )
dnl
dnl JNI_INC    - returned JNI include flag
dnl JDK_TOPDIR - optional Java SDK directory.  If supplied, it will be updated
dnl              to reflect the JDK_TOPDIR used in JNI_INC
dnl
AC_DEFUN([JAC_JNI_HEADERS], [
AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
AC_REQUIRE([AC_PROG_CPP])dnl
is_jni_working=no
ifelse([$2],, :, [
    if test "x[$]$2" != "x" ; then
        jac_JDK_TOPDIR="[$]$2"
        AC_MSG_CHECKING([if $jac_JDK_TOPDIR exists])
        if test -d "$jac_JDK_TOPDIR" ; then
            AC_MSG_RESULT(yes)
            jac_jni_working=yes
        else
            AC_MSG_RESULT(no)
            jac_jni_working=no
        fi
 
        if test "$jac_jni_working" = "yes" ; then
            AC_MSG_CHECKING([for <jni.h> include flag])
            jac_JDK_INCDIR="$jac_JDK_TOPDIR/include"
            if test -d "$jac_JDK_INCDIR" -a -f "$jac_JDK_INCDIR/jni.h" ; then
                jac_JNI_INC="-I$jac_JDK_INCDIR"
                if test "$build_os" = "cygwin" ; then
                    jac_JAVA_ARCH=win32
                else
                    changequote(,)dnl
                    jac_JAVA_ARCH="`echo $build_os | sed -e 's%[-0-9].*%%'`"
                    changequote([,])dnl
                fi
                if test -d "$jac_JDK_INCDIR/$jac_JAVA_ARCH" ; then
                    jac_JNI_INC="$jac_JNI_INC -I$jac_JDK_INCDIR/$jac_JAVA_ARCH"
dnl             these 2 lines handle blackdown's JDK 117_v3
dnl             elif test -d "$jac_JDK_INCDIR/genunix" ; then
dnl                 jac_JNI_INC="$jac_JNI_INC -I$jac_JDK_INCDIR/genunix"
                fi
                AC_MSG_RESULT([found $jac_JNI_INC])
                jac_jni_working=yes
            else
                AC_MSG_RESULT([not found])
                jac_jni_working=no
            fi
        fi

        if test "$jac_jni_working" = "yes" ; then
            AC_MSG_CHECKING([for <jni.h> usability])
            jac_save_CPPFLAGS="$CPPFLAGS"
            CPPFLAGS="$jac_save_CPPFLAGS $jac_JNI_INC"
dnl Explicitly test for JNIEnv and jobject.
dnl <stdio.h> and <stdlib.h> are here to make sure include path like
dnl -I/usr/include/linux dnl won't be accepted.
            AC_TRY_COMPILE([
#include <jni.h>
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
                           ], [
    JNIEnv  *env;
    jobject  obj;
                           ], [jac_jni_working=yes], [jac_jni_working=no])
            CPPFLAGS="$jac_save_CPPFLAGS"
            if test "$jac_jni_working" = "yes" ; then
                $1="$jac_JNI_INC"
                ifelse($2,, :, [$2="$jac_JDK_TOPDIR"])
                AC_MSG_RESULT(yes)
            else
                $1=""
                AC_MSG_RESULT(no)
            fi
        fi

        if test "$jac_jni_working" = "yes" ; then
            is_jni_working=yes
        else
            is_jni_working=no
        fi
    fi
])

if test "$is_jni_working" = "no" ; then
    JAC_PATH_PROG(jac_JH, javah, [
dnl
        changequote(,)dnl
        jac_JDK_TOPDIR="`echo $jac_JH | sed -e 's%\(.*\)/[^/]*/[^/]*$%\1%'`"
        changequote([,])dnl
        AC_MSG_CHECKING([if $jac_JDK_TOPDIR exists])
        if test "X$jac_JDK_TOPDIR" != "X" -a -d "$jac_JDK_TOPDIR" ; then
            AC_MSG_RESULT(yes)
            jac_jni_working=yes
        else
            AC_MSG_RESULT(no)
            jac_jni_working=no
        fi
dnl
        if test "$jac_jni_working" = "yes" ; then
            AC_MSG_CHECKING([for <jni.h> include flag])
            jac_JDK_INCDIR="$jac_JDK_TOPDIR/include"
            if test -d "$jac_JDK_INCDIR" -a -f "$jac_JDK_INCDIR/jni.h" ; then
                jac_JNI_INC="-I$jac_JDK_INCDIR"
                if test "$build_os" = "cygwin" ; then
                    jac_JAVA_ARCH=win32
                else
                    changequote(,)dnl
                    jac_JAVA_ARCH="`echo $build_os | sed -e 's%[-0-9].*%%'`"
                    changequote([,])dnl
                fi
                if test -d "$jac_JDK_INCDIR/$jac_JAVA_ARCH" ; then
                    jac_JNI_INC="$jac_JNI_INC -I$jac_JDK_INCDIR/$jac_JAVA_ARCH"
dnl             these 2 lines handle blackdown's JDK 117_v3
dnl             elif test -d "$jac_JDK_INCDIR/genunix" ; then
dnl                 jac_JNI_INC="$jac_JNI_INC -I$jac_JDK_INCDIR/genunix"
                fi
                AC_MSG_RESULT([found $jac_JNI_INC])
                jac_jni_working=yes
            else
                AC_MSG_RESULT(no)
                jac_jni_working=no
            fi
        fi
dnl
        if test "$jac_jni_working" = "yes" ; then
            AC_MSG_CHECKING([for <jni.h> usability])
            jac_save_CPPFLAGS="$CPPFLAGS"
            CPPFLAGS="$jac_save_CPPFLAGS $jac_JNI_INC"
dnl Explicitly test for JNIEnv and jobject.
dnl <stdio.h> and <stdlib.h> are here to make sure include path like
dnl -I/usr/include/linux dnl won't be accepted.
            AC_TRY_COMPILE([
#include <jni.h>
#if defined( STDC_HEADERS ) || defined( HAVE_STDIO_H )
#include <stdio.h>
#endif
#if defined( STDC_HEADERS ) || defined( HAVE_STDLIB_H )
#include <stdlib.h>
#endif
                           ], [
    JNIEnv  *env;
    jobject  obj;
                           ], [jac_jni_working=yes], [jac_jni_working=no])
            CPPFLAGS="$jac_save_CPPFLAGS"
            AC_MSG_RESULT([$jac_jni_working])
        fi
        if test "$jac_jni_working" = "yes" ; then
            $1="$jac_JNI_INC"
            ifelse($2,, :, [$2="$jac_JDK_TOPDIR"])
            jac_prog_working=yes
        else
            $1=""
            jac_prog_working=no
        fi
    ])
fi
])dnl
dnl
dnl JAC_TRY_RUN - test the execution of a java class file
dnl
dnl JAC_TRY_RUN( JVM, JVMFLAGS, CLASS-FILE
dnl              [, ACTION-IF-WORKING [ , ACTION-IF-NOT-WORKING ] ] )
dnl JVM           - java virtual machine
dnl JVMFLAGS      - jVM flags, like options: -d and -classpath, ...
dnl CLASS-FILE    - java byte code, .class file, is assumed located at $srcdir
dnl                 i.e. relative path name from $srcdir
dnl
AC_DEFUN(JAC_TRY_RUN, [
dnl - set internal JVM and JVMFLAGS variables
jac_CPRP=cp
jac_JVM="$1"
jac_JVMFLAGS="$2"
dnl - set the testing java program
changequote(,)dnl
jac_basename="`echo $3 | sed -e 's%.*/\([^/]*\)$%\1%'`"
changequote([,])dnl
jac_baseclass="`echo $jac_basename | sed -e 's%.class$%%'`"
if test ! -f "$jac_basename" ; then
    if test -f "$srcdir/$3" ; then
        $jac_CPRP $srcdir/$3 .
    else
        AC_MSG_ERROR([$srcdir/$3 does NOT exist!])
    fi
fi
dnl
    jac_command='${jac_JVM} ${jac_JVMFLAGS} ${jac_baseclass} 1>&AC_FD_CC'
    if AC_TRY_EVAL(jac_command) ; then
        ifelse([$4],, :,[$4])
    else
        ifelse([$5],, :,[$5])
    fi
])dnl
dnl
dnl JAC_TRY_RUNJAR - test the execution of a java jar file
dnl
dnl JAC_TRY_RUNJAR( JVM, JVMFLAGS, JAR-FILE
dnl                 [, ACTION-IF-WORKING [ , ACTION-IF-NOT-WORKING ] ] )
dnl JVM           - java virtual machine
dnl JVMFLAGS      - jVM flags, like options: -d and -classpath, ...
dnl JAR-FILE      - java executable jar file, is assumed located at $srcdir
dnl                 i.e. relative path name from $srcdir
dnl
AC_DEFUN(JAC_TRY_RUNJAR, [
dnl - set internal JVM and JVMFLAGS variables
jac_CPRP=cp
jac_JVM="$1"
jac_JVMFLAGS="$2"
dnl - set the testing java program
changequote(,)dnl
jac_basename="`echo $3 | sed -e 's%.*/\([^/]*\)$%\1%'`"
changequote([,])dnl
if test ! -f "$jac_basename" ; then
    if test -f "$srcdir/$3" ; then
        $jac_CPRP $srcdir/$3 .
    else
        AC_MSG_ERROR([$srcdir/$3 does NOT exist!])
    fi
fi
dnl
    jac_command='${jac_JVM} ${jac_JVMFLAGS} -jar ${jac_basename} 1>&AC_FD_CC'
    if AC_TRY_EVAL(jac_command) ; then
        ifelse([$4],, :,[$4])
    else
        ifelse([$5],, :,[$5])
    fi
])dnl
dnl
dnl JAC_CHECK_CLASSPATH - check and fix the classpath
dnl
AC_DEFUN(JAC_CHECK_CLASSPATH, [
AC_MSG_CHECKING([if CLASSPATH is set])
if test "x$CLASSPATH" != "x" ; then
    AC_MSG_RESULT([yes])
    AC_MSG_CHECKING([if CLASSPATH contains current path])
    IFS="${IFS=   }"; jac_saved_ifs="$IFS"; IFS=":"
    jac_hasCurrPath=no
    for path_elem in $CLASSPATH ; do
        if test "X$path_elem" = "X." ; then
            jac_hasCurrPath=yes
        fi
    done
    IFS="$jac_saved_ifs"
    if test "$jac_hasCurrPath" = "no" ; then
        AC_MSG_RESULT([no, prepend . to CLASSPATH])
        CLASSPATH=".:$CLASSPATH"
        export CLASSPATH
    else
        AC_MSG_RESULT(yes)
    fi
else
    AC_MSG_RESULT([no, good to go])
fi
])
dnl
dnl JAC_CHECK_CYGPATH - check and set the cygpath
dnl
AC_DEFUN(JAC_CHECK_CYGPATH, [
AC_MSG_CHECKING([for cygpath])
jac_hasProg=no
IFS="${IFS=   }"; pac_saved_ifs="$IFS"; IFS=":"
dnl need to "" $path_elem because PATH may contains ...::...,
dnl hence $path_elem could be empty
for path_elem in $PATH ; do
    if test -d "$path_elem" -a -x "$path_elem/cygpath" ; then
        jac_hasProg=yes
        break
    fi
done
IFS="$pac_saved_ifs"
if test "$jac_hasProg" = "yes" ; then
    $1="\`cygpath -w "
    $2="\`"
    AC_MSG_RESULT(yes)
else
    $1=""
    $2=""
    AC_MSG_RESULT(no)
fi
])
