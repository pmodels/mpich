dnl This version of aclocal.m4 simply includes all of the individual
dnl components
builtin(include,aclocal_am.m4)
builtin(include,aclocal_bugfix.m4)
builtin(include,aclocal_cache.m4)
builtin(include,aclocal_cc.m4)
builtin(include,aclocal_atomic.m4)
builtin(include,aclocal_cross.m4)
builtin(include,aclocal_cxx.m4)
builtin(include,aclocal_f77.m4)
builtin(include,aclocal_util.m4)
builtin(include,aclocal_subcfg.m4)
builtin(include,aclocal_make.m4)
builtin(include,aclocal_mpi.m4)
builtin(include,aclocal_shl.m4)
builtin(include,fortran90.m4)
builtin(include,aclocal_libs.m4)
builtin(include,aclocal_attr_alias.m4)
builtin(include,ax_tls.m4)
dnl Add the libtool files that libtoolize wants
dnl Comment these out until libtool support is enabled.
dnl May need to change this anyway, since libtoolize 
dnl does not seem to understand builtin
dnl builtin(include,libtool.m4)
dnl builtin(include,ltoptions.m4)
dnl builtin(include,ltversion.m4)
dnl builtin(include,ltsugar.m4)
dnl builtin(include,lt~obsolete.m4)
