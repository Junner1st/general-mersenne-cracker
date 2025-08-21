#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include <stdlib.h>

#include "mt19937.h"

typedef struct {
    PyObject_HEAD
    int n;
    int word_size;       // fixed as 64
    int num_words;       // number of 64-bit words per row
    uint64_t *rows;      // flat buffer: n * num_words
} BitMatrixObject;

/* Deallocate */
static void
BitMatrix_dealloc(BitMatrixObject *self)
{
    free(self->rows);
    Py_TYPE(self)->tp_free((PyObject *) self);
}


void
BitMatrix_matrix_builder(BitMatrixObject *self)
{
    for (int j = 0; j < self->n; j++) {
        int word_idx = j >> 5;
        int bit_pos = j % 32;

        // init state
        uint32_t state[624] = {0};
        state[word_idx] = 1U << bit_pos;

        // Mimics MT19937, extract the front n bits (MSB)
        MT19937_C mt;
        mt_init(&mt, state);

        for (int i = 0; i < self->n; i++) {
            uint32_t y = mt_extract(&mt);
            int msb = (y >> 31) & 1;
            if (msb) {
                int w = j >> 6;
                int b = j % 64;
                self->rows[i * self->num_words + w] |= ((uint64_t)1 << b);
            }
        }
    }
}

/* __init__(self, n) */
static int
BitMatrix_init(BitMatrixObject *self, PyObject *args, PyObject *kwds)
{
    int n;
    if (!PyArg_ParseTuple(args, "i", &n))
        return -1;

    self->n = n;
    self->word_size = 64;
    self->num_words = (n + 63) / 64;
    self->rows = calloc((size_t) self->n * self->num_words, sizeof(uint64_t));
    if (!self->rows) {
        return -1;
    }
    BitMatrix_matrix_builder(self);
    
    return 0;
}

/* get_bit(self, r, c) -> bool */
static PyObject *
_bitmatrix_BitMatrix_get_bit_impl(PyObject *self, PyObject *args)
{
    BitMatrixObject *obj = (BitMatrixObject *) self;
    int r, c;
    if (!PyArg_ParseTuple(args, "ii", &r, &c))
        return NULL;
    int w = c / 64;
    int b = c % 64;
    uint64_t val = obj->rows[(size_t) r * obj->num_words + w];
    if (val & ((uint64_t)1 << b))
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
    uint64_t *row1 = &obj->rows[(size_t) r1 * obj->num_words];
    uint64_t *row2 = &obj->rows[(size_t) r2 * obj->num_words];
    for (int i = 0; i < obj->num_words; i++) {
        row1[i] ^= row2[i];
    }
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
    uint64_t *row1 = &obj->rows[(size_t) r1 * obj->num_words];
    uint64_t *row2 = &obj->rows[(size_t) r2 * obj->num_words];
    for (int i = 0; i < obj->num_words; i++) {
        uint64_t tmp = row1[i];
        row1[i] = row2[i];
        row2[i] = tmp;
    }
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
