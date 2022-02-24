# Nemesis TCP Netmod State Machine

## Legend

All states in the diagram have an implied prefix of `CONN_STATE_`. The
comment below from `socksm.h` should help explain the naming convention.

```
/*
CONN_STATE - Connection states of socket
TC = Type connected (state of a socket that was issued a connect on)
TA = Type Accepted (state of a socket returned by accept)
TS = Type Shared (state of either TC or TA)

C - Connection sub-states
D - Disconnection sub-states
*/
```

Edges labeled in  indicate places where head to head connections are
dealt with.

## Diagram

**TODO:** add transition labels.

```suggestion
<graphviz> digraph tcp_conn_state_machine {

`   node [shape = ellipse];`
`   TS_CLOSED`
`   TC_C_CNTING`
`   TC_C_CNTD`
`   TC_C_RANKSENT`
`   TA_C_CNTD`
`   TA_C_RANKRCVD`
`   TS_COMMRDY`
`   TS_D_QUIESCENT`
`   TC_C_TMPVCSENT`
`   TA_C_TMPVCRCVD;`

`   /* connect() path */`
`   TS_CLOSED     -> TC_C_CNTING    [ ];`
`   TC_C_CNTING   -> TC_C_CNTD      [ ];`
`   TC_C_CNTD     -> TC_C_RANKSENT  [ color = "red" ];`
`   TC_C_RANKSENT -> TS_COMMRDY     [ ];`

`   /* accept() path */`
`   TS_CLOSED     -> TA_C_CNTD      [ ];`
`   TA_C_CNTD     -> TA_C_RANKRCVD  [ ];`
`   TA_C_RANKRCVD -> TS_COMMRDY     [ color = "red" ];`

`   /* tmpvc connect() path for dynamic processes */`
`   TC_C_CNTD      -> TC_C_TMPVCSENT [];`
`   TC_C_TMPVCSENT -> TS_COMMRDY [];`

`   /* tmpvc accept() path for dynamic processes */`
`   TA_C_CNTD      -> TA_C_TMPVCRCVD [];`
`   TA_C_TMPVCRCVD -> TS_COMMRDY [];`

`   /* disconnect */`
`   TS_COMMRDY   -> TS_D_QUIESCENT  [ ];`

`   /* I think that red transitions indicate points where vcch->conn may be assigned to. */`
`   /*`
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

`   CONNECTED       -> CLOSING         [ label = " MPIDI_CH3_Connection_terminate\n-------------\npost_close()" ];`

`   undefined       -> CONNECT_ACCEPT  [ label = "MPIDI_CH3I_Connect_to_root_sock\n-------------\npost_connect()\nacceptpkt->port_name_tag=port_name_tag" color = "red" ];`
`   CONNECT_ACCEPT  -> OPEN_CSEND      [ label = "handle_connect_event\n-------------\npost_send_pkt()" ];`

`   CLOSING         -> CLOSED          [ label = " handle_close_event\nconn!=null\n-------------\ndestroy(conn)" color = "red" ];`

`   undefined       -> LISTENING       [ label = "MPIDU_CH3I_SetupListener\n-------------\nMPIDU_Sock_listen()" ]`
`   LISTENING       -> CLOSED          [ label = "handle_close_event\nconn!=null\n-------------\nProgress_signal_completion()\ndestroy(conn)" color = "red" ];`
`   */`

}

</graphviz>
```
