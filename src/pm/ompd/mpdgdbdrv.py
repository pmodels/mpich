#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
This program is not to be executed from the command line.  It is 
exec'd by mpdman to support mpigdb.
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
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
    
    # print "RMB:GDBDRV: ARGS=", argv
    if len(argv) > 2:
        args = ' '.join(argv[2:])
    else:
        args = ''
    gdb_info = Popen4('gdb -q %s %s' % (argv[1],args), 0 )
    gdbPid = gdb_info.pid
    # print "PID=%d GDBPID=%d" % (getpid(),gdbPid) ; stdout.flush()
    gdb_sin = gdb_info.tochild
    gdb_sin_fileno = gdb_sin.fileno()
    gdb_sout_serr = gdb_info.fromchild
    gdb_sout_serr_fileno = gdb_sout_serr.fileno()
    write(gdb_sin_fileno,'set prompt (gdb)\\n\n')
    gdb_line = gdb_sout_serr.readline() 
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
    mpd_print(0000, "LINE6=|%s|" % (gdb_line.rstrip()))
    gdb_line = ''
    while not gdb_line.startswith('hi1'):
        gdb_line = gdb_sout_serr.readline() 
        mpd_print(0000, "LINEx=|%s|" % (gdb_line.rstrip()))
    
    write(gdb_sin_fileno,'b main\n')
    gdb_line = ''
    while not gdb_line.startswith('Breakpoint'):
        try:
            (readyFDs,unused1,unused2) = select([gdb_sout_serr_fileno],[],[],3)
        except error, data:
            if data[0] == EINTR:    # interrupted by timeout for example
                continue
            else:
                print 'mpdgdb_drv: main loop: select error: %s' % strerror(data[0])
        if not readyFDs:
            mpd_print(1, 'timed out waiting for initial Breakpoint response')
            exit(-1)
        gdb_line = gdb_sout_serr.readline()  # drain breakpoint response
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
                mpd_print(1, 'mpdgdb_drv: main loop: select error: %s' % strerror(data[0]))
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
                    # we have already set breakpoint 1 in main
                    write(gdb_sin_fileno,user_line)
                    gdb_line = gdb_sout_serr.readline()  # drain starting msg
                    if not gdb_line.startswith('Starting program'):
                        mpd_print(1, 'expecting "Starting program", got :%s:' % \
                                  (gdb_line))
                        exit(-1)
                    while 1:    # drain to a prompt
                        gdb_line = gdb_sout_serr.readline()  # drain one line
                        if gdb_line.startswith('(gdb)'):
                            break
                    write(gdb_sin_fileno,'info program\n')
                    gdb_line = gdb_sout_serr.readline().lstrip()  # get pid
                    if gdb_line.startswith('Using'):
                        if gdb_line.find('process') >= 0:
                            appPid = findall(r'Using .* image of child process (\d+)',gdb_line)
                        elif gdb_line.find('Thread') >= 0:
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
                        if gdb_line.startswith('(gdb)'):
                            break
                    write(gdb_sin_fileno,'c\n')
                else:
                    write(gdb_sin_fileno,user_line)
