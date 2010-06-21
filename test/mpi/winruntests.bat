IF '%1' == '' GOTO RUN_DEBUG
IF '%1' == 'Debug' GOTO RUN_DEBUG
IF '%1' == 'Release' GOTO RUN_RELEASE
REM Usage: winruntests [config]
REM config = Debug or Release
GOTO END:
:RUN_RELEASE
cscript //T:3600 runtests.wsf /config:Release /channel:nemesis /out:summary_nemesis.xml
cscript //T:3600 runtests.wsf /config:Release /channel:sock /out:summary.xml
GOTO RUN_END
:RUN_DEBUG
cscript //T:3600 runtests.wsf /config:Debug
:RUN_END
rem cleanup
del test.ord
del testfile
del testfile.0
del testfile.1
del testfile.2
del testfile.3
del iotest.txt
del cxx\io\iotest.txt
del f77\io\iotest.txt
del .test.ord.shfp.*
:END
