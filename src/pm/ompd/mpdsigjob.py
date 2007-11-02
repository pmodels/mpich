#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#
"""
usage: mpdsigjob  sigtype  [-j jobid OR -a jobalias] [-s|g]
    sigtype must be the first arg
    jobid can be obtained via mpdlistjobs and is of the form:
        jobnum@mpdid where mpdid is mpd where first process runs, e.g.:
            1@linux02_32996 (may need \@ in some shells)
            1  is sufficient if you are on the machine where the job started
    one of -j or -a must be specified (but only one)
    -s or -g specify whether to signal the single user process or its group (default is g)
Delivers a Unix signal to all the application processes in the job
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""


from os     import environ, getuid, close
from sys    import argv, exit
from socket import socket, fromfd, AF_UNIX, SOCK_STREAM
from signal import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpd_raise, mpdError, mpd_send_one_line

def mpdsigjob():
    mpd_set_my_id('mpdsigjob_')
    if len(argv) < 3  or  argv[1] == '-h'  or  argv[1] == '--help':
        usage()
    username = mpd_get_my_username()
    if environ.has_key('UNIX_SOCKET'):
        conFD = int(environ['UNIX_SOCKET'])
        conSocket = fromfd(conFD,AF_UNIX,SOCK_STREAM)
        close(conFD)
    else:
        if environ.has_key('MPD_CON_EXT'):
            conExt = '_' + environ['MPD_CON_EXT']
        else:
            conExt = ''
        consoleName = '/tmp/mpd2.console_' + username + conExt
        conSocket = socket(AF_UNIX,SOCK_STREAM)  # note: UNIX socket
        try:
            conSocket.connect(consoleName)
        except Exception, errmsg:
            print 'mpdsigjob: cannot connect to local mpd (%s); possible causes:' % consoleName
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
	    print 'you can start an mpd with the "mpd" command; to get help, run:'
	    print '    mpd -h'
            exit(-1)
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
        msgToSend = 'realusername=%s\n' % username
        mpd_send_one_line(conSocket,msgToSend)
    sigtype = argv[1]
    if sigtype.startswith('-'):
        sigtype = sigtype[1:]
    if sigtype.startswith('SIG'):
        sigtype = sigtype[3:]
    import signal as tmpsig  # just to get valid SIG's
    if sigtype.isdigit():
        if int(sigtype) > tmpsig.NSIG:
            print 'invalid signum: %s' % (sigtype)
            exit(-1)
    else:
	if not tmpsig.__dict__.has_key('SIG' + sigtype):
	    print 'invalid sig type: %s' % (sigtype)
	    exit(-1)
    jobalias = ''
    jobnum = ''
    mpdid = ''
    single_or_group = 'g'
    i = 2
    while i < len(argv):
        if argv[i] == '-a':
            if jobnum:      # should not have both alias and jobid
                print '** cannot specify both jobalias and jobid'
                usage()
            jobalias = argv[i+1]
            i += 1
            jobnum = '0'
        elif argv[i] == '-j':
            if jobalias:    # should not have both alias and jobid
                print '** cannot specify both jobalias and jobid'
                usage()
            jobid = argv[i+1]
            i += 1
            sjobid = jobid.split('@')
            jobnum = sjobid[0]
            if len(sjobid) > 1:
                mpdid = sjobid[1]
        elif argv[i] == '-s':
            single_or_group = 's'
        elif argv[i] == '-g':
            single_or_group = 'g'
        else:
            print '** unrecognized arg: %s' % (argv[i])
            usage()
        i += 1
    msgToSend = {'cmd' : 'mpdsigjob', 'sigtype': sigtype,
                 'jobnum' : jobnum, 'mpdid' : mpdid, 'jobalias' : jobalias,
                 's_or_g' : single_or_group, 'username' : username }
    mpd_send_one_msg(conSocket, msgToSend)
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdsigjob_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
        exit(-1)
    if not msg['handled']:
        print 'job not found'
        exit(-1)
    conSocket.close()


def signal_handler(signum,frame):
    if signum == SIGALRM:
        pass
    else:
        exit(-1)

def recv_one_msg_with_timeout(sock,timeout):
    oldTimeout = alarm(timeout)
    msg = mpd_recv_one_msg(sock)    # fails WITHOUT a msg if sigalrm occurs
    alarm(oldTimeout)
    return(msg)

def usage():
    print __doc__
    exit(-1)


if __name__ == '__main__':
    signal(SIGINT,signal_handler)
    signal(SIGALRM,signal_handler)
    try:
	mpdsigjob()
    except mpdError, errmsg:
	print 'mpdsigjob failed: %s' % (errmsg)
