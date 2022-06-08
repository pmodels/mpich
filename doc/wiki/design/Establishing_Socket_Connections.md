# Establishing Socket Connections
****

[Sock conn protocol](Sock_conn_protocol.md) is a related
document, although it was created independently from this document.

## Socket Connections

Connections in MPICH are established as necessary, providing better
scalability and reducing startup time. In addition, this approach
reduces the consumption of Unix file descriptors; there are a limited
number of these (as few as 1024 in some systems), and while this may
seem like a lot, clusters with more nodes than this are becoming common.

The connection process is best described with a state diagram and events
that cause transitions between states. When the MPI dynamic process
features are included, a connection my be made, closed, and reopened,
possibly many times during a computation.

Basic events that change state

1.  Try to connect to remote side
2.  Receive connection request
3.  Receive close request
4.  Receive EOF (unexpected close)

Basic States

1.  Unconnected
2.  Connected

ToDo: expand the states to include all transitions (including failure
and connection requests received when the connection is not in the
unconnected state (e.g., in closing and reopening).

Also ToDo: describe related information (e.g., PMI process group info).

Likely additional states include

1.  wait for connect info
2.  connect handshake
3.  wait for close handshake

A common problem is the one of two processes each opening connections to
each other. The socket code assume that the sockets are bidirectional,
thus only one socket is needed by each pair of connected processes, not
one socket for each member of the pair.

ToDo: refactor the states and state machine into a clear set of VC
connection states and connection states.

There are three related objects used during a connection event. They are
the connection itself (a structure specific to the communication method,
sockets in the case of this note), the virtual connection, and the
process group to which the virtual connection belongs. Note that the
reference counts on the VC and the process group are independent of
whether there is a connection; the reference counts on the VC indicate
how many *communicators* refer to that VC; the reference count on the
process group indicates how many VCs are part of some communicator.
Thus, none of the connection operations (whether open or close) change
the reference count on either a VC or a process group.

If a socket connection between two processes is established, there are
always two sides: The connecting side and the accepting side. The
connecting side sends an active message to the accepting side. This
sides first accepts the connection. However, if both processes try to
connect to each other (head-to-head situation), the processes n have
both, a connecting and an accepting connection. In this situation, one
of the connections is refused/discarded - while the other connection is
established. This is decided on the accepting side. If the connection is
accepted, i

The following describes the process of establishing a connection, based
on analysing the code (note that this should have been documented first
rather than created in an ad hoc fashion). Therefore, it has no claim
for completeness and infallibility

Note that the VC must already exist (all VCs are created with the
associated process group). Simplified State Diagram for the connect
side, if a connection is established. If a connection is closed (ack =
FALSE) the connection was refused and is the looser in a head-to-head
situation. More detailed explanation below.

### State Machines For Establishing A Connection

The graphs show the simplified diagram of the state machines, if a
connection is established. The red states mark the states an VC is
associated to the connection. More detailed explanation below 
<!---
<graphviz>
digraph connecting_side_state_machine {

`   node [shape = rectangle];`
`   "connect side"  `
`   node [shape = ellipse];`
`   UNCONNECTED `
`   CONNECTING[color=red]`
`   OPEN_CSEND[color=red]`
`   OPEN_CRECV[color=red]`
`   CLOSING`
`   CLOSED`
`   CONNECTED[color=red];`

`  UNCONNECTED -> CONNECTING [label=" Sock_post_connect_ifaddr", color = red];`
`  CONNECTING -> OPEN_CSEND  [label = " event =  MPIDU_SOCK_OP_CONNECT \n remote side accept connection\n  send pgid and pgid_length to remote side "];`
`  OPEN_CSEND -> OPEN_CRECV  [label = " event = SOCK_OP_WRITE \n data successful written to remote side \n  "];`
`  OPEN_CRECV -> CLOSING [label = " event = SOCK_OP_READ\n read answer form remote side\n ack = false "] ;`
`  OPEN_CRECV -> CONNECTED [label = " event = SOCK_OP_READ\n read answer form remote side\n ack = true  "] ;`
`  CLOSING -> CLOSED [label = " event = SOCK_OP_CLOSED"];`

node \[shape = rectangle\];

`   "accept side"   `

node \[shape = ellipse\];

`   UNDEFINED`
`   OPEN_LRECV_PKT`
`   OPEN_LRECV_DATA`
`   LSEND[color=red]`
`   CLOSING`
`   CLOSED`
`   CONNECTED[color=red];`

UNDEFINED -\> OPEN_LRECV_PKT \[label = "event = SOCK_OP_ACCEPT \\n
post receive to event queue " \]; OPEN_LRECV_PKT -\> OPEN_LRECV_DATA
\[label = " event = SOCK_OP_READ \\n pkt = SC_OPEN_REQ \\n post read
to event queue "\]; OPEN_LRECV_DATA -\> LSEND \[label = " event =
SOCK_OP_READ \\n read pg_id, accept (yes) or refuse (no)
connection\\n send response to remote side", color = red\]
LSEND-\>CLOSING \[label =" event = SOCK_OP_WRITE \\n send response
complete \\n ack = false"\]; LSEND -\> CONNECTED \[label =" event =
SOCK_OP_WRITE \\n send response complete \\n ack = true"\];

}

</graphviz>
-->

### Connect Side

The connecting side tries to establish a connection by sending an active
message to the remote side. If the connection is accepted, the pg_id is
send to the remote side. Now, the process waits, until the connection is
finally accepted or refused. For this decision, the remote side requires
the gp_id . Based on the answer from the remote side (ack = yes or ack
= no) the connection is connected or closed.

```
An attempt to send a message detects that the VC is in state
VC_STATE_UNCONNECTED (e.g., in ch3_istartmsg).  Then:
   save message in a request added to the send queue (SendQ_enqueue(vc))
   VC_post_connect(vc)
       set vc->state to VC_STATE_CONNECTING
       get connection string from PG, get connection info from string
       Connection_alloc(&conn) /*allocates space for Connection_t and pgid*/
                /* we need the pgid to identify the VC (??)
                (note: instead of pgid, connections should simply
                 point at the PG object)
                (note: this step should initialize the conn fields)*/
         Sock_post_connect_ifaddr(conn)
          /* creates the socket, sets sock opts, allocates internal sock
             structure, adds the socket to the poll list (sock_set)
             execute system connect (sock)
             set sock state to CONNECTED_RW, CONNECTING, or (on error)
             DISCONNECTED
             returns the socket object (MPIDU_Sock *) */
      conn->state = CONN_STATE_CONNECTING
      init conn fields (note: move into allow)
/*Now the connection waits until the connection is accepted on the remote side.
This will cause and MPIDU_SOCK_OP_CONNECT event in the connection queue.*/

in ch3_progress, in
    Handle_sock_event
    if (event->op == MPIDU_SOCK_OP_CONNECT)
         /* Note that when we get a connection request, we don't yet know
            what VC this connection is for.  We get that information
            by being send the pg_id for the process group and the
           rank of the VC within that process group. */
         Sockconn_handle_connect_event()
         (note that this routine checks for event error; not the right place)
         if (conn == CONN_STATE_CONNECTING)
              conn->state = CONN_STATE_OPEN_CSEND
              initialise a packet contained within the conn structure to
              PKT_SC_OPEN_REQ, send length of pg_id and pg_rank
              (note: for hetero, these need to be in fixed byteorder, length)
              connection_post_send_pkt_and_pgid
                  also sends the pgid itself
                  (note: perhaps this and related routines should be a
                  general ch3 routine)
              (note: should move formation of all data to send into a
               single routine)
              Sock_post_writev() for this (pkt + pg_id)
              return to handle_sock_event

     if (event->op == MPIDU_SOCK_OP_WRITE)
        if (!conn->send_active)
           (assumes finishing connection write)
           Sockconn_handle_connwrite()
               if (state == CONN_STATE_OPEN_CSEND)
                   conn->state = CONN_STATE_OPEN_CRECV
                   connection_post_recv_pkt
                   return to handle_sock_event

     if (event->op == MPIDU_SOCK_OP_READ) {
          If (pkt_type == PKT_SC_OPEN_RESP)
             if (pkt->ack is true)
                 conn->state = STATE_CONNECTED
                 vc->state = VC_STATE_CONNECTED
                 connection_post_recv_pkt(conn)
                 connection_post_sendq_req(conn)
                     If we had enqueued a send of a request, start it
             else
                 # Close connection because this was refused on the
                 # other side (head-to-head connection race).
                 conn->state = CONN_STATE_CLOSING
                 Sock_post_close(conn->sock)
                 conn->vc = NULL
                 /* note: vc state itself is unchanged. I probably has
                    accept another connection . */
```

### Accept Side

The accept side receives a connection request on the listening socket.
In the first instance, it accepts the connection an allocates the
required structures (conn, socket). Then, the connection waits for the
pg_id of the remote side to assign the socket-connection to a VC. The
decision, if a connection is accepted or refused, is based on the
following steps:

1.  the VC has an active connection (`vc->conn == NULL`) : The new
    connection is accepted
2.  the VC has an active connection
    1.  if `my_pg_id < remote_pg_id`: accept and discard other
        connection
    2.  if `my_pg_id > remote_pg_id`: refuse

The answer is send to the remote note. Below is the pseudo code for the
connection process:

```
Progress_handle_sock_event
    if (event->op == MPIDU_SOCK_OP_ACCEPT)
     /*Accept event on the listening socket */
        Sockconn_handle_accept_event()
            Allocate connection (same routine as in connect)
            MPIDU_Sock_accept( conn )
              /*  executes accept, set sock opts
                (note: there should be a common routine for setting the
                 sock opts on connect and accept)
                 initialize sock and associated poll structures */
           /* initialize conn fields */
            conn->state = OPEN_LRECV_PKT
            connection_post_recv_pkt
             Sock_post_read
                   /* adds this buffer to pending reads on the socket-fd
                      Set it active in the sock_set */
              return from handle


Progress_handle_sock_event
    if (event->op == MPIDU_SOCK_OP_READ)
        if (conn->state not recognized) (note: bad code style)
            Sockconn_handle_conn_event( conn ) (conn comes from user_ptr in
            event)
                if (conn->pkt is PKT_SC_OPEN_REQ)
                    (added check that conn state == OPEN_LRECV_PKT)
                    conn->state = OPEN_LRECV_DATA
                    Sock_post_read(pg_id,pkt->pg_id_len)
                      (post a a non-blocking read to read the remote pg_id)

Progress_handle_sock_event
     if (event->op == MPIDU_SOCK_OP_READ) /" remote data are read */
         if (conn->state == OPEN_LRECV_DATA) { /*since the state is LRECV_DATE, we read the pg_id */
             Sockconn_handle_connopen_event(conn)
                 /* read the pg_id from the even */
                 (find the corresponding process group.  We are
                  guaranteed to find the pg)
                 MPIDI_PG_Find(conn->pg_id,&pg).

                 Find the corresponding virtual connection (note that on
                 an accept operation, we don't know until this point the
                 vc for this connection request)
                 MPIDI_PG_Get_vc(pg,pg_rank,&vc);

                (at this point, we need to check for head-to-head connections,
                since we may already be attempting to form this VC, having
                originated a connection from this side). If VC != NULL, we are in
                an head-to-haed situation, otherwise the connection is accepted)

                if (vc->conn == NULL || (mypg < pg) ||
                 (pg == mypg && myrank < pg_rank of conn) )
                 not head to head OR winner of head-to-head.
                 Continue with connection. If the winner of a head-to-head
                 connection: old connection is discarded.
                 VC state is now initialised to VC_STATE_CONNECTING
                 vc->conn is set to this connection, and the associated
                 sock is also set
                 conn->vc = vc
                 In all cases, return an ack:
                 conn->state = OPEN_LSEND (note, even when refusing connection)
                 conn->pkt = MPIDI_CH3I_PKT_SC_OPEN_RESP
                   ptk.ack = true if accepting, false if refusing
                   Sock_post_write(pkt) /*send answer to remote side */

      if (event->op == MPIDU_SOCK_OP_WRITE)
         if (conn->state == OPEN_LSEND) {
             finished sending response packet.
             if (conn.pkt->ack is true)
                 (note: this should use the same code as the connect branch)
                 conn->state = CONN_STATE_CONNECTED
                 connection_post_recv_pkt
                 connection_post_sendq_req
                 vc->state = VC_STATE_CONNECTED
             else
              /* The connection was refused. and ack=false
                 was send to remote side, so this connection can be closed */
                 conn->state = CONN_STATE_CLOSING
                 Sock_post_close(conn->sock)
                    This primarily enquees SOCK_OP_CLOSE event
```

On a close sock event: (to do; this isn't ready since it is clear that
additional events occur before we'd get to this point)

```
   case SOCK_OP_CLOSE:
       Sockconn_handle_close_event( conn)
         if(conn->vc!=NULL)  
          /* This is only done, if the connection was accepted before, 
              so was NOT the looser of a head-to-head connection 
              if a refused or discarded connection is closes, this
              has no bearing to the VC*/ 
              conn->vc->ch.state = STATE_UNCONNECTED
              Handle_connection(vc,EVENT_TERMINATED)
               (EVENT_TERMINATED is the only event type,
                we should integrate this with the other connection 
                state change events)
               switch (vc->state)
                VC_STATE_CLOSE_ACKED: (this must be set because the
                                         default generates an error)
         free_destroy(conn) 
```

Note also that `mpid_finalize.c` contains some vc close code (most of
this, including all code that performs state changes, should not be in
this file).

### Head-to-Head Connections And MPI-Finalize

Known problems with head-to-head connections are wrong reconnects, after
MPI finalized is called. This is what is happening:

1.  MPI_Finalize is called, therefore all VC are closed (for all
    connections in all process groups the closing protocol is started)
2.  Discarded or refused connections are not recognized with this
    closing protocol, since they are not associated with and VC
3.  The discadred/refused/not used connections still have got pending
    events, which may be handled anytime during the close protocol of
    the other connections

This leads to the following misbehaviors:

1.  A connection, which actually should be refused, is accepted. This
    happens because in the transition from `LRECV_PACKET` to `LRECV_DATA`,
    the VC has no associated connection (the connection was closed
    because of `MPI_FINALIZE`) – to the process handles the connection as
    new/reconnection connection and accepts it.
2.  The socket-set with the discarded/refused connections is destroyed,
    before the sockets are closed and the memory for the structures is
    freed. This leads to both: memory leaks and errors on the remote
    side, since the socket is closed without the close-protocol.

### Unresolved Questions

There are at least three sets of states:

```
conn->state - defined in ch3/channels/*/include/mpidi_ch3_impl.h 
vc->state - defined in ch3/include/mpidpre.h 
vc->ch.state - defined in  ch3/channels/*/include/mpidi_ch3_pre.h
```

Why is there a separate vc state and vc channel state? Are the
`vc->ch.state` values really different from `vc->state`, and how do we
ensure that changes to `vc->state` and `vc->ch.state` are made
consistently?

## Old

Here is a guess at the state transitions in the current implementation.

On accept side:

```
CONN_STATE_OPEN_LRECV_PKT
    VC_STATE_CONNECTING
CONN_STATE_OPEN_LSEND
Enqueuing accept connection
CONN_STATE_CONNECTED
    VC_STATE_CONNECTED
Dequeueing accept connection
    VC_STATE_LOCAL_CLOSE
    VC_STATE_CLOSE_ACKED
    VC_STATE_CLOSE_ACKED   (yes, state was set twice) (? perhaps separate vc?)
CONN_STATE_CLOSING
    VC_STATE_UNCONNECTED
    VC_STATE_INACTIVE
CONN_STATE_CLOSED
   ( this appears to be the connection used to establish the original
     intercomm; the connection is closed for some reason)

   ( then an ch3_istartmsg causes the following )
   posting connect and enqueuing request
   VC_STATE_CONNECTING
CONN_STATE_CONNECTING
CONN_STATE_OPEN_CSEND
CONN_STATE_OPEN_CRECV
CONN_STATE_OPEN_LRECV_DATA
```
