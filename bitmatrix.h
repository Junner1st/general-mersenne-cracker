#ifndef BITMATRIX_H
#define BITMATRIX_H

#include <stdint.h>


typedef struct {
    int n;
    int word_size;       // fixed as 64
    int num_words;       // number of 64-bit words per row
    uint64_t *rows;      // flat buffer: n * num_words
} BitMatrix_C;

int get_bit(BitMatrix_C *bm, int row_index, int column_index);
void xor_row(BitMatrix_C *bm, int row1_index, int row2_index);
void swap_row(BitMatrix_C *bm, int row1_index, int row2_index);
int bm_init(BitMatrix_C *bm, int n);

#endif // BITMATRIX_H