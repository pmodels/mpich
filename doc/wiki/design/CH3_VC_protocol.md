# CH3 VC Protocol

This document describes the protocol CH3 uses when closing a VC. The
close protocol ensures that both sides of the connection are ready to
close and all communication has finished.

## Overview

When a VC is released, and its ref count drops to 0, the VC is closed.
Similarly in `MPID_Finalize()`, `MPIDI_PG_Close_VCs()` is called which
goes through all VCs in all process groups and closes them. A VC is
closed by calling `MPIDI_CH3U_VC_SendClose()`.

In `MPID_Finalize()`, `MPIDI_CH3U_VC_WaitForClose()` is called to wait
for all VCs to finish closing. In `MPIDI_CH3U_VC_SendClose()`,
`outstanding_close_ops` is incremented to keep track of the number of
VCs that are in the process of closing. When the close protocol
completes for a VC, `outstanding_close_ops` is decremented. In this way,
`MPIDI_CH3U_VC_WaitForClose()` can simply keep calling progress while
`outstanding_close_ops` is non-zero.

All VCs start in the `INACTIVE` state at init time. When a message is
sent over a VC the VC is connected by the channel, if necessary, and its
state is changed to `ACTIVE`.

## State Diagram

Below is the state diagram for the close protocol. There are eight
states in the protocol

  - `ACTIVE` -- Communication has been initiated and has not yet been
    completed
  - `LOCAL_CLOSE` -- Local process has initiated the close protocol
  - `REMOTE_CLOSE` -- Remote process has initiated the close protocol
  - `CLOSE_ACKED` -- Local process has initiated the close protocol and
    has acknowledged the remote process's CLOSE packet
  - `CLOSED` -- Local process has received and ack from the remote
    process
  - `INACTIVE` -- Either no communication has been initiated, or the VC
    has been successfully closed and resources have been released
  - `INACTIVE_CLOSED` -- The process is in finalize and this VC should
    not be opened
  - `MORIBUND` -- This connection has terminated prematurely. There may
    be outstanding sends or receives on this VC. Currently, `MORIBUND`
    is a terminal state, but in the future it may be possible to
    activate the VC again.

In the labels for the transitions, the part above the line indicate the
event triggering the transition. The part below the line describes the
actions taken in the transition (In the diagram a label for a transition
generally appears to the right of the edge). Note that the error
transitions are shown with red and green edges to simplify the diagram.
A key describing the transitions is shown at the bottom of the diagram.

