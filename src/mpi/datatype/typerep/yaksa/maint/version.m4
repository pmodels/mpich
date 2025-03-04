[#] start of __file__
##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

m4_define([YAKSA_VERSION_m4],[unreleased])dnl
m4_define([YAKSA_RELEASE_DATE_m4],[unreleased development copy])dnl

# For libtool ABI versioning rules see:
# http://www.gnu.org/software/libtool/manual/libtool.html#Updating-version-info
#
#     1. If the library source code has changed at all since the last
#     update, then increment revision (`c:r:a' becomes `c:r+1:a').
#
#     2. If any interfaces have been added, removed, or changed since
#     the last update, increment current, and set revision to 0.
#
#     3. If any interfaces have been added since the last public
#     release, then increment age.
#
#     4. If any interfaces have been removed since the last public
#     release, then set age to 0.

m4_define([libyaksa_so_version_m4],[0:0:0])dnl

[#] end of __file__
