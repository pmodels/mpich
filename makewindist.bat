@echo off
IF "%1" == "" GOTO HELP
IF "%1" == "--help" GOTO HELP
REM ### SVN_HOST = SVN URL for extracting source code
REM ### UNIX_HOST = Host used for running scripts remotely
IF "%SVN_HOST%" == "" set SVN_HOST=https://svn.mcs.anl.gov
IF "%UNIX_HOST%" == "" set UNIX_HOST=shakey.mcs.anl.gov
GOTO AFTERHELP
:HELP
REM
REM Usage:
REM    makewindist --help
REM      display this help message
REM    makewindist --with-checkout
REM      1) check out mpich2 from cvs
REM         the environment variable USERNAME is used to checkout mpich2
REM         If this variable is not set or is set to the wrong mcs user:
REM           set USERNAME=mymcsusername
REM      2) configure
REM      3) build the release
REM      this will create an mpich2 directory under the current directory
REM		 (The "--with-checkout" option is for ANL INTERNAL USE ONLY)
REM    makewindist --with-configure
REM      1) configure the current directory
REM      2) build the release
REM      mpich2 must be the current directory
REM		 (The "--with-configure" option is for ANL INTERNAL USE ONLY)
REM    makewindist --with-curdir
REM      1) build the release
REM      mpich2 must be the current directory and it must have been configured
REM		 (Use the "--with-curdir" option if you downloaded the MPICH2 source
REM		  from the MPICH2 downloads webpage)
REM    makewindist [%1] --win64
REM     Builds x86_64 windows binaries/installer
REM
REM
REM Prerequisites:
REM  Microsoft Developer Studio .NET 2003 or later
REM  Intel Fortran 8.0 or later
REM  cygwin with at least ssh, cvs, and perl
REM  cygwin must be in your path so commands like cvs can be executed
REM  This batch file should be run in a command prompt with the MS DevStudio environment variables set
GOTO END
:AFTERHELP
IF "%DevEnvDir%" == "" GOTO WARNING
GOTO AFTERWARNING
:WARNING
REM
REM Warning: It is recommended that you use the prompt started from the "Visual Studio Command Prompt" shortcut in order to set the paths to the tools.  You can also call vsvars32.bat to set the environment before running this script.
PAUSE
if %errorlevel% NEQ 0 goto END
REM
:AFTERWARNING
IF "%1" == "--win64" GOTO HELP
IF "%1" == "--with-checkout" GOTO CHECKOUT
GOTO AFTERCHECKOUT
:CHECKOUT
set CVS_RSH=ssh
IF "%2" == "" GOTO CHECKOUT_HEAD
svn --username %USERNAME% export -r %2 %SVN_HOST%/repos/mpi/mpich2/trunk mpich2
GOTO AFTER_CHECKOUT_HEAD
:CHECKOUT_HEAD
svn --username %USERNAME% export -r HEAD %SVN_HOST%/repos/mpi/mpich2/trunk mpich2
:AFTER_CHECKOUT_HEAD
if %errorlevel% NEQ 0 goto CVSERROR
pushd mpich2
GOTO CONFIGURE
:AFTERCHECKOUT
IF "%1" == "--with-configure" GOTO CONFIGURE
GOTO AFTERCONFIGURE
:CONFIGURE
echo cd /sandbox/%USERNAME% > sshcmds.txt
echo mkdir dotintmp >> sshcmds.txt
echo cd dotintmp >> sshcmds.txt
if "%2" == "" GOTO EXPORT_HEAD
echo svn export -r %2 %SVN_HOST%/repos/mpi/mpich2/trunk mpich2 >> sshcmds.txt
GOTO AFTER_EXPORT_HEAD
:EXPORT_HEAD
echo svn export -r HEAD %SVN_HOST%/repos/mpi/mpich2/trunk mpich2 >> sshcmds.txt
:AFTER_EXPORT_HEAD
echo cd mpich2 >> sshcmds.txt
echo maint/updatefiles >> sshcmds.txt
echo tar cvf dotin.tar `find . -name "*.h.in"` >> sshcmds.txt
echo gzip dotin.tar >> sshcmds.txt
echo exit >> sshcmds.txt
dos2unix sshcmds.txt
ssh -l %USERNAME% %UNIX_HOST% < sshcmds.txt
scp %USERNAME%@%UNIX_HOST%:/sandbox/%USERNAME%/dotintmp/mpich2/dotin.tar.gz .
ssh -l %USERNAME% %UNIX_HOST% rm -rf /sandbox/%USERNAME%/dotintmp
del sshcmds.txt
tar xvfz dotin.tar.gz
del dotin.tar.gz
cscript winconfigure.wsf --cleancode --enable-timer-type=queryperformancecounter
GOTO BUILD_RELEASE
:AFTERCONFIGURE
IF "%1" == "--with-curdir" GOTO BUILD_RELEASE
REM
REM Unknown option: %1
REM
GOTO HELP
:BUILD
:BUILD_DEBUG
IF "%2" == "--win64" GOTO BUILD_WIN64
IF "%2" == "" GOTO BUILD_RELEASE
REM Building the Debug targets
devenv.com mpich2.sln /build ch3sockDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /project mpich2s /build ch3sockDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build Debug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build fortDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build gfortDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build sfortDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build ch3sockmtDebug
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com examples\examples.sln /project cpi /build Debug
if %errorlevel% NEQ 0 goto BUILDERROR
:BUILD_RELEASE
IF "%2" == "--win64" GOTO BUILD_WIN64
echo Building MPICH2 Release version on windows
echo ===========================================
echo  Please refer to the MPICH2 Visual Studio sln file, mpich2.sln, for
echo  more information on the various projects/configs built for MPICH2 
echo -------------------------------------------------------------------
echo Building CH3+SOCK channel ...
devenv.com mpich2.sln /build ch3sockRelease > make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building mpich2s project...
devenv.com mpich2.sln /project mpich2s /build ch3sockRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building CH3+SOCK channel (profiled version)...
devenv.com mpich2.sln /build ch3sockPRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building mpich2s project (profiled version)...
devenv.com mpich2.sln /project mpich2s /build ch3sockPRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building MPI user lib, MPE, MPIEXEC, SMPD ...
devenv.com mpich2.sln /build Release >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building FORTRAN interface ...
devenv.com mpich2.sln /build fortPRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build fortRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build gfortRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
devenv.com mpich2.sln /build sfortRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building CH3+SOCK channel(multithreaded)...
devenv.com mpich2.sln /build ch3sockmtRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building CH3+SOCK channel(multithreaded - profiled version)...
devenv.com mpich2.sln /build ch3sockmtPRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building CH3+NEMESIS channel...
devenv.com mpich2.sln /build ch3nemesisRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building CH3+NEMESIS channel (profiled version)...
devenv.com mpich2.sln /build ch3nemesisPRelease >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building C++ Interface ...
devenv.com mpich2.sln /project cxx /build Release >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building RLOG tools ...
devenv.com src\util\logging\rlog\rlogtools.sln /build Release >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
cd maint
call makegcclibs.bat
cd ..
devenv.com examples\examples.sln /project cpi /build Release > make_example.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo Building MPICH2 installer ...
devenv.com mpich2.sln /build Installer >> make.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo MPICH2 build completed successfully.... - See make.log for the compiler output
GOTO END
:BUILD_WIN64
if "%CPU%" == "X64" goto AFTER_WIN64_SANITY_CHECK
if "%CPU%" == "AMD64" goto AFTER_WIN64_SANITY_CHECK
echo WARNING: WIN64 Build environment setup manually ...
set CPU=x64
set NODEBUG=1
set APPVER=5.02
:AFTER_WIN64_SANITY_CHECK
echo Building MPICH2 x64 ...
REM Adding hpc sdk import libs to PATH so that VS can detect it 
set PATH=%CCP_LIB64%;%PATH%
cd winbuild
CALL build.bat > ..\make_x64.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
cd ..
echo Building MPICH2 examples ...
devenv.com examples\examples.sln /project cpi /build "Release|x64" >> make_x64.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building GNU libs ...
cd maint
CALL makegcclibs_64.bat
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
cd ..
echo Building Wrapper utils
devenv.com mpich2.sln /build "wrapperUtilRelease" >> make_x64.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo Building MPICH2 x64 installer ...
devenv.com mpich2.sln /build "Installerx64|x64" >> make_x64.log
if %errorlevel% NEQ 0 goto BUILDERROR
echo .....................................................SUCCESS
echo MPICH2 x64 build completed successfully.... - See make_x64.log for the compiler output
GOTO END
:BUILDERROR
echo ERROR : BUILD FAILED ! - See make.log for details
GOTO END
:CVSERROR
REM cvs returned a non-zero exit code while attempting to check out mpich2
GOTO END
:END
IF "%1" == "--with-checkout" popd
