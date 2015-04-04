#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied.
See the License for the specific language governing permissions and
limitations under the License. */

/*  Simple utility to calculate variable sized blocks of an input file.
    Uses Rabin fingerprint to determine block boundaries.  */


struct node {
    unsigned long long value;
    struct node *next;
};


struct state {
    struct node *cycle_curr;
    struct node *curr;
    struct node *head;
    unsigned long long *map;
    unsigned long long fingerprint;
    unsigned long long blocksize;
    int prime;
    int avgsize;
    int minsize;
    int maxsize;
};


unsigned long long *char_nth_prime(int windowsize, int prime) {
    // Calculates result = prime ^ windowsize first
    // After that multiplies all 256 bytes with result and returns static map
    static unsigned long long map[256];
    unsigned long long result = 1;
    int i;
    for (i=1; i<=windowsize; i++) { result *= prime; } 
    for (i=0; i<=256; i++) { map[i] = i * result; }
    return map;
}


struct node* init_cyclic_ring(int windowsize) {
    struct node *cycle_head = NULL;
    struct node *cycle_curr = NULL;
    cycle_head = malloc(sizeof(struct node));
    cycle_curr = cycle_head;
    cycle_curr->value = 0;
    int i = 0;
    for (i = 0; i < (windowsize-1); i++) {
        cycle_curr->next = malloc(sizeof(struct node));
        cycle_curr = cycle_curr->next;
        cycle_curr->value = 0;
    }
    cycle_curr->next = cycle_head; //close the ring
    return cycle_curr;
}


void update(unsigned char *buffer, int read, struct state **internal_state) {
    int ch;
    for (int i=0; i < read; i++) {
        ch = buffer[i]+1; //add 1 to make immune to long sequences of 0
        (*internal_state)->fingerprint *= (*internal_state)->prime;
        (*internal_state)->fingerprint += ch;
        (*internal_state)->fingerprint -= ((*internal_state)->map)[(*internal_state)->cycle_curr->value];

        (*internal_state)->cycle_curr->value = ch;
        (*internal_state)->cycle_curr = (*internal_state)->cycle_curr->next;

        if (((*internal_state)->blocksize) > ((*internal_state)->minsize)) {
            if (
                    (((*internal_state)->fingerprint & (*internal_state)->avgsize) == 1) ||
                    ((*internal_state)->blocksize > (*internal_state)->maxsize)
                ) {
                (*internal_state)->curr->value = (*internal_state)->blocksize;
                (*internal_state)->curr->next = malloc(sizeof(struct node));
                (*internal_state)->curr = (*internal_state)->curr->next;
                (*internal_state)->curr->next = NULL;
                (*internal_state)->blocksize=0;
            }
        }
        (*internal_state)->blocksize++;
    }
}


struct node* rabin(FILE *fp, int avgsize, int minsize, int maxsize, int prime, int windowsize) {

    // initialize linked list for returned blocksizes
    struct node *head = NULL;
    struct node *curr = NULL; 
    head = malloc(sizeof(struct node));  
    curr = head;
    curr->value = 0;
    curr->next = NULL;

    struct state *internal_state = NULL;
    internal_state = malloc(sizeof(struct state));
    internal_state->blocksize=0;
    internal_state->fingerprint=0;
    internal_state->prime = prime;
    internal_state->avgsize = avgsize;
    internal_state->minsize = minsize;
    internal_state->maxsize = maxsize;
    internal_state->map = char_nth_prime(windowsize, prime);
    internal_state->cycle_curr = init_cyclic_ring(windowsize);

    internal_state->curr = curr;
    internal_state->head = head;

    unsigned char buffer[8192];  // reading file in chunks is much faster
    int read = 0;
    while ((read = fread(buffer, 1, sizeof(buffer), fp))) {
        update(buffer, read, &internal_state);
    }
    
    // add last block if not yet done 
    if (internal_state->blocksize != 0) {
            internal_state->curr->value = internal_state->blocksize;
            internal_state->curr->next = malloc(sizeof(struct node));
            internal_state->curr = internal_state->curr->next;
            internal_state->curr->next = NULL;
            internal_state->blocksize=0;
    }

    return internal_state->head;
}


static PyObject * pyrabin(PyObject *self, PyObject *args, PyObject *kwargs) {
    char *filename;
    int avgsize = 64*1024-1;
    int minsize = 32*1024;
    int maxsize = 256*1024;
    int prime = 3;
    int windowsize = 48;
    static char *kwlist[] = {"filename", "avgsize", "minsize",
                             "maxsize", "prime", "windowsize", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|IIIII", kwlist,
                                     &filename, &avgsize, &minsize,
                                     &maxsize, &prime, &windowsize))
        return NULL;

    PyObject *list;
    PyObject *pynode = NULL;
    if (!(list=PyList_New(0))) return NULL;

    FILE *fp;
    fp = fopen(filename, "rb");
    if (!fp) { PyErr_SetFromErrno(PyExc_IOError); return NULL; }

    struct node* curr = rabin(fp, avgsize, minsize, maxsize, prime, windowsize);
    fclose(fp);

    if (curr == NULL) { return NULL; } //returns IOError
    if (curr->next == NULL) { return list; } // return empty list
    while (curr->next != NULL) {
        pynode=Py_BuildValue("l", curr->value);
        PyList_Append(list, pynode);
        curr = curr->next;
    }
    Py_DECREF(pynode);
    return list;
}


static PyObject * pyrabin_fd(PyObject *self, PyObject *args, PyObject *kwargs) {
    int filedesc;
    int avgsize = 64*1024-1;
    int minsize = 32*1024;
    int maxsize = 256*1024;
    int prime = 3;
    int windowsize = 48;
    static char *kwlist[] = {"filedesc", "avgsize", "minsize",
                             "maxsize", "prime", "windowsize", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "I|IIIII", kwlist,
                                     &filedesc, &avgsize, &minsize,
                                     &maxsize, &prime, &windowsize))
        return NULL;

    PyObject *list;
    PyObject *pynode = NULL;
    if (!(list=PyList_New(0))) return NULL;

    FILE *fp;
    fp = fdopen(filedesc, "rb");
    if (!fp) { PyErr_SetFromErrno(PyExc_IOError); return NULL; }

    struct node* curr = rabin(fp, avgsize, minsize, maxsize, prime, windowsize);

    if (curr == NULL) { return NULL; } //returns IOError
    if (curr->next == NULL) { return list; } // return empty list
    while (curr->next != NULL) {
        pynode=Py_BuildValue("l", curr->value);
        PyList_Append(list, pynode);
        curr = curr->next;
    }
    Py_DECREF(pynode);
    return list;
}


static PyMethodDef IndexerMethods[] = {
    {"chunksizes_from_filename", (PyCFunction)pyrabin, METH_VARARGS|METH_KEYWORDS},
    {"chunksizes_from_fd", (PyCFunction)pyrabin_fd, METH_VARARGS|METH_KEYWORDS},
    // Deprecated method name
    {"rabin", (PyCFunction)pyrabin, METH_VARARGS|METH_KEYWORDS},
    {NULL, NULL}
};


#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "rabin",
    NULL,
    -1,
    IndexerMethods,
};

PyMODINIT_FUNC PyInit_rabin(void) {
    return PyModule_Create(&moduledef);
}
#else
PyMODINIT_FUNC initrabin(void) {
        (void) Py_InitModule("rabin", IndexerMethods);
};
#endif
