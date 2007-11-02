dnl
dnl Borrowed (and altered) from Bill Gropp's confdb PAC_PROG_CHECK_INSTALL_WORKS.
dnl
dnl Fixes to bugs in AC_xxx macros
dnl 
dnl MPD_AC_PROG_CHECK_INSTALL_WORKS - Check whether the install program in INSTALL
dnl works.
dnl
dnl Synopsis:
dnl MPD_AC_PROG_CHECK_INSTALL_WORKS
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
AC_DEFUN([MPD_AC_PROG_CHECK_INSTALL_WORKS],[
# Check that this install really works
rm -f conftest
echo "Test file" > conftest
if test ! -d .conftest ; then mkdir .conftest ; fi
AC_MSG_CHECKING([whether install $INSTALL works])
if $INSTALL -m 644 conftest .conftest >/dev/null 2>&1 ; then
    installOk=yes
else
    installOk=no
fi
rm -rf .conftest conftest
AC_MSG_RESULT($installOk)
dnl if test "$installOk" = no ; then
    dnl AC_MSG_ERROR([Unable to find working install])
dnl fi
dnl installOk=no  # RMB DEBUG
])
