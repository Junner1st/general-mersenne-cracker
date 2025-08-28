#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdlib.h>

#include "bitmatrix.h"
#include "mt19937.h"

typedef struct {
    PyObject_HEAD
    BitMatrix_C bm;
} BitMatrixObject;

/* Deallocate */
static void
BitMatrix_dealloc(BitMatrixObject *self)
{
    free(self->bm.rows);
    Py_TYPE(self)->tp_free((PyObject *) self);
}


static void
BitMatrix_matrix_builder(BitMatrix_C *bm)
{
    const int n = bm->n;
    for (int j = 0; j < n; ++j) {
        int word_idx = j >> 5;
        int bit_pos = j & 31;

        // init state
        uint32_t state[624] = {0};
        state[word_idx] = 1U << bit_pos;

        // Mimics MT19937, extract the front n bits (MSB)
        MT19937_C mt;
        mt_init(&mt, state);

        for (int i = 0; i < n; ++i) {
            uint32_t y = mt_extract(&mt);
            int msb = (y >> 31) & 1;
            if (msb) {
                int w = j >> 6;
                int b = j & 63;
                bm->rows[i * bm->num_words + w] |= ((uint64_t)1 << b);
            }
        }
    }
}

static void
BitMatrix_matrix_builder_k(BitMatrix_C *bm, int k)
{
    const int n_state = bm->n;
    const int outputs_needed = (n_state + k - 1) / k;

    for (int j = 0; j < n_state; ++j) {
        int state_word = j >> 5;
        int state_bit  = j & 31;

        uint32_t state[624] = {0};
        state[state_word] = 1U << state_bit;

        MT19937_C mt;
        mt_init(&mt, state);

        for (int out_idx = 0; out_idx < outputs_needed; ++out_idx) {
            uint32_t y = mt_extract(&mt);
            for (int b = 0; b < k; ++b) {
                int row = out_idx * k + b;
                if (row >= n_state) break;
                int bitpos_in_y = 31 - b;
                int bit = (y >> bitpos_in_y) & 1U;
                if (bit) {
                    int w = j >> 6;
                    int off = j & 63;
                    bm->rows[(size_t)row * bm->num_words + w] |= ((uint64_t)1ULL << off);
                }
            }
        }
    }
}



int
bm_init(BitMatrix_C *bm, int n, int num_bits)
{
    bm->n = n;
    bm->obs_len = num_bits;
    bm->word_size = 64;
    bm->num_words = (n + 63) >> 6;
    bm->rows = calloc((size_t) bm->n * bm->num_words, sizeof(uint64_t));
    if (!bm->rows) {
        return -1;
    }
    // BitMatrix_matrix_builder(bm);    // old imp
    BitMatrix_matrix_builder_k(bm, num_bits);

    return 0;
}

/* __init__(self, n) */
static int
BitMatrix_init(BitMatrixObject *self, PyObject *args, PyObject *kwds)
{
    int n, obs_len;
    if (!PyArg_ParseTuple(args, "ii", &n, &obs_len))
        return -1;

    return bm_init(&self->bm, n, obs_len);
}

int
get_bit(BitMatrix_C *bm, int r, int c)
{
    if (r < 0 || r >= bm->n || c < 0 || c >= bm->n) {
        return 0;
    }
    int w = c >> 6;
    int b = c & 63;
    uint64_t val = bm->rows[(size_t) r * bm->num_words + w];
    return ((val >> b) & 1);
}

void
xor_row(BitMatrix_C *self, int r1, int r2)
{
    uint64_t *row1 = &self->rows[(size_t) r1 * self->num_words];
    uint64_t *row2 = &self->rows[(size_t) r2 * self->num_words];
    for (int i = 0; i < self->num_words; i++) {
        row1[i] ^= row2[i];
    }
}

void
swap_row(BitMatrix_C *bm, int r1, int r2)
{
    uint64_t *row1 = &bm->rows[(size_t) r1 * bm->num_words];
    uint64_t *row2 = &bm->rows[(size_t) r2 * bm->num_words];
    for (int i = 0; i < bm->num_words; i++) {
        uint64_t tmp = row1[i];
        row1[i] = row2[i];
        row2[i] = tmp;
    }
}

/* get_bit(self, r, c) -> bool */
static PyObject *
_bitmatrix_BitMatrix_get_bit_impl(PyObject *self, PyObject *args)
{
    BitMatrixObject *obj = (BitMatrixObject *) self;
    int r, c;
    if (!PyArg_ParseTuple(args, "ii", &r, &c))
        return NULL;

    if (get_bit(&obj->bm, r, c))
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

/* xor_row(self, r1, r2) */
static PyObject *
_bitmatrix_BitMatrix_xor_row_impl(PyObject *self, PyObject *args)
{
    BitMatrixObject *obj = (BitMatrixObject *) self;
    int r1, r2;
    if (!PyArg_ParseTuple(args, "ii", &r1, &r2))
        return NULL;

    xor_row(&obj->bm, r1, r2);

    Py_RETURN_NONE;
}

/* swap_row(self, r1, r2) */
static PyObject *
_bitmatrix_BitMatrix_swap_row_impl(PyObject *self, PyObject *args)
{
    BitMatrixObject *obj = (BitMatrixObject *) self;
    int r1, r2;
    if (!PyArg_ParseTuple(args, "ii", &r1, &r2))
        return NULL;

    swap_row(&obj->bm, r1, r2);

    Py_RETURN_NONE;
}

static PyMethodDef BitMatrix_methods[] = {
    {"get_bit",  _bitmatrix_BitMatrix_get_bit_impl,  METH_VARARGS, PyDoc_STR("Get a bit")},
    {"xor_row",  _bitmatrix_BitMatrix_xor_row_impl,  METH_VARARGS, PyDoc_STR("XOR two rows")},
    {"swap_row", _bitmatrix_BitMatrix_swap_row_impl, METH_VARARGS, PyDoc_STR("Swap two rows")},
    {NULL}
};

static PyTypeObject BitMatrixType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "bitmatrix.BitMatrix",
    .tp_basicsize = sizeof(BitMatrixObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = "BitMatrix class (C extension)",
    .tp_methods = BitMatrix_methods,
    .tp_init = (initproc) BitMatrix_init,
    .tp_dealloc = (destructor) BitMatrix_dealloc,
    .tp_new = PyType_GenericNew,
};

static PyModuleDef bitmatrixmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "bitmatrix",
    .m_doc = "BitMatrix module in C",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_bitmatrix(void)
{
    PyObject *m;
    if (PyType_Ready(&BitMatrixType) < 0)
        return NULL;

    m = PyModule_Create(&bitmatrixmodule);
    if (!m)
        return NULL;

    Py_INCREF(&BitMatrixType);
    if (PyModule_AddObject(m, "BitMatrix", (PyObject *) &BitMatrixType) < 0) {
        Py_DECREF(&BitMatrixType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
