#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include  <stdio.h>
#include "mt19937cracker.h"

typedef struct {
    PyObject_HEAD
    MT19937Cracker_C mc;
} MT19937CrackerObject;

static PyObject *
MT19937Cracker_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MT19937CrackerObject *self;
    self = (MT19937CrackerObject *)type->tp_alloc(type, 0);
    if (self != NULL) {
        memset(&self->mc, 0, sizeof(MT19937Cracker_C));
    }
    return (PyObject *)self;
}

static int
MT19937Cracker_init(MT19937CrackerObject *self, PyObject *args, PyObject *kwds)
{
    mc_init(&self->mc);
    return 0;
}

static void
MT19937Cracker_dealloc(MT19937CrackerObject *self)
{
    if (self->mc.x) {
        free(self->mc.x);
        self->mc.x = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject *)self);
}

void
mc_init(MT19937Cracker_C *self)
{
    self->n = 19968;
    self->x = (uint32_t *)calloc(self->n, sizeof(uint32_t)); // allocate and zero
    for (int i = 0; i < N; ++i) {
        self->state[i] = 0;
    }
    self->state_recovered = 0;
}


int
gaussian_elimination(MT19937Cracker_C *mc, uint32_t observation[], BitMatrix_C *bm, int *pivot_col)
{
    int current_row = 0;
    int n = mc->n;
    for (int col = 0; col < n; ++col) {
        int pivot_row = -1;
        for (int row = current_row; row < n; ++row)
            if (get_bit(bm, row, col) == 1) {
                pivot_row = row;
                break;
            }
        if (pivot_row == -1)
            continue;
        swap_row(bm, current_row, pivot_row);
        uint32_t tmp = observation[current_row];
        observation[current_row] = observation[pivot_row];
        observation[pivot_row] = tmp;
        pivot_col[current_row] = col;
        for (int row = 0; row < n; ++row)
            if (row != current_row && get_bit(bm, row, col)) {
                xor_row(bm, row, current_row);
                observation[row] ^= observation[current_row];
            }
        ++current_row;
    }

    return current_row;
}

void
back_substitution(MT19937Cracker_C *mc, int current_row, int pivot_col[], BitMatrix_C *bm, uint32_t observation[])
{
    for (int i = current_row-1; i >= 0; --i) {
        int col = pivot_col[i];
        uint32_t sum_val = observation[i];
        for (int j = col+1; j < mc->n; ++j)
            if (get_bit(bm, i, j))
                sum_val ^= mc->x[j];
        mc->x[col] = sum_val;
    }
}

int
consistency_checker(MT19937Cracker_C *mc, int current_row, uint32_t observation[])
{
    printf("rank=%d\n", current_row);
    for (int row = current_row; row < mc->n; ++row)
        if (observation[row] != 0) {
            fprintf(stderr, "Singular matrix, inconsistent system\n");
            return EXIT_FAILURE;
        }
    if (current_row != mc->n - 31) {
        fprintf(stderr, "underdetermined system, rank=%d\n", current_row);
        return EXIT_FAILURE;
    }
    return 0;
    // should return 0 or 1
}
// ranks={19937,19937,19552,19937,18947,19117,18890,19337,18572,18948,19401,18298,17940,18170,18864,19937,18866,18024,17402,16969,16692,16557,16539,16625,16808,17069,17407,17807,18266,18777,19336,19937}

void
reconstruct_state(MT19937Cracker_C *mc)
{
    int word_idx, bit_pos;
    for (int j = 0; j < mc->n; ++j)
        if (mc->x[j]) {
            word_idx = j >> 5;
            bit_pos = j & 31;
            mc->state[word_idx] |= (1 << bit_pos);
        }
}

void
advance_to_current(MT19937Cracker_C *mc, int bits)
{
    int steps = (mc->n + bits - 1) / bits;
    mt_init(&mc->mt, mc->state);
    for (int i = 0; i < steps; ++i)
        mt_extract(&mc->mt);
}

void
cracker(MT19937Cracker_C *mc, uint32_t observation[], int bits)
{
    if (bits!=32) {
        if (bits >= 16) bits = 16;
        else if (bits >= 8) bits = 8;
        else if (bits >= 4) bits = 4;
        else if (bits >= 2) bits = 2;
    }
    int n = mc->n;
    int *pivot_col = malloc(n * sizeof(int));
    if (!pivot_col) return;

    BitMatrix_C bm;
    bm_init(&bm, n, bits);

    for (int i = 0; i < n; ++i) {
        pivot_col[i] = -1;
    }
    int current_row = gaussian_elimination(mc, observation, &bm, pivot_col);
    back_substitution(mc, current_row, pivot_col, &bm, observation);
    if (consistency_checker(mc, current_row, observation))
        return;
    reconstruct_state(mc);
    advance_to_current(mc, bits);
    mc->state_recovered = 1;

    free(pivot_col);
}

