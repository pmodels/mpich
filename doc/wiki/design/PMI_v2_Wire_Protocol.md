# PMI-2 Wire Protocol

This is a very rough and early draft of a design for version 2 of the
wire protocol for the Process Management Interface used with MPICH. See
[PMI_v2_Design_Thoughts](PMI_v2_Design_Thoughts.md) for
background on the issues and requirements behind this design.

## Design Considerations

### Basic Concepts

To be written. What follows is a start.

Commands are exchanged between the PMI client and server in a simple,
key equal value format. There are a number of predefined keys that are
used consistently in the commands; additional keys may be defined as
necessary to provide new services (though the expectation is that few if
any further changes will be required to this specification).

The PMI server indicates whether an error was detected by setting the
value of the key `rc` to a non-negative integer (defined below). If the
value is positive (indicating an error), the command may optionally
include the key `errmsg`, which provides an error message string that
the client may choose to print. This allows the client side of the code
to give the user more detailed information about a problem.

### Requirements for Backward Compatibility

We provide the very minimum. The first command is in the form understood
by PMI version 1; this allows a system that implements PMI version 1 to
detect and cleanly report on the mismatch. There is no expectation that
MPICH program or process managers will support both versions 1 and 2 of
the PMI wire protocol.

### Overall Design Issues

See the design discussion mentioned above.

### Thread Safety and Implementing Spawn and Get

In version 1 of the PMI wire protocol, many operations are implemented
as request-response transactions over the wire. This requires that only
one thread access the wire during this transaction. This is acceptable
for many of the PMI operations (such as `PMI_KVS_Put`) but is not
appropriate for Spawn, since it may take a great deal of time
(particularly on a batch scheduled system) for the new processes to be
launched. This requires that spawn be a split operation, allowing other
threads to use the wire while the spawn operation is completing. There
are two possibilities: allow the spawning thread to poll for the
response or require the PMI implementation to accept the spawn response
at any time and provide a way to wake up the spawning thread.

Similarly, `PMPI_KVS_Get`, depending on the implementation of the KVS
Get operation, may be non-local (e.g., involve communications with other
processes), and thus could block other threads in the same process
unless the operation is non-blocking.

To implement support for these split operations, any thread that is
reading responses from the PMI server needs to be able to idenitfy the
operation that caused the response and update the necessary information.
The easiest way to implement this is to have each nonblocking operation
create a process-unique identifier and enqueue the operation on a
pending operation list. The id need only be unique to the process since
data comes to a particular process; in addition, since the PMI API calls
are all blocking, the operation id need only identify a particular
thread (e.g., it could even be the thread-id, it doesn't need to be a
unique value for each operation on a process). However, this does mean
that the operation id needs to be provided when any nonblocking request
is made. This is provided in the field "`thrid`". Responses to this
request include this field with the same value. `thrid` is short for
"thread id", and identifies the thread in the client that originated the
request. The client is free to use any unique value for the `thrid`;
further, the value need not be the same from operation to operation,
even from the same client thread (this allows a client to add additional
error checking or other information to the operation, for example).

## PMI Wire Protocol Version 2

### Specification Conventions

  - Boolean values are always one of the strings "`true`" or "`false`".
  - As mentioned previously, return codes (`rc`) are non-negative
    integers that may be represented by a `int32_t` integer in C (using
    a signed integer gives more flexibility in the calling code).

### The Protocol

The client establishes a connection to the server. The client may choose
to, and/or the server may require that the client, use a secure
connection, such as OpenSSL. This part of the wire protocol is not
defined. It is recommended that clients and servers either use OpenSSL
or use plain sockets, making use of the authentication extensions
defined below on the `fullinit` command.

