dnl Utilities in shell functions - reduces size in configure script
dnl These functions sets $pac_retval
dnl
AC_DEFUN([PAC_DATATYPE_UTILS], [
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
