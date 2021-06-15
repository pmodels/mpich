# this is needed until we can use the system hwloc (2.2.0) in EL8.4
# as the build hardening breaks the embedded hwloc build
%undefine _hardened_build

Summary:        A high-performance implementation of MPI
Name:           mpich
Version:        4.0~a1
Release:        1.g032b3aeb2%{?dist}
License:        MIT
URL:            http://www.mpich.org/

# upstream_version is version with ~ removed
%{lua:
    rpm.define("upstream_version " .. string.gsub(rpm.expand("%{version}"), "~", ""))
}

Source0:        http://www.mpich.org/static/downloads/%{upstream_version}/%{name}-%{upstream_version}.tar.gz
Source1:        mpich.macros
Source2:        mpich.pth.py3
Patch0:         mpich-modules.patch
Patch1:         fix-version.patch

BuildRequires:  gcc-gfortran
%ifarch %{valgrind_arches}
BuildRequires:  valgrind-devel
%endif
# For %%{python3_sitearch}
BuildRequires:  python3-devel
BuildRequires:  rpm-mpi-hooks
BuildRequires:  automake
BuildRequires:  libtool >= 2.4.4
BuildRequires:  daos-devel
# this really should be Requires: by daos-devel
BuildRequires:  libuuid-devel
Provides:       mpi
Requires:       environment(modules)
Provides:       bundled(hwloc) = 2.4.1

# Make sure this package is rebuilt with correct Python version when updating
# Otherwise mpi.req from rpm-mpi-hooks doesn't work
# https://bugzilla.redhat.com/show_bug.cgi?id=1705296
Requires:       (python(abi) = %{python3_version} if python3)

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
Group:          System Environment/Base
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description autoload
This package contains profile files that make mpich automatically loaded.

%package devel
Summary:        Development files for mpich
Group:          Development/Libraries
Provides:       %{name}-devel-static = %{version}-%{release}
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       pkgconfig
Requires:       gcc-gfortran
Requires:       rpm-mpi-hooks
Requires:       redhat-rpm-config
Requires:       daos-devel

%description devel
Contains development headers and libraries for mpich

%package doc
Summary:        Documentations and examples for mpich
Group:          Documentation
BuildArch:      noarch
Requires:       %{name}-devel = %{version}-%{release}

%description doc
Contains documentations, examples and man-pages for mpich

%package -n python3-mpich
Summary:        mpich support for Python 3
Group:          Development/Libraries
Provides: python-mpich
Requires:       %{name}%{?_isa} = %{version}-%{release}
Requires:       python(abi) = %{python3_version}

%description -n python3-mpich
mpich support for Python 3.

%prep
%autosetup -p1 -n mpich-%{upstream_version}

%build
./autogen.sh

CONFIGURE_OPTS=(
        --enable-sharedlibs=gcc
        --enable-shared
        --enable-static=no
        --enable-lib-depend
        --disable-rpath
        --disable-silent-rules
        --enable-fc
        --with-device=ch3:nemesis
        --with-pm=hydra:gforker
        --includedir=%{_includedir}/%{name}-%{_arch}
        --bindir=%{_libdir}/%{name}/bin
        --libdir=%{_libdir}/%{name}/lib
        --datadir=%{_datadir}/%{name}
        --mandir=%{_mandir}/%{name}-%{_arch}
        --docdir=%{_datadir}/%{name}/doc
        --htmldir=%{_datadir}/%{name}/doc
        --with-hwloc=embedded
        --enable-fortran=all
        --enable-romio
        --with-file-system=ufs+daos
        --with-daos=/usr
        --with-cart=/usr
        --disable-checkerrors
        --disable-perftest
        --disable-large-tests
        --disable-ft-tests
        --disable-comm-overlap-tests
        --enable-threads=single
)

%configure "${CONFIGURE_OPTS[@]}"

# Remove rpath
sed -r -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -r -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

#Try and work around 'unused-direct-shlib-dependency' rpmlint warnning
sed -i -e 's| -shared | -Wl,--as-needed\0|g' libtool

%make_build VERBOSE=1

%install
%make_install