In order to initialize the connection between the client and the server,
the client needs to know how to establish the connection. These may be
specified by the following environment variables:

    - PMI_FD - Use this FD (encoded as a character)
    - PMI_PORT - Use this host and port, encoded as `hostname:port-number`, i.e., `myhost.edu:12674`
    - PMI_SOCKTYPE - Use this to specify an alternative socket type. The predefined names are
        - POSIX - Standard POSIX (Unix) sockets
        - OpenSSL - Open SSL (Secure Sockets)
If `PMI_SOCKTYPE` is not specified, `POSIX` is assumed.

Once the socket connection is established, the client sends the
following:

```
cmd=init pmi_version=2 pmi_subversion=xx\n
```

where the subversion number is the same as the value of
`PMI_SUBVERSION`. This must be the first set of bytes on the wire, and
they must be in exactly this form. This allows systems that support
version 1 of PMI to detect that there is a version mismatch, and to
differentiate between PMI version 2 and other possible input (such as a
bogus connection on the same socket).

After this one line, PMI version 2 switches to a new encoding that
simplifies the handling of special characters. Each command is of the
form

```
[length][commandline]
```

where `length` is exactly 6 ASCII digits (blank padded on left if
necessary) giving the length and the command is exactly that many bytes
(*not* characters, since PMIv2 uses UTF-8 and there may be some
multi-byte characters). The `commandline` is of the form

```
cmd=name;key1=value1;key2=value2 ...;
```

If a semicolon needs to be part of a key or value, it needs to be
escaped by being doubled. No other characters (not even new lines) are
special. The semicolon was chosen because it is often used to terminate
commands, and hence is rarely used in the sort of values that are likely
to be communicated with PMI.  Command names are more limited and must be
letters, digits, and hyphens only.

Key names are limited to letters, digits, hyphens, and underscores. To
be precise, the regular expression for keys is

```
[-_A-Za-z0-9]{1,64}
```
Note that values are not limited to those characters, and may contain
spaces, equal signs, newlines, and even nulls *(but not semicolons)*.

The maximum length for keys and values are defined by the
[PMI v2 API](PMI_v2_API.md)
in the `pmi.h` header file. Keys are limited to the value defined for
`PMI_MAX_KEYLEN` and values are limited to the value defined for
`PMI_MAX_VALLEN`. These values are currently 64 and 1024 respectively.

Implementations must be prepared to accept values that contain the `=`
character. See notes on the character set below.

Note that the semicolon is the command *terminator*, not *separator*.
This slightly simplifies processing of commands.

There are many predefined commands. For most commands, there is a
response; in that case, the name of the command adds `-response`.
Responses normally include a return code; that is given by a key of
`rc`. A typical sequence is to send

```
cmd=commandname;option=value;
```

and receive

```
cmd=commandname-response;result=value;rc=0;
```

For any commands that occur after MPI initialization, there is an
additional `thrid` field on both the command and the response. This is
used to support thread safety, and was described in more detail above.
This field should be the first field after the command.

In some cases, a value or a command may require an excessive number of
characters. Rather than require that the PMI client and server support
arbitrarily long commands, a command may be split into multiple messages
by adding a

```
concat=id;
```

at the end and a

```
cmd=concat;concatid=id;
```

at the beginning of the continuation. The `id` value is arbitrary, but
should be sufficient to identify a stream of commands. For example,
simply using the process or thread id will usually be sufficient.

Many commands have a response form that includes an `errmsg=string`
item. If there is no error (`rc=0`), this field may be omitted.

