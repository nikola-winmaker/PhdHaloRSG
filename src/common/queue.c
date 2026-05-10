

#define RING_SIZE (512 - (3*sizeof(unsigned int)))
#define true    1
#define false   0

typedef struct {
    char buf[RING_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
}  simple_ring_t;

void simple_init(simple_ring_t *r) {
    r->head = r->tail = r->count = 0;
}
unsigned int simple_put(simple_ring_t *r, unsigned int byte) {
    if (r->count == RING_SIZE) return false;
    r->buf[r->head] = byte;
    r->head = (r->head + 1) % RING_SIZE;
    r->count++;
    return true;
}
unsigned int simple_get(simple_ring_t *r, unsigned int *byte) {
    if (r->count == 0) return false;
    *byte = r->buf[r->tail];
    r->tail = (r->tail + 1) % RING_SIZE;
    r->count--;
    return true;
}
