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


struct listelement {
    unsigned long long size;
    struct listelement *next;
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


struct listelement* rabin(char *filename) {
    unsigned long long *map = char_nth_prime(WINDOWSIZE, PRIME);

    struct listelement *head = NULL;
    struct listelement *curr = NULL; 

    head = malloc(sizeof(struct listelement));  
    curr = head;

    curr->size = 0;

    FILE *fp;
    fp = fopen(filename, "rb");
    if (!fp) { PyErr_SetFromErrno(PyExc_IOError); return NULL; }

    //inizalize ring
    int i=0;
    int window[WINDOWSIZE];
    for (i = 0; i <= WINDOWSIZE; i++) { window[i] = 0; }
    
    unsigned long long current_position=0;
    unsigned long long last_position=0;

    int ch=0;
    unsigned long long fingerprint=0;
    unsigned long long oldest_byte = 0;
    int newest_byte_position=0;
	int oldest_byte_position=1;
    unsigned long long blocksize = 0;
    while (ch != EOF) {
    	ch=getc(fp);
        
        if (newest_byte_position > WINDOWSIZE) { newest_byte_position=0; }
        if (oldest_byte_position > WINDOWSIZE) { oldest_byte_position=0; }
        
        window[newest_byte_position]=ch;
        oldest_byte = window[oldest_byte_position];
        
        fingerprint *= PRIME;
        fingerprint += ch;
        fingerprint -= map[oldest_byte];
        
        blocksize = current_position - last_position;
        if (blocksize > MINSIZE) {
            if (((fingerprint & AVGSIZE) == (AVGSIZE-1)) || 
                (blocksize > MAXSIZE)) {
                    if (curr->size!=0) { //only 0 if first element in list
                        curr->next = malloc(sizeof(struct listelement)); 
                        curr = curr->next;
                        curr->next = NULL;
                    }
                    curr->size = blocksize;
                    last_position = current_position;
            }
        }
        current_position++;
        newest_byte_position++; 
        oldest_byte_position++;
    }
    fclose(fp);

    return head;
}


void print_rabin(char *filename) {
    struct listelement* head = rabin(filename);
    struct listelement *curr = head;
    while (curr != NULL) {
        printf("%llu\n", curr->size);
        curr = curr->next;
    }
}


int main(int argc, char *argv[]){
    if (argc != 2) {
        printf("Missing filename!\n");
        return 1;
    }
    print_rabin(argv[1]);
    return 0;
}


static PyObject * pyrabin(PyObject *self, PyObject *args) {
    const char *filename;
    if (!PyArg_ParseTuple(args, "s", &filename)) { return NULL; }
    struct listelement* head = rabin(filename);
    
    if (head == NULL) { return NULL; } //returns IOError
    
    struct listelement *curr = head;
    PyObject *list, *pylistelement;
    if (!(list=PyList_New(0))) return NULL;
    while (curr != NULL) {
        pylistelement=Py_BuildValue("l", curr->size);
        PyList_Append(list, pylistelement);
        curr = curr->next;
    }
    Py_DECREF(pylistelement);
    return list;
}

static PyMethodDef IndexerMethods[] = {
    {"rabin",  pyrabin, METH_VARARGS, "Rabin fingerprinting a file"},
    {NULL, NULL, 0, NULL}     
};

PyMODINIT_FUNC initrabin(void) {
        (void) Py_InitModule("rabin", IndexerMethods);
};
