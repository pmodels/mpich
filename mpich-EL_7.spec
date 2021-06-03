%global daos_major 1

Summary:        A high-performance implementation of MPI
Name:           mpich
Version:        3.4~a2
Release:        3%{?dist}
License:        MIT
URL:            http://www.mpich.org/

Source0:        http://www.mpich.org/static/downloads/%{version}/%{name}-%{version}.tar.gz
Source1:        mpich.macros
Source2:        mpich.pth.py2
Source3:        mpich.pth.py3
Patch0:         mpich-modules.patch
# Source0 is already patched, taken directly from the git branch
# sadly we cannot do this on EL7 due to git being too old
##Patch10:        daos_adio-all.patch
#Patch10:        daos_adio.patch
#Patch11:        daos_adio-hwloc.patch
#Patch12:        daos_adio-izem.patch
#Patch13:        daos_adio-libfabric.patch
#Patch14:        daos_adio-ucx.patch
Patch1:         fix-version.patch

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  gcc-gfortran
BuildRequires:  hwloc-devel >= 1.8
%ifnarch s390 %{mips}
BuildRequires:  valgrind-devel
%endif
# For python[23]_sitearch
%if (0%{?fedora} >= 30)
BuildRequires:  python2-devel
BuildRequires:  python3-devel
BuildRequires:  rpm-mpi-hooks
%else
BuildRequires:  python-devel
BuildRequires:  python36-devel
%endif
BuildRequires:  automake >= 1.15
BuildRequires:  libtool >= 2.4.4
BuildRequires:  daos-devel
BuildRequires:  libuuid-devel
Provides:       mpi
Provides:       mpich2 = %{version}
Obsoletes:      mpich2 < 3.0
# the standard EL7 compatibility package
Obsoletes:      mpich-3.0 < 3.1
# and it's standard package
Obsoletes:      mpich-3.2 < 3.3
Requires:       environment(modules)
Provides:       bundled(hwloc) = 2.0.1rc2
Provides:       %{name}-daos-%{daos_major}

%description
MPICH is a high-performance and widely portable implementation of the Message
Passing Interface (MPI) standard (MPI-1, MPI-2 and MPI-3). The goals of MPICH
are: (1) to provide an MPI implementation that efficiently supports different
computation and communication platforms including commodity clusters (desktop
systems, shared-memory systems, multicore architectures), high-speed networks
(10 Gigabit Ethernet, InfiniBand, Myrinet, Quadrics) and proprietary high-end
computing systems (Blue Gene, Cray) and (2) to enable cutting-edge research in
MPI through an easy-to-extend modular framework for other derived
implementations.

The mpich binaries in this RPM packages were configured to use the default
process manager (Hydra) using the default device (ch3). The ch3 device
was configured with support for the nemesis channel that allows for
shared-memory and TCP/IP sockets based communication.

This build also include support for using the 'module environment' to select
which MPI implementation to use when multiple implementations are installed.
If you want MPICH support to be automatically loaded, you need to install the
mpich-autoload package.

%package autoload
Summary:        Load mpich automatically into profile
Requires:       mpich = %{version}-%{release}
Provides:       mpich2-autoload = 3.0.1
Obsoletes:      mpich2-autoload < 3.0
# the standard EL7 compatibility package
Obsoletes:      mpich-3.0-autoload < 3.1
# and it's standard package
Obsoletes:      mpich-3.2-autoload < 3.3
Provides:       %{name}-autoload-daos-%{daos_major}

%description autoload
This package contains profile files that make mpich automatically loaded.

%package devel
Summary:        Development files for mpich
Provides:       %{name}-devel-static = %{version}-%{release}
Requires:       %{name} = %{version}-%{release}
Requires:       pkgconfig
Requires:       gcc-gfortran
%if (0%{?fedora} >= 30)
Requires:       rpm-mpi-hooks
%endif
Requires:       daos-devel
Provides:       mpich2-devel = 3.0.1
Obsoletes:      mpich2-devel < 3.0
# the standard EL7 compatibility package
Obsoletes:      mpich-3.0-devel < 3.1
# and it's standard package
Obsoletes:      mpich-3.2-devel < 3.3

