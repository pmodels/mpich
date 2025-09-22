# Fault Tolerance in MPICH

MPICH provides experimental support for fault tolerance through process failure tolerance and checkpointing capabilities. These features are designed for advanced users who need resilient MPI applications.

## Overview

MPICH supports two main fault tolerance mechanisms:

1. **Tolerance to Process Failures** - Allows applications to continue running despite failed processes
2. **Checkpoint and Restart** - Enables saving and restoring application state using BLCR

**Important Note**: The fault tolerance features described here should be considered experimental. They have not been fully tested, and the behavior may change in future releases.

## Tolerance to Process Failures

### Error Handling

**ERROR RETURNS**: Communication failures in MPICH are not fatal errors. If the user sets the error handler to `MPI_ERRORS_RETURN`, MPICH will return an appropriate error code in the event of a communication failure. When a process detects a failure when communicating with another process, it will consider the other process as having failed and will no longer attempt to communicate with that process. The user can, however, continue making communication calls to other processes. Any outstanding send or receive operations to a failed process, or wildcard receives (i.e., with `MPI_ANY_SOURCE`) posted to communicators with a failed process, will be immediately completed with an appropriate error code.

### Collective Operations

**COLLECTIVES**: For collective operations performed on communicators with a failed process, the collective would return an error on some, but not necessarily all processes. A collective call returning `MPI_SUCCESS` on a given process means that the part of the collective performed by that process has been successful.

### Process Manager Integration

**PROCESS MANAGER**: If used with the Hydra process manager, Hydra will detect failed processes and notify the MPICH library. Users can query the list of failed processes using `MPIX_Comm_group_failed()`. This function returns a group consisting of the failed processes in the communicator. The function `MPIX_Comm_remote_group_failed()` is provided for querying failed processes in the remote processes of an intercommunicator.

Note that Hydra by default will abort the entire application when any process terminates before calling `MPI_Finalize`. In order to allow an application to continue running despite failed processes, you will need to pass the `-disable-auto-cleanup` option to mpiexec.

### Failure Notification

**FAILURE NOTIFICATION**: THIS IS AN UNSUPPORTED FEATURE AND WILL ALMOST CERTAINLY CHANGE IN THE FUTURE!

In the current release, Hydra notifies the MPICH library of failed processes by sending a SIGUSR1 signal. The application can catch this signal to be notified of failed processes. If the application replaces the library's signal handler with its own, the application must be sure to call the library's handler from its own handler. Note that you cannot call any MPI function from inside a signal handler.

## Checkpoint and Restart

MPICH supports checkpointing and restart fault-tolerance using BLCR (Berkeley Lab Checkpoint/Restart).

### Configuration

First, you need to have BLCR version 0.8.2 or later installed on your machine. If it's installed in the default system location, you don't need to do anything.

If BLCR is not installed in the default system location, you'll need to tell MPICH's configure where to find it. You might also need to set the LD_LIBRARY_PATH environment variable so that BLCR's shared libraries can be found. In this case add the following options to your configure command:

```bash
--with-blcr=<BLCR_INSTALL_DIR>
LD_LIBRARY_PATH=<BLCR_INSTALL_DIR>/lib
```

where `<BLCR_INSTALL_DIR>` is the directory where BLCR has been installed (whatever was specified in --prefix when BLCR was configured).

After it's configured compile as usual (e.g., make; make install).

**Note**: Checkpointing is only supported with the Hydra process manager.

### Verifying Checkpointing Support

Make sure MPICH is correctly configured with BLCR. You can do this using:

```bash
mpiexec -info
```

This should display 'BLCR' under 'Checkpointing libraries available'.

### Checkpointing the Application

There are two ways to cause the application to checkpoint:

#### Automatic Periodic Checkpointing

You can ask mpiexec to periodically checkpoint the application using the mpiexec option `-ckpoint-interval` (seconds):

```bash
mpiexec -ckpointlib blcr -ckpoint-prefix /tmp/app.ckpoint \
    -ckpoint-interval 3600 -f hosts -n 4 ./app
```

#### Manual Checkpointing

Alternatively, you can also manually force checkpointing by sending a SIGUSR1 signal to mpiexec.

#### Environment Variables

The checkpoint/restart parameters can also be controlled with the environment variables:

- `HYDRA_CKPOINTLIB`
- `HYDRA_CKPOINT_PREFIX`
- `HYDRA_CKPOINT_INTERVAL`

### Restarting from Checkpoint

To restart a process:

```bash
mpiexec -ckpointlib blcr -ckpoint-prefix /tmp/app.ckpoint -f hosts -n 4 -ckpoint-num <N>
```

where `<N>` is the checkpoint number you want to restart from.

## Additional Resources

- [MPICH Checkpointing Wiki](https://github.com/pmodels/mpich/blob/main/doc/wiki/design/Checkpointing.md)
- BLCR documentation for detailed checkpoint/restart information
- Hydra process manager documentation for process failure handling

## Limitations and Considerations

- Fault tolerance features are experimental and may change in future releases
- Checkpointing requires BLCR library and Hydra process manager
- Process failure tolerance requires specific error handling in application code
- Performance impact should be considered when enabling fault tolerance features
- Not all MPI operations may be supported in fault-tolerant mode

## Examples

### Basic Fault Tolerant Application

```c
#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    // Set error handler to return errors instead of aborting
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Your fault-tolerant application logic here

    MPI_Finalize();
    return 0;
}
```

### Running with Fault Tolerance

```bash
# Run with auto-cleanup disabled to allow continuation after failures
mpiexec -disable-auto-cleanup -n 4 ./fault_tolerant_app

# Run with checkpointing enabled
mpiexec -ckpointlib blcr -ckpoint-prefix /tmp/app -ckpoint-interval 1800 -n 4 ./app
```