IF "%1" == "--help" GOTO HELP
IF "%CVS_HOST%" == "" set CVS_HOST=harley.mcs.anl.gov
GOTO AFTERHELP
:HELP
REM
REM Usage:
REM    getdotin
REM      1) check out mpich2 from cvs
REM         the environment variable USERNAME is used to checkout mpich2
REM         If this variable is not set or is set to the wrong mcs user:
REM           set USERNAME=mymcsusername
REM
REM Prerequisites:
REM  ssh in your path
GOTO END
:AFTERHELP
set CVS_RSH=ssh
REM bash -c "echo cd `pwd` && echo maint/updatefiles" > bashcmds.txt
REM bash --login < bashcmds.txt
echo cd /sandbox/%USERNAME% > sshcmds.txt
echo mkdir dotintmp >> sshcmds.txt
echo cd dotintmp >> sshcmds.txt
if "%1" == "" GOTO EXPORT_HEAD
echo cvs -d /home/MPI/cvsMaster export -r %1 mpich2allWithMPE >> sshcmds.txt
GOTO AFTER_EXPORT_HEAD
:EXPORT_HEAD
echo cvs -d /home/MPI/cvsMaster export -r HEAD mpich2allWithMPE >> sshcmds.txt
:AFTER_EXPORT_HEAD
echo cd mpich2 >> sshcmds.txt
echo maint/updatefiles >> sshcmds.txt
echo tar cvf dotin.tar `find . -name "*.h.in"` >> sshcmds.txt
echo gzip dotin.tar >> sshcmds.txt
echo exit >> sshcmds.txt
ssh -l %USERNAME% %CVS_HOST% < sshcmds.txt
scp %USERNAME%@%CVS_HOST%:/sandbox/%USERNAME%/dotintmp/mpich2/dotin.tar.gz .
ssh -l %USERNAME% %CVS_HOST% rm -rf /sandbox/%USERNAME%/dotintmp
del sshcmds.txt
tar xvfz dotin.tar.gz
del dotin.tar.gz
REM cscript winconfigure.wsf --cleancode --enable-timer-type=queryperformancecounter
