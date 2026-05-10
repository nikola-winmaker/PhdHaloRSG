

#define RING_SIZE (512 - (3*sizeof(unsigned int)))
#define true    1
#define false   0

typedef struct {
    char buf[RING_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;
}  simple_ring_t;

void simple_init(simple_ring_t *r);
unsigned int simple_put(simple_ring_t *r, unsigned int byte);
unsigned int simple_get(simple_ring_t *r, unsigned int *byte);
