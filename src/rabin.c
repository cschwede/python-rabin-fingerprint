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


struct node* rabin(FILE *fp, int avgsize, int minsize, int maxsize, int prime, int windowsize) {
    unsigned long long *map = char_nth_prime(windowsize, prime);

    // initialize linked list for returned blocksizes
    struct node *head = NULL;
    struct node *curr = NULL; 
    head = malloc(sizeof(struct node));  
    curr = head;
    curr->value = 0;
    curr->next = NULL;
    
    // inizalize cyclic ring
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

    int ch=0;
    unsigned long long fingerprint = 0;
    unsigned long long blocksize = 0;
    unsigned char buffer[8192];  // reading file in chunks is much faster
    int read = 0;
    while ((read = fread(buffer, 1, sizeof(buffer), fp))) {
        for (int i=0; i < read; i++) {
            ch = buffer[i];
            fingerprint *= prime;
            fingerprint += (ch+1); //add 1 to make immune to long sequences of 0
            fingerprint -= map[cycle_curr->value];

            cycle_curr->value = ch+1;
            cycle_curr = cycle_curr->next;

            if (blocksize > minsize) {
                if (((fingerprint & avgsize) == 1) || (blocksize > maxsize)) {
                    curr->value = blocksize;
                    curr->next = malloc(sizeof(struct node));
                    curr = curr->next;
                    curr->next = NULL;
                    blocksize=0;
                }
            }
            blocksize++;
        }
    }
    
    // add last block if not yet done 
    if (blocksize != 0) {
            curr->value = blocksize;
            curr->next = malloc(sizeof(struct node)); 
            curr = curr->next;
            curr->next = NULL;
            blocksize=0;
    }

    return head;
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


static PyMethodDef IndexerMethods[] = {
    {"chunksizes_from_filename", (PyCFunction)pyrabin, METH_VARARGS|METH_KEYWORDS},
    // Deprecated method name
    {"rabin", (PyCFunction)pyrabin, METH_VARARGS|METH_KEYWORDS},
    {NULL, NULL}
};


PyMODINIT_FUNC initrabin(void) {
        (void) Py_InitModule("rabin", IndexerMethods);
};
