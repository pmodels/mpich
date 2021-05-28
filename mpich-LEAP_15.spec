#
# spec file for package mpich
#
# Copyright (c) 2019 SUSE LINUX GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via https://bugs.opensuse.org/
#

%global daos_major 1

# Static libraries are disabled by default
# for non HPC builds
# To enable them, simply uncomment:
# % define build_static_devel 1

%define pname mpich
%define vers  3.4~a2
%define _vers 3_4

%define build_flavor ofi
%{bcond_with hpc}

%if "%{build_flavor}" != "verbs"
%define pack_suff %{?build_flavor:-%{build_flavor}}
%endif

%if %{without hpc}
%define module_name mpich
%define p_prefix /usr/%_lib/mpi/gcc/%{module_name}
%define p_bindir  %{p_prefix}/bin
%define p_datadir %{p_prefix}/share
%define p_includedir %{p_prefix}/include
%define p_mandir  %{p_datadir}/man
%define p_libdir  %{p_prefix}/%{_lib}
%define p_libexecdir %{p_prefix}/%{_lib}
%define _moduledir /usr/share/modules/gnu-%{module_name}
%define package_name %{pname}
%else
%{hpc_init -M -c %compiler_family %{?c_f_ver:-v %{c_f_ver}} -m mpich %{?pack_suff:-e %{build_flavor}} %{?mpi_f_ver:-V %{mpi_f_ver}}}
%define p_prefix   %{hpc_prefix}
%define p_bindir   %{hpc_bindir}
%define p_datadir  %{hpc_datadir}
%define p_includedir  %{hpc_includedir}
%define p_mandir   %{hpc_mandir}
%define p_libdir   %{hpc_libdir}
%define p_libexecdir  %{hpc_libexecdir}
%define package_name %{pname}%{?pack_suff}_%{_vers}-%{compiler_family}%{?c_f_ver}-hpc

%global hpc_mpich_dep_version %(VER=%{?m_f_ver}; echo -n ${VER})
%global hpc_mpich_dir mpich
%global hpc_mpich_pack_version %{hpc_mpich_dep_version}
%endif

Name:           %{package_name}%{?testsuite:-testsuite}
Version:        %{vers}
Release:        4
Summary:        High-performance and widely portable implementation of MPI
License:        MIT
Group:          Development/Libraries/Parallel
Url:            http://www.mpich.org/
Source0:        http://www.mpich.org/static/downloads/%{version}/mpich-%{vers}.tar.gz
Source1:        mpivars.sh
Source2:        mpivars.csh
Source3:        macros.hpc-mpich
# bjm - borrowed from the EL7 spec.  not sure how SUSE typically does this
Source4:        mpich.pth.py2
Source5:        mpich.pth.py3

Source100:      _multibuild
# PATCH-FIX-UPSTREAM 0001-Drop-GCC-check.patch (bnc#1129421)
# It's been merged upstream, should be removed with the next release
#Patch0:         0001-Drop-GCC-check.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

BuildRequires:  fdupes
BuildRequires:  hwloc-devel >= 1.6
BuildRequires:  libtool
BuildRequires:  pkg-config
BuildRequires:  python2-devel
BuildRequires:  python3-devel
%ifnarch s390 s390x %{arm}
BuildRequires:  valgrind-devel
%endif
BuildRequires:  infiniband-diags-devel
BuildRequires:  libibumad-devel
BuildRequires:  libibverbs-devel
BuildRequires:  librdmacm-devel
%ifnarch s390 s390x armv7hl
BuildRequires:  libnuma-devel
%endif
BuildRequires:  libtool
BuildRequires:  mpi-selector
BuildRequires:  sysfsutils
BuildRequires:  libfabric-devel
BuildRequires:  daos-devel
Provides:       %{package_name}-daos-%{daos_major}

Provides:       mpi
%if %{without hpc}
# bjm - we use lua-lmod here
# BuildRequires:  Modules
BuildRequires:  lua-lmod
BuildRequires:  gcc-c++
BuildRequires:  gcc-fortran
BuildRequires:  mpi-selector
Requires:       mpi-selector
Requires(preun): mpi-selector
%else
BuildRequires:  %{compiler_family}%{?c_f_ver}-compilers-hpc-macros-devel
BuildRequires:  lua-lmod
BuildRequires:  suse-hpc
%hpc_requires
%endif