The following commands and their arguments are defined. These are the
ones needed to implement the PMI version 2 API. The first command is the
one sent by the client to the PMI server; the second (always a
`-response` form) is sent back to the client. The key `rc` is used
consistently for a return code. A zero return code is success; non-zero
is failure and are [documented below](#pmi-wireprotocol-error-codes). Note that some
commands have an instance-specific number of values (such as info keys
or arguments to a command that is to be executed).

  - PMI_Init:

After the standard init with version string,

```
cmd=fullinit;pmijobid=string;pmirank=r;threaded=bool;authtype=name;authinfo=string;
cmd=fullinit-response;pmi-version=n;pmi-subversion=m;rank=r;size=s;appnum=a;
   spawner-jobid=string;debugged=bool;
   pmiverbose=flag;rc=code;errmsg=string;
```

The client process sends a request to begin the initialization by using
the `fullinit` command. This has three optional keys:

    - `pmijobid` - This is a string that allows the PMI server to identify processes that
      belong to the same parallel job. In the simple PMI implementation, if
      the processes are started by `mpiexec`, the environment variable
      `PMI_JOBID` is set with this string; the processes can check in using
      this string. This is only required if the server will require a way to
      identify the processes in a job; if no `PMI_JOBID` is set, then the
      `pmijobid` key is not required.

      Another use for `pmijobid` is for processes that are started outside of
      the process management system but that still need the PMI services, such
      as the information on the rank and size of the job. This may occur if,
      for example, a parallel debugger starts the processes.

    - `pmirank` - The rank in `MPI_COMM_WORLD` of this process. This is provided only if
      the process already knows this value; that may happen if, for example,
      the processes are started by some outside system, or if the environment
      variable `PMI_RANK` is set.

    - `threaded` - If true, then PMI will require thread ids in messages in order to
      provide thread safety. If this false, the wire protocol need not provide
      thread id values. This provides a modest optimization.

    - `authtype` - An authentication type; this is a string name that the client and server
      agree upon. See the example below for details. This is optional; if the
      server does not need authentication (for example, the connection is
      provided through a pre-existing file descriptor), this field is not
      required.

    - `authinfo` - Information to be used to establish authentication.

If the job was spawned, the key `spawner-jobid` is given with the job id
of the spawner. If the process was not spawned (e.g., created with
`mpiexec`), then this key is not provided in the `fullinit-response`.
This `jobid` can be used in `job-connect`.

The `authtype` and `authinfo` strings are used to allow the PMI client
and server to negotiate an authorization method. In relatively secure
environments, particularly ones with a shared secret, this can use a
challenge-response handshake (which will take place before the
`fullinit-response` command is returned). In this case, the sequence
looks something like this:

Client sends:

```
cmd=fullinit;authtype=challenge-sha256;
```

(authinfo in this case is not needed and thus not sent).

The server takes this and returns the following:

```
cmd=auth-response;authinfo=n;
```

where "n" is a random integer. Then the client forms a string by
concatenating the shared secret with n and then creating the sha-256
hash of that value. It sends to the server the command

```
cmd=auth-response-complete;authinfo=string
```

where string is the sha-256 hash. At this point, the server can decide
if it is willing to accept the client. If so, it returns the
`fullinit-response` command; otherwise it closes the connection (and may
wish to log the failed attempt).

For additional security, the random integer itself would be encrypted,
and some function of the integer would be used by the client (this is
what Kerberos does).

This is a simple example. Note that in this case, the connection itself
is not secured or encrypted. Other strategies may require additional
exchanges; this is easily accomplished by sending `cmd=auth-response`
between the client and server until one or the other indicates
completion of the handshake by sending the `auth-response-complete`
command.

  - PMI_Finalize:

```
cmd=finalize;thrid=string;
cmd=finalize-response;thrid=string;rc=code;errmsg=string;
```

  - PMI_Abort:

```
cmd=abort;isworld=flag;msg=string;
```

There is no response for this command.

  - PMI_Job_Spawn:

This is a complex command because there are many fields and the command
may take a long time to execute. A response will be sent when the
operation completes; in a multi-threaded environment, the PMI client
implementation must allow other threads to make PMI requests while
waiting for the spawn request to complete. The command `spawn` and each
`spawn-cmd` must be sent consecutively (e.g., no other thread may access
the PMI wire until the entire command is sent). However, once the
`spawn-cmd` is sent, other threads should be allowed to make PMI calls.
However, the PMI implementation must be prepared to accept a
`spawn-response` command at any time.

```
   cmd=spawn;thrid=string;ncmds=count;preputcount=n;ppkey0=name;ppval0=string;...;
   cmd=spawn-cmd;thrid=string;maxprocs=n;argc=narg;argv0=name;argv1=name;...;
       infokeycount=n;infokey0=key;infoval0=string;...;
   ... one for each command (this is spawn multiple)

   cmd=spawn-response;third=string;rc=code;errmsg=string;
       jobid=string;nerrs=count;err0=e0;err1=e1;...;
```

Jobid in the `spawn-response` command is the JobId that may be used in
`PMI_Job_Connect`. This information is may be needed by other processes
in order to use `PMI_KVS_Get`.

The `thrid` for the `spawn` and `spawn-cmd` commands must be the same
(for the same spawn command).

The info keys include the ones defined in the MPI-2 specification
(Section 5.3.4) and in addition include these

  - timeout:

A timeout limit, in milliseconds (integer)

  - credential:

A string containing credentials needed to start a job (this may include
user, charge group, and pass phrases). Note that it is essential that
this be encrypted if any sensitive information, such as passwords or
pass phrases, are included. The PMI client and server should agree on
the method for securing this information.

  - threadsPerRank:

The number of anticipated threads per MPI Process (integer). This may be
used by a resource manager in allocating processor resourses.

Note the use of `infokey`*i*`=key` and `infovalue`*i*`=value` instead of
`key=value`. This avoids both possible conflict with predefined names in
the commands and with key names that contain special characters.

**Question:** Note that this supports the `preput` operations used in
PMI version 1. We may want to change this to make that data arrive in
the `fullinit` command when the job is spawned.

  - PMI_Job_GetId:

<!-- end list -->

```
 cmd=job-getid;thrid=string;
 cmd=job-getid-response;thrid=string;jobid=string;rc=code;errmsg=string;
```

**Question**: Do we still need this, or is it unnecessary because the
init step could return the job id (though it does not yet)?

  - PMI_Job_Connect:

<!-- end list -->

```
  cmd=job-connect;thrid=string;jobid=string;
  cmd=job-connect-response;thrid=string;kvscopy=val;rc=code;errmsg=string;
```

The `kvscopy` value is yes if the PMI client needs to help the PMI
servers share KVS information. This is necessary when the PMI servers
for the jobs are different and have no way to connect. In this case, the
commands `kvsgetall` and `kvsputall` are also used (note that these do
not have PMI Client API equivalents).

  - kvsgetall:

<!-- end list -->

```
    cmd=kvs-getall;thrid=string;jobid=string;
    cmd=kvs-getall-paircount-response;thrid=string;npair=n;
    cmd=kvs-getall-pair-response;thrid=string;key=val;val=val;
    cmd=kvs-getall-response;thrid=string;rc=code;errmsg=string;
```

After the `kvs-getall` command, the server returns the number of KVS
pairs with the `kvs-getall-paircount-response` command. Following that
is one `kvs-getall-pair-response` for each key in the KVS space. The
`kvs-getall-response` is sent after all key/value pairs are returned.

  - kvsputall:

<!-- end list -->

```
    cmd=kvs-putall;thrid=string;jobid=string;npair=n;
    cmd=kvs-putall-pair;thrid=string;key=val;val=val;
    cmd=kvs-putall-response;thrid=string;rc=code;errmsg=string;
```

The client sends one `kvs-putall-pair` command for each of the keyval
pairs. This approach helps ensure that individual messages are of
reasonable length, since the number of pairs will often be at least as
large as the number of processes in `MPI_COMM_WORLD`.

  - PMI_Job_Disconnect:

<!-- end list -->

```
  cmd=job-disconnect;thrid=string;jobid=string;
  cmd=job-disconnect-response;thrid=string;rc=code;errmsg=string;
```

  - PMI_KVS_Put:

<!-- end list -->

```
   cmd=kvs-put;thrid=string;key=name;value=string;
   cmd=kvs-put-response;thrid=string;rc=code;errmsg=string;
```

This command does not need an `thrid` because it can only be used when
all processes are guaranteed to be able to issue a PMI Fence operation.
Effectively, that can only happen before `MPI_Init` returns, so there
can only be one thread processing these operations. However, a `thrid`
is included to eliminate differences in handling commands.

Note that because of the requirements on `PMI_KVS_Fence`, `PMI_KVS_Put`
cannot be used after `MPI_Init`, except possibly in collective functions
on `MPI_COMM_WORLD`.

  - PMI_KVS_Get:

<!-- end list -->

```
   cmd=kvs-get;thrid=string;jobid=string;srcid=id;key=name;
   cmd=kvs-get-response;thrid=string;flag=found;value=string;rc=code;errmsg=string;
```

The `jobid` field is optional; if it is not given, then the job id for
this process is assumed. The value of the `jobid` value must be one of
the connected jobids.

The `srcid` field is a hint indicating which process might have PUT the
corresponding key value pair. If the hint is incorrect, the server
should still return the correct value (e.g., if process 1 performed a
put of `key=foo` and `value=bar`, then later a get is performed with
`srcid=0` and `key=foo`, the server should respond with `flag=TRUE` and
`value=bar`). If a negative value is passed in `srcid`, or if the field
is omitted altogether, then the hint is ignored. This field is optional.

  - PMI_KVS_Fence:

<!-- end list -->

```
   cmd=kvs-fence;thrid=string;
   cmd=kvs-fence-response;thrid=string;rc=code;errmsg=string;
```

The `thrid` isn't necessary in practice but is included to simplify
command handling.

  - PMI_Info_GetNodeAttr:

<!-- end list -->

```
  cmd=info-getnodeattr;thrid=string;key=name;wait=bool;
  cmd=info-getnodeattr-response;thrid=string;flag=bool;value=string;rc=code;errmsg=string;
```

Note that the PMI API defines some key names; others may be added. If
the key is unknown or there is no associated value for that key, the
value of `flag` is `false`. If the `wait` key is set with the value
`true`, then the server will not respond until the value becomes
available (in that case, `flag` will always be set to found). If the job
exits before the value becomes available, the server will treat that as
any other unexpected termination of the job.

  - PMI_Info_PutNodeAttr:

```
  cmd=info-putnodeattr;thrid=string;key=name;value=string;
  cmd=info-putnodeattr-response;thrid=string;rc=code;errmsg=string;
```

  - PMI_Info_GetJobAttr:

```
  cmd=info-getjobattr;thrid=string;key=name;
  cmd=info-getjobattr-response;thrid=string;flag=found;value=string;rc=code;errmsg=string;
```

  - PMI_Nameserv_publish:

```
  cmd=name-publish;thrid=string;name=servicename;port=portname;
      infokeycount=n;infokey0=name;infoval0=string;infokey1=name;...;
  cmd=name-publish-response;thrid=string;rc=code;errmsg=string;
```

  - PMI_Nameserv_lookup:

```
  cmd=name-lookup;thrid=string;name=servicename;
    infokeycount=n;infokey0=name;infoval0=string;...;
  cmd=name-lookup-response;thrid=string;value=string;flag=found;rc=code;errmsg=string;
```

  - PMI_Nameserv_unpublish:

```
  cmd=name-unpublish;thrid=string;name=servicename;
      infokeycount=n;infokey0=name;infoval0=string;infokey1=name;...;
  cmd=name-unpublish-response;thrid=string;rc=code;errmsg=string;
```

### Out-of-Band Messaging

Together with synchronous communication between the MPI process and the
PMI server, there can optionally be a separate connection for
out-of-band messaging.

The PMI server expects at least one connect and a follow-on message
which says "cmd=init pmi_version=..." from the client. This is the
"regular" connection.

The client can optionally open a second connection and a follow-on
message which says "cmd=init_oob signal=SIGSTOP pmi_version=...". This
will be the "out-of-band" connection. The signal can be one of the
signals supported on that platform or NONE. If the signal requested is
NONE, the server will not signal the MPI process when a message is sent.
Note that if the application uses the same signal for its own
processing, then it can be a problem. But that part is out-of-scope for
this proposal; the MPI process can figure out what signal to use based
on some coordination with the application (e.g., environment variable).

All out-of-band communication happens on the out-of-band socket. Each
OoB message is initiated by the server and sent to the MPI process and
is of the form "cmd=checkpoint ..." or "cmd=abort ...".

The MPI process can either request for a signal during the
initialization (in which case the PMI server has to send the message and
follow it up with a signal) or request for no signal (in which case the
MPI process has to continuously monitor this socket using either a SIGIO
or a separate thread blocking on the socket).

### Character Set

If PMI was used solely for commands, any simple character set, such as
ASCII, would be fine. However, some commands may return error messages
and others, such as the name publishing routines, may need
user-specified strings. To avoid problems with internationalization, PMI
v2 uses [UTF-8](http://en.wikipedia.org/wiki/UTF-8), which provides
backward compatibility to ASCII. In particular, PMIv1, which used ASCII,
used an UTF-8 subset. One feature of UTF-8 is that bytes that represent
ASCII characters are unique - all other characters have at least the
high bit set. This means that a character, such as the semicolon that
PMIv2 uses as the terminator, can be found without worrying about
whether the same byte is part of a longer UTF-8 multi-byte character -
that can never happen. In particular, the PMIv2 code can simply copy
message strings that use multibyte UTF-8 without needing to process
them. More information on using UTF-8 may be found at
<http://www.cl.cam.ac.uk/~mgk25/unicode.html>.

### PMI Wireprotocol Error Codes

This is a non-exhaustive list of error codes from the PMI server. These
cannot be MPI error codes because the PMI server is independent of any
particular instance of MPI (even of MPICH). Error codes are
non-negative.

  - PMI_SUCCESS:

Also 0, means no error

  - PMI_ERR_COMMFAILED:

Communication failure with PMI server

  - PMI_ERR_UNKNOWNCMD:

Unrecognized command

## Discussion Items

An easy way to provide security for the PMI traffic is to use OpenSSL.
Because this changes the way in which the socket is connected, we've
added a `PMI_SOCKTYPE` environment variable. Note that a PMI
implementation is free to require SSL or some other secure communication
mechanism.

The length-message format use here is easy on the reader but can be
awkward for the writer, particularly for the commands that may have
large number of key/value pairs (such as spawn with many command-line
arguments and info values). For lines that exceed this maximum length,
there is a special operation to concatenate lines. PMI implementations
may choose to bound the total size of a command (e.g., to be limited by
the maximum command line in the typical shell, which is often around
64k).

Here are some thoughts for how to implement processing of the PMI wire
protocol in the multithreaded case, particularly on the client side
(where multiple threads may make blocking PMI calls).

Sending (from the client) is easy. We may assume that the server is
working as fast as possible to process requests, so any PMI call can
enter a PMI-write critical section, perform the write, then exit the
critical section. If the write blocks, that's ok, as the server will
soon unblock it, and no other PMI call that needs to write would be able
to make progress.

Receiving is more difficult. Define a routine, `PMIR_Progress( int
mythrid )`, that reads from the PMI fd and processes each message. If
the message is for the specified `mythrid`, then exit after that message
is read. Otherwise, read the message and enqueue it by `thrid`; signal
(using a condition variable) the relevant thread. Note that because all
of the PMI calls are blocking, at most one message per thread will be
pending. Thus, copying out and enqueuing the message for each thread is
a small burden and simplifies the implementation.

If a routine enters `PMIR_Progress` and some routine is already using
`PMIR_Progress`, then enter a condition wait.
