dnl
dnl PAC_GET_STDINT_HEADER - Get system header that defines C99 intXX_t.
dnl
dnl PAC_GET_STDINT_HEADER( STDINT_H )
dnl
dnl STDINT_H      - returned header which defines C99 intXX_t. Empty string
dnl                 if there is no system header that defines intXX_t.
dnl
AC_DEFUN( [PAC_GET_STDINT_HEADER], [
AC_MSG_CHECKING( [for header that defines C99 integer types, intXX_t] )
$1=""
for hdr in stdint.h inttypes.h sys/inttypes.h ; do
    AC_COMPILE_IFELSE( [
        AC_LANG_PROGRAM( [#include <$hdr>], [
            int8_t   ii08 = 0;
            int16_t  ii16 = 0;
            int32_t  ii32 = 0;
            int64_t  ii64 = 0;
        ] )
    ], [
        $1=$hdr
        break
    ] )
done
if test -n "[$]$1" ; then
    AC_MSG_RESULT( [<[$]$1>] )
else
    AC_MSG_RESULT( [none] )
fi
] )dnl
dnl
dnl
dnl PAC_GET_BASIC_INT_TYPES - Get stdint types based on basic types, i.e.
dnl                           char, short, int, long & long long.
dnl
dnl PAC_GET_BASIC_INT_TYPES( INT8, INT16, INT32, INT64 )
dnl
dnl INT8              - returned int8_t.
dnl INT16             - returned int16_t.
dnl INT32             - returned int32_t.
dnl INT64             - returned int64_t.
dnl
AC_DEFUN( [PAC_GET_BASIC_INT_TYPES], [
#   dnl Determine the sizeof(char)
    AC_CHECK_SIZEOF(char)
    if test "$ac_cv_sizeof_char" = 1 ; then
        $1="char"
    else
        PAC_MSG_ERROR( $enable_softerror,
                       [sizeof(char) isn't 1! Aborting...] )
    fi

#   dnl Determine the sizeof(short)
    AC_CHECK_SIZEOF(short)
    if test "$ac_cv_sizeof_short" = 2 ; then
        $2="short"
    else
        PAC_MSG_ERROR( $enable_softerror,
                       [sizeof(short) isn't 2! Aborting...] )     fi

#   dnl Determine the sizeof(int)
    AC_CHECK_SIZEOF(int)
    if test "$ac_cv_sizeof_int" = 4 ; then
        $3="int"
    else
        PAC_MSG_ERROR( $enable_softerror,
                       [sizeof(int) isn't 4! Aborting...] )
    fi

#   dnl Determine the sizeof(long) and sizeof(long long)
    AC_CHECK_SIZEOF(long)
    AC_CHECK_SIZEOF(long long)
    if test "$ac_cv_sizeof_long" = 8 ; then
        $4=long
    else
        if test "$ac_cv_sizeof_long_long" = 8 ; then
            $4="long long"
        else
            PAC_MSG_ERROR( $enable_softerror,
                [Neither sizeof(long) or sizeof(long long) is 8! Aborting...] )
        fi
    fi
] ) dnl
dnl
dnl
dnl PAC_GET_STDINT_FORMATS - Get printf format specifiers for intXX_t's
dnl
dnl PAC_GET_STDINT_FORMATS( INT8, INT16, INT32, INT64,
dnl                         INT8FMT, INT16FMT, INT32FMT, INT64FMT,
dnl                         INTFMT_H )
dnl
dnl INT8                  - int8_t or char.
dnl INT16                 - int16_t or short.
dnl INT32                 - int32_t or int.
dnl INT64                 - int64_t or long/long long.
dnl INT8FMT               - returned format specifiers for int8_t.
dnl INT16FMT              - returned format specifiers for int16_t.
dnl INT32FMT              - returned format specifiers for int32_t.
dnl INT64FMT              - returned format specifiers for int64_t.
dnl INTFMT_H              - returned headers for the format specifiers.
dnl
AC_DEFUN( [PAC_GET_STDINT_FORMATS], [
mpe_c99_inttypes_h=""
if test "$3" = "int32_t" -a "$4" = "int64_t" ; then
    AC_CHECK_HEADERS( inttypes.h, [
    dnl If inttypes.h exists, check for PRIdXX
        AC_MSG_CHECKING( [whether <inttypes.h> defines the PRIdXX macros] )
        AC_COMPILE_IFELSE( [
            AC_LANG_PROGRAM( [
                #include <inttypes.h>
                #ifdef HAVE_STDIO_H
                #include <stdio.h>
                #endif
                #ifdef HAVE_SYS_TYPES_H
                #include <sys/types.h>
                #endif
                #ifdef HAVE_SYS_BITYPES_H
                #include <sys/bitypes.h>
                #endif
            ], [
                printf( "%" PRId8 "\n",  ($1) 1 );
                printf( "%" PRId16 "\n", ($2) 1 );
                printf( "%" PRId32 "\n", ($3) 1 );
                printf( "%" PRId64 "\n", ($4) 1 );
            ] )
        ], [
            AC_MSG_RESULT(yes)
            mpe_c99_inttypes_h="inttypes.h"
        ], [
            AC_MSG_RESULT(no)
        ] ) dnl Endof AC_COMPILE_IFELSE
    ] ) dnl Endof AC_CHECK_HEADERS
fi
if test "X$mpe_c99_inttypes_h" != "X" ; then
    $5="\"%\"PRId8"
    $6="\"%\"PRId16"
    $7="\"%\"PRId32"
    $8="\"%\"PRId64"
    $9="$mpe_c99_inttypes_h"
else
    $5="\"%d\""
    $6="\"%d\""
    $7="\"%d\""
    if test "$4" = "long" ; then
        $8="\"%ld\""
    else
        $8="\"%lld\""
    fi
    $9=""
fi
] ) dnl
dnl
dnl
dnl PAC_OUTPUT_STDINT_HEADER - Create/Output an header file that define
dnl                            integer types.
dnl
dnl PAC_OUTPUT_STDINT_HEADER( OUTPUT_H, STDINT_H, PRIDXX_H,
dnl                           INT8, INT16, INT32, INT64,
dnl                           INT8FMT, INT16FMT, INT32FMT, INT64FMT,
dnl
dnl OUTPUT_H              - output header
dnl STDINT_H              - header that defines integer types
dnl PRIDXX_H              - header that defines integer printf format specifiers
dnl INT8                  - int8_t or char.
dnl INT16                 - int16_t or short.
dnl INT32                 - int32_t or int.
dnl INT64                 - int64_t or long/long long.
dnl INT8FMT               - returned format specifiers for int8_t.
dnl INT16FMT              - returned format specifiers for int16_t.
dnl INT32FMT              - returned format specifiers for int32_t.
dnl INT64FMT              - returned format specifiers for int64_t.
dnl
AC_DEFUN( [PAC_OUTPUT_STDINT_HEADER], [
AC_CONFIG_COMMANDS( [$pac_output_h], [
    AC_MSG_NOTICE( [creating $pac_output_h] )
    rm -f $pac_output_h
    echo "/* MPE logging header for C99 integer types */" > $pac_output_h
    echo "#if !defined( _CLOG_STDINT )" >> $pac_output_h
    echo "#define  _CLOG_STDINT" >> $pac_output_h
    echo >> $pac_output_h
#   dnl /* Put in the system headers */
    if test "X$pac_c99_stdint_h" != "X" ; then
        echo "#include <$pac_c99_stdint_h>" >> $pac_output_h
        if test "X$pac_c99_pridxx_h" != "X" ; then
            if test "$pac_c99_pridxx_h" != "$pac_c99_stdint_h" ; then
                echo "#include <$pac_c99_pridxx_h>" >> $pac_output_h
            fi
        fi
    fi
    echo >> $pac_output_h
    echo "typedef $pac_int8_t     CLOG_int8_t;"  >> $pac_output_h
    echo "typedef $pac_int16_t    CLOG_int16_t;" >> $pac_output_h
    echo "typedef $pac_int32_t    CLOG_int32_t;" >> $pac_output_h
    echo "typedef $pac_int64_t    CLOG_int64_t;" >> $pac_output_h
    echo >> $pac_output_h
#   dnl /* Define iXXfmt printf format specifiers */
    echo "#define i8fmt     $pac_int8_fmt"  >> $pac_output_h
    echo "#define i16fmt    $pac_int16_fmt" >> $pac_output_h
    echo "#define i32fmt    $pac_int32_fmt" >> $pac_output_h
    echo "#define i64fmt    $pac_int64_fmt" >> $pac_output_h
    echo >> $pac_output_h
    echo "#endif" >> $pac_output_h
], [
    PACKAGE="$PACKAGE"
    VERSION="$VERSION"
    pac_output_h="$1"
    pac_c99_stdint_h="$2"
    pac_c99_pridxx_h="$3"
    pac_int8_t="$4"
    pac_int16_t="$5"
    pac_int32_t="$6"
    pac_int64_t="$7"
    pac_int8_fmt="$8"
    pac_int16_fmt="$9"
    pac_int32_fmt="$10"
    pac_int64_fmt="$11"
] ) dnl Endof AC_CONFIG_COMMANDS()
] ) dnl
