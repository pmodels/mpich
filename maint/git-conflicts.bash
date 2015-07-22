#! /bin/bash

max_abbrev=12

total=`git rev-list --all | wc -l`

echo "Conflicts:"
echo "Format:   [abbrev length]: number of conflicts / total number of refs (percentage conflicts)"
for ((x = 1 ; x <= ${max_abbrev} ; x++)) ; do
    conflicts=`git rev-list --all --abbrev=$x --abbrev-commit | sort | uniq -D -w$x | wc -l`
    percent=`echo "scale=8 ; 100 * $conflicts / $total" | bc | awk '{ printf("%1.2f\n", $0); }'`
    echo "    [$x]: $conflicts / $total ($percent %)"
    if test "$conflicts" = "0" ; then break ; fi
done

##
## probability calculation is based on this random link, that I found
## online:
## http://blog.cuviper.com/2013/11/10/how-short-can-git-abbreviate/
##
## I have not verified the accuracy of the calculation at that link,
## but if it is online, it must be true, of course. ;-)
##

echo
echo "Probability of collision on your next commit:"
echo "Format:   [abbrev length]: probability of collision"
for ((; x <= ${max_abbrev} ; x++)) ; do
    percent=`echo "scale=8 ; 100 * ($total) / (16 ^ $x)" | bc | awk '{ printf("%1.4f\n", $0); }'`
    echo "    [$x]: $percent %"
done
