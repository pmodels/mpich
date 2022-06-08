# Nemesis Dynamic Processes And Connections

This page describes how to implement features required for a network
module to support dynamic processes (e.g., `MPI_Comm_spawn()`) and
dynamic connections (e.g., `MPI_Comm_connect()` and `MPI_Comm_accept()`.
We start by giving an overview of the steps needed to spawn a process,
then we describe what needs to be implemented by the network module to
support this.

## Overview

At a very high level, when spawn is called, a set of processes are
created, then these processes connect back to the processes in the
communicator from which the spawn call was made.

### Spawning Processes

The table below summarizes the major steps taken in
`MPIDI_Comm_spawn_multiple()` (for the spawner processes) and
`MPID_Init()` (for the spawned processes). In the table, the column
labeled <b>Root spawner</b> lists the actions taken by the process that
is specified as the root in the `MPI_Comm_spawn()` call. The column
labeled <b>Other spawners</b> lists the actions taken by the other
processes in the communicator from which `MPI_Comm_spawn()` was called.
The column labeled <b>Spawnees</b> lists the actions taken by the
spawned processes.

|   | `MPIDI_Comm_spawn_multiple()`                                                                                                                   | `MPID_Init()`                                                                                                                                                 |
| - | ----------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|   | Root spawner                                                                                                                                    | Other spawners                                                                                                                                                |
| 1 | Call `MPID_Open_port()` to open a port and get the port_name                                                                                   |                                                                                                                                                               |
| 2 | Call `PMI_Spawn_multiple()` to create the processes. The port name is included in this call to be pre-loaded in the spawnee's KVS space.        |                                                                                                                                                               |
| 3 | Broadcast flag from root spawner indicating whether spawn succeeded.                                                                            |                                                                                                                                                               |
| 4 | Broadcast total number of processes created                                                                                                     |                                                                                                                                                               |
| 5 | Broadcast return code of the spawn call                                                                                                         |                                                                                                                                                               |
| 6 | If spawn succeeded, call `MPID_Comm_accept(port_name)` to wait for connection request from spawnees. This function returns an intercommunicator. | Call `MPID_Comm_connect(parent_port_name)` with the port name of the port opened by the parent (Spawner process). This function returns an intercommunicator. |
| 7 | Call `MPID_Close_port()`                                                                                                                        |                                                                                                                                                               |

 

### Dynamic Connections

As seen above, to connect a two intracommunicators, *A* and *B*, a
process in communicator *A* opens a port, then all of the processes in
the communicator call the accept function, which will block until the
connection is made. The processes in communicator *B* call the connect
function which also blocks until the connection is made. The process
that opened the port now closes it.

A *port name* is returned when a port is opened. This is then passed in
to the connect call by the remote communicator in order to specify which
port is being opened. During the connect/accept process, the processes
from both communicators exchange business cards of all the processes in
each process group. Now any process in either communicator has the
information to communicate with any processes in the other communicator.

In this section we describe the four dynamic connection functions:
`MPID_Open_port`, `MPID_Close_port`, `MPID_Comm_accept` and
`MPID_Comm_connect`. These functions are overridable by implementing
`MPIDI_CH3_PortFnsInit()` and returning a pointer to a `MPIDI_PortFns`
struct. By default, the `MPID_*` functions are implemented by
corresponding `MPIDI_*` functions (e.g., `MPID_Open_port` is implemented
by `MPIDI_Open_port`).

  - `MPID_Open_port()` : returns a <i>port name</i> which consists of a
    unique <i>port number</i> and the business card of the calling
    process. When the port name is passed to another process the
    business card portion of the port name describes how it should
    connect to that process over the network. The port number allows a
    process to determine to which `MPID_Open_port()` call an incoming
    connection request corresponds.

<!-- end list -->

  - `MPID_Close_port()` : frees the port number associated with the port
    name.

The table below shows the steps involved in the connect/accept process.

<table>
<thead>
<tr class="header">
<th><p> </p></th>
<th><p><code>MPID_Comm_accept()</code></p></th>
<th><p><code>MPID_Comm_connect()</code></p></th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><p> </p></td>
<td><p>Other spawners</p></td>
<td><p>Root spawner</p></td>
</tr>
<tr class="even">
<td><p>1</p></td>
<td><p> </p></td>
<td><p><i>When a new temporary connection is established the process calls <code>MPIDI_CH3I_Acceptq_enqueue()</code> with the port number to indicate that a connection request has been received.</i><br />
Calls <code>MPIDI_CH3I_Acceptq_dequeue()</code> to check whether the corresponding connect request has been received, otherwise waits. This returns a temporary VC corresponding to that temporary connection.</p></td>
</tr>
<tr class="odd">
<td><p>2</p></td>
<td><p> </p></td>
<td><p>Calls <code>MPIDI_CH3I_Initialize_tmp_comm()</code> which creates a temporary intrercommunicator for the temporary VC</p></td>
</tr>
<tr class="even">
<td><p>3</p></td>
<td><p> </p></td>
<td><p>Call <code>ExtractLocalPGInfo()</code> to create a list of process groups of the communicator passed to <code>MPI_Comm_accept()</code> / <code>MPI_Comm_connect()</code>. Then exchange info about how many PGs were found with the other root.</p></td>
</tr>
<tr class="odd">
<td><p>4</p></td>
<td><p>Broadcast this info to the other spawner processes.</p></td>
<td><p>Broadcast this info to the other spawnee processes.</p></td>
</tr>
<tr class="even">
<td><p>5</p></td>
<td><p> </p></td>
<td><p>Exchange PG strings with the other root.</p></td>
</tr>
<tr class="odd">
<td><p>6</p></td>
<td><p>Broadcast PG strings to other spawner processes.</p></td>
<td><p>Broadcast PG strings to other spawnee processes.</p></td>
</tr>
<tr class="even">
<td><p>7</p></td>
<td><p> </p></td>
<td><p>Exchange rank &lt;--&gt; (PG id, PG rank) translations for the communicator with the other root.</p></td>
</tr>
<tr class="odd">
<td><p>8</p></td>
<td><p>Broadcast translations to other spawner processes.</p></td>
<td><p>Broadcast translations to other spawnee processes.</p></td>
</tr>
<tr class="even">
<td><p>9</p></td>
<td><p>Set up new intercommunicator.</p></td>
<td></td>
</tr>
<tr class="odd">
<td><p>10</p></td>
<td><p> </p></td>
<td><p>Synchronize with other root</p></td>
</tr>
<tr class="even">
<td><p>11</p></td>
<td><p>Barrier</p></td>
<td><p>Barrier</p></td>
</tr>
<tr class="odd">
<td><p>12</p></td>
<td><p> </p></td>
<td><p>Free temporary communicator and VC.</p></td>
</tr>
</tbody>
</table>

 

## Supporting Dynamic Processes and Connections in a Network Module

In order to support dynamic connections, the netmod needs to be able to
connect one process to another identified by a business card and
communicate the port number to the second process. There are two pieces
that need to be implemented: (1) the `connect_to_root()` function, which
initiates the connection, and, (2) the receiving end of the connection
which will create a VC and call `MPIDI_CH3I_Acceptq_enqueue()`.

<b> `int connect_to_root(const char *business_card, MPIDI_VC_t
*new_vc);` </b>

`business_card` - This is a string containing the port name of the
port which is to be connected to. The port name consists of the business
card of the process which opened the port and the port number. The
business card info can be extracted in the usual way. The port number
can be extracted using the `MPIDI_GetTagFromPort()` routine.

`new_vc` - A pointer to the VC which has been created for this
connection. Note that since this is a temporary VC, fields like pg and
pg_rank are not valid.

This function will initiate a connection to the process described by the
business card. During this process, the port number is communicated. The
tcp netmod implementation extracts the IP address, TCP port of the
remote process, along with the port number, and sets the appropriate
fields in the VC. It then calls `MPID_nem_tcp_connect()` which initiates
the connection protocol to the remote process. The connection protocol
(in `socksm.c`) detects that this is a temporary VC, because the pg
field is set to NULL, and sends the port number as part of the
handshake. An alternative implementation could send the port number as
the very first message after the connection has been established.
