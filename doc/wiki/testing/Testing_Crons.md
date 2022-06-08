# Testing Crons

This page should help in debugging issues running the old nightly tests.

## Raw crontab info

Dump of crontabs from thakur@, wbland@ and balaji@ on relevant hosts as
of 2013-05-02. These cover the **old** nightly tests.

2015/03/20: cron job change from wbland@ to huiweilu@

2015/08/27: cron job change from huiweilu@ to raffenet@

```
==== dumping thakur's contab on steamroller
# m h  dom mon dow   command
# updatecvs updates the mpich2 in /home/MPI/testing/mpich2/mpich2 and
# runs maint/updatefiles over it
30 20 * * * /mcs/mpich/cron/tests/updatecvs
01 09 * * * /mcs/mpich/cron/tests/specialtest -dir=/sandbox/thakur
==== dumping thakur's contab on crush
# m h  dom mon dow   command
30 21 * * * /mcs/mpich/cron/tests/basictests -arch=x86_64-Linux-GNU
==== dumping thakur's contab on octagon
# m h  dom mon dow   command
30 22 * * * /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-graphics -soft=intel -cc="icc" -cxx="icpc" -f77="ifort" -fc="ifort" -arch=IA32-Linux-Intel
==== dumping thakur's contab on octopus
# m h  dom mon dow   command
07 09 * * * /mcs/mpich/cron/tests/updatesum
07 07 * * *  /mcs/mpich/cron/tests/updatesum
00 13 * * *  /mcs/mpich/cron/tests/updatesum
07 17 * * *  /mcs/mpich/cron/tests/cleanold
00 19 * * *  /mcs/mpich/cron/tests/updatetsuites
# Update the mpich todo list.  Make this update happen after the
# check for runaways
09 08 * * * /mcs/mpich/cron/tests/updatetodo
# Make sure that the ftp logs are being collected
#27 08 * * * /home/MPI/maint/ftplogs/checklogs
# Get ftp log info
#17 03 1 * * /home/MPI/maint/ftplogs/get_aftp_monthly
30 05 * * * /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-shared -arch=IA32-Linux-GNU-no-shared
==== dumping thakur's contab on vanquish
# m h  dom mon dow   command
00 21 * * * /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-graphics -soft=intel -cc=icc -cxx=icpc -f77=ifort -fc=ifort -arch=x86_64-Linux-Intel
05 05 * * * /mcs/mpich/cron/tests/getcoverage -updateweb -device=ch3:nemesis -logfile=/sandbox/thakur/mpich2-cov/getcov.log
==== dumping thakur's contab on trounce
# m h  dom mon dow   command
15 21 * * * /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-totalview -soft=pgi -cc="pgcc" -cxx="pgc++" -f77="pgf77" -fc="pgf90" -arch=x86_64-Linux-PG
==== dumping raffenet's contab on octopus
00 22 * * * /mcs/mpich/cron/tests/basictests -arch=IA32-Linux-GNU -tmpdir=/sandbox/raffenet/mpich_nightly/IA32-Linux-GNU
==== dumping raffenet's contab on steamroller
00 21 * * * /mcs/mpich/cron/tests/basictests -soft=nag-f95 -cc="gcc" -cxx="g++" -f77="nagfor" -fc="nagfor" -otherargs="-env=FFLAGS=-mismatch_all" -otherargs="-env=FCFLAGS=-mismatch_all" -arch=x86_64-Linux-GNU-NAG -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-NAG"
==== dumping thakur's contab on thwomp
00 23 * * * /mcs/mpich/cron/tests/basictests -soft=intel -cc="gcc" -cxx="g++" -f77="ifort" -fc="ifort" -arch=x86_64-Linux-GNU-Intel -tmpdir="/sandbox/thakur/mpich_nightly/x86_64-Linux-GNU-Intel"
==== dumping raffenet's contab on stomp
00 22 * * * /mcs/mpich/cron/tests/basictests -soft=pgi -cc="gcc" -cxx="g++" -f77="pgf77" -fc="pgf90" -arch=x86_64-Linux-GNU-PG -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-PG"
==== dumping raffenet's contab on crank
30 23 * * * /mcs/mpich/cron/tests/basictests -soft=absoft -cc="gcc" -cxx="g++" -f77="af77" -fc="af90" -arch=x86_64-Linux-GNU-Absoft -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-Absoft"

==== dumping balaji's crontab on lucifer
00 00   *   *   *   /mcs/mpich/cron/snapshots/snapshot
00 00   *   *   *   /mcs/mpich/cron/snapshots/hydra-snapshot
```

The above output was created by concatenating the results of the
following command as run by each user (along with some minor trimming):

