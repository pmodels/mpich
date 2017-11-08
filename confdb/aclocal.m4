dnl This version of aclocal.m4 simply includes all of the individual
dnl components
builtin(include,aclocal_am.m4)
builtin(include,aclocal_bugfix.m4)
builtin(include,aclocal_cache.m4)
builtin(include,aclocal_cc.m4)
builtin(include,aclocal_atomic.m4)
dnl aclocal_cross.m4 uses autoconf features dated back to 2.13.
dnl too old to be useful, 07/14/2010.
dnl builtin(include,aclocal_cross.m4)
builtin(include,aclocal_cxx.m4)
builtin(include,aclocal_f77.m4)
dnl aclocal_f77old.m4 contains PAC_PROG_F77_CMDARGS which is NOT used in MPICH
dnl but it is still used by other packages, so leave it in confdb.
dnl builtin(include,aclocal_f77old.m4)
builtin(include,aclocal_util.m4)
builtin(include,aclocal_subcfg.m4)
builtin(include,aclocal_make.m4)
builtin(include,aclocal_mpi.m4)
builtin(include,aclocal_shl.m4)
dnl fortran90.m4 defines [Fortran 90] as an AC_LANG
dnl which works for autoconf 2.63 and older, 07/14/2010.
dnl builtin(include,fortran90.m4)
builtin(include,aclocal_runlog.m4)
builtin(include,aclocal_fc.m4)
builtin(include,aclocal_libs.m4)
builtin(include,aclocal_attr_alias.m4)
builtin(include,ax_tls.m4)
builtin(include,aclocal_romio.m4)
dnl Add the libtool files that libtoolize wants
dnl Comment these out until libtool support is enabled.
dnl May need to change this anyway, since libtoolize 
dnl does not seem to understand builtin
dnl builtin(include,libtool.m4)
dnl builtin(include,ltoptions.m4)
dnl builtin(include,ltversion.m4)
dnl builtin(include,ltsugar.m4)
dnl builtin(include,lt~obsolete.m4)