int
getrandbits_C(MT19937Cracker_C *mc, uint32_t *out)
{
    if (mc->state_recovered != 1) {
        return -1;
    }
    *out = mt_extract(&mc->mt);
    return 0;
}

uint32_t
getrandbits(MT19937Cracker_C *mc)
{
    if (mc->state_recovered != 1) {
        printf("status error");
        return 0;
    }
    uint32_t val;
    int status = getrandbits_C(mc, &val);
    if (status != 0) {
        printf("getrandbits_C error");
        return 0;
    }

    return val;
}


uint32_t *
getstate(MT19937Cracker_C *mc)
{
    if (!mc->state_recovered)
        return NULL;
    return mc->state;
}


static PyObject *
_mt19937cracker_MT19937Cracker_cracker_impl(PyObject *self, PyObject *args) {
    MT19937CrackerObject *obj = (MT19937CrackerObject *) self;

    int n = obj->mc.n;
    PyObject *observe_list;
    int bits;
    if (!PyArg_ParseTuple(args, "Oi", &observe_list, &bits)) {
        return NULL;
    }

    if (!PyList_Check(observe_list)) {
        PyErr_SetString(PyExc_TypeError, "Observation must be a list");
        return NULL;
    }

    Py_ssize_t length = PyList_Size(observe_list);
    if (length != n) {
        PyErr_Format(PyExc_ValueError, "Observation list must have exactly %d elements", n);
        return NULL;
    }

    uint32_t observation[n];
    for (int i = 0; i < n; ++i) {
        PyObject *item = PyList_GetItem(observe_list, i);
        if (!PyLong_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "All observation elements must be integers");
            return NULL;
        }
        observation[i] = PyLong_AsUnsignedLong(item) & 0xFFFFFFFFU;
    }
    cracker(&obj->mc, observation, bits);
    Py_RETURN_NONE;
}


static PyObject *
_mt19937cracker_MT19937Cracker_getrandbits_impl(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    MT19937CrackerObject *obj = (MT19937CrackerObject *) self;

    uint32_t val;
    int status = getrandbits_C(&obj->mc, &val);
    if (status != 0) {
        PyErr_SetString(PyExc_RuntimeError, "State not recovered yet.");
        return NULL;
    }
    return PyLong_FromUnsignedLong(val);
}


static PyObject *
_mt19937cracker_MT19937Cracker_getstate_impl(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    MT19937CrackerObject *obj = (MT19937CrackerObject *) self;
    
    if (!obj->mc.state_recovered) {
        PyErr_SetString(PyExc_RuntimeError, "State not recovered yet.");
        return NULL;
    }
    PyObject *list = PyList_New(N);
    if (!list) return NULL;
    for (int i = 0; i < N; ++i) {
        PyList_SetItem(list, i, PyLong_FromUnsignedLong(obj->mc.state[i]));
    }
    return list;
}

static PyMethodDef MT19937Cracker_methods[] = {
    {"cracker",     _mt19937cracker_MT19937Cracker_cracker_impl,     METH_VARARGS, PyDoc_STR("Recover MT19937 state from observations.")},
    {"getrandbits", _mt19937cracker_MT19937Cracker_getrandbits_impl, METH_NOARGS,  PyDoc_STR("Get next random number.")},
    {"getstate",    _mt19937cracker_MT19937Cracker_getstate_impl,    METH_NOARGS,  PyDoc_STR("Get recovered MT19937 state.")},
    {NULL}
};

static PyTypeObject MT19937CrackerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "mt19937cracker.MT19937Cracker",
    .tp_doc = "Cracker of Mersenne Twister",
    .tp_basicsize = sizeof(MT19937CrackerObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = MT19937Cracker_new,
    .tp_init = (initproc)MT19937Cracker_init,
    .tp_dealloc = (destructor)MT19937Cracker_dealloc,
    .tp_methods = MT19937Cracker_methods,
};

static PyModuleDef mt19937Crackermodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "mt19937cracker",
    .m_doc = "A cracker of Mersenne Twister implementation",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_mt19937cracker(void)
{
    PyObject *m;
    if (PyType_Ready(&MT19937CrackerType) < 0)
        return NULL;

    m = PyModule_Create(&mt19937Crackermodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&MT19937CrackerType);
    if (PyModule_AddObject(m, "MT19937Cracker", (PyObject *)&MT19937CrackerType) < 0) {
        Py_DECREF(&MT19937CrackerType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
