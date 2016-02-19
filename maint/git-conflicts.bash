#! /bin/bash

max_abbrev=12
scale=10

total=`git rev-list --all | wc -l`

echo "Conflicts:"
echo "Format:   [abbrev length]: number of conflicts / total number of refs (percentage conflicts)"
for ((x = 1 ; x <= ${max_abbrev} ; x++)) ; do
    conflicts=`git rev-list --all --abbrev=$x --abbrev-commit | sort | uniq -D -w$x | wc -l`
    percent=`echo "scale=$scale ; 100 * $conflicts / $total" | bc | awk '{ printf("%1.2f\n", $0); }'`
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

min_abbrev=$x

echo
echo "Probability of collision on your next commit:"
echo "Format:   [abbrev length]: probability of collision"
for ((x = ${min_abbrev}; x <= ${max_abbrev} ; x++)) ; do
    percent=`echo "scale=$scale ; 100 * ($total) / (16 ^ $x)" | bc | awk '{ printf("%1.4f\n", $0); }'`
    echo "    [$x]: $percent %"
done

commits_last_year=`git rev-list --all --since="1 year ago" | wc -l`

echo
echo "Probability of collision in the next year (based on commits last year):"
echo "Format:   [abbrev length]: probability of collision"
for ((x = ${min_abbrev}; x <= ${max_abbrev} ; x++)) ; do
    percent=`echo "scale=$scale ; 100 * (1 - ((1 - ($total / (16 ^ $x))) ^ ${commits_last_year}))" | bc | awk '{ printf("%1.4f\n", $0); }'`
    echo "    [$x]: $percent %"
done
