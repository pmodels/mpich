# ABI Compatibility Initiative

## Goals of the ABI Compatibility Initiative

The ABI compatibility initiative is an understanding between various
MPICH derived MPI implementations to maintain runtime compatibility
between each other. That is, an application can be compiled with one of
them and executed with another. The following are the partners and
package release dates involved in the ABI compatibility initiative:

  - MPICH v3.1 (Released February 2014)
  - Intel® MPI Library v5.0 (2014)
  - Cray MPT v7.0.0 (June 2014)
  - MVAPICH2 2.0 (Release June 2014)
  - Parastation MPI 5.1.7-1 (Released December 2016)
  - RIKEN MPI 1.0 (Released August 2016)

## ABI Compliance Requirements

In order to maintain ABI compliance, the members of the initiative have
agreed on the following requirements:

  - All releases by the above MPI implementations will use the libtool
    ABI string **12:x:0**. **x** can vary based on release number.
  - The following names will be used for the libraries:
      - C bindings and internal symbols: libmpi
      - C++ bindings: libmpicxx
      - Fortran bindings: libmpifort
  - Functions that are not a part of the MPI standard (MPIX_ functions)
    and F08 bindings are not covered by the ABI.
  - All wrapper compilers will only use the frontend libraries (libmpi,
    libmpicxx, mpifort) as dependencies; the backend libraries (libmpl,
    libopa) will not be added as explicit dependencies (they will be
    pulled in using libtool interlibrary dependencies).

When the ABI string current version or age is being updated, this will
be done in coordination with the remaining initiative members. The next
likely change point will be either when **MPI-3.1** is released or when
we decide to include F08 bindings in the ABI string. We might merge both
of these into a single ABI string change. At that point, assuming no ABI
breakage, we would move to the ABI string **13:0:1**.

## ABI String Updates

The first ABI release for each partner is using the following ABI
string: **12:0:0** (or **12:x:0** for some x). This will result in the
following list of files (names updated for simplicity):

    % ls -l install/lib
    total 64584
    -rw-r--r-- 1 balaji collab 44071674 Mar 31 14:48 libmpi.a
    -rwxr-xr-x 1 balaji collab     1102 Mar 31 14:48 libmpi.la
    lrwxrwxrwx 1 balaji collab       16 Mar 31 14:48 libmpi.so -> libmpi.so.12.0.0
    lrwxrwxrwx 1 balaji collab       16 Mar 31 14:48 libmpi.so.12 -> libmpi.so.12.0.0
    -rwxr-xr-x 1 balaji collab 18584233 Mar 31 14:48 libmpi.so.12.0.0
    -rw-r--r-- 1 balaji collab   541354 Mar 31 14:48 libmpicxx.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 14:48 libmpicxx.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 14:48 libmpicxx.so -> libmpicxx.so.12.0.0
    lrwxrwxrwx 1 balaji collab       19 Mar 31 14:48 libmpicxx.so.12 -> libmpicxx.so.12.0.0
    -rwxr-xr-x 1 balaji collab   307420 Mar 31 14:48 libmpicxx.so.12.0.0
    -rw-r--r-- 1 balaji collab  1718388 Mar 31 14:48 libmpif77.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 14:48 libmpif77.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 14:48 libmpif77.so -> libmpif77.so.12.0.0
    lrwxrwxrwx 1 balaji collab       19 Mar 31 14:48 libmpif77.so.12 -> libmpif77.so.12.0.0
    -rwxr-xr-x 1 balaji collab   674233 Mar 31 14:48 libmpif77.so.12.0.0
    -rw-r--r-- 1 balaji collab    62816 Mar 31 14:48 libmpifort.a
    -rwxr-xr-x 1 balaji collab     1221 Mar 31 14:48 libmpifort.la
    lrwxrwxrwx 1 balaji collab       20 Mar 31 14:48 libmpifort.so -> libmpifort.so.12.0.0
    lrwxrwxrwx 1 balaji collab       20 Mar 31 14:48 libmpifort.so.12 -> libmpifort.so.12.0.0
    -rwxr-xr-x 1 balaji collab    42788 Mar 31 14:48 libmpifort.so.12.0.0
    -rw-r--r-- 1 balaji collab    29910 Mar 31 14:48 libmpl.a
    -rwxr-xr-x 1 balaji collab      929 Mar 31 14:48 libmpl.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 14:48 libmpl.so -> libmpl.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 14:48 libmpl.so.1 -> libmpl.so.1.0.0
    -rwxr-xr-x 1 balaji collab    27039 Mar 31 14:48 libmpl.so.1.0.0
    -rw-r--r-- 1 balaji collab     4148 Mar 31 14:48 libopa.a
    -rwxr-xr-x 1 balaji collab      939 Mar 31 14:48 libopa.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 14:48 libopa.so -> libopa.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 14:48 libopa.so.1 -> libopa.so.1.0.0
    -rwxr-xr-x 1 balaji collab     8320 Mar 31 14:48 libopa.so.1.0.0
    drwxr-xr-x 2 balaji collab     4096 Mar 31 14:48 pkgconfig

