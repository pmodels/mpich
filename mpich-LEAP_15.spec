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

%global cart_major 4
%global daos_major 0

%global flavor @BUILD_FLAVOR@%nil

# Static libraries are disabled by default
# for non HPC builds
# To enable them, simply uncomment:
# % define build_static_devel 1

%define pname mpich
%define vers  3.4~a2
%define _vers 3_4

%if "%{flavor}" == ""
ExclusiveArch:  do_not_build
%{bcond_with hpc}
%endif

%if "%{flavor}" == "standard"
%define build_flavor verbs
%{bcond_with hpc}
%endif
%if "%{flavor}" == "testsuite"
%define build_flavor verbs
%define testsuite 1
%{bcond_with hpc}
%endif

%if "%{flavor}" == "ofi"
%define build_flavor ofi
%{bcond_with hpc}
%endif
%if "%{flavor}" == "ofi-testsuite"
%define build_flavor ofi
%define testsuite 1
%{bcond_with hpc}
%endif

%if "%flavor" == "gnu-hpc"
%define compiler_family gnu
%undefine c_f_ver
%define build_flavor verbs
%define build_static_devel 1
%{bcond_without hpc}
%endif
%if "%flavor" == "gnu-hpc-testsuite"
%define compiler_family gnu
%undefine c_f_ver
%define testsuite 1
%define build_flavor verbs
%{bcond_without hpc}
%endif

%if "%flavor" == "gnu-hpc-ofi"
%define compiler_family gnu
%undefine c_f_ver
%define build_flavor ofi
%define build_static_devel 1
%{bcond_without hpc}
%endif
%if "%flavor" == "gnu-hpc-ofi-testsuite"
%define compiler_family gnu
%undefine c_f_ver
%define testsuite 1
%define build_flavor ofi
%{bcond_without hpc}
%endif

%if "%flavor" == "gnu7-hpc"
%define compiler_family gnu
%define c_f_ver 7
%define build_flavor verbs
%define build_static_devel 1
%{bcond_without hpc}
%endif
%if "%flavor" == "gnu7-hpc-testsuite"
%define compiler_family gnu
%define c_f_ver 7
%define testsuite 1
%define build_flavor verbs
%{bcond_without hpc}
%endif

%if "%flavor" == "gnu7-hpc-ofi"
%define compiler_family gnu
%define c_f_ver 7
%define build_flavor ofi
%define build_static_devel 1
%{bcond_without hpc}
%endif
%if "%flavor" == "gnu7-hpc-ofi-testsuite"
%define compiler_family gnu
%define c_f_ver 7
%define testsuite 1
%define build_flavor ofi
%{bcond_without hpc}
%endif

%if "%{build_flavor}" != "verbs"
%define pack_suff %{?build_flavor:-%{build_flavor}}
%endif

%if %{without hpc}
%define module_name mpich%{?pack_suff}
%define p_prefix /usr/%_lib/mpi/gcc/%{module_name}
%define p_bindir  %{p_prefix}/bin
%define p_datadir %{p_prefix}/share
%define p_includedir %{p_prefix}/include
%define p_mandir  %{p_datadir}/man
%define p_libdir  %{p_prefix}/%{_lib}
%define p_libexecdir %{p_prefix}/%{_lib}
%define _moduledir /usr/share/modules/gnu-%{module_name}
%define package_name %{pname}%{?pack_suff}
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
Release:        1
Summary:        High-performance and widely portable implementation of MPI
License:        MIT
Group:          Development/Libraries/Parallel
Url:            http://www.mpich.org/
Source0:        http://www.mpich.org/static/downloads/%{version}/mpich-%{vers}.tar.gz
Source1:        mpivars.sh
Source2:        mpivars.csh
Source3:        macros.hpc-mpich
Source100:      _multibuild
# PATCH-FIX-UPSTREAM 0001-Drop-GCC-check.patch (bnc#1129421)
# It's been merged upstream, should be removed with the next release
#Patch0:         0001-Drop-GCC-check.patch
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

