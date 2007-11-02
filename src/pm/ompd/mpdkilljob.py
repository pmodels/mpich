#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdkilljob  jobnum  [mpdid]  # as obtained from mpdlistjobs
   or: mpdkilljob  -a jobalias      # as obtained from mpdlistjobs
    mpdid is mpd where process with 'rank' 0 starts
    mpdid of form 1@linux02_32996 (may need \@ in csh)
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

def mpdkilljob():
    mpd_set_my_id('mpdkilljob_')
    if len(argv) < 2  or  argv[1] == '-h'  or  argv[1] == '--help':
        print __doc__
        exit(-1)
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
            print 'mpdkilljob: cannot connect to local mpd (%s); possible causes:' % consoleName
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
	    print 'you can start an mpd with the "mpd" command; to get help, run:'
	    print '    mpd -h'
            exit(-1)
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
        msgToSend = 'realusername=%s\n' % username
        mpd_send_one_line(conSocket,msgToSend)
    mpdid = ''
    if argv[1] == '-a':
        jobalias = argv[2]
        jobnum = '0'
    else:
        jobalias = ''
        jobid = argv[1]
        sjobid = jobid.split('@')
        jobnum = sjobid[0]
        if len(sjobid) > 1:
            mpdid = sjobid[1]
    mpd_send_one_msg(conSocket, {'cmd':'mpdkilljob',
                                 'jobnum' : jobnum, 'mpdid' : mpdid, 'jobalias' : jobalias,
                                 'username' : username})
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdkilljob_ack':
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


if __name__ == '__main__':
    signal(SIGINT,signal_handler)
    signal(SIGALRM,signal_handler)
    try:
	mpdkilljob()
    except mpdError, errmsg:
	print 'mpdkilljob failed: %s' % (errmsg)
