#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdallexit (no args)
causes all mpds in the ring to exit
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.1 $"
__credits__ = ""


from os     import environ, close
from sys    import argv, exit
from socket import socket, AF_UNIX, SOCK_STREAM, fromfd
from signal import signal, alarm, SIG_DFL, SIGINT, SIGTSTP, SIGCONT, SIGALRM
from mpdlib import mpd_set_my_id, mpd_send_one_msg, mpd_recv_one_msg, \
                   mpd_get_my_username, mpdError, mpd_raise, mpd_send_one_line

def mpdallexit():
    mpd_set_my_id('mpdallexit_')
    if len(argv) > 1  and  ( argv[1] == '-h'  or  argv[1] == '--help') :
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
        conSocket = socket(AF_UNIX, SOCK_STREAM)             # note: UNIX socket
        try:
            conSocket.connect(consoleName)
        except Exception, errmsg:
            print 'mpdallexit: cannot connect to local mpd (%s); possible causes:' % consoleName
            print '    1. no mpd running on this host'
            print '    2. mpd is running but was started without a "console" (-n option)'
	    print 'you can start an mpd with the "mpd" command; to get help, run:'
	    print '    mpd -h'
            exit(-1)
            # mpd_raise('cannot connect to local mpd; errmsg: %s' % (str(errmsg)) )
        msgToSend = 'realusername=%s\n' % username
        mpd_send_one_line(conSocket,msgToSend)
    mpd_send_one_msg(conSocket, {'cmd':'mpdallexit'})
    msg = recv_one_msg_with_timeout(conSocket,5)
    if not msg:
        mpd_raise('no msg recvd from mpd before timeout')
    if msg['cmd'] != 'mpdallexit_ack':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
            exit(-1)
        elif msg['cmd'] == 'invalid_username_to_make_this_request':
            print 'you can not stop this mpd; it must have been started by root'
            exit(-1)
        else:
            print 'mpdallexit failed: unexpected message from mpd: %s' % (msg)
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
        mpdallexit()
    except mpdError, errmsg:
        print 'mpdallexit failed: %s' % (errmsg)