```
for h in octopus crush vanquish trounce steamroller thwomp stomp crank; do echo "==== dumping ${USER}'s contab on ${h}" ; ssh ${h}.mcs.anl.gov crontab -l ; done
```
## Human Readable Timeline

| Host        | Crontab Owner | Start Time | Days of the Week | Command                                                                                                                                                                                                                                                                      |
| ----------- | ------------- | ---------- | ---------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| steamroller | thakur        | 9:01am     | \*               | /mcs/mpich/cron/tests/specialtest -dir=/sandbox/thakur                                                                                                                                                                                                                       |
| steamroller | thakur        | 8:30pm     | \*               | /mcs/mpich/cron/tests/updatecvs \# updatecvs updates the mpich2 source in /home/MPI/testing/mpich2/mpich2 and runs autogen.sh over it                                                                                                                                        |
| octopus     | thakur        | 7:07am     | \*               | /mcs/mpich/cron/tests/updatesum \# update the results on the web                                                                                                                                                                                                             |
| octopus     | thakur        | 9:07am     | \*               | /mcs/mpich/cron/tests/updatesum                                                                                                                                                                                                                                              |
| octopus     | thakur        | 1:00pm     | \*               | /mcs/mpich/cron/tests/updatesum                                                                                                                                                                                                                                              |
| octopus     | thakur        | 7:00pm     | \*               | /mcs/mpich/cron/tests/updatetsuites                                                                                                                                                                                                                                          |
| octopus     | thakur        | 8:09am     | \*               | /mcs/mpich/cron/tests/updatetodo \# Update the mpich todo list (old)                                                                                                                                                                                                         |
| octopus     | thakur        | 5:07pm     | \*               | /mcs/mpich/cron/tests/cleanold                                                                                                                                                                                                                                               |
| octopus     | thakur        | 5:30am     | \*               | /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-shared -arch=IA32-Linux-GNU-no-shared                                                                                                                                                                          |
| octagon     | thakur        | 10:30pm    | \*               | /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-graphics -soft=intel -cc="icc" -cxx="icpc" -f77="ifort" -fc="ifort" -arch=IA32-Linux-Intel                                                                                                                     |
| crush       | thakur        | 9:30pm     | \*               | /mcs/mpich/cron/tests/basictests -arch=x86_64-Linux-GNU                                                                                                                                                                                                                     |
| vanquish    | thakur        | 9:00pm     | \*               | /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-graphics -soft=intel -cc=icc -cxx=icpc -f77=ifort -fc=ifort -arch=x86_64-Linux-Intel                                                                                                                          |
| vanquish    | thakur        | 5:05am     | \*               | /mcs/mpich/cron/tests/getcoverage -updateweb -device=ch3:nemesis -logfile=/sandbox/thakur/mpich2-cov/getcov.log                                                                                                                                                              |
| trounce     | thakur        | 9:15pm     | \*               | /mcs/mpich/cron/tests/basictests -otherargs=-enable=--disable-totalview -soft=pgi -cc="pgcc" -cxx="pgCC" -f77="pgf77" -fc="pgf90" -arch=x86_64-Linux-PG                                                                                                                     |
| octopus     | raffenet      | 10:00pm    | \*               | /mcs/mpich/cron/tests/basictests -arch=IA32-Linux-GNU -tmpdir="/sandbox/raffenet/mpich_nightly/IA32-Linux-GNU"                                                                                                                                                              |
| crank       | raffenet      | 11:30pm    | \*               | /mcs/mpich/cron/tests/basictests -soft=absoft -cc="gcc" -cxx="g++" -f77="af77" -fc="af90" -arch=x86_64-Linux-GNU-Absoft -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-Absoft"                                                                                 |
| thwomp      | thakur        | 11:00pm    | \*               | /mcs/mpich/cron/tests/basictests -soft=intel -cc="gcc" -cxx="g++" -f77="ifort" -fc="ifort" -arch=x86_64-Linux-GNU-Intel -tmpdir="/sandbox/thakur/mpich_nightly/x86_64-Linux-GNU-Intel"                                                                                    |
| stomp       | raffenet      | 11:15pm    | \*               | /mcs/mpich/cron/tests/basictests -soft=pgi -cc="gcc" -cxx="g++" -f77="pgf77" -fc="pgf90" -arch=x86_64-Linux-GNU-PG -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-PG"                                                                                          |
| steamroller | raffenet      | 9:00pm     | \*               | /mcs/mpich/cron/tests/basictests -soft=nag-f95 -cc="gcc" -cxx="g++" -f77="nagfor" -fc="nagfor" -otherargs="-env=FFLAGS=-mismatch_all" -otherargs="-env=FCFLAGS=-mismatch_all" -arch=x86_64-Linux-GNU-NAG -tmpdir="/sandbox/raffenet/mpich_nightly/x86_64-Linux-GNU-NAG" |