%description devel
Contains development headers and libraries for mpich

%package doc
Summary:        Documentations and examples for mpich
BuildArch:      noarch
Requires:       %{name}-devel = %{version}-%{release}
Provides:       mpich2-doc = 3.0.1
Obsoletes:      mpich2-doc < 3.0
# the standard EL7 compatibility package
Obsoletes:      mpich-3.0-doc < 3.1
# and it's standard package
Obsoletes:      mpich-3.2-doc < 3.3

%description doc
Contains documentations, examples and man-pages for mpich

%package -n python2-mpich
Summary:        mpich support for Python 2
Provides:       %{name}-python2-mpich-daos-%{daos_major}

%description -n python2-mpich
mpich support for Python 2.

%package -n python3-mpich
Summary:        mpich support for Python 3
Provides:       %{name}-python3-mpich-daos-%{daos_major}

%description -n python3-mpich
mpich support for Python 3.

# We only compile with gcc, but other people may want other compilers.
# Set the compiler here.
%{!?opt_cc: %global opt_cc gcc}
%{!?opt_fc: %global opt_fc gfortran}
%{!?opt_f77: %global opt_f77 gfortran}
# Optional CFLAGS to use with the specific compiler...gcc doesn't need any,
# so uncomment and undefine to NOT use
%{!?opt_cc_cflags: %global opt_cc_cflags %{optflags}}
%{!?opt_fc_fflags: %global opt_fc_fflags %{optflags}}
#%%{!?opt_fc_fflags: %%global opt_fc_fflags %%{optflags} -I%%{_fmoddir}}
%{!?opt_f77_fflags: %global opt_f77_fflags %{optflags}}

%ifarch s390
%global m_option -m31
%else
%global m_option -m%{__isa_bits}
%endif

%ifarch %{arm} aarch64 %{mips}
%global m_option ""
%endif

%global selected_channels ch3:nemesis

%ifarch %{ix86} x86_64 s390 %{arm} aarch64
%global XFLAGS -fPIC
%endif

%prep
%setup -q
# we patched autoconf.ac (and friends) so need to regnerate configure
./autogen.sh
%patch0 -p1
%patch1 -p1

%build
%configure      \
        --enable-sharedlibs=gcc                                 \
        --enable-shared                                         \
        --enable-static=no                                      \
        --enable-lib-depend                                     \
        --disable-rpath                                         \
        --disable-silent-rules                                  \
        --enable-fc                                             \
        --with-device=%{selected_channels}                      \
        --with-pm=hydra:gforker                                 \
        --includedir=%{_includedir}/%{name}-%{_arch}            \
        --bindir=%{_libdir}/%{name}/bin                         \
        --libdir=%{_libdir}/%{name}/lib                         \
        --datadir=%{_datadir}/%{name}                           \
        --mandir=%{_mandir}/%{name}-%{_arch}                    \
        --docdir=%{_datadir}/%{name}/doc                        \
        --htmldir=%{_datadir}/%{name}/doc                       \
        --with-hwloc=embedded                                   \
        --enable-fortran=all                                    \
        --enable-romio                                          \
        --with-file-system=ufs+daos                             \
        --with-daos=/usr                                        \
        --with-cart=/usr                                        \
        --disable-checkerrors                                   \
        --disable-perftest                                      \
        --disable-large-tests                                   \
        --disable-ft-tests                                      \
        --disable-comm-overlap-tests                            \
        --enable-threads=single                                 \
        FC=%{opt_fc}                                            \
        F77=%{opt_f77}                                          \
        CFLAGS="%{m_option} -O2 %{?XFLAGS}"                     \
        CXXFLAGS="%{m_option} -O2 %{?XFLAGS}"                   \
        FCFLAGS="%{m_option} -O2 %{?XFLAGS}"                    \
        FFLAGS="%{m_option} -O2 %{?XFLAGS}"                     \
        LDFLAGS='-Wl,-z,noexecstack'                            \
        MPICHLIB_CFLAGS="%{?opt_cc_cflags}"                     \
        MPICHLIB_CXXFLAGS="%{optflags}"                         \
        MPICHLIB_FCFLAGS="%{?opt_fc_fflags}"                    \
        MPICHLIB_FFLAGS="%{?opt_f77_fflags}"
