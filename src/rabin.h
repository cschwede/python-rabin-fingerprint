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
