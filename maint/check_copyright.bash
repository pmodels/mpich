#! /bin/bash

for x in `find . ! -type d | grep -v .git | egrep -v '(src/openpa)'` ; do

    if test  -z "`git log $x`" ; then
	continue
    fi

    listed=`grep "(C) [0-9]* by Argonne National Laboratory" $x | head -1 | \
	sed -e 's/.*(C) \([0-9]*\) by Argonne National Laboratory.*/\1/g'`
    if test -z "$listed" ; then
	continue
    fi
    expected=`date --date="\`git log --follow 6a1cbdcf..HEAD $x | grep ^Date: | tail -1 | \
	sed -e 's/Date: *//g' | cut -f1-5 -d' '\`" +'%Y'`

    if test $listed -le 2007 ; then
	# echo "ignoring $x because of cvs->svn migration date loss"
	continue
    fi

    if test "$expected" != "$listed" ; then
	# echo "$x (expected: $expected; listed: $listed)"
	sed -i "s/(C) $listed by Argonne National Laboratory/(C) $expected by Argonne National Laboratory/g" $x
    fi

done
