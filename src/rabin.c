#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*  Simple utility to calculate variable sized blocks of an input file.
    Uses Rabin fingerprint to determine block boundaries.  */

const int PRIME = 3;
const int WINDOWSIZE = 48;
const unsigned int AVGSIZE = 64*1024-1;
const int MAXSIZE = 256*1024;
const int MINSIZE = 8*1024;


struct listelement {
    unsigned int size;
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

    FILE *fp;
    fp = fopen(filename, "rb");
    if (!fp) { return head; }

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
    int blocksize = 0;
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
        printf("%d\n", curr->size);
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