mkdir -p %{buildroot}%{_fmoddir}/%{name}
mv  %{buildroot}%{_includedir}/%{name}-*/*.mod %{buildroot}%{_fmoddir}/%{name}/
sed -r -i 's|^modincdir=.*|modincdir=%{_fmoddir}/%{name}|' %{buildroot}%{_libdir}/%{name}/bin/mpifort

# Install the module file
mkdir -p %{buildroot}%{_datadir}/modulefiles/mpi
sed -r 's|%{_bindir}|%{_libdir}/%{name}/bin|;
        s|@LIBDIR@|%{_libdir}/%{name}|;
        s|@MPINAME@|%{name}|;
        s|@py3sitearch@|%{python3_sitearch}|;
        s|@ARCH@|%{_arch}|;
        s|@fortranmoddir@|%{_fmoddir}|;
     ' \
     <src/packaging/envmods/mpich.module \
     >%{buildroot}%{_datadir}/modulefiles/mpi/%{name}-%{_arch}

mkdir -p %{buildroot}%{_sysconfdir}/profile.d
cat >%{buildroot}%{_sysconfdir}/profile.d/mpich-%{_arch}.sh <<EOF
# Load mpich environment module
module load mpi/%{name}-%{_arch}
EOF
cp -p %{buildroot}%{_sysconfdir}/profile.d/mpich-%{_arch}.{sh,csh}

# Install the RPM macros
install -pDm0644 %{SOURCE1} %{buildroot}%{_rpmconfigdir}/macros.d/macros.%{name}

# Install the .pth files
mkdir -p %{buildroot}%{python3_sitearch}/%{name}
install -pDm0644 %{SOURCE2} %{buildroot}%{python3_sitearch}/%{name}.pth

find %{buildroot} -type f -name "*.la" -delete

%check
make check VERBOSE=1 \
%ifarch ppc64le
|| :
# The test results are ignored on ppc64le. The tests started failing
# in the bundled openpa checksuite. Upstream has already removed it,
# so the issue should resolve itself for the next release and I don't
# think it's worth the time to solve it here.
%else
|| :
# ignored here due to https://github.com/pmodels/mpich/issues/4534
%endif

%ldconfig_scriptlets

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
%{_datadir}/modulefiles/mpi/

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

%files -n python3-mpich
%dir %{python3_sitearch}/%{name}
%{python3_sitearch}/%{name}.pth

%changelog
* Thu Jun 03 2021 Brian J. Murrell <brian.murrell@intel.com> - 4.0~a1-1
- Build with DAOS
- Update to 4.0a1 git hash 032b3aeb2
- Added switches to configure:
        --disable-checkerrors
        --disable-perftest
        --disable-large-tests
        --disable-ft-tests
        --disable-comm-overlap-tests
        --enable-threads=single

* Thu May 27 2021 Honggang Li <honli@redhat.com> - 3.4.1-1
- Sync with Fedora build
- Update to latest upstream release 3.4.1
- Resolves: rhbz#1960076

* Thu Oct 15 2020 Honggang Li <honli@redhat.com> - 3.3.2-9
- Fix a dependency typo
- Related: rhbz#1850080

* Thu Oct 15 2020 Honggang Li <honli@redhat.com> - 3.3.2-8
- Sync with Fedora build
- Update to latest upstream release 3.3.2
- Resolves: rhbz#1657316, rhbz#1732982, rhbz#1850080

* Fri Sep 21 2018 Jarod Wilson <jarod@redhat.com> - 3.2.1-9
- Use proper distro compile flags throughout build
- Related: rhbz#1624144

* Thu Sep 13 2018 Jarod Wilson <jarod@redhat.com> - 3.2.1-8
- Remove python2 bits entirely, fix mpi lib dependencies
- Remove obsolete Provides/Obsoletes for mpich2
- Resolves: rhbz#1628628

* Fri Aug  3 2018 Florian Weimer <fweimer@redhat.com> - 3.2.1-7
- Honor %%{valgrind_arches}

* Thu May 17 2018 Charalampos Stratakis <cstratak@redhat.com> - 3.2.1-6
- Do not build the python2 subpackage on EL > 7

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
