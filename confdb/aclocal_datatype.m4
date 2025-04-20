dnl Utilities in shell functions - reduces size in configure script
dnl These functions sets $pac_retval
dnl
AC_DEFUN([PAC_DATATYPE_UTILS], [
# Take decimal and turn it into two hex digits
to_hex() {
    len=$[]1
    if test -z $len ; then
        pac_retval="00"
    elif test $len -le 9 ; then
        dnl avoid subshell for speed
        pac_retval="0${len}"
    elif test $len -eq 16 ; then
        pac_retval="10"
    elif test $len -eq 32 ; then
        pac_retval="20"
    else
        dnl printf is portable enough
        pac_retval=`printf "%02x" $len`
    fi
}

# Convert hex values in the form of 0x######## to decimal for
# datatype constants in Fortran.
to_dec() {
    orig_value=$[]1
    dnl printf is portable enough
    pac_retval=`printf "%d" $orig_value`
}

# return the C type matching integer of size
get_c_int_type() {
    len=$[]1
    pac_retval=
    for c_type in char short int long "long_long" __int128 ; do
        eval ctypelen=\$"ac_cv_sizeof_$c_type"
        if test "$len" = "$ctypelen" -a "$ctypelen" -gt 0 ; then
            if test "$c_type" = "long_long" ; then
                pac_retval=`echo $c_type | sed -e 's/_/ /g'`
            else
                pac_retval=$c_type
            fi
            return
        fi
    done
    pac_retval="unavailable"
}

# return the C type matching float of size
get_c_float_type() {
    len=$[]1
    pac_retval=
    for c_type in float double _Float16 __float128 ; do
        eval ctypelen=\$"ac_cv_sizeof_$c_type"
        if test "$len" = "$ctypelen" -a "$ctypelen" -gt 0 ; then
            if test "$c_type" = "long_double" ; then
                pac_retval=`echo $c_type | sed -e 's/_/ /g'`
            else
                pac_retval=$c_type
            fi
            return
        fi
    done
    pac_retval="unavailable"
}

# return (via $pac_retval) the C type matching integer of size
get_c_bool_type() {
    len=$[]1
    pac_retval=
    for c_type in _Bool unsigned_char unsigned_short unsigned_int unsigned_long unsigned_long_long ; do
        eval ctypelen=\$"ac_cv_sizeof_$c_type"
        if test "$len" = "$ctypelen" -a "$ctypelen" -gt 0 ; then
            if test "$c_type" != "_Bool" ; then
                pac_retval=`echo $c_type | sed -e 's/_/ /g'`
            else
                pac_retval=$c_type
            fi
            return
        fi
    done
    pac_retval="unavailable"
}
])
