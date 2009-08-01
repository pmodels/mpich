dnl
dnl PAC_APPEND_UNIQ_TOKEN( VARIABLE_NAME, TOKEN )
dnl token is defined as string without any blank space in it.
dnl
AC_DEFUN([PAC_APPEND_UNIQ_TOKEN],[
dnl Since PAC_APPEND_UNIQ_TOKEN may be called by PAC_MAKE_UNIQ_STRING
dnl saved IFS should be in a different variable than MAKE_UNIQ_STRING's.
dnl Otherwise bad thing will happen.
save_IFS="$IFS"
dnl eval flags=\$$1
flags="[$]$1"
token=$2
dnl
IFS=' '
match=no
for flag in $flags ; do
    if test "$flag" = "$token" ; then
        match=yes
        break
    fi
done
if test "$match" = "no" ; then
    flags="$flags $token"
fi
dnl
dnl eval $1='$flags'
$1="$flags"
dnl
IFS="$save_IFS"
])dnl
dnl
dnl
dnl PAC_MAKE_UNIQ_STRING - Eliminate the duplicated substrings in the input
dnl                        string variable.
dnl Assumptions:
dnl 1) substring separator of the input string is blank space.
dnl 2) the input string does not contain any '\n' or '\&' character.
dnl
dnl The function has the following properties:
dnl 1) preserves the blank space within pair of double and single quotes. 
dnl 2) preserves the order of the unique substring.
dnl 3) elminated duplicated substring will be later in order.
dnl
dnl PAC_MAKE_UNIQ_STRING( VARIABLE_NAME, [REMOVED_NAME] )
dnl
dnl Input/Output
dnl VARIABLE_NAME - name of the variable containing duplicated substrings.
dnl                 On output, VARIABLE will contain no duplicated substring.
dnl REMOVED_NAME  - name of the (optional) variable containing the elminated
dnl                 duplicated substings.
dnl
AC_DEFUN([PAC_MAKE_UNIQ_STRING], [
saveIFS="$IFS"
dnl eval inflags=\$$1
inflags="[$]$1"
dnl Define separator to be used as replacement of blanks in "..." or '...'
pac_ifs='\&'
dnl
dnl For Double Quote: Replace "pppp  qqqq" as "pppp&&qqqq"
dnl
IFS=$pac_ifs
dnl Prepend '\&' before beginning '\"', Append '\&' after ending '\"'
outflags=`echo $inflags | sed -e 's%\"\([^\"]*\)\"%'"$pac_ifs"'&'"$pac_ifs"'%g'`
dnl
dnl Use \& as separator to locate "...." where ' ' will be replaced by '\&'
dnl
tokens=""
for outflag in $outflags ; do
    case $outflag in
    \"*\")
        dnl Replace ' ' by $pac_ifs
        flag=`echo $outflag | sed -e 's/ /'"$pac_ifs"'/g'`
        tokens=$tokens$flag
        ;;
    *)
        tokens=$tokens$outflag
        ;;
    esac
done
inflags=$tokens
dnl
dnl For Single Quote: Replace 'pppp  qqqq' as 'pppp&&qqqq'
dnl
IFS=$pac_ifs
dnl Prepend '\&' before beginning '\'', Append '\&' after ending '\''
outflags=`echo $inflags | sed -e "s%\'\([^\']*\)\'%""$pac_ifs""&""$pac_ifs""%g"`
dnl
dnl Use \& as separator to locate '....' where ' ' will be replaced by '\&'
dnl
tokens=""
for outflag in $outflags ; do
    case $outflag in
    \'*\')
        dnl Replace ' ' by $pac_ifs
        flag=`echo $outflag | sed -e 's/ /'"$pac_ifs"'/g'`
        tokens=$tokens$flag
        ;;
    *)
        tokens=$tokens$outflag
        ;;
    esac
done
dnl
dnl Compute the unique token list and duplicated token list.
dnl
IFS=' '
uniqtokens=""
dupltokens=""
for token in $tokens ; do
    match=no
    for uniqtoken in $uniqtokens ; do
        if test "$uniqtoken" = "$token" ; then
            match=yes
            break
        fi
    done
    if test "$match" = "yes" ; then
        dupltokens="$dupltokens $token"
    else
        uniqtokens="$uniqtokens $token"
    fi
done
dnl Restore $pac_ifs to the original, i.e. ' '.
uniqflags=`echo $uniqtokens | sed -e 's%'"$pac_ifs"'% %g'`
duplflags=`echo $dupltokens | sed -e 's%'"$pac_ifs"'% %g'`
dnl eval $1='$uniqflags'
$1="$uniqflags"
ifelse([$2],[],:,[$2="$duplflags"])
dnl
IFS="$saveIFS"
])dnl
dnl the following macros are from aclocal_flags.m4
dnl NOTE: do not use these macros recursively, you will be very sad
dnl FIXME this probably should be modified to take a namespacing parameter
AC_DEFUN([PAC_SAVE_FLAGS],[
	pac_save_CFLAGS=$CFLAGS
	pac_save_CXXFLAGS=$CXXFLAGS
	pac_save_FFLAGS=$FFLAGS
	pac_save_F90FLAGS=$F90FLAGS
	pac_save_LDFLAGS=$LDFLAGS
])

dnl NOTE: do not use these macros recursively, you will be very sad
dnl FIXME this probably should be modified to take a namespacing parameter
AC_DEFUN([PAC_RESTORE_FLAGS],[
	CFLAGS=$pac_save_CFLAGS
	CXXFLAGS=$pac_save_CXXFLAGS
	FFLAGS=$pac_save_FFLAGS
	F90FLAGS=$pac_save_F90FLAGS
	LDFLAGS=$pac_save_LDFLAGS
])

dnl Usage: PAC_APPEND_FLAG([-02], [$CFLAGS])
dnl need a clearer explanation and definition of how this is called
AC_DEFUN([PAC_APPEND_FLAG],[dnl
AC_REQUIRE([AC_PROG_FGREP])dnl
AS_IF(dnl
 [echo "$$2" | $FGREP -e '$1' >/dev/null 2>&1],
 [echo "$2(='$$2') contains '$1', not appending" >&AS_MESSAGE_LOG_FD],
 [echo "$2(='$$2') does not contain '$1', appending" >&AS_MESSAGE_LOG_FD
  $2="$$2 $1"])dnl
])dnl
