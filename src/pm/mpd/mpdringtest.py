#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

"""
usage: mpdringtest [number of loops]
Times a single message going around the ring of mpds [num] times (default once)
"""
from time import ctime
__author__ = "Ralph Butler and Rusty Lusk"
__date__ = ctime()
__version__ = "$Revision: 1.14 $"
__credits__ = ""

import sys, os, signal

from  time    import  time
from  mpdlib  import  mpd_set_my_id, mpd_uncaught_except_tb, mpd_print, \
                      mpd_handle_signal, mpd_get_my_username, MPDConClientSock, MPDParmDB

def mpdringtest():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(sys.argv) > 1  and  ( sys.argv[1] == '-h'  or  sys.argv[1] == '--help' ) :
        usage()
    if len(sys.argv) < 2: 
	numLoops = 1
    else:
	numLoops = int(sys.argv[1])
    signal.signal(signal.SIGINT, sig_handler)
    mpd_set_my_id(myid='mpdringtest')

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

    msgToSend = { 'cmd' : 'mpdringtest', 'numloops' : numLoops }
    conSock.send_dict_msg(msgToSend)
    starttime = time()
    msg = conSock.recv_dict_msg()
    etime = time() - starttime
    if not msg:
        print 'mpdringtest terminated early'
    elif msg['cmd'] != 'mpdringtest_done':
        if msg['cmd'] == 'already_have_a_console':
            print 'mpd already has a console (e.g. for long ringtest); try later'
        else:
            print 'unexpected message from mpd: %s' % (msg)
    else:
	print 'time for %d loops =' % numLoops, etime, 'seconds' 

def sig_handler(signum,frame):
    mpd_handle_signal(signum,frame)  # not nec since I exit next
    sys.exit(-1)

def usage():
    print __doc__
    sys.exit(-1)

if __name__ == '__main__':
    mpdringtest()
