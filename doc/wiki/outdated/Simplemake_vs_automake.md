Here are some of the reasons we are using comparing whether we should
use simplemake or automake for MPICH2. It is currently using simplemake.

**Flexibility:** we can modify simplemake to add things like Pretty
Make. We don't have to depend on automake developers to fix the problems
for us.

**Libtool:** automake seems to rely on libtool which is the main problem
for me. Correct me if I am wrong. It seems that using automake can be
equated to our willingness to using libtool in create static and shared
libraries. I could be biased (i.e. less experienced in libtool), I feel
like supporting libtool is more difficult than supporting autoconf.

If we think the original Makefile output from simplemake is horrible
(before Pretty Make), adding libtool in the equations is 10 times worse.
Also, the build will likely take much longer.

**NFS timestamps:** I think the issue has to do with Makefile rules and
timestamp resolution of the file than the simplemake vs automake.