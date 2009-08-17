#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
This program is not to be executed from the command line.  It is 
exec'd by mpdman to support mpigdb.
"""

# workaround to suppress deprecated module warnings in python2.6
# see https://trac.mcs.anl.gov/projects/mpich2/ticket/362 for tracking
import warnings
warnings.filterwarnings('ignore', '.*the popen2 module is deprecated.*', DeprecationWarning)

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.17 $"
__credits__ = ""


from sys    import argv, exit, stdin, stdout, stderr
from os     import kill, getpid, write, strerror
from popen2 import Popen4
from signal import signal, SIGUSR1, SIGINT, SIGKILL
from errno  import EINTR
from select import select, error
from re     import findall, sub
from mpdlib import mpd_set_my_id, mpd_print

global appPid, gdbPID


def sig_handler(signum,frame):
    global appPid, gdbPID
    if signum == SIGINT:
        try:
            kill(appPid,SIGINT)
        except:
            pass
    elif signum == SIGUSR1:
        try:
            kill(gdbPid,SIGKILL)
        except:
            pass
        try:
            kill(appPid,SIGKILL)
        except:
            pass


if __name__ == '__main__':    # so I can be imported by pydoc
    signal(SIGINT,sig_handler)
    signal(SIGUSR1,sig_handler)
    mpd_set_my_id('mpdgdbdrv')
    
    ## mpd_print(1,"RMB:GDBDRV: ARGS=%s" % argv)
    if argv[1] == '-attach':
        gdb_args = '%s %s' % (argv[2],argv[3])  # userpgm and userpid
    else:
        if len(argv) > 2:
            mpd_print(1, "when using gdb, pass cmd-line args to user pgms via the 'run' cmd")
            exit(-1)
        gdb_args = argv[1]
    gdb_info = Popen4('gdb -q %s' % (gdb_args), 0 )
    gdbPid = gdb_info.pid
    # print "PID=%d GDBPID=%d" % (getpid(),gdbPid) ; stdout.flush()
    gdb_sin = gdb_info.tochild
    gdb_sin_fileno = gdb_sin.fileno()
    gdb_sout_serr = gdb_info.fromchild
    gdb_sout_serr_fileno = gdb_sout_serr.fileno()
    write(gdb_sin_fileno,'set prompt (gdb)\\n\n')
    gdb_line = gdb_sout_serr.readline() 
    # check if gdb reports any errors
    if findall(r'.*: No such file or directory.',gdb_line) != []:
        print gdb_line, ; stdout.flush()
        exit(-1)
    mpd_print(0000, "LINE1=|%s|" % (gdb_line.rstrip()))
    write(gdb_sin_fileno,'set confirm off\n')
    gdb_line = gdb_sout_serr.readline() 
    mpd_print(0000, "LINE2=|%s|" % (gdb_line.rstrip()))
    write(gdb_sin_fileno,'handle SIGUSR1 nostop noprint\n')
    gdb_line = gdb_sout_serr.readline() 
    mpd_print(0000, "LINE3=|%s|" % (gdb_line.rstrip()))
    write(gdb_sin_fileno,'handle SIGPIPE nostop noprint\n')
    gdb_line = gdb_sout_serr.readline() 
    mpd_print(0000, "LINE4=|%s|" % (gdb_line.rstrip()))
    write(gdb_sin_fileno,'set confirm on\n')
    gdb_line = gdb_sout_serr.readline() 
    mpd_print(0000, "LINE5=|%s|" % (gdb_line.rstrip()))
    write(gdb_sin_fileno,'echo hi1\n')
    gdb_line = gdb_sout_serr.readline() 
    # mpd_print(0000, "LINE6=|%s|" % (gdb_line.rstrip()))
    # gdb_line = ''
    while not gdb_line.startswith('hi1'):
        gdb_line = gdb_sout_serr.readline() 
        mpd_print(0000, "LINEx=|%s|" % (gdb_line.rstrip()))

    if argv[1] != '-attach':
        write(gdb_sin_fileno,'b main\n')
        gdb_line = ''
        while not gdb_line.startswith('Breakpoint'):
            try:
                (readyFDs,unused1,unused2) = select([gdb_sout_serr_fileno],[],[],10)
            except error, data:
                if data[0] == EINTR:    # interrupted by timeout for example
                    continue
                else:
                    print 'mpdgdb_drv: main loop: select error: %s' % strerror(data[0])
            if not readyFDs:
                mpd_print(1, 'timed out waiting for initial Breakpoint response')
                exit(-1)
            gdb_line = gdb_sout_serr.readline()  # drain breakpoint response
            gdb_line = gdb_line.strip()
            mpd_print(0000, "gdb_line=|%s|" % (gdb_line.rstrip()))
        if not gdb_line.startswith('Breakpoint'):
            mpd_print(1, 'expecting "Breakpoint", got :%s:' % (gdb_line) )
            exit(-1)
        gdb_line = gdb_sout_serr.readline()  # drain prompt
        mpd_print(0000, "gdb_line=|%s|" % (gdb_line.rstrip()))
        if not gdb_line.startswith('(gdb)'):
            mpd_print(1, 'expecting "(gdb)", got :%s:' % (gdb_line) )
            exit(-1)
    
    print '(gdb)\n', ; stdout.flush()    # initial prompt to user
    
    user_fileno = stdin.fileno()
    while 1:
        try:
            (readyFDs,unused1,unused2) = select([user_fileno,gdb_sout_serr_fileno],[],[],1)
        except error, data:
            if data[0] == EINTR:    # interrupted by timeout for example
                continue
            else:
                mpd_print(1, 'mpdgdbdrv: main loop: select error: %s' % strerror(data[0]))
        # print "READY=", readyFDs ; stdout.flush()
        for readyFD in readyFDs:
            if readyFD == gdb_sout_serr_fileno:
                gdb_line = gdb_sout_serr.readline()
                if not gdb_line:
                    print "MPIGDB ENDING" ; stdout.flush()
                    exit(0)
                # print "LINE |%s|" % (gdb_line.rstrip()) ; stdout.flush()
                print gdb_line, ; stdout.flush()
            elif readyFD == user_fileno:
                user_line = stdin.readline()
                # print "USERLINE=", user_line, ; stdout.flush()
                if not user_line:
                    mpd_print(1, 'mpdgdbdrv: problem: expected user input but got none')
                    exit(-1)
                if user_line.startswith('r'):
                    write(gdb_sin_fileno,'show prompt\n')
                    gdb_line = gdb_sout_serr.readline()
                    gdb_prompt = findall(r'Gdb\'s prompt is "(.+)"\.',gdb_line)
                    if gdb_prompt == []:
                        mpd_print(1, 'expecting gdb\'s prompt, got :%s:' % (gdb_line))
                        exit(-1)
                    gdb_prompt = gdb_prompt[0]
                    # cut everything after first escape character (including it)
                    p = gdb_prompt.find("\\")
                    if p > 0:
                        gdb_prompt = gdb_prompt[0:p]
                    gdb_line = gdb_sout_serr.readline() # drain one line

                    write(gdb_sin_fileno,'show confirm\n')
                    gdb_line = gdb_sout_serr.readline()
                    gdb_confirm = findall(r'Whether to confirm potentially dangerous operations is (on|off)\.',gdb_line)
                    if gdb_confirm == []:
                        mpd_print(1, 'expecting gdb\'s confirm state, got :%s:' % (gdb_line))
                        exit(-1)
                    gdb_confirm = gdb_confirm[0]
                    gdb_line = gdb_sout_serr.readline() # drain one line

                    # set confirm to 'on' to get 'Starting program' message
                    write(gdb_sin_fileno,'set confirm on\n')
                    gdb_line = gdb_sout_serr.readline()

                    # we have already set breakpoint 1 in main
                    write(gdb_sin_fileno,user_line)
                    # ignore any warnings befor starting msg
                    while 1:
                        gdb_line = gdb_sout_serr.readline()  # drain one line
                        if not gdb_line.startswith('warning:'):
                            break
                        else:
                            print gdb_line, ; stdout.flush()
                    # drain starting msg
                    if not gdb_line.startswith('Starting program'):
                        mpd_print(1, 'expecting "Starting program", got :%s:' % \
                                  (gdb_line))
                        exit(-1)
                    while 1:    # drain to a prompt
                        gdb_line = gdb_sout_serr.readline()  # drain one line
                        if gdb_line.startswith(gdb_prompt):
                            break
                    # try to get the pid
                    write(gdb_sin_fileno,'info pid\n')  # macosx
                    gdb_line = gdb_sout_serr.readline().lstrip()
                    if gdb_line.find('process ID') >= 0:  # macosx
                        appPid = findall(r'.* has process ID (\d+)',gdb_line)
                        appPid = int(appPid[0])
                    else:
                        while 1:    # drain to a prompt
                            gdb_line = gdb_sout_serr.readline()  # drain one line
                            if gdb_line.startswith(gdb_prompt):
                                break
                        write(gdb_sin_fileno,'info program\n')
                        gdb_line = gdb_sout_serr.readline().lstrip()
                        if gdb_line.startswith('Using'):
                            if gdb_line.find('process') >= 0:
                                appPid = findall(r'Using .* image of child process (\d+)',gdb_line)
                            elif gdb_line.find('Thread') >= 0:  # solaris
                                appPid = findall(r'Using .* image of child .* \(LWP (\d+)\).',gdb_line)
                            else:
                                mpd_print(1, 'expecting process or thread line, got :%s:' % \
                                          (gdb_line))
                                exit(-1)
                            appPid = int(appPid[0])
                        else:
                            mpd_print(1, 'expecting line with "Using"; got :%s:' % (gdb_line))
                            exit(-1)
                    while 1:    # drain to a prompt
                        gdb_line = gdb_sout_serr.readline()  # drain one line
                        if gdb_line.startswith(gdb_prompt):
                            break
                    write(gdb_sin_fileno,'c\n')
                    # set confirm back to original state
                    write(gdb_sin_fileno,'set confirm %s\n' % (gdb_confirm))
                else:
                    write(gdb_sin_fileno,user_line)