A later release, say **mpich-3.1.1**, which only updates the library
implementation, but does not change the public interface would go to the
ABI string **12:1:0**. The resultant files would be:

    % ls -l install/lib/
    total 64584
    -rw-r--r-- 1 balaji collab 44071674 Mar 31 15:09 libmpi.a
    -rwxr-xr-x 1 balaji collab     1102 Mar 31 15:09 libmpi.la
    lrwxrwxrwx 1 balaji collab       16 Mar 31 15:09 libmpi.so -> libmpi.so.12.0.1
    lrwxrwxrwx 1 balaji collab       16 Mar 31 15:09 libmpi.so.12 -> libmpi.so.12.0.1
    -rwxr-xr-x 1 balaji collab 18584233 Mar 31 15:09 libmpi.so.12.0.1
    -rw-r--r-- 1 balaji collab   541354 Mar 31 15:09 libmpicxx.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 15:09 libmpicxx.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:09 libmpicxx.so -> libmpicxx.so.12.0.1
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:09 libmpicxx.so.12 -> libmpicxx.so.12.0.1
    -rwxr-xr-x 1 balaji collab   307420 Mar 31 15:09 libmpicxx.so.12.0.1
    -rw-r--r-- 1 balaji collab  1718388 Mar 31 15:09 libmpif77.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 15:09 libmpif77.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:09 libmpif77.so -> libmpif77.so.12.0.1
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:09 libmpif77.so.12 -> libmpif77.so.12.0.1
    -rwxr-xr-x 1 balaji collab   674233 Mar 31 15:09 libmpif77.so.12.0.1
    -rw-r--r-- 1 balaji collab    62816 Mar 31 15:09 libmpifort.a
    -rwxr-xr-x 1 balaji collab     1221 Mar 31 15:09 libmpifort.la
    lrwxrwxrwx 1 balaji collab       20 Mar 31 15:09 libmpifort.so -> libmpifort.so.12.0.1
    lrwxrwxrwx 1 balaji collab       20 Mar 31 15:09 libmpifort.so.12 -> libmpifort.so.12.0.1
    -rwxr-xr-x 1 balaji collab    42788 Mar 31 15:09 libmpifort.so.12.0.1
    -rw-r--r-- 1 balaji collab    29910 Mar 31 15:09 libmpl.a
    -rwxr-xr-x 1 balaji collab      929 Mar 31 15:09 libmpl.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:09 libmpl.so -> libmpl.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:09 libmpl.so.1 -> libmpl.so.1.0.0
    -rwxr-xr-x 1 balaji collab    27039 Mar 31 15:09 libmpl.so.1.0.0
    -rw-r--r-- 1 balaji collab     4148 Mar 31 15:09 libopa.a
    -rwxr-xr-x 1 balaji collab      939 Mar 31 15:09 libopa.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:09 libopa.so -> libopa.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:09 libopa.so.1 -> libopa.so.1.0.0
    -rwxr-xr-x 1 balaji collab     8320 Mar 31 15:09 libopa.so.1.0.0
    drwxr-xr-x 2 balaji collab     4096 Mar 31 15:09 pkgconfig

