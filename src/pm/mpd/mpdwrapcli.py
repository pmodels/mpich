#!/usr/bin/env python
#
#   (C) 2001 by Argonne National Laboratory.
#       See COPYRIGHT in top-level directory.
#

import sys, os, socket

manPort = int(sys.argv[1])
manSock = socket.socket()
manSock.connect((socket.gethostname(),manPort))
msg = manSock.recv(3)    # 'go\n'
if not msg.startswith('go'):
    print 'mpscliwrap: invalid go msg from man'
    sys.exit(-1)

try:     max_fds = os.sysconf('SC_OPEN_MAX')
except:  max_fds = 1024
for fd in range(3,max_fds):
    try:     os.close(fd)
    except:  pass
# can NOT use syslog (including inside an mpd_print) below here without
# doing a new openlog because any syslog fd would have been closed

os.execvpe(sys.argv[2],sys.argv[3:],os.environ)    # client
