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


const int PRIME = 3;
const int WINDOWSIZE = 48;
const unsigned int AVGSIZE = 64*1024-1;
const int MAXSIZE = 256*1024;
const int MINSIZE = 8*1024;


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


struct node* rabin(char *filename) {
    FILE *fp;
    fp = fopen(filename, "rb");
    if (!fp) { PyErr_SetFromErrno(PyExc_IOError); return NULL; }

    unsigned long long *map = char_nth_prime(WINDOWSIZE, PRIME);

    // initialize linked list for returned blocksizes
    struct node *head = NULL;
    struct node *curr = NULL; 
    head = malloc(sizeof(struct node));  
    curr = head;
    curr->value = 0;
    
    // inizalize cyclic ring
    struct node *cycle_head = NULL;
    struct node *cycle_curr = NULL; 
    cycle_head = malloc(sizeof(struct node));  
    cycle_curr = cycle_head;
    cycle_curr->value = 0;
    int i = 0;
    for (i = 0; i < (WINDOWSIZE-1); i++) {
        cycle_curr->next = malloc(sizeof(struct node)); 
        cycle_curr = cycle_curr->next;
        cycle_curr->value = 0;
    }
    cycle_curr->next = cycle_head; //close the ring

    int ch=0;
    unsigned long long fingerprint = 0;
    unsigned long long blocksize = 0;
    while (ch != EOF) {
    	ch=getc(fp);

        fingerprint *= PRIME;
        fingerprint += ch;
        fingerprint -= map[cycle_curr->value];
        
        cycle_curr->value = ch;
        cycle_curr = cycle_curr->next;

        if (blocksize > MINSIZE) {
            if (((fingerprint & AVGSIZE) == 1) || (blocksize > MAXSIZE)) {
                curr->value = blocksize;
                curr->next = malloc(sizeof(struct node)); 
                curr = curr->next;
                curr->next = NULL;
                blocksize=0;
            }
        }
        blocksize++;
    }
    fclose(fp);
    return head;
}


static PyObject * pyrabin(PyObject *self, PyObject *args) {
    const char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) { return NULL; }

    PyObject *list;
    PyObject *pynode = NULL;
    if (!(list=PyList_New(0))) return NULL;
    
    struct node* curr = rabin(filename);

    if (curr == NULL) { return NULL; } //returns IOError
    while (curr->next != NULL) {
        pynode=Py_BuildValue("l", curr->value);
        PyList_Append(list, pynode);
        curr = curr->next;
    }
    Py_DECREF(pynode);
    return list;
}


static PyMethodDef IndexerMethods[] = {
    {"rabin", 
    pyrabin, 
    METH_VARARGS, 
    "Rabin fingerprinting a file"},
    {NULL, NULL, 0, NULL}     
};


PyMODINIT_FUNC initrabin(void) {
        (void) Py_InitModule("rabin", IndexerMethods);
};
