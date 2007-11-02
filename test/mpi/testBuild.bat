IF "%DevEnvDir%" == "" GOTO RETRYMSVCINIT
GOTO AFTERWARNING
:RETRYMSVCINIT
IF "%MSVCInitScriptName%" == "" GOTO WARNING
CALL %MSVCInitScriptName%
IF "%DevEnvDir%" == "" GOTO WARNING
GOTO AFTERWARNING
:WARNING
REM
REM Warning: It is recommended that you use the prompt started from the "Visual Studio Command Prompt" shortcut in order to set the paths to
 the tools.  You can also call vsvars32.bat to set the environment before running this script.
PAUSE
:AFTERWARNING
IF "%1" == "Debug" GOTO BUILDDEBUG
devenv.com test.sln /build Release > maketest.log
IF "%1" == "Release" GOTO END
:BUILDDEBUG
devenv.com test.sln /build Debug >> maketest.log
GOTO END
:HELP
REM #########################################
REM Usage is : testBuild.bat [Debug, Release]
REM 
REM If nothing is specified Debug & Release are built 
REM #########################################
:END
