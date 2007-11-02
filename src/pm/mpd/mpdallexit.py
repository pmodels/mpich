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
__version__ = "$Revision: 1.19 $"
__credits__ = ""


import sys, os

from mpdlib import mpd_set_my_id, mpd_get_my_username, mpd_uncaught_except_tb, \
                   mpd_print, MPDConClientSock, MPDParmDB


def mpdallexit():
    import sys    # to get access to excepthook in next line
    sys.excepthook = mpd_uncaught_except_tb
    if len(sys.argv) > 1  and  (sys.argv[1] == '-h'  or  sys.argv[1] == '--help') :
        print __doc__
        sys.exit(-1)
    mpd_set_my_id(myid='mpdallexit')

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

    msgToSend = { 'cmd' : 'mpdallexit' }
    conSock.send_dict_msg(msgToSend)
    msg = conSock.recv_dict_msg(timeout=8.0)
    if not msg:
        mpd_print(1,'no msg recvd from mpd before timeout')
    elif msg['cmd'] != 'mpdallexit_ack':
        mpd_print(1,'unexpected msg from mpd :%s:' % (msg) )
        sys.exit(-1)
    conSock.close()

if __name__ == '__main__':
    mpdallexit()
