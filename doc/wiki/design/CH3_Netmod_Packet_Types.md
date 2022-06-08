# CH3 Netmod Packet Types

We need a way to allow netmods to create their own packet types without
having to modify upper layer headers. For convenience and speed, we want
packet type IDs to be constants, but at the same time we need to avoid
ID collisions between netmods. CH3 and Nemesis packets type IDs have
been hard coded in header files. But if we're to support multiple
netmods and third party netmods, this makes things tricky.

## Netmod-local packet types

The solution we chose is to define a wrapper packet which has two
mandatory fields, the standard packet type (which should be set to
`MPIDI_NEM_PKT_NETMOD`) followed by a subtype. Using the subtype, the
packet handler would index the netmod-local packet handler table stored
in the VC (In `MPIDI_CH3I_VC` two fields were added: an array of packet
handlers called `pkt_handler` and the size of the array
`num_pkt_handlers`). Below is the typedef for the wrapper packet:

``` c
typedef struct MPID_nem_pkt_netmod
{
    MPID_nem_pkt_type_t type;
    unsigned subtype;
}
MPID_nem_pkt_netmod_t;
```

## Example

### Define packet types

``` c

typedef enum MPIDI_nem_my_netmod_pkt_type
{
    MPIDI_NEM_MY_NETMOD_PKT_FOO,
    MPIDI_NEM_MY_NETMOD_PKT_BAR,
    MPIDI_NEM_MY_NETMOD_NUM_PKTS
} MPIDI_nem_my_netmod_pkt_type_t;

/* FOO is a fixed length packet */
typedef struct MPIDI_nem_my_netmod_pkt_foo
{
    MPID_nem_pkt_type_t type; /* mandatory field */
    unsigned subtype;         /* mandatory field */

    int a_field;              /* netmod field */
}
MPIDI_nem_my_netmod_pkt_foo_t;

/* BAR is a variable length packet */
typedef struct MPIDI_nem_my_netmod_pkt_bar
{
    MPID_nem_pkt_type_t type; /* mandatory field */
    unsigned subtype;         /* mandatory field */

    int important_info;       /* netmod field */
    size_t datalen;           /* netmod field */
}
MPIDI_nem_my_netmod_pkt_bar_t;
```

### Define packet handlers

``` c
static int foo_pkt_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                           MPIDI_msg_sz_t *buflen, MPID_Request **req)
{
    MPIDI_nem_my_netmod_pkt_foo_t *foo_pkt = (MPIDI_nem_my_netmod_pkt_foo_t *) pkt;
    int mpi_errno = MPI_SUCCESS;

    /* do something with the packet */
    MPIDI_nem_my_netmod_got_a_foo_pkt = TRUE;

    /* we've received the entire packet */
    *buflen = sizeof(MPIDI_CH3_Pkt_t);
    *rreqp = NULL;

 fn_fail:
    return mpi_errno;
}

static int bar_pkt_handler(MPIDI_VC_t *vc, MPIDI_CH3_Pkt_t *pkt,
                           MPIDI_msg_sz_t *buflen, MPID_Request **req)
{
    MPIDI_nem_my_netmod_pkt_bar_t *bar_pkt = (MPIDI_nem_my_netmod_pkt_bar_t *) pkt;
    int mpi_errno = MPI_SUCCESS;

    /* check if we've received the entire packet */
    /* note: we're always guaranteed to receive the first sizeof(MPIDI_CH3_Pkt_t) of the packet */
    if (*buflen >= sizeof(MPIDI_CH3_Pkt_t) + bar_pkt->datalen) {
        /* do something with the data */
        MPIU_memcpy(my_buffer_for_bar_data, bar_pkt + 1, bar_pkt->datalen);

        /* indicate how much of the buffer we've consumed */
        *buflen = sizeof(MPIDI_CH3_Pkt_t) + bar_pkt->datalen;
        *rreqp = NULL;
    } else {
        /* we haven't received all of the data */
        MPID_Request *rreq;

        ...create and init a request...

        /* set iov to point to where we want the data */
    rreq->ch3.iov[0].buf = my_buffer_for_bar_data;
    rreq->ch3.iov[0].len = bar_pkt->datalen;

        /* only consume the header, and let Nemesis copy all of the data */
        *buflen = sizeof(MPIDI_CH3_Pkt_t);
        *rreqp = rreq;
    }
 fn_fail:
    return mpi_errno;
}
```

### Define handler table

Declare the table:

``` c
static MPIDI_CH3_PktHandler_Fcn *my_pkt_handlers[MPIDI_NEM_MY_NETMOD_NUM_PKTS];
```

Initialize the table in, e.g., netmod init:

``` c
my_pkt_handlers[MPIDI_NEM_MY_NETMOD_PKT_FOO] = foo_pkt_handler;
my_pkt_handlers[MPIDI_NEM_MY_NETMOD_PKT_BAR] = bar_pkt_handler;
```

Then in VC init, set the pointer to the handler table:

``` c
vc_ch->pkt_handler = my_pkt_handlers;
vc_ch->num_pkt_handlers = MPIDI_NEM_MY_NETMOD_NUM_PKTS;
```

### Send a packet

Sending a netmod local packet is just like sending any other packet.
