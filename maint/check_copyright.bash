#! /bin/bash

numfiles=`git ls-files | egrep -v '(src/openpa)' | wc -l`

count=1
for x in `git ls-files | egrep -v '(src/openpa)'` ; do

    if test ! -f $x ; then continue ; fi

    echo -n "[$count/$numfiles] $x... "
    count=$(($count + 1))

    listed=`grep "(C) [0-9]* by Argonne National Laboratory" $x | head -1 | \
	sed -e 's/.*(C) \([0-9]*\) by Argonne National Laboratory.*/\1/g'`
    if test -z "$listed" ; then
	echo "no copyright (ignoring)"
	continue
    fi
    expected=`date --date="\`git log --follow --find-copies-harder -M -C 6a1cbdcf..HEAD $x | \
		grep ^Date: | tail -1 | sed -e 's/Date: *//g' | cut -f1-5 -d' '\`" +'%Y'`

    if test $listed -le 2007 ; then
	# echo "ignoring $x because of cvs->svn migration date loss"
	echo "pre-svn (ignoring)"
	continue
    fi

    if test "$expected" != "$listed" ; then
	# echo "$x (expected: $expected; listed: $listed)"
	sed -i "s/(C) $listed by Argonne National Laboratory/(C) $expected by Argonne National Laboratory/g" $x
	echo "incorrect (fixed)"
	continue
    fi

    echo "nothing to fix"

done
