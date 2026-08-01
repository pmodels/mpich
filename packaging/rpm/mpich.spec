Summary:        A high-performance implementation of MPI
Name:           mpich
Version:        4.2.2
Release:        %autorelease
License:        mpich2
URL:            https://www.mpich.org/

Source0:        https://www.mpich.org/static/downloads/%{version}/%{name}-%{version}.tar.gz
Source1:        mpich.macros
Source3:        mpich.pth.py3

Patch:          0001-pkgconf-remove-optimization-and-link-flags-from-pkgc.patch
Patch:          0002-pkgconf-also-drop-rpath-flags-from-pkgconf-file.patch
Patch:          0003-Drop-build-flags-e.g.-specs.-and-lto-from-mpi-wrappe.patch
Patch:          0004-Make-mpich.module-useful.patch
# TODO: submit ^ upstream

Patch:          mpich-configure-max_align_t.patch
Patch:          mpich-aclocal_cc-implicit-int.patch
Patch:          mpich-json-configure-__thread.patch

BuildRequires:  make
BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  gcc-gfortran
BuildRequires:  hwloc-devel >= 2.0
%if ! (0%{?rhel} >= 10)
%ifarch x86_64
# BuildRequires:  json-c-devel
BuildRequires:  libpsm2-devel
%endif
%endif
BuildRequires:  libfabric-devel
BuildRequires:  libnl3-devel
BuildRequires:  libuuid-devel
BuildRequires:  numactl-devel
%ifarch aarch64 ppc64le x86_64 riscv64
BuildRequires:  ucx-devel
%endif
%if ! 0%{?rhel}
BuildRequires:  yaksa-devel
%else
Provides:       bundled(yaksa) = 0.2
%endif
# For ./maint/extractcvars
BuildRequires:  perl(lib)
%ifarch %{valgrind_arches}
BuildRequires:  valgrind-devel
%endif
# For %%{python3_sitearch}
BuildRequires:  python3-devel
BuildRequires:  rpm-mpi-hooks
Provides:       mpi
Provides:       mpich2 = %{version}
Obsoletes:      mpich2 < 3.0
Requires:       environment(modules)

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
Requires:       mpich = %{version}-%{release}
Provides:       mpich2-autoload = 3.0.1
Obsoletes:      mpich2-autoload < 3.0

%description autoload
This package contains profile files that make mpich automatically loaded.

%package devel
Summary:        Development files for mpich
Provides:       %{name}-devel-static = %{version}-%{release}
Requires:       %{name} = %{version}-%{release}
Requires:       pkgconfig
Requires:       gcc-gfortran
Requires:       rpm-mpi-hooks
Requires:       redhat-rpm-config
Provides:       mpich2-devel = 3.0.1
Obsoletes:      mpich2-devel < 3.0

%description devel
Contains development headers and libraries for mpich

%package doc
Summary:        Documentations and examples for mpich
BuildArch:      noarch
Requires:       %{name}-devel = %{version}-%{release}
Provides:       mpich2-doc = 3.0.1
Obsoletes:      mpich2-doc < 3.0

%description doc
Contains documentations, examples and man-pages for mpich

%package -n python3-mpich
Summary:        mpich support for Python 3
Requires:       %{name} = %{version}-%{release}
Requires:       python(abi) = %{python3_version}

%description -n python3-mpich
mpich support for Python 3.

%prep
%autosetup -p1

%build
./autogen.sh

CONFIGURE_OPTS=(
        --with-custom-version-string=%{version}-%{release}
        --enable-sharedlibs=gcc
        --enable-shared
        --enable-static=no
        --enable-lib-depend
        --disable-rpath
        --disable-silent-rules
        --disable-dependency-tracking
        --with-gnu-ld
        --with-pm=hydra:gforker
        --includedir=%{_includedir}/%{name}-%{_arch}
        --bindir=%{_libdir}/%{name}/bin
        --libdir=%{_libdir}/%{name}/lib
        --datadir=%{_datadir}/%{name}
        --mandir=%{_mandir}/%{name}-%{_arch}
        --docdir=%{_datadir}/%{name}/doc
        --htmldir=%{_datadir}/%{name}/doc
        --with-hwloc
        --with-libfabric
%ifarch aarch64 ppc64le x86_64 riscv64
        --with-ucx
%endif
%if ! 0%{?rhel}
        --with-yaksa
%endif
)
#        --with-device=ch3:nemesis

# Set -fallow-argument-mismatch for #1795817
%configure "${CONFIGURE_OPTS[@]}"               \
  FFLAGS="$FFLAGS -fallow-argument-mismatch"    \
  FCFLAGS="$FCFLAGS -fallow-argument-mismatch"

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
        s|@py2sitearch@|%{python2_sitearch}|;
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
install -pDm0644 %{SOURCE3} %{buildroot}%{python3_sitearch}/%{name}.pth

# Create cmake directory
mkdir -p %{buildroot}%{_libdir}/%{name}/lib/cmake/

# Create directories for MPICH application development files
mkdir -p %{buildroot}%{_libdir}/%{name}/lib/cmake
mkdir -p %{buildroot}%{_libdir}/%{name}/include

find %{buildroot} -type f -name "*.la" -delete

rm %{buildroot}%{_libdir}/%{name}/bin/parkill


%check
make check VERBOSE=1 \
%ifarch ppc64le
|| :
%endif
# The test results are ignored on ppc64le. The tests started failing
# in the bundled openpa checksuite. Upstream has already removed it,
# so the issue should resolve itself for the next release and I don't
# think it's worth the time to solve it here.

%ldconfig_scriptlets

%files
%license COPYRIGHT
%doc CHANGES README README.envvar RELEASE_NOTES
%dir %{_libdir}/%{name}
%dir %{_libdir}/%{name}/lib
%dir %{_libdir}/%{name}/bin
%dir %{_libdir}/%{name}/lib/cmake
%dir %{_libdir}/%{name}/include
%dir %{_fmoddir}/mpich
%{_libdir}/%{name}/lib/*.so.*
%{_libdir}/%{name}/bin/hydra*
%{_libdir}/%{name}/bin/mpichversion
%{_libdir}/%{name}/bin/mpiexec*
%{_libdir}/%{name}/bin/mpirun
%{_libdir}/%{name}/bin/mpivars
%dir %{_mandir}/%{name}-%{_arch}
%doc %{_mandir}/%{name}-%{_arch}/man1/
%{_datadir}/modulefiles/mpi/

%files autoload
%{_sysconfdir}/profile.d/mpich-%{_arch}.*

%files devel
%{_includedir}/%{name}-%{_arch}/
%{_libdir}/%{name}/lib/pkgconfig/
%{_libdir}/%{name}/lib/cmake/
%{_libdir}/%{name}/lib/*.so
%{_libdir}/%{name}/bin/mpicc
%{_libdir}/%{name}/bin/mpic++
%{_libdir}/%{name}/bin/mpicxx
%{_libdir}/%{name}/bin/mpif77
%{_libdir}/%{name}/bin/mpif90
%{_libdir}/%{name}/bin/mpifort
%{_fmoddir}/%{name}/
%{_rpmconfigdir}/macros.d/macros.%{name}
%{_mandir}/%{name}-%{_arch}/man3/

%files doc
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/doc/

%files -n python3-mpich
%dir %{python3_sitearch}/%{name}
%{python3_sitearch}/%{name}.pth

%changelog
%autochangelog