%if 0%{?testsuite}
BuildRequires:  %package_name = %{version}
%endif

%description
MPICH is a high performance and widely portable implementation of the Message
Passing Interface (MPI) standard.

The goals of MPICH are:

 * to provide an MPI implementation that efficiently supports different
   computation and communication platforms including commodity clusters
   (desktop systems, shared-memory systems, multicore architectures),
   high-speed networks and proprietary high-end computing systems
   (Blue Gene, Cray)
 * to enable cutting-edge research in MPI through an easy-to-extend modular
   framework for other derived implementations

%if 0%{!?testsuite:1}

%package devel
Summary:        SDK for MPICH %{?with_hpc:HPC} version %{version}
Group:          Development/Libraries/Parallel
Requires:       %{name} = %{version}
%if %{without hpc}
Requires:       libstdc++-devel
%else
%hpc_requires_devel
%endif
Requires:       %{name} = %{version}
Requires:       daos-devel

%description devel
MPICH is a freely available, portable implementation of MPI, the
Standard for message-passing libraries. This package contains manpages,
headers and libraries needed for developing MPI applications.

This RPM contains all the wrappers necessary to compile, link, and run
Open MPI jobs.

%if %{with hpc}
%package        macros-devel
Summary:        HPC Macros for MPICH version %{version}
Group:          Development/Libraries/Parallel
Requires:       %{name}-devel = %{version}
Provides:       %{pname}-hpc-macros-devel = %{version}
Conflicts:      otherproviders(%{pname}-hpc-macros-devel)

%description macros-devel
HPC Macros for building RPM packages for MPICH version %{version}.
%endif

%if 0%{?build_static_devel}
%package        devel-static
Summary:        Static libraries for MPICH %{?with_hpc:HPC} version %{version}
Group:          Development/Libraries/Parallel
Requires:       %{name}-devel = %{version}

%description devel-static
MPICH is a freely available, portable implementation of MPI, the
Standard for message-passing libraries. This package contains manpages,
headers and libraries needed for developing MPI applications.

This RPM contains the static library files, which are packaged separately from
the dynamic library and headers.
%endif

%if %{with hpc}
%{hpc_master_package -L -a}
%{hpc_master_package -a devel}
%{hpc_master_package macros-devel}
%{hpc_master_package -a devel-static}
%endif # ?with_hpc

%endif # ! testsuite

%prep
%if %{with hpc}
echo with HPC
%endif
%if %{without hpc}
echo without HPC
%endif
%setup -q -n mpich-%{version}%{?rc_ver}
#patch0

%build
./autogen.sh
%{?with_hpc:%hpc_debug}
%if %{with hpc}
%{hpc_setup}
%{hpc_configure} \
%else
%configure \
    --enable-sharedlibs=gcc \
    --enable-shared \
    --enable-static=no \
    --enable-lib-depend \
    --disable-rpath \
    --disable-silent-rules \
    --enable-fc \
   --with-device=ch3:nemesis \
    --prefix=%{p_prefix} \
    --exec-prefix=%{p_prefix} \
    --libexecdir=%{p_libexecdir} \
%endif
    --with-pm=hydra:gforker \
    --includedir=%{p_includedir} \
    --bindir=%{p_bindir} \
    --libdir=%{p_libdir} \
    --datadir=%{p_datadir} \
    --mandir=%{p_mandir} \
    --docdir=%{_datadir}/doc/%{name} \
    --with-hwloc-prefix=embedded \
    --with-hwloc=embedded \
    --enable-romio \
    --with-file-system=ufs+daos \
    --with-daos=/usr \
    --with-cart=/usr \
    --disable-checkerrors \
    --disable-perftest \
    --disable-large-tests \
    --disable-ft-tests \
    --disable-comm-overlap-tests \
    --enable-threads=single \
   
	CFLAGS="%optflags -fPIC"			\
	CXXLAGS="%optflags -fPIC"			\
	MPICHLIB_CFLAGS="%{optflags}"			\
	MPICHLIB_CXXFLAGS="%{optflags}"

make %{?_smp_mflags} VERBOSE=1

%install
make DESTDIR=%{buildroot} install