#       MPICHLIB_LDFLAGS='-Wl,-z,noexecstack'                   \
#       MPICH_MPICC_FLAGS="%{m_option} -O2 %{?XFLAGS}"  \
#       MPICH_MPICXX_FLAGS="%{m_option} -O2 %{?XFLAGS}" \
#       MPICH_MPIFC_FLAGS="%{m_option} -O2 %{?XFLAGS}"  \
#       MPICH_MPIF77_FLAGS="%{m_option} -O2 %{?XFLAGS}"
#       --with-openpa-prefix=embedded                           \

#       FCFLAGS="%{?opt_fc_fflags} -I%{_fmoddir}/%{name} %{?XFLAGS}"    \

# fix aclocal-1.15 references.  i am sure there is a more proper
# way to do this
#exit 1
#find . -name Makefile | xargs sed -i -e 's/aclocal-1.15/aclocal/g'

# Remove rpath
sed -r -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -r -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

#Try and work around 'unused-direct-shlib-dependency' rpmlint warnning
sed -i -e 's| -shared | -Wl,--as-needed\0|g' libtool

%make_build VERBOSE=1

# We want the ROMIO test suite also
# See comment above about why we cannot
#pushd src/mpi/romio/test
#%make_build VERBOSE=1
#popd

%install
%make_install

