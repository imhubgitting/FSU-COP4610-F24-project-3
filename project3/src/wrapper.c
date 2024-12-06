#include <stdio.h>
#include <string.h>

#define BUF_LEN 1000

typedef struct {
    int size;
    char ** items;
} tokenlist;

FILE * fp;

void read(tokenlist * tokens) {
    (void)tokens; // Cast to void to avoid unused parameter warning

    // calculate size
    fseek(fp , 0, SEEK_END);
    unsigned long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char buf[BUF_LEN];
    memset(buf, 0, BUF_LEN);
    fread(&buf, 1, size, fp);       // fp is at offset 29
    printf("String: %s\n", buf);
}

// Wrapper function that takes a function pointer as an argument
void wrapper(void (*func)(tokenlist * tokens), tokenlist * tokens) {
    unsigned long init_pos = ftell(fp);     // save old pos before moving fp around
    printf("Calling wrapped function...\n");
    func(tokens);                           // called wrapped function
    fseek(fp, init_pos, SEEK_SET);          // reset fp to old pos
}
