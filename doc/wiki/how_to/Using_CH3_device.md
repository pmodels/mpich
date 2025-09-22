# Using the CH3 Device

The CH3 device is a legacy communication device in MPICH. For new installations, we recommend using the CH4 device (which is the default). This document provides information about CH3 for users who need to use it for compatibility or specific requirements.

## CH3 Device Overview

The ch3 device contains different internal communication options called "channels". We currently support nemesis (default) and sock channels.

## Nemesis Channel

Nemesis provides communication using different networks (tcp, mx) as well as various shared-memory optimizations. To configure MPICH with nemesis, you can use the following configure option:

```bash
--with-device=ch3:nemesis
```

### Shared Memory Configuration

Shared-memory optimizations are enabled by default to improve performance for multi-processor/multi-core platforms. They can be disabled (at the cost of performance) either by setting the environment variable MPICH_NO_LOCAL to 1, or using the following configure option:

```bash
--enable-nemesis-dbg-nolocal
```

The `--with-shared-memory=` configure option allows you to choose how Nemesis allocates shared memory. The options are "auto", "sysv", and "mmap". Using "sysv" will allocate shared memory using the System V shmget(), shmat(), etc. functions. Using "mmap" will allocate shared memory by creating a file (in /dev/shm if it exists, otherwise /tmp), then mmap() the file. The default is "auto". Note that System V shared memory has limits on the size of shared memory segments so using this for Nemesis may limit the number of processes that can be started on a single node.

### OFI Network Module (CH3)

The ofi netmod provides support for the OFI network programming interface. To enable, configure with the following option:

```bash
--with-device=ch3:nemesis:ofi
```

If the OFI include files and libraries are not in the normal search paths, you can specify them with the following options:

```bash
--with-ofi-include= and --with-ofi-lib=
```

... or the if lib/ and include/ are in the same directory, you can use the following option:

```bash
--with-ofi=
```

If the OFI libraries are shared libraries, they need to be in the shared library search path. This can be done by adding the path to /etc/ld.so.conf, or by setting the LD_LIBRARY_PATH variable in your environment. It's also possible to set the shared library search path in the binary. If you're using gcc, you can do this by adding

```bash
LD_LIBRARY_PATH=/path/to/lib
```

(and)

```bash
LDFLAGS="-Wl,-rpath -Wl,/path/to/lib"
```

... as arguments to configure.

## Sock Channel

sock is the traditional TCP sockets based communication channel. It uses TCP/IP sockets for all communication including intra-node communication. So, though the performance of this channel is worse than that of nemesis, it should work on almost every platform. This channel can be configured using the following option:

```bash
--with-device=ch3:sock
```

## Migration Notes

If you are currently using CH3 and considering migrating to CH4 (recommended):

- CH4 is the default device and provides better performance and features
- CH4 supports both OFI and UCX network modules
- Most CH3 configurations have equivalent CH4 options
- Refer to the main README.md for CH4 configuration examples