Another release, say **mpich-3.2**, that adds new interfaces, but does
not disturb the old interfaces (i.e., neither updates them nor deletes
them) would result in the ABI string **13:0:1**. The resultant files
would be:

    % ls -l install/lib/
    total 64584
    -rw-r--r-- 1 balaji collab 44071674 Mar 31 15:16 libmpi.a
    -rwxr-xr-x 1 balaji collab     1102 Mar 31 15:16 libmpi.la
    lrwxrwxrwx 1 balaji collab       16 Mar 31 15:16 libmpi.so -> libmpi.so.12.1.0
    lrwxrwxrwx 1 balaji collab       16 Mar 31 15:16 libmpi.so.12 -> libmpi.so.12.1.0
    -rwxr-xr-x 1 balaji collab 18584233 Mar 31 15:16 libmpi.so.12.1.0
    -rw-r--r-- 1 balaji collab   541354 Mar 31 15:16 libmpicxx.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 15:16 libmpicxx.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:16 libmpicxx.so -> libmpicxx.so.12.1.0
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:16 libmpicxx.so.12 -> libmpicxx.so.12.1.0
    -rwxr-xr-x 1 balaji collab   307420 Mar 31 15:16 libmpicxx.so.12.1.0
    -rw-r--r-- 1 balaji collab  1718388 Mar 31 15:16 libmpif77.a
    -rwxr-xr-x 1 balaji collab     1167 Mar 31 15:16 libmpif77.la
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:16 libmpif77.so -> libmpif77.so.12.1.0
    lrwxrwxrwx 1 balaji collab       19 Mar 31 15:16 libmpif77.so.12 -> libmpif77.so.12.1.0
    -rwxr-xr-x 1 balaji collab   674233 Mar 31 15:16 libmpif77.so.12.1.0
    -rw-r--r-- 1 balaji collab    62816 Mar 31 15:16 libmpifort.a
    -rwxr-xr-x 1 balaji collab     1221 Mar 31 15:16 libmpifort.la
    lrwxrwxrwx 1 balaji collab       20 Mar 31 15:16 libmpifort.so -> libmpifort.so.12.1.0
    lrwxrwxrwx 1 balaji collab       20 Mar 31 15:16 libmpifort.so.12 -> libmpifort.so.12.1.0
    -rwxr-xr-x 1 balaji collab    42788 Mar 31 15:16 libmpifort.so.12.1.0
    -rw-r--r-- 1 balaji collab    29910 Mar 31 15:16 libmpl.a
    -rwxr-xr-x 1 balaji collab      929 Mar 31 15:16 libmpl.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:16 libmpl.so -> libmpl.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:16 libmpl.so.1 -> libmpl.so.1.0.0
    -rwxr-xr-x 1 balaji collab    27039 Mar 31 15:16 libmpl.so.1.0.0
    -rw-r--r-- 1 balaji collab     4148 Mar 31 15:16 libopa.a
    -rwxr-xr-x 1 balaji collab      939 Mar 31 15:16 libopa.la
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:16 libopa.so -> libopa.so.1.0.0
    lrwxrwxrwx 1 balaji collab       15 Mar 31 15:16 libopa.so.1 -> libopa.so.1.0.0
    -rwxr-xr-x 1 balaji collab     8320 Mar 31 15:16 libopa.so.1.0.0
    drwxr-xr-x 2 balaji collab     4096 Mar 31 15:16 pkgconfig

An application that was compiled with **mpich-3.1** would have
dependencies such as the following:

    % mpicc ../mpich/examples/cpi.c -o cpi
    % ldd ./cpi
        linux-vdso.so.1 =>  (0x00007fff101d5000)
        libmpi.so.12 => /sandbox/balaji/build/install/lib/libmpi.so.12 (0x00007f60d757c000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f60d718a000)
        libmpl.so.1 => /sandbox/balaji/build/install/lib/libmpl.so.1 (0x00007f60d6f83000)
        libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007f60d6d66000)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007f60d6b50000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f60d7e1c000)

Notice that the application only has a dependency on **libmpi.so.12**,
and not on **libmpi.so.12.0.0**. Thus, the application can work
correctly with other MPI releases (either within MPICH or other ABI
initiative partners) that exposes the same library dependency:
**libmpi.so.12**.

## ABI Points of Contact

  - **Argonne:** Ken Raffenetti (primary) and Pavan Balaji (secondary)
  - **Intel:** Dmitry Durnov
  - **Cray:** Steve Oyanagi (primary) and Mark Pagel (secondary)
  - **MVAPICH2:** Hari Subramoni
  - **Parastation MPI:** Norbert Eicker
  - **RIKEN MPI:** Masamichi Takagi

## ABI Compatibility Testing

Coming soon.
