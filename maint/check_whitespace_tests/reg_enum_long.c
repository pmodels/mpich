/* expect: none */
struct s {
    enum { MPIR_STREAM_COMM_NONE, MPIR_STREAM_COMM_SINGLE, MPIR_STREAM_COMM_MULTIPLEX_LONGER_NAME }
        stream_comm_type;
};

typedef enum MPID_nem_tcp_Listen_State { LISTEN_STATE_INVALID = -1, LISTEN_STATE_, LISTEN_STATE_SIZE
} MPID_nem_tcp_Listen_State_t;
