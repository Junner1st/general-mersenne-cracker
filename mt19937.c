#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdint.h>
#include "mt19937.h"


typedef struct {
    PyObject_HEAD
    MT19937_C mt;
} MT19937Object;

static void
MT19937_dealloc(MT19937Object *self)
{
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
MT19937_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    MT19937Object *self;
    self = (MT19937Object *)type->tp_alloc(type, 0);
    if (self != NULL) {
        for (int i = 0; i < N; i++) {
            self->mt.state[i] = 0;
        }
        self->mt.index = N + 1;
    }
    return (PyObject *)self;
}

void
mt_init(MT19937_C *mt, uint32_t *state_array)
{
    memcpy(mt->state, state_array, N * sizeof(uint32_t));
    mt->index = N;
}

static int
MT19937_init(MT19937Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *state_list;
    if (!PyArg_ParseTuple(args, "O", &state_list)) {
        return -1;
    }

    if (!PyList_Check(state_list)) {
        PyErr_SetString(PyExc_TypeError, "state must be a list");
        return -1;
    }

    Py_ssize_t length = PyList_Size(state_list);
    if (length != N) {
        PyErr_Format(PyExc_ValueError, "State list must have exactly %d elements", N);
        return -1;
    }

    uint32_t state_array[N];
    for (int i = 0; i < N; i++) {
        PyObject *item = PyList_GetItem(state_list, i);
        if (!PyLong_Check(item)) {
            PyErr_SetString(PyExc_TypeError, "All state elements must be integers");
            return -1;
        }
        state_array[i] = PyLong_AsUnsignedLong(item) & 0xFFFFFFFF;
    }
    mt_init(&self->mt, state_array);

    return 0;
}

static void
twist(MT19937_C *self)
{
    for (int i = 0; i < N; i++) {
        uint32_t x = (self->state[i] & UPPER_MASK) | (self->state[(i + 1) % N] & LOWER_MASK);
        uint32_t xA = x >> 1;
        if (x & 1) {
            xA ^= MATRIX_A;
        }
        self->state[i] = self->state[(i + M) % N] ^ xA;
    }
    self->index = 0;
}

uint32_t
mt_extract(MT19937_C *self)
{
    if (self->index >= N){
        twist(self);
    }
    uint32_t y = self->state[self->index++];
    y ^= y >> 11;
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    y ^= y >> 18;
    return y;
}

static PyObject *MT19937_extract_number(MT19937Object *self, PyObject *Py_UNUSED(ignored)) {
    uint32_t y = mt_extract(&(self->mt));
    return PyLong_FromUnsignedLong(y);
}


static PyMethodDef MT19937_methods[] = {
    {"extract_number", (PyCFunction)MT19937_extract_number, METH_NOARGS,
     PyDoc_STR("Extract a random number")},
    {NULL}
};

static PyTypeObject MT19937Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "mt19937.MT19937",
    .tp_doc = "Mersenne Twister 19937 pseudorandom number generator",
    .tp_basicsize = sizeof(MT19937Object),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = MT19937_new,
    .tp_init = (initproc)MT19937_init,
    .tp_dealloc = (destructor)MT19937_dealloc,
    .tp_methods = MT19937_methods,
};

static PyModuleDef mt19937module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "mt19937",
    .m_doc = "Simple Mersenne Twister implementation",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_mt19937(void)
{
    PyObject *m;
    if (PyType_Ready(&MT19937Type) < 0)
        return NULL;

    m = PyModule_Create(&mt19937module);
    if (m == NULL)
        return NULL;

    Py_INCREF(&MT19937Type);
    if (PyModule_AddObject(m, "MT19937", (PyObject *)&MT19937Type) < 0) {
        Py_DECREF(&MT19937Type);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}