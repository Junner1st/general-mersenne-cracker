#ifndef MT19937_H
#define MT19937_H

#include <stdint.h>

#define N 624
#define M 397
#define MATRIX_A   0x9908b0dfU
#define UPPER_MASK 0x80000000U
#define LOWER_MASK 0x7fffffffU

typedef struct {
    uint32_t state[N];
    int index;
} MT19937_C;

void mt_init(MT19937_C *mt, uint32_t *state_array);
uint32_t mt_extract(MT19937_C *mt);

#endif // MT19937_H