#!/bin/sh
#
# Copyright © 2012-2017 Inria.  All rights reserved.
# See COPYING in top-level directory.
#

set -e
set -x

# environment variables
test -f $HOME/.ciprofile && . $HOME/.ciprofile
branch=$( echo $GIT_BRANCH | sed -r -e 's@^.*/([^/]+)$@\1@' )
if test -d $HOME/local/hwloc-$branch ; then
  export PATH=$HOME/local/hwloc-${branch}/bin:$PATH
  echo using specific $HOME/local/hwloc-$branch
else
  export PATH=$HOME/local/hwloc-master/bin:$PATH
  echo using generic $HOME/local/hwloc-master
fi

# remove previous artifacts so that they don't exported again by this build
rm -f hwloc-*.tar.gz hwloc-*.tar.bz2 || true

# display autotools versions
automake --version | head -1
libtool --version | head -1
autoconf --version | head -1

# better version string
snapshot=$branch-`date +%Y%m%d.%H%M`.git`git show --format=format:%h -s`
sed	-e 's/^snapshot_version=.*/snapshot_version='$snapshot/ \
	-e 's/^snapshot=.*/snapshot=1/' \
	-i VERSION

# let's go
./autogen.sh
./configure
make
make distcheck
