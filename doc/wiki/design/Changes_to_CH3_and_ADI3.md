# Changes to CH3 and ADI3

Note this document is somewhat out of date. A lot has changed in CH3 and
ADI3 since these were written. It's been said that no documentation is
better than bad documentation, so read the above at your own risk.


This page describes some proposed changes to the CH3 and ADI3
interfaces. These changes are motivated by performance and software
engineering analysis of the current code.

## CH3 Changes

Missing from the current CH3 implementation are optimizations for short
messages and short RMA (e.g., single word put/get) operations. In
addition, the progress engine is very complex, extremely hard to modify,
and the different channels include large amounts of duplicated code.
There are a number of other problems with the software engineering of
the code and with the code's performance. The following is a list of
possible changes; additions can be made to this list as necessary.

**Separate out contiguous communication**
There are a number of tests that are repeated (starting in fact at the
ADI3 level) that might be eliminated if the important special case of
contiguous buffers is handled separately.

**Allow arrays of items in progress**
This is particularly important for Waitall when the implementation is
optimizing for on-NIC handling of pending requests (needed for Quadrics,
possibly Cray XT3).

**Use function pointers in progress engine**
Rather than a huge, hard to maintain file with a enormous switch
statement. This change has already been made for RMA operations; that
also makes it easier to select different approaches to progress without
using \#ifdefs.

(Note that this has mostly been done.)

**Place logically related code in the same file**
At one point, the CH3 implementation used a separate file for nearly
every routine, leading to extraneous global variables and public
typedefs and structures that where only needed in a few routines. Some
of the code has be updated to move related routines and the datatypes
needed only by the internals of those routines into single files but
more needs to be done.

**Eliminate the need for the KVS cache**
When dynamic processes are included, the KVS name space provided by the
process manager does not provide the functionality needed by MPICH.
Specifically, once two process groups are connected, the processes
within them need access to the KVS name spaces of the other process
group. There is currently no way in PMI to accomplish this, so the
work-around has been to have one process in each (PMI) process group
extract the entire contents of the KVS space and then communicate that
to MPI processes in the other process group. This information is stored
in a "KVS cache" within each MPI process; every MPICH routine that needs
to access data in the KVS space must use these special routines instead
of the PMI routines.

Note that PMI v2 addresses this in part.

**Simplify error handling and reporting by using one set of error
codes**
The current code base contains implementations of various routines that
use their own error code conventions, as if those routines would be used
in some other package. This introduces an unnecessary set of error code
translations that must be duplicated at many places in the code. In
fact, this creates a maintenance hazard since it is difficult to ensure
that this translation is handled uniformly and accurately. The PMI
routines and some of the string utility routines should be changed to
return MPI error codes; there are probably additional routines that
should be updated.

**Make greater use of abstractions and routines**
There are many places in the current CH3 code where the same
functionality is provided by what is essentially copying of the code.
The problem with this is two-fold: first, bug fixes don't get propagated
to each place where the code is copied. Second, the code is much harder
to read and understand because the operations are not encapsulated into
a single method, forcing the reader of the code to repeatedly check the
code to make sure that it does all that it should (e.g., in 20 lines of
code, was something forgotten)?

**Include the total message length in the "v" versions of the CH3
routines**. In the cases where it makes sense to use the "v" versions of
the routines (possibly not for packet + contiguous data, which perhaps
should have its own routine(s)), pass in the total message length. This
allows the "send" routines to quickly check that they've sent all data,
rather than running through the iov to check that all of the data has
been sent. As the routine that forms the iov can easily create this
value, this will save time and simplify the common code path.

**Unified Posted and Unexpected Queue**. This is a more radical idea,
and may not be worth-while. However, to unify the queue operations,
consider a list of requests with three special pointers: head of posted,
head of unexpected, and middle. This list is organized as:  Then to post
an element, start from the unexpected head. If the entry is found,
remove it. Else, if you reach the middle, add the element immediately
after the middle. This puts it into the posted part of the queue.

To post and incoming message, start at the posted head. If the entry is
found, remove it. Else, if you reach the middle, add the element
immediately after the middle. This puts it into the unexpected part of
the queue.

The advantage of this approach is that lock-free operations can be used
for the queue updates, as long as insert and delete can be accomplished
atomically. The heads and the middle elements need to be permanently
allocated dummy elements so that no separate operations to modify those
pointers needs to be made atomically.

## ADI3 Change

**Separate out contiguous communication**
After error checks (including testing for contiguous datatypes), call
either a contiguous or general communication routine. E.g., something
like `MPID_SendContig` vs. `MPID_SendNoncontig`.

**Make use of the original design**
The original design included hooks for memory registration and
persistent requests. Use them or, if they aren't quite right, modify
them so that the fit the needs of MPI.