<!---
<graphviz> digraph ch3_conn_fsm {

`  // uncomment below for printing`
`   //size="7.5,10"`

`   // MPIDI_VC_STATE_ prefix on all of these states`
`   node [shape = ellipse];`
`   INACTIVE_CLOSED [peripheries=2]`
`   { rank = min; ACTIVE}`
`   INACTIVE`
`   REMOTE_CLOSE`
`   CLOSE_ACKED`
`   { rank = same; MORIBUND [peripheries=2]; LOCAL_CLOSE }`
`   CLOSED;`

`   edge [fontsize=10];`
`   // happens in MPID_VCRT_Release and MPIDI_PG_Close_VCs`
`   ACTIVE -> LOCAL_CLOSE        [ label = "MPIDI_CH3U_VC_SendClose\n-------------\nsend close(ack=FALSE)\n++MPIDI_Outstanding_close_ops" ];`

`   ACTIVE -> REMOTE_CLOSE       [ label = "MPIDI_CH3_PktHandler_Close\n(ack==FALSE)\n-------------\n" ];`

`   LOCAL_CLOSE -> CLOSE_ACKED   [ label = "MPIDI_CH3_PktHandler_Close\n(ack==FALSE)\n-------------\nsend close(ack=TRUE)" ];`
`   LOCAL_CLOSE -> CLOSED        [ label = "MPIDI_CH3_PktHandler_Close\n(ack==TRUE)\n-------------\nsend close(ack=TRUE)\nMPIDI_CH3_Connection_terminate" ];`

`   REMOTE_CLOSE -> CLOSE_ACKED  [ label = "MPIDI_CH3U_VC_SendClose\n-------------\nsend close(ack=TRUE)\n++MPIDI_Outstanding_close_ops" ];`

`   CLOSE_ACKED -> CLOSED        [ label = "MPIDI_CH3_PktHandler_Close\n(ack==TRUE)\n-------------\nMPIDI_CH3_Connection_terminate" ];`
`   CLOSED -> INACTIVE           [ label = "MPIDI_CH3U_Handle_connection\nevent==MPIDI_VC_EVENT_TERMINATED\n-------------\nMPIDI_PG_release_ref(vc->pg)\n--MPIDI_Outstanding_close_ops" ];`
`   INACTIVE -> INACTIVE_CLOSED  [ label = "MPID_Finalize\n-------------\n" ];`

`   INACTIVE -> ACTIVE           [ label = "Communication initiated\n-------------" constraint=false ]`

`   ACTIVE -> MORIBUND           [ color = red ];`
`   REMOTE_CLOSE -> MORIBUND     [ color = red constraint=false ];`
`   LOCAL_CLOSE -> MORIBUND      [ color = green constraint=false ];`
`   CLOSE_ACKED -> MORIBUND      [ color = green constraint=false ];`

`   //Using a key for the MORIBUND labels to make the diagram less cluttered`
`   subgraph clusterkey {`
`       node [ style=invis ];`
`       {rank=max A; B; C; D}`
`       A -> B [ color = red label = "connection closed prematurely\n-------------\nMPIDI_PG_release_ref(vc->pg)" ];`
`       C -> D [ color = green label = "connection closed prematurely\n-------------\nMPIDI_PG_release_ref(vc->pg)\n--MPIDI_Outstanding_close_ops" ];`
`       label="Key for\nMORIBUND edges";`
`   }`

`   INACTIVE_CLOSED -> D [lhead=clusterkey style=invis]`

} </graphviz>

## Old version

<graphviz> digraph ch3_conn_fsm {

`   // uncomment below for printing`
`   //size="7.5,10"`

`   /*`
`   node [shape = rectangle];`
`   undefined;`
`   */`

`   // MPIDI_VC_STATE_ prefix on all of these states`
`   node [shape = ellipse];`
`   INACTIVE`
`   ACTIVE`
`   LOCAL_CLOSE`
`   REMOTE_CLOSE`
`   CLOSE_ACKED;`

`   // happens in MPID_VCRT_Release and MPIDI_PG_Close_VCs`
`   ACTIVE -> LOCAL_CLOSE        [ label = "MPIDI_CH3U_VC_SendClose\n-------------\nsend close(ack=FALSE)\n++MPIDI_Outstanding_close_ops" ];`

`   ACTIVE -> REMOTE_CLOSE       [ label = "MPIDI_CH3_PktHandler_Close\nack==FALSE\n-------------\n" ];`

`   LOCAL_CLOSE -> CLOSE_ACKED   [ label = "MPIDI_CH3_PktHandler_Close\n(ack==TRUE)\n-------------\nsend close(ack=TRUE)" ];`
`   LOCAL_CLOSE -> CLOSE_ACKED   [ label = "MPIDI_CH3_PktHandler_Close\n(ack==FALSE)\n-------------\nsend close(ack=TRUE)\nMPIDI_CH3_Connection_terminate(vc)" ];`

`   REMOTE_CLOSE -> CLOSE_ACKED  [ label = "MPIDI_CH3U_VC_SendClose\n-------------\nsend close(ack=TRUE)\n++MPIDI_Outstanding_close_ops" ];`

`   CLOSE_ACKED -> CLOSE_ACKED   [ label = "MPIDI_CH3_PktHandler_Close\nack==TRUE\n-------------\nMPIDI_CH3_Connection_terminate(vc)" ];`
`   CLOSE_ACKED -> INACTIVE      [ label = "MPIDI_CH3U_Handle_connection\nevent==MPIDI_VC_EVENT_TERMINATED\n-------------\nMPIDI_PG_release_ref(vc->pg)\n--MPIDI_Outstanding_close_ops" ];`

} </graphviz>
-->
