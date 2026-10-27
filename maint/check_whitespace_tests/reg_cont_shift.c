/* expect: C2 */
int g(void)
{
    switch (x) {
        case 1:
        snprintf(pktmsg_aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa, sizeof(pktmsg),
                     "EAGER_SEND - (%d,%d,%d,)%" PRIdPTR,
                     pkt->eager_send.match.parts.context_id);
            break;
    }
    return 0;
}