# sanitize .la files
list="$(find %{buildroot} -name "*.la" -printf "%%h\n" | sort | uniq)"
for dir in ${list}
do
    deps="${deps} -L${dir##%{buildroot}}"
done
for dir in ${list}
do
%if !0%{?build_static_devel}
    rm -f ${dir}/*.la
%else
    for file in ${dir}/*.la
    do
        sed -i -e "s@ [^[:space:]]*home[^[:space:]\']*@${deps}@" \
            -e "s@ [^[:space:]]*home[^[:space:]\']*@@g" \
            -e "s@-L.*.libs @@g" ${file}
    done
%endif
done
# sanitize .la files
%if !0%{?build_static_devel}
find %{buildroot} -name "*.a" -delete
%endif

%fdupes %{buildroot}%{p_mandir}
%fdupes %{buildroot}%{p_datadir}
%fdupes %{buildroot}%{p_libdir}/pkgconfig

%if 0%{?testsuite}
# Remove everything from testsuite package
# It is all contained by mpich packages
rm -rf %{buildroot}/*

%check
# disabled due to https://github.com/pmodels/mpich/issues/4534
#make testing

%else

%if %{without hpc}
# make and install mpivars files
install -m 0755 -d %{buildroot}%{_bindir}
sed -e 's,prefix,%p_prefix,g' -e 's,libdir,%{p_libdir},g' %{S:1} > %{buildroot}%{p_bindir}/mpivars.sh
sed -e 's,prefix,%p_prefix,g' -e 's,libdir,%{p_libdir},g' %{S:2} > %{buildroot}%{p_bindir}/mpivars.csh

mkdir -p %{buildroot}%{_moduledir}

# bjm - Not really sure why SUSE doesn't just use the 
#       src/packaging/envmods/mpich.module that ships with mpich
cat << EOF > %{buildroot}%{_moduledir}/%{version}
#%%Module
proc ModulesHelp { } {
        global dotversion
        puts stderr "\tLoads the gnu - mpich %{version}  Environment"
}

module-whatis  "Loads the gnu mpich %{version} Environment."
conflict gnu-mpich
setenv       MPI_PYTHON_SITEARCH %{python2_sitearch}/%{name}
setenv       MPI_PYTHON2_SITEARCH %{python2_sitearch}/%{name}
setenv       MPI_PYTHON3_SITEARCH %{python3_sitearch}/%{name}
prepend-path PATH %{p_bindir}
prepend-path INCLUDE %{p_includedir}
prepend-path INCLUDE %{p_libdir}
prepend-path MANPATH %{p_mandir}
prepend-path LD_LIBRARY_PATH %{p_libdir}
EOF

cat << EOF > %{buildroot}%{_moduledir}/.version
#%%Module1.0
set ModulesVersion "%{version}"
EOF
%else # with hpc

install -d -m 755 %{buildroot}%{_sysconfdir}/rpm
cp %{S:3} %{buildroot}%{_sysconfdir}/rpm

%hpc_write_modules_files
#%%Module1.0#####################################################################

proc ModulesHelp { } {

puts stderr " "
puts stderr "This module loads the %{pname} library built with the %{compiler_family} toolchain."
puts stderr "\nVersion %{version}\n"

}
module-whatis "Name: %{pname} built with %{compiler_family} toolchain"
module-whatis "Version: %{version}"
module-whatis "Category: runtime library"
module-whatis "Description: %{SUMMARY:0}"
module-whatis "URL: %{url}"

set     version                     %{version}

prepend-path    PATH                %{hpc_prefix}/bin
prepend-path    MANPATH             %{hpc_prefix}/man
prepend-path    LD_LIBRARY_PATH     %{hpc_prefix}/%_lib
prepend-path    MODULEPATH          %{hpc_modulepath}
prepend-path    MPI_DIR             %{hpc_prefix}
%{hpc_modulefile_add_pkgconfig_path}

family "MPI"
EOF
cat <<EOF >  %{buildroot}/%{p_bindir}/mpivars.sh
%hpc_setup_compiler
module load %{hpc_mpi_family}/%{version}
EOF
sed -e "s/export/setenv/" -e "s/=/ /" \
    %{buildroot}/%{p_bindir}/mpivars.sh > \
    %{buildroot}/%{p_bindir}/mpivars.csh
mkdir -p %{buildroot}%{_sysconfdir}/rpm
%endif # with hpc

# Install the .pth files
# bjm - borrowed from the EL7 spec.  not sure how SUSE typically does this
mkdir -p %{buildroot}%{python2_sitearch}/%{name}
install -pDm0644 %{SOURCE4} %{buildroot}%{python2_sitearch}/%{name}.pth
mkdir -p %{buildroot}%{python3_sitearch}/%{name}
install -pDm0644 %{SOURCE5} %{buildroot}%{python3_sitearch}/%{name}.pth

find %{buildroot} -type f -name "*.la" -exec rm -f {} ';'

%fdupes -s %{buildroot}

%post
/sbin/ldconfig
%if %{without hpc}
# Always register. We might be already registered in the case of an udate
# but mpi-selector handles it fine
/usr/bin/mpi-selector \
        --register %{name} \
        --source-dir %{p_bindir} \
        --yes
%endif

%postun
/sbin/ldconfig
%if %{without hpc}
# Only unregister when uninstalling
if [ "$1" = "0" ]; then
	/usr/bin/mpi-selector --unregister %{name} --yes
	# Deregister the default if we are uninstalling it
	if [ "$(/usr/bin/mpi-selector --system --query)" = "%{name}" ]; then
		/usr/bin/mpi-selector --system --unset --yes
	fi
fi
%else
%hpc_module_delete_if_default
%endif

%files
%defattr(-,root,root)
%doc CHANGES COPYRIGHT README README.envvar RELEASE_NOTES
%if %{without hpc}
%dir /usr/%_lib/mpi
%dir /usr/%_lib/mpi/gcc
%dir /usr/share/modules
%{python2_sitearch}/%{name}.pth
%{python3_sitearch}/%{name}.pth
%{_moduledir}
%else
%hpc_mpi_dirs
%hpc_modules_files
%endif
%doc %{_datadir}/doc
%dir %{p_prefix}
%dir %{p_bindir}
#%dir #{p_datadir}
%dir %{p_includedir}
#%dir #{p_mandir}
#%dir #{p_mandir}/man1
#%dir #{p_mandir}/man3
%dir %{p_libdir}
%{p_bindir}/*
##{p_mandir}/man1/*
%{p_libdir}/*.so.*

%files devel
%defattr(-,root,root)
%dir %{p_libdir}/pkgconfig
#{p_mandir}/man3/*
%{p_includedir}
%{p_libdir}/*.so
%{p_libdir}/pkgconfig/mpich.pc
#%{p_libdir}/pkgconfig/json-c.pc
#%{p_libdir}/pkgconfig/yaksa.pc

%if 0%{?build_static_devel}
%files devel-static
%defattr(-,root,root)
%{p_libdir}/*.a
%endif

%if %{with hpc}
%files macros-devel
%defattr(-,root,root)
%config %{_sysconfdir}/rpm/macros.hpc-mpich
%endif # with hpc

%endif # !testsuite

%changelog
* Thu May 27 2021 Mohamad Chaarawi <mohamad.chaarawi@intel.com> - 3.4~a2-5
- Add --with-hwloc=embedded to configure

* Wed Jan 20 2021 Kenneth Cain <kenneth.c.cain@intel.com> - 3.4~a2-4
- Update packaging for building with libdaos.so.1

* Mon Jun 22 2020 Brian J. Murrell <brian.murrell@intel.com> - 3.4~a2-3
- Add Requires: daos-devel to devel subpackage

* Mon Jun 22 2020 Brian J. Murrell <brian.murrell@intel.com> - 3.4~a2-2
- Port Python functoinality from EL 7 spec
- Use configure options from EL 7 spec to get a build that works
  with mpi4py

* Wed May 13 2020 Brian J. Murrell <brian.murrell@intel.com> - 3.4~a2-1
- Update to 3.4a2
- Reduce "folavor"s down to just "ofi"
- Add a couple of more pkgconfig/ files
- Disabled %check due to https://github.com/pmodels/mpich/issues/4534
- Added switches to configure:
    --disable-checkerrors \
    --disable-perftest \
    --disable-large-tests \
    --disable-ft-tests \
    --disable-comm-overlap-tests \
    --enable-threads=single \

* Wed Dec 18 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-5
- Rebuild with CaRT SO version 4
- Add Provides: to allow consumers to target cart and daos ABI versions
