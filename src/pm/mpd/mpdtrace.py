#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdtrace [-l]
Lists the (short) hostname of each of the mpds in the ring
The -l (long) option shows full hostnames and listening ports and ifhn
"""

from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.18 $"
__credits__ = ""

import sys, os, signal

from  re      import  sub
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB

def mpdtrace():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(sys.argv) > 1:
        if (sys.argv[1] == '-h' or sys.argv[1] == '--help') or (sys.argv[1] != '-l'):
            usage()
    signal.signal(signal.SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdtrace')

    parmdb = MPDParmDB(orderedSources=['cmdline','xml','env','rcfile','thispgm'])
    parmsToOverride = {
                        'MPD_USE_ROOT_MPD'            :  0,
                        'MPD_SECRETWORD'              :  '',
                      }
    for (k,v) in parmsToOverride.items():
        parmdb[('thispgm',k)] = v
    parmdb.get_parms_from_env(parmsToOverride)
    parmdb.get_parms_from_rcfile(parmsToOverride)
    if (hasattr(os,'getuid')  and  os.getuid() == 0)  or  parmdb['MPD_USE_ROOT_MPD']:
        fullDirName = os.path.abspath(os.path.split(sys.argv[0])[0])  # normalize
        mpdroot = os.path.join(fullDirName,'mpdroot')
        conSock = MPDConClientSock(mpdroot=mpdroot,secretword=parmdb['MPD_SECRETWORD'])
    else:
        conSock = MPDConClientSock(secretword=parmdb['MPD_SECRETWORD'])

    msgToSend = { 'cmd' : 'mpdtrace' }
    conSock.send_dict_msg(msgToSend)
    # Main Loop
    done = 0
    while not done:
        msg = conSock.recv_dict_msg(timeout=5.0)
        if not msg:    # also get this on ^C
            mpd_print(1, 'got eof on console')
            sys.exit(-1)
        elif not msg.has_key('cmd'):
            print 'mpdtrace: unexpected msg from mpd=:%s:' % (msg)
            sys.exit(-1)
        if msg['cmd'] == 'mpdtrace_info':
            if len(sys.argv) > 1 and sys.argv[1] == '-l':
                print '%s (%s)' % (msg['id'],msg['ifhn'])
            else:
                pos = msg['id'].find('.')
                if pos < 0:
                    pos = msg['id'].rfind('_')
                print msg['id'][:pos]    # strip off domain and port
        elif msg['cmd'] == 'mpdtrace_trailer':
            done = 1
    conSock.close()

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    sys.exit(-1)

def usage():
    print __doc__
    sys.exit(-1)

if __name__ == '__main__':
    mpdtrace()