mkdir -p %{buildroot}%{_fmoddir}/%{name}
mv  %{buildroot}%{_includedir}/%{name}-*/*.mod %{buildroot}%{_fmoddir}/%{name}/
sed -r -i 's|^modincdir=.*|modincdir=%{_fmoddir}/%{name}|' %{buildroot}%{_libdir}/%{name}/bin/mpifort

# Install the module file
mkdir -p %{buildroot}%{_sysconfdir}/modulefiles/mpi
sed -r 's|%{_bindir}|%{_libdir}/%{name}/bin|;
        s|@LIBDIR@|%{_libdir}/%{name}|;
        s|@MPINAME@|%{name}|;
        s|@py2sitearch@|%{python2_sitearch}|;
        s|@py3sitearch@|%{python3_sitearch}|;
        s|@ARCH@|%{_arch}|;
        s|@fortranmoddir@|%{_fmoddir}|;
     ' \
     <src/packaging/envmods/mpich.module \
     >%{buildroot}%{_sysconfdir}/modulefiles/mpi/%{name}-%{_arch}

mkdir -p %{buildroot}%{_sysconfdir}/profile.d
cat >%{buildroot}%{_sysconfdir}/profile.d/mpich-%{_arch}.sh <<EOF
# Load mpich environment module
module load mpi/%{name}-%{_arch}
EOF
cp -p %{buildroot}%{_sysconfdir}/profile.d/mpich-%{_arch}.{sh,csh}

# Install the RPM macros
install -pDm0644 %{SOURCE1} %{buildroot}%{_rpmconfigdir}/macros.d/macros.%{name}

# Install the .pth files
mkdir -p %{buildroot}%{python2_sitearch}/%{name}
install -pDm0644 %{SOURCE2} %{buildroot}%{python2_sitearch}/%{name}.pth
mkdir -p %{buildroot}%{python3_sitearch}/%{name}
install -pDm0644 %{SOURCE3} %{buildroot}%{python3_sitearch}/%{name}.pth

# We want the ROMIO test suite also
# but we cannot build it here.  it expects mpich to be installed
# it doesn't know how to use the mpich already built in the local
# source tree
#pushd src/mpi/romio/test
#%make_install
#popd

find %{buildroot} -type f -name "*.la" -delete

%check
# disabled due to https://github.com/pmodels/mpich/issues/4534
#make check VERBOSE=1

%if (0%{?fedora} >= 30)
%ldconfig_scriptlets
%else
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%endif


%files
%license COPYRIGHT
%doc CHANGES README README.envvar RELEASE_NOTES
%dir %{_libdir}/%{name}
%dir %{_libdir}/%{name}/lib
%dir %{_libdir}/%{name}/bin
%{_libdir}/%{name}/lib/*.so.*
%{_libdir}/%{name}/bin/hydra*
%{_libdir}/%{name}/bin/mpichversion
%{_libdir}/%{name}/bin/mpiexec*
%{_libdir}/%{name}/bin/mpirun
%{_libdir}/%{name}/bin/mpivars
%{_libdir}/%{name}/bin/parkill
#dir #{_mandir}/#{name}-#{_arch}
#doc #{_mandir}/#{name}-#{_arch}/man1/
%{_sysconfdir}/modulefiles/mpi/

%files autoload
%{_sysconfdir}/profile.d/mpich-%{_arch}.*

%files devel
%{_includedir}/%{name}-%{_arch}/
%{_libdir}/%{name}/lib/pkgconfig/
%{_libdir}/%{name}/lib/*.so
%{_libdir}/%{name}/bin/mpicc
%{_libdir}/%{name}/bin/mpic++
%{_libdir}/%{name}/bin/mpicxx
%{_libdir}/%{name}/bin/mpif77
%{_libdir}/%{name}/bin/mpif90
%{_libdir}/%{name}/bin/mpifort
%{_fmoddir}/%{name}/
%{_rpmconfigdir}/macros.d/macros.%{name}
#{_mandir}/#{name}-#{_arch}/man3/

%files doc
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/doc/

%files -n python2-mpich
%dir %{python2_sitearch}/%{name}
%{python2_sitearch}/%{name}.pth

%files -n python3-mpich
%dir %{python3_sitearch}/%{name}
%{python3_sitearch}/%{name}.pth

%changelog
* Thu May 27 2021 Mohamad Chaarawi <mohamad.chaarawi@intel.com> - 3.4~a2-3
- Replace --with-hwloc-prefix with --with-hwloc on configure command

* Wed Jan 20 2021 Kenneth Cain <kenneth.c.cain@intel.com> - 3.4~a2-2
- Update packaging for building with libdaos.so.1

* Tue May 12 2020 Brian J. Murrell <brian.murrell@intel.com> - 3.4~a2-1
- Update to 3.4a2
- Disabled %check due to https://github.com/pmodels/mpich/issues/4534
- Added switches to configure:
        --disable-checkerrors
        --disable-perftest
        --disable-large-tests
        --disable-ft-tests
        --disable-comm-overlap-tests
        --enable-threads=single
- Added switch from configure:
        --enable-cxx                                            \

* Wed Dec 18 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-5
- Rebuild with CaRT SO version 4
- Add Provides: to allow consumers to target cart and daos ABI versions

* Tue Dec 10 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-4
- Another Update packaging standards

* Fri Nov 22 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-3
- Rebuild with newer CaRT SO version

* Sat Nov 02 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-2
- Another update packaging standards

* Fri Aug 30 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-1
- Update packaging standards

* Fri Aug 30 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-0.04
- Fix ABI version after upstream master merge

* Sat Jul 13 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-0.03
- Update python3 requirements to python36

* Tue Jul 09 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-0.02
- Add some commented out code to build the romio test suite
- Update from upstream git repo

* Tue Jun 18 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-0.01
- Tweak and build for EL7
- Replace the standard EL7 3.0 compatibility package and 3.2 package
- Add DAOS support

* Fri Feb 01 2019 Fedora Release Engineering <releng@fedoraproject.org> - 3.2.1-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_30_Mass_Rebuild

* Fri Jul 13 2018 Fedora Release Engineering <releng@fedoraproject.org> - 3.2.1-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_29_Mass_Rebuild

* Tue Jun 19 2018 Miro Hrončok <mhroncok@redhat.com> - 3.2.1-7
- Rebuilt for Python 3.7

* Tue Jun 19 2018 Miro Hrončok <mhroncok@redhat.com> - 3.2.1-6
- Rebuilt for Python 3.7

* Wed Apr  4 2018 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.2.1-5
- Update MANPATH so that normal man pages can still be found (#1533717)

* Thu Feb 08 2018 Fedora Release Engineering <releng@fedoraproject.org> - 3.2.1-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_28_Mass_Rebuild

* Thu Feb 01 2018 Ralf Corsépius <corsepiu@fedoraproject.org> - 3.2.1-3
- Rebuilt for GCC-8.0.1.

* Sun Nov 12 2017 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.2.1-2
- Update $modincdir in mpifort after moving .mod files (#1301533)
- Move compiler wrappers to mpich-devel (#1353621)
- Remove bogus rpath (#1361586)

* Sun Nov 12 2017 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.2.1-1
- Update to latest bugfix release (#1512188)

* Thu Aug 03 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.2-10
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Binutils_Mass_Rebuild

* Wed Jul 26 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.2-9
- Rebuilt for https://fedoraproject.org/wiki/Fedora_27_Mass_Rebuild

* Fri Feb 10 2017 Fedora Release Engineering <releng@fedoraproject.org> - 3.2-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_26_Mass_Rebuild

* Mon Dec 19 2016 Miro Hrončok <mhroncok@redhat.com> - 3.2-7
- Rebuild for Python 3.6

* Wed Nov 2 2016 Orion Poplawski <orion@cora.nwra.com> - 3.2-7
- Split python support into sub-packages

* Wed Mar 30 2016 Michal Toman <mtoman@fedoraproject.org> - 3.2-6
- Fix build on MIPS

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 3.2-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Fri Jan 22 2016 Orion Poplawski <orion@cora.nwra.com> - 3.2-4
- Add patch to allow -host localhost to work on builders

* Wed Jan 20 2016 Orion Poplawski <orion@cora.nwra.com> - 3.2-3
- Use nemesis channel on all platforms

* Wed Dec  9 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.2-2
- Soften version check (#1289779)

* Tue Dec  1 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.2-1
- Update to latest version

* Mon Nov 16 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.1.4-9
- Update requires and fix MPI_FORTRAN_MOD_DIR var

* Mon Nov 16 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.1.4-8
- Move fortran .mod files to %%{_fmoddir}/mpich (#1154991)
- Move man pages to arch-specific dir (#1264359)

* Tue Nov 10 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1.4-7
- Rebuilt for https://fedoraproject.org/wiki/Changes/python3.5

* Thu Aug 27 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.1.4-6
- Use .pth files to set the python path (https://fedorahosted.org/fpc/ticket/563)
- Cleanups to the spec file

* Sun Jul 26 2015 Sandro Mani <manisandro@gmail.com> - 3.1.4-5
- Require, BuildRequire: rpm-mpi-hooks

* Wed Jun 17 2015 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1.4-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_23_Mass_Rebuild

* Sat May  9 2015 Zbigniew Jędrzejewski-Szmek <zbyszek@in.waw.pl> - 3.1.4-3
- Change MPI_SYCONFIG to /etc/mpich-x86_64 (#1196728)

* Fri Mar 13 2015 Orion Poplawski <orion@cora.nwra.com> - 3.1.4-2
- Set PKG_CONFIG_DIR (bug #1113627)
- Fix modulefile names and python paths (bug#1201343)

* Wed Mar 11 2015 Orion Poplawski <orion@cora.nwra.com> - 3.1.4-1
- Update to 3.1.4
- Own and set PKG_CONFIG_DIR (bug #1113627)
- Do not ship old modulefile location (bug #921534)

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Sat Jun 07 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.1-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Fri Feb 21 2014 Ville Skyttä <ville.skytta@iki.fi> - 3.1-2
- Install rpm macros to %%{_rpmconfigdir}/macros.d as non-%%config.

* Fri Feb 21 2014 Deji Akingunola <dakingun@gmail.com> - 3.1-1
- Update to 3.1

* Mon Jan  6 2014 Peter Robinson <pbrobinson@fedoraproject.org> 3.0.4-7
- Set the aarch64 compiler options

* Fri Dec 13 2013 Peter Robinson <pbrobinson@fedoraproject.org> 3.0.4-6
- Now have valgrind on ARMv7
- No valgrind on aarch64

* Fri Aug 23 2013 Orion Poplawski <orion@cora.nwra.com> - 3.0.4-5
- Add %%check

* Sat Aug 03 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 3.0.4-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_20_Mass_Rebuild

* Sat Jul 20 2013 Deji Akingunola <dakingun@gmail.com> - 3.0.4-3
- Add proper Provides and Obsoletes for the sub-packages

* Thu Jul 18 2013 Deji Akingunola <dakingun@gmail.com> - 3.0.4-2
- Fix some of the rpmlint warnings from package review (BZ #973493)

* Wed Jun 12 2013 Deji Akingunola <dakingun@gmail.com> - 3.0.4-1
- Update to 3.0.4

* Thu Feb 21 2013 Deji Akingunola <dakingun@gmail.com> - 3.0.2-1
- Update to 3.0.2
- Rename to mpich.
- Drop check for old alternatives' installation

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.5-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Thu Nov 1 2012 Orion Poplawski <orion@cora.nwra.com> - 1.5-1
- Update to 1.5
- Drop destdir-fix and mpicxx-und patches
- Update rpm macros to use the new module location

* Wed Oct 31 2012 Orion Poplawski <orion@cora.nwra.com> - 1.4.1p1-9
- Install module file in mpi subdirectory and conflict with other mpi modules
- Leave existing module file location for backwards compatibility for a while

* Fri Jul 20 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.4.1p1-8
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Wed Feb 15 2012 Peter Robinson <pbrobinson@fedoraproject.org> - 1.4.1p1-7
- Rebuild for new hwloc

* Wed Feb 15 2012 Peter Robinson <pbrobinson@fedoraproject.org> - 1.4.1p1-6
- Update ARM build configuration

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.4.1p1-5
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Mon Jan  2 2012 Jussi Lehtola <jussilehtola@fedoraproject.org> - 1.4.1p1-4
- Bump spec.

* Wed Nov 16 2011 Jussi Lehtola <jussilehtola@fedoraproject.org> - 1.4.1p1-3
- Comply to MPI guidelines by separating autoloading into separate package
  (BZ #647147).

* Tue Oct 18 2011 Deji Akingunola <dakingun@gmail.com> - 1.4.1p1-2
- Rebuild for hwloc soname bump.

* Sun Sep 11 2011 Deji Akingunola <dakingun@gmail.com> - 1.4.1p1-1
- Update to 1.4.1p1 patch update
- Add enable-lib-depend to configure flags

* Sat Aug 27 2011 Deji Akingunola <dakingun@gmail.com> - 1.4.1-1
- Update to 1.4.1 final
- Drop the mpd subpackage, the PM is no longer supported upstream
- Fix undefined symbols in libmpichcxx (again) (#732926)

* Wed Aug 03 2011 Jussi Lehtola <jussilehtola@fedoraproject.org> - 1.4-2
- Respect environment module guidelines wrt placement of module file.

* Fri Jun 17 2011 Deji Akingunola <dakingun@gmail.com> - 1.4-1
- Update to 1.4 final
