#ifndef MT19937CRACKER_H
#define MT19937CRACKER_H

#include <stdint.h>

#include "mt19937.h"
#include "bitmatrix.h"

#define EXIT_FAILURE 1

typedef struct {
    int n;
    uint32_t *x;
    uint32_t state[N];
    int state_recovered;
    MT19937_C mt;
} MT19937Cracker_C;


void mc_init(MT19937Cracker_C *mc);
int gaussian_elimination(MT19937Cracker_C *mc, uint32_t observation[], BitMatrix_C *bm, int *pivot_col);
void back_substitution(MT19937Cracker_C *mc, int current_row, int *pivot_col, BitMatrix_C *bm, uint32_t observation[]);
int consistency_checker(MT19937Cracker_C *mc, int current_row, uint32_t observation[]);
void reconstruct_state(MT19937Cracker_C *mc);
void advance_to_current(MT19937Cracker_C *mc, int bits);

void cracker(MT19937Cracker_C *mc, uint32_t observation[], int bits);
uint32_t getrandbits(MT19937Cracker_C *mc);
uint32_t *getstate(MT19937Cracker_C *mc);


#endif // MT19937CRACKER_H
