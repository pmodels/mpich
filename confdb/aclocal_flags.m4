AC_DEFUN([PAC_SAVE_FLAGS],[
	pac_save_CFLAGS=$CFLAGS
	pac_save_CXXFLAGS=$CXXFLAGS
	pac_save_FFLAGS=$FFLAGS
	pac_save_F90FLAGS=$F90FLAGS
	pac_save_LDFLAGS=$LDFLAGS
])

AC_DEFUN([PAC_RESTORE_FLAGS],[
	CFLAGS=$pac_save_CFLAGS
	CXXFLAGS=$pac_save_CXXFLAGS
	FFLAGS=$pac_save_FFLAGS
	F90FLAGS=$pac_save_F90FLAGS
	LDFLAGS=$pac_save_LDFLAGS
])

dnl Usage: PAC_APPEND_FLAGS(-02, CFLAGS)
AC_DEFUN([PAC_APPEND_FLAGS],[
	accepted_flags=""
	for input_flag in $1 ; do
	    found=no
	    for flag in ${$2} ; do
	    	if test "$input_flag" = "$flag" ; then
		   found=yes
		fi
	    done
	    if test "$found" = "no" ; then
	       accepted_flags="$accepted_flags $input_flag"
	    fi
	done
	$2="${$2} $accepted_flags"
])
