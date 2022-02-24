# Socket Connection Protocol

While investigating req 3669 I (goodell@) had to figure out the state
diagram for the ch3:sock channel's conn protocol. It is attached below,
along with the source for the diagram. Some parts of the diagram have
'?' because I only spent time on the part of the diagram that was
relevant to the bug.

[CH3 VC protocol](CH3_VC_protocol.md) is a related node, about
the FSM at the layer above this one.

## Diagram
!<---

View the wiki source for this node in order to see the graphviz source,
it's rendered inline via
[Extension:Graphviz](http://www.mediawiki.org/wiki/Extension:GraphViz).
Red transitions indicate points where `vcch->conn` may be assigned to.

<graphviz> digraph ch3_sock_conn_state_machine {

`   // uncomment below for printing`
`   //size="7.5,10"`

`   node [shape = rectangle];`
`   undefined;`

`   node [shape = ellipse];`
`   UNCONNECTED`
`   CONNECTING`
`   OPEN_CSEND`
`   OPEN_CRECV`
`   OPEN_LRECV_PKT`
`   OPEN_LRECV_DATA`
`   OPEN_LSEND`
`   CONNECTED`
`   CONNECT_ACCEPT`
`   CLOSING`
`   CLOSED`
`   LISTENING`
`   FAILED;`

`   /* I think that red transitions indicate points where vcch->conn may be assigned to. */`
`   undefined       -> CONNECTING      [ label = "MPIDI_CH3I_Sock_connect\n-------------\n?" color = "red" ];`
`   undefined       -> OPEN_LRECV_PKT  [ label = "handle_accept_event\n-------------\n?" ];`
`   CONNECTING      -> OPEN_CSEND      [ label = "handle_connect_event\n-------------\npost_send_pkt_and_pgid()" ];`
`   OPEN_CSEND      -> OPEN_CRECV      [ label = "handle_connwrite\n-------------\npost_recv_pkt()" ];`
`   OPEN_CRECV      -> CONNECTED       [ label = "handle_conn_event\nconn->pkt.type==PKT_SC_OPEN_RESP\nopenpkt->ack==TRUE\n-------------\npost_recv_pkt()\npost_sendq_req()" ];`
`   OPEN_CRECV      -> CLOSING         [ label = "handle_conn_event\nconn->pkt.type==PKT_SC_OPEN_RESP\nopenpkt->ack==FALSE\n-------------\npost_close()" ];`

`   OPEN_LRECV_PKT  -> OPEN_LRECV_DATA [ label = "handle_conn_event\nconn->pkt_type==PKT_SC_OPEN_REQ\n-------------\npost_read()" ];`
`   OPEN_LRECV_PKT  -> OPEN_LSEND      [ label = "handle_conn_event\nconn->pkt.type==MPIDI_CH3I_PKT_SC_CONN_ACCEPT\n-------------\nVC_Init()\nopenresp->ack=TRUE\npost_send_pkt()\nMPIDI_CH3I_Acceptq_enqueue()" color = "red" ];`
`   OPEN_LRECV_DATA -> OPEN_LSEND      [ label = "handle_connopen_event\n-------------\nopenresp->ack=break_head_to_head_tie()\npost_send_pkt()" color = "red" ];`
`   CONNECT_ACCEPT  -> OPEN_LSEND      [ label = "handle_conn_event\nconn->pkt.type==MPIDI_CH3I_PKT_SC_CONN_ACCEPT\n-------------\nVC_Init()\nopenresp->ack=TRUE\npost_send_pkt()\nMPIDI_CH3I_Acceptq_enqueue()" color = "red" ];`

`   OPEN_LSEND      -> CONNECTED       [ label = "handle_connwrite\nopenresp->ack==TRUE\n-------------\npost_recv_pkt()\npost_sendq_req()" ];`
`   OPEN_LSEND      -> CLOSING         [ label = "handle_connwrite\nopenresp->ack==FALSE\n-------------\npost_close()" ];`

`   CONNECTED       -> CLOSING         [ label = " MPIDI_CH3_Connection_terminate\n-------------\npost_close()\nvcch->conn->vc=NULL\nvcch->conn=NULL" color = "red" ];`

`   undefined       -> CONNECT_ACCEPT  [ label = "MPIDI_CH3I_Connect_to_root_sock\n-------------\npost_connect()\nacceptpkt->port_name_tag=port_name_tag" color = "red" ];`
`   CONNECT_ACCEPT  -> OPEN_CSEND      [ label = "handle_connect_event\n-------------\npost_send_pkt()" ];`

`   CLOSING         -> CLOSED          [ label = " handle_close_event\nconn!=null\n-------------\ndestroy(conn)" color = "red" ];`

`   undefined       -> LISTENING       [ label = "MPIDU_CH3I_SetupListener\n-------------\nMPIDU_Sock_listen()" ]`
`   LISTENING       -> CLOSED          [ label = "handle_close_event\nconn!=null\n-------------\nProgress_signal_completion()\ndestroy(conn)" color = "red" ];`

}

</graphviz>
-->