BuildRequires:  fdupes
BuildRequires:  hwloc-devel >= 1.6
BuildRequires:  libtool
BuildRequires:  pkg-config
BuildRequires:  python-devel
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
BuildRequires:  libtool
BuildRequires:  mpi-selector
BuildRequires:  python-devel
BuildRequires:  sysfsutils
#if "%{flavor}" == "ofi"
BuildRequires:  libfabric-devel
#endif
BuildRequires:  daos-devel
Provides:       %{package_name}-cart-%{cart_major}-daos-%{daos_major}

Provides:       mpi
%if %{without hpc}
BuildRequires:  Modules
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
echo FLAVOR %{flavor}
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
    --prefix=%{p_prefix} \
    --exec-prefix=%{p_prefix} \
    --datadir=%{p_datadir} \
    --bindir=%{p_bindir} \
    --includedir=%{p_includedir} \
    --libdir=%{p_libdir} \
    --libexecdir=%{p_libexecdir} \
    --mandir=%{p_mandir} \
%endif
    --with-file-system=ufs+daos \
    --with-daos=/usr \
    --with-cart=/usr \
    --docdir=%{_datadir}/doc/%{name} \
    --disable-rpath      \
    --disable-wrapper-rpath      \
%if "%{flavor}" == "ofi"
   --with-device=ch3:nemesis:ofi \
%endif
   
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
make check

%else

%if %{without hpc}
# make and install mpivars files
install -m 0755 -d %{buildroot}%{_bindir}
sed -e 's,prefix,%p_prefix,g' -e 's,libdir,%{p_libdir},g' %{S:1} > %{buildroot}%{p_bindir}/mpivars.sh
sed -e 's,prefix,%p_prefix,g' -e 's,libdir,%{p_libdir},g' %{S:2} > %{buildroot}%{p_bindir}/mpivars.csh

mkdir -p %{buildroot}%{_moduledir}

cat << EOF > %{buildroot}%{_moduledir}/%{version}
#%%Module
proc ModulesHelp { } {
        global dotversion
        puts stderr "\tLoads the gnu - mpich %{version}  Environment"
}

module-whatis  "Loads the gnu mpich %{version} Environment."
conflict gnu-mpich
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
module load %{hpc_mpi_family}%{?pack_suff}/%{version}
EOF
sed -e "s/export/setenv/" -e "s/=/ /" \
    %{buildroot}/%{p_bindir}/mpivars.sh > \
    %{buildroot}/%{p_bindir}/mpivars.csh
mkdir -p %{buildroot}%{_sysconfdir}/rpm
%endif # with hpc

find %{buildroot} -type f -name "*.la" -exec rm -f {} ';'

%fdupes -s %{buildroot}

%post
/sbin/ldconfig
%if %{without hpc}
# Always register. We might be already registered in the case of an udate
# but mpi-selector handles it fine
/usr/bin/mpi-selector \
        --register %{name}%{?pack_suff} \
        --source-dir %{p_bindir} \
        --yes
%endif

%postun
/sbin/ldconfig
%if %{without hpc}
# Only unregister when uninstalling
if [ "$1" = "0" ]; then
	/usr/bin/mpi-selector --unregister %{name}%{?pack_suff} --yes
	# Deregister the default if we are uninstalling it
	if [ "$(/usr/bin/mpi-selector --system --query)" = "%{name}%{?pack_suff}" ]; then
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
%dir %{_moduledir}
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
%{p_libdir}/pkgconfig/openpa.pc

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
* Tue May 12 2020 Brian J. Murrell <brian.murrell@intel.com> - 3.4~a2-1
- Update to 3.4a2
- Always install librabric-devel as it's needed for ch4

* Wed Dec 17 2019 Brian J. Murrell <brian.murrell@intel.com> - 3.3-5
- Rebuild with CaRT SO version 4
- Add Provides: to allow consumers to target cart and daos ABI versions
