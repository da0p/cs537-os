#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main (int argc, char *argv[]) {

    char *l = NULL;
    FILE *fp = NULL;
    size_t n = 0;

    if (argc == 1) {
        printf("wgrep: searchterm [file ...]\n");
        return 1;
    }

    if (argc == 2) {
        while (getline(&l, &n, stdin) != -1) {
            if (strstr(l, argv[1]) != NULL)
               printf("%s", l); 
        }

        free(l);

        return 0;
    }

    for (int i = 2; i < argc; i++) {
        fp = fopen(argv[i], "r");
        if (fp == NULL) {
            printf("wgrep: cannot open file\n");
            return 1;
        }
        while (getline(&l, &n, fp) != -1) {
            if (strstr(l, argv[1]) != NULL)
                printf("%s", l);
        }
        fclose(fp);
    }

    free(l);
    
    return 0;
}
