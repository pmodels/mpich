#!/bin/sh

echo 
echo "RMB: tests between magpie and cluster hosts"
echo 

if [ ! -x $HOME/mpich2/test/mpi/spawn/spawn1 ] ; then
    echo '***  build the spawn pgms first'
    exit -1
fi

test1.py
test2.py
test3.py
test4.py
test5.py
testroot.py
