/* expect: none */
static const char *(user_class_msgs[10]) = {
0};

static struct {
    int *head, *tail;
} freeq = {
NULL, NULL};

struct fns ops = {
    f_open,     /* Open */
    f_coll, /* OpenColl */ /*XXX*/
        f_read, /* ReadContig */
    f_write     /* WriteContig */
};
