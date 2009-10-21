#! /bin/sh

for subdir in . tools/bind/hwloc/hwloc ; do
    echo "=== Entering subdir $subdir ==="
    if (cd $subdir && autoreconf -vif) ; then
	:
    else
	echo "        autoreconf failed!"
	exit
    fi
    echo "=== Done with subdir $subdir ==="
done